/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <QEvent>
#include <QImageReader>
#include <QInputDialog>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStringList>
#include <QTabBar>
#include "QsLog.h"

#include "application.h"
#include "buyoutmanager.h"
#include "column.h"
#include "datamanager.h"
#include "filesystem.h"
#include "filters.h"
#include "modsfilter.h"
#include "flowlayout.h"
#include "item.h"
#include "itemlocation.h"
#include "itemsmanager.h"
#include "itemsmanagerworker.h"
#include "logpanel.h"
#include "shop.h"
#include "util.h"
#include "verticalscrollarea.h"

const std::string POE_WEBCDN = "http://webcdn.pathofexile.com";

MainWindow::MainWindow(std::unique_ptr<Application> app):
    app_(std::move(app)),
    ui(new Ui::MainWindow),
    current_search_(nullptr),
    search_count_(0),
    auto_online_(app_->data_manager(), app_->sensitive_data_manager())
{
#ifdef Q_OS_WIN32
    createWinId();
    taskbar_button_ = new QWinTaskbarButton(this);
    taskbar_button_->setWindow(this->windowHandle());
#endif
    image_cache_ = new ImageCache(Filesystem::UserDir() + "/cache");

    InitializeUi();
    InitializeLogging();
    InitializeSearchForm();
    NewSearch();

    image_network_manager_ = new QNetworkAccessManager;
    connect(image_network_manager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(OnImageFetched(QNetworkReply*)));

    connect(&app_->items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(OnItemsRefreshed()));
    connect(&app_->items_manager(), SIGNAL(StatusUpdate(ItemsFetchStatus)), this, SLOT(OnItemsManagerStatusUpdate(ItemsFetchStatus)));
    connect(&update_checker_, &UpdateChecker::UpdateAvailable, this, &MainWindow::OnUpdateAvailable);
    connect(&auto_online_, &AutoOnline::Update, this, &MainWindow::OnOnlineUpdate);
}

void MainWindow::InitializeLogging() {
    LogPanel *log_panel = new LogPanel(this, ui);
    QsLogging::DestinationPtr log_panel_ptr(log_panel);
    QsLogging::Logger::instance().addDestination(log_panel_ptr);
}

void MainWindow::InitializeUi() {
    ui->setupUi(this);
    status_bar_label_ = new QLabel("Ready");
    status_bar_label_->setMargin(6);
    statusBar()->addWidget(status_bar_label_);

    tab_bar_ = new QTabBar;
    tab_bar_->installEventFilter(this);
    tab_bar_->setExpanding(false);
    tab_bar_->setDocumentMode(true);
    tab_bar_->setTabsClosable(true);
    ui->mainLayout->insertWidget(0, tab_bar_);
    tab_bar_->addTab("+");
    tab_bar_->tabButton(0, QTabBar::RightSide)->resize(0, 0);
    connect(tab_bar_, SIGNAL(currentChanged(int)), this, SLOT(OnTabChange(int)));
    connect(tab_bar_, SIGNAL(tabCloseRequested(int)), this, SLOT(OnTabClose(int)));


    QAction* action = tab_context_menu_.addAction("Rename Tab...");
    connect(action, &QAction::triggered, [this] {
        int index = tab_bar_->tabAt(tab_bar_->mapFromGlobal(tab_context_menu_.pos()));
        if (index != -1) {
            Search* search = searches_[index];
            bool ok;
            QString caption = QInputDialog::getText(this, "Tab Caption", "Enter tab caption:", QLineEdit::Normal,
                search->GetCaption(false), &ok);
            if (ok && !caption.isEmpty()) {
                search->SetCaption(caption);
                tab_bar_->setTabText(index, search->GetCaption());
            }
        }
    });

    action = new QAction(this);
    action->setShortcutContext(Qt::WindowShortcut);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(action, &QAction::triggered, [this] {
        // Select "+"
        tab_bar_->setCurrentIndex(tab_bar_->count() - 1);
    });
    this->addAction(action);


    action = tab_context_menu_.addAction("Close Tab");
    action->setShortcutContext(Qt::WindowShortcut);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    this->addAction(action);
    connect(action, &QAction::triggered, [this] {
        int index = tab_bar_->tabAt(tab_bar_->mapFromGlobal(tab_context_menu_.pos()));
        if (tab_context_menu_.isVisible() && index != -1) {
            RemoveTab(index);
        }
        else {
            RemoveTab(tab_bar_->currentIndex());
        }
    });

    tab_bar_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tab_bar_, &QTreeView::customContextMenuRequested, [&](const QPoint &pos) {
        int index = tab_bar_->tabAt(pos);
        if (index != -1 && index != tab_bar_->count() - 1) {
            tab_context_menu_.popup(tab_bar_->mapToGlobal(pos));
        }
    });

    Util::PopulateBuyoutTypeComboBox(ui->buyoutTypeComboBox);
    Util::PopulateBuyoutCurrencyComboBox(ui->buyoutCurrencyComboBox);

    connect(ui->buyoutCurrencyComboBox, SIGNAL(activated(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutTypeComboBox, SIGNAL(activated(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutValueLineEdit, SIGNAL(textEdited(QString)), this, SLOT(OnBuyoutChange()));

    ui->buyoutCurrencyComboBox->hide();

    ui->actionAutomatically_refresh_items->setChecked(app_->items_manager().auto_update());
    UpdateShopMenu();

    ui->searchAreaWidget->setVisible(false);
    ui->imageWidget->hide();

    ui->menuBar->hide();

    ui->splitter_2->setSizes({(int)(this->width() * 0.75), (int)(this->width() * 0.25)});
    ui->splitter_2->setStretchFactor(0, 1);
    ui->splitter_2->setStretchFactor(1, 0);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    default_context_menu_.addAction("Expand All", this, SLOT(OnExpandAll()));
    default_context_menu_.addAction("Collapse All", this, SLOT(OnCollapseAll()));
    default_context_menu_.addSeparator();
    default_context_menu_showhidden_ = default_context_menu_.addAction("Show Hidden");
    default_context_menu_showhidden_->setCheckable(true);
    connect(default_context_menu_showhidden_, SIGNAL(toggled(bool)), this, SLOT(ToggleShowHiddenBuckets(bool)));

    bucket_context_menu_toggle_ = bucket_context_menu_.addAction("Hide Bucket", this, SLOT(ToggleBucketAtMenu()));

    connect(ui->treeView, &QTreeView::customContextMenuRequested, [&](const QPoint &pos) {
        QModelIndex index = ui->treeView->indexAt(pos);
        if (index.isValid() && !index.parent().isValid()) {
            QString hash = ui->treeView->model()->data(index, ItemsModel::HashRole).toString();
            if (current_search_->IsBucketHidden(hash)) {
                bucket_context_menu_toggle_->setText("Show Bucket");
            }
            else {
                bucket_context_menu_toggle_->setText("Hide Bucket");
            }
            bucket_context_menu_.popup(ui->treeView->viewport()->mapToGlobal(pos));
        }
        else {
            default_context_menu_.popup(ui->treeView->viewport()->mapToGlobal(pos));
        }
    });

    ui->propertiesLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    online_label_.setMargin(6);
    statusBar()->addPermanentWidget(&online_label_);
    UpdateOnlineGui();

    update_button_.setStyleSheet("color: blue; font-weight: bold;");
    update_button_.setFlat(true);
    update_button_.hide();
    statusBar()->addPermanentWidget(&update_button_);
    connect(&update_button_, &QPushButton::clicked, [=](){
        UpdateChecker::AskUserToUpdate(this);
    });

    UpdateSettingsBox();
}

void MainWindow::ExpandCollapse(TreeState state) {
    // we block signals so that ResizeTreeColumns isn't called for every item, which is damn slow!
    ui->treeView->blockSignals(true);
    if (state == TreeState::kExpand)
        ui->treeView->expandAll();
    else
        ui->treeView->collapseAll();
    ui->treeView->blockSignals(false);

    ResizeTreeColumns();
}

void MainWindow::ToggleBucketAtMenu() {
    QPoint pos = ui->treeView->viewport()->mapFromGlobal(bucket_context_menu_.pos());
    QModelIndex index = ui->treeView->indexAt(pos);
    if (index.isValid()) {
        QString hash = ui->treeView->model()->data(index, ItemsModel::HashRole).toString();
        if (current_search_->IsBucketHidden(hash)) {
            current_search_->HideBucket(hash, false);
        }
        else {
            current_search_->HideBucket(hash);
        }
        current_search_->Activate(app_->items(), ui->treeView);
    }
}

void MainWindow::ToggleShowHiddenBuckets(bool checked) {
    current_search_->ShowHiddenBuckets(checked);
    current_search_->Activate(app_->items(), ui->treeView);
}

void MainWindow::OnExpandAll() {
    ExpandCollapse(TreeState::kExpand);
}

void MainWindow::OnCollapseAll() {
    ExpandCollapse(TreeState::kCollapse);
}

void MainWindow::ResizeTreeColumns() {
    for (int i = 0; i < ui->treeView->header()->count(); ++i)
        ui->treeView->resizeColumnToContents(i);
}

void MainWindow::OnBuyoutChange() {
    app_->shop().ExpireShopData();

    Buyout bo;
    bo.type = static_cast<BuyoutType>(ui->buyoutTypeComboBox->currentIndex());
    if (ui->buyoutCurrencyComboBox->isVisible()) {
        bo.currency = static_cast<Currency>(ui->buyoutCurrencyComboBox->currentIndex());
        bo.value = ui->buyoutValueLineEdit->text().replace(',', ".").toDouble();
    }
    else {
        // Parse input like Procurement (e.g. 1 fuse)
        QStringList parts = ui->buyoutValueLineEdit->text().split(" ", QString::SkipEmptyParts);
        if (parts.size() > 2 || parts.size() == 0) {
            bo.currency = CURRENCY_NONE;
            bo.value = 0;
        }
        else {
            double val;
            QString curr;
            if (parts.size() == 1) {
                curr = parts.first();
                parts.clear();
                QRegularExpression reg("[0-9.,]+");
                QRegularExpressionMatch match = reg.match(curr);
                if (match.hasMatch()) {
                    parts.append(match.captured());
                    parts.append(curr.mid(match.capturedLength()));
                }
                else {
                    parts.append("0");
                    parts.append("chaos");
                }
            }

            val = parts.first().replace(",", ".").toDouble();
            curr = parts.last();

            bo.currency = CURRENCY_NONE;
            if (!curr.isEmpty()){
                // i = 1 to skip empty
                for (uint i = 1; i < CurrencyAsTag.size(); i++) {
                    QString currency = QString::fromStdString(CurrencyAsTag.at(i));
                    if (currency.startsWith(curr) ||
                        curr.startsWith(currency)) {
                        bo.currency = static_cast<Currency>(i);
                        break;
                    }
                }
            }

            bo.value = val;
        }
    }

    bo.last_update = QDateTime::currentDateTime();

    if (bo.type == BUYOUT_TYPE_NONE) {
        ui->buyoutCurrencyComboBox->setEnabled(false);
        ui->buyoutValueLineEdit->setEnabled(false);
    } else {
        ui->buyoutCurrencyComboBox->setEnabled(true);
        ui->buyoutValueLineEdit->setEnabled(true);
    }

    bool isUpdated = true;
    if (current_item_) {
        if (app_->buyout_manager().Exists(*current_item_)) {
            Buyout currBo = app_->buyout_manager().Get(*current_item_);
            if (currBo.currency == bo.currency &&
                currBo.type == bo.type &&
                currBo.value == bo.value) {
                isUpdated = false;
            }
        }

        if (isUpdated) {
            if (bo.type == BUYOUT_TYPE_NONE)
                app_->buyout_manager().Delete(*current_item_);
            else
                app_->buyout_manager().Set(*current_item_, bo);
        }
    } else {
        std::string tab = current_bucket_.location().GetUniqueHash();
        if (app_->buyout_manager().ExistsTab(tab)) {
            Buyout currBo = app_->buyout_manager().GetTab(tab);
            if (currBo.currency == bo.currency &&
                currBo.type == bo.type &&
                currBo.value == bo.value) {
                isUpdated = false;
            }
        }

        if (isUpdated) {
            if (bo.type == BUYOUT_TYPE_NONE)
                app_->buyout_manager().DeleteTab(tab);
            else
                app_->buyout_manager().SetTab(tab, bo);
        }
    }
    // refresh treeView to immediately reflect price changes
    if (isUpdated) {
        ui->treeView->model()->layoutChanged();
        ResizeTreeColumns();
    }
}

void MainWindow::OnItemsManagerStatusUpdate(const ItemsFetchStatus &status) {
    QString str = QString("Receiving stash data, %1/%2").arg(status.fetched).arg(status.total);
    if (status.throttled)
        str += " (throttled, sleeping 60 seconds)";
    if (status.fetched == status.total)
        str = "Received all tabs";
    status_bar_label_->setText(str);

#ifdef Q_OS_WIN32
    QWinTaskbarProgress *progress = taskbar_button_->progress();
    progress->setVisible(status.fetched != status.total);
    progress->setMinimum(0);
    progress->setMaximum(status.total);
    progress->setValue(status.fetched);
    progress->setPaused(status.throttled);
#endif
}

bool MainWindow::eventFilter(QObject *o, QEvent *e) {
    if (o == tab_bar_ && e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouse_event = static_cast<QMouseEvent *>(e);
        if (mouse_event->button() == Qt::MidButton) {
            // middle button pressed on a tab
            int index = tab_bar_->tabAt(mouse_event->pos());
            // remove tab and Search if it's not "+"
            if (index < tab_bar_->count() - 1) {
                RemoveTab(index);
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}

void MainWindow::OnImageFetched(QNetworkReply *reply) {
    std::string url = reply->url().toString().toStdString();
    if (reply->error()) {
        QLOG_WARN() << "Failed to download item image," << url.c_str();
        return;
    }
    QImageReader image_reader(reply);
    image_reader.setBackgroundColor(Qt::transparent);
    QImage image = image_reader.read();

    image_cache_->Set(url, image);

    if (current_item_ && (url == current_item_->icon() || url == POE_WEBCDN + current_item_->icon()))
        UpdateCurrentItemIcon(image);
}

void MainWindow::OnSearchFormChange() {
    // Update whether or not we're showing hidden buckets
    default_context_menu_showhidden_->setChecked(current_search_->ShowingHiddenBuckets());

    // Save buyouts
    app_->buyout_manager().Save();

    QStringList expandedHashes;
    if (ui->treeView->model()) {
        for (int i = 0; i < ui->treeView->model()->rowCount(); i++) {
            QModelIndex index = ui->treeView->model()->index(i, 0);
            if (ui->treeView->isExpanded(index)) {
                QString hash = ui->treeView->model()->data(index, ItemsModel::HashRole).toString();
                expandedHashes.append(hash);
            }
        }
    }

    current_search_->Activate(app_->items(), ui->treeView);
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(OnTreeChange(const QModelIndex&, const QModelIndex&)));
    ui->treeView->reset();

    if (current_search_->items().size() <= MAX_EXPANDABLE_ITEMS)
        ExpandCollapse(TreeState::kCollapse);

    if (!expandedHashes.isEmpty()) {
        for (int i = 0; i < ui->treeView->model()->rowCount(); i++) {
            QModelIndex index = ui->treeView->model()->index(i, 0);
            QString hash = ui->treeView->model()->data(index, ItemsModel::HashRole).toString();
            if (expandedHashes.contains(hash)) {
                ui->treeView->expand(index);
            }
        }
    }

    ResizeTreeColumns();

    tab_bar_->setTabText(tab_bar_->currentIndex(), current_search_->GetCaption());
}

void MainWindow::OnTreeChange(const QModelIndex &current, const QModelIndex & /* previous */) {
    QModelIndex actualCurrent = current_search_->GetIndex(current);
    if (!actualCurrent.parent().isValid()) {
        // clicked on a bucket
        current_item_ = nullptr;
        current_bucket_ = *current_search_->buckets()[actualCurrent.row()];
        UpdateCurrentBucket();
    } else {
        current_item_ = current_search_->buckets()[actualCurrent.parent().row()]->items()[actualCurrent.row()];
        UpdateCurrentItem();
    }
    UpdateCurrentBuyout();
    GenerateCurrentItemHeader();
}

void MainWindow::OnTabChange(int index) {
    // "+" clicked
    if (static_cast<size_t>(index) == searches_.size()) {
        NewSearch();
    } else {
        current_search_ = searches_[index];
        current_search_->ToForm();
    }
    OnSearchFormChange();
}

void MainWindow::RemoveTab(int index) {
    if (index < tab_bar_->count() - 1 && tab_bar_->count() > 2) {
        tab_bar_->removeTab(index);

        // If we're down to one tab and the "+" tab
        if (tab_bar_->count() == 2) {
            // Disable button
            tab_bar_->tabButton(0, QTabBar::RightSide)->resize(0, 0);
        }

        delete searches_[index];
        searches_.erase(searches_.begin() + index);
        if (static_cast<size_t>(tab_bar_->currentIndex()) == searches_.size())
            tab_bar_->setCurrentIndex(searches_.size() - 1);
        OnTabChange(tab_bar_->currentIndex());
        // that's because after removeTab text will be set to previous search's caption
        // which is because my way of dealing with "+" tab is hacky and should be replaced by something sane
        tab_bar_->setTabText(tab_bar_->count() - 1, "+");
    }
}

void MainWindow::OnTabClose(int index) {
    RemoveTab(index);
}

void MainWindow::AddSearchGroup(QLayout *layout, const std::string &name="") {
    if (!name.empty()) {
        auto label = new QLabel(("<h3>" + name + "</h3>").c_str());
        if (name == "Mods")
            ui->searchModsWidget->layout()->addWidget(label);
        else
            ui->searchOptionsWidget->layout()->addWidget(label);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    auto layout_container = new QWidget;
    layout_container->setLayout(layout);
    if (name == "Mods")
        ui->searchModsWidget->layout()->addWidget(layout_container);
    else
        ui->searchOptionsWidget->layout()->addWidget(layout_container);
}

void MainWindow::InitializeSearchForm() {
    auto name_search = std::make_unique<NameSearchFilter>(ui->searchBox);
    auto offense_layout = new FlowLayout;
    auto defense_layout = new FlowLayout;
    auto sockets_layout = new FlowLayout;
    auto requirements_layout = new FlowLayout;
    auto misc_layout = new FlowLayout;
    auto misc_flags_layout = new FlowLayout;
    auto mods_layout = new QVBoxLayout;

    AddSearchGroup(offense_layout, "Offense");
    AddSearchGroup(defense_layout, "Defense");
    AddSearchGroup(sockets_layout, "Sockets");
    AddSearchGroup(requirements_layout, "Requirements");
    AddSearchGroup(misc_layout, "Misc");
    AddSearchGroup(misc_flags_layout);
    AddSearchGroup(mods_layout, "Mods");

    using move_only = std::unique_ptr<Filter>;
    move_only init[] = {
        std::move(name_search),
        // Offense
        // new DamageFilter(offense_layout, "Damage"),
        std::make_unique<SimplePropertyFilter>(offense_layout, "Critical Strike Chance", "Crit."),
        std::make_unique<ItemMethodFilter>(offense_layout, [](Item* item) { return item->DPS(); }, "DPS"),
        std::make_unique<ItemMethodFilter>(offense_layout, [](Item* item) { return item->pDPS(); }, "pDPS"),
        std::make_unique<ItemMethodFilter>(offense_layout, [](Item* item) { return item->eDPS(); }, "eDPS"),
        std::make_unique<SimplePropertyFilter>(offense_layout, "Attacks per Second", "APS"),
        // Defense
        std::make_unique<SimplePropertyFilter>(defense_layout, "Armour"),
        std::make_unique<SimplePropertyFilter>(defense_layout, "Evasion Rating", "Evasion"),
        std::make_unique<SimplePropertyFilter>(defense_layout, "Energy Shield", "Shield"),
        std::make_unique<SimplePropertyFilter>(defense_layout, "Chance to Block", "Block"),
        // Sockets
        std::make_unique<SocketsFilter>(sockets_layout, "Sockets"),
        std::make_unique<LinksFilter>(sockets_layout, "Links"),
        std::make_unique<SocketsColorsFilter>(sockets_layout),
        std::make_unique<LinksColorsFilter>(sockets_layout),
        // Requirements
        std::make_unique<RequiredStatFilter>(requirements_layout, "Level", "R. Level"),
        std::make_unique<RequiredStatFilter>(requirements_layout, "Str", "R. Str"),
        std::make_unique<RequiredStatFilter>(requirements_layout, "Dex", "R. Dex"),
        std::make_unique<RequiredStatFilter>(requirements_layout, "Int", "R. Int"),
        // Misc
        std::make_unique<DefaultPropertyFilter>(misc_layout, "Quality", 0),
        std::make_unique<SimplePropertyFilter>(misc_layout, "Level"),
        std::make_unique<MTXFilter>(misc_flags_layout, "", "MTX"),
        std::make_unique<AltartFilter>(misc_flags_layout, "", "Alt. art"),
        std::make_unique<ModsFilter>(mods_layout)
    };
    filters_ = std::vector<move_only>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init)));

    mods_layout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void MainWindow::NewSearch() {
    current_search_ = new Search(app_->buyout_manager(), QString("Search %1").arg(++search_count_).toStdString(), filters_);
    tab_bar_->setTabText(tab_bar_->count() - 1, current_search_->GetCaption());
    tab_bar_->tabButton(tab_bar_->count() - 1, QTabBar::RightSide)->resize(16, 16);
    tab_bar_->addTab("+");
    tab_bar_->tabButton(tab_bar_->count() - 1, QTabBar::RightSide)->resize(0, 0);

    if (tab_bar_->count() > 2) {
        // Enable button on first tab
        tab_bar_->tabButton(0, QTabBar::RightSide)->resize(16, 16);
    }
    else {
        tab_bar_->tabButton(0, QTabBar::RightSide)->resize(0, 0);
    }

    // this can't be done in ctor because it'll call OnSearchFormChange slot
    // and remove all previous search data
    current_search_->ResetForm();
    searches_.push_back(current_search_);
    OnSearchFormChange();
}

void MainWindow::UpdateCurrentBucket() {
    ui->nameLabel->hide();
    ui->imageLabel->hide();
    ui->minimapLabel->hide();
    ui->locationLabel->hide();
    ui->propertiesLabel->hide();

    ui->imageWidget->hide();

    ItemLocationType location = current_bucket_.location().type();
    QString pos = "Stash Tab";
    if (location == ItemLocationType::CHARACTER)
        pos = "Character";
    ui->typeLineLabel->setText(pos + ": " + QString::fromStdString(current_bucket_.location().GetHeader()));
    ui->typeLineLabel->show();
}

void MainWindow::UpdateCurrentItem() {
    ui->typeLineLabel->show();
    ui->imageLabel->show();
    ui->minimapLabel->show();
    ui->locationLabel->show();
    ui->propertiesLabel->show();

    ui->imageWidget->show();

    app_->buyout_manager().Save();
    ui->typeLineLabel->setText(current_item_->typeLine().c_str());
    if (current_item_->name().empty())
        ui->nameLabel->hide();
    else {
        ui->nameLabel->setText(current_item_->name().c_str());
        ui->nameLabel->show();
    }
    ui->imageLabel->setText("Loading...");
    ui->imageLabel->setStyleSheet("QLabel { background-color : rgb(12, 12, 43); color: white }");
    ui->imageLabel->setFixedSize(QSize(current_item_->w(), current_item_->h()) * PIXELS_PER_SLOT);

    UpdateCurrentItemProperties();
    UpdateCurrentItemMinimap();

    std::string icon = current_item_->icon();
    if (icon.size() && icon[0] == '/')
        icon = POE_WEBCDN + icon;
    if (!image_cache_->Exists(icon))
        image_network_manager_->get(QNetworkRequest(QUrl(icon.c_str())));
    else
        UpdateCurrentItemIcon(image_cache_->Get(icon));

    ui->locationLabel->setText("Location: " + QString::fromStdString(current_item_->location().GetHeader()));
}

void MainWindow::GenerateCurrentItemHeader() {
    QString key = "";
    QColor color;
    int frame = FRAME_TYPE_NORMAL;
    if (current_item_) {
        frame = current_item_->frameType();
    }
    switch(frame) {
    case FRAME_TYPE_MAGIC:
        key = "Magic";
        color = QColor(0x83, 0x83, 0xF7);
        break;
    case FRAME_TYPE_GEM:
        key = "Gem";
        color = QColor(0x1b, 0xa2, 0x9b);
        break;
    case FRAME_TYPE_CURRENCY:
        key = "Currency";
        color = QColor(0x77, 0x6e, 0x59);
        break;
    case FRAME_TYPE_RARE:
        key = "Rare";
        color = QColor(Qt::darkYellow);
        break;
    case FRAME_TYPE_UNIQUE:
        key = "Unique";
        color = QColor(234, 117, 0);
        break;
    case FRAME_TYPE_NORMAL:
    default:
        key = "White";
        color = QColor(Qt::white);
        break;
    }
    if (key.isEmpty()) return;

    switch(frame) {
    case FRAME_TYPE_NORMAL:
    case FRAME_TYPE_MAGIC:
    case FRAME_TYPE_GEM:
    case FRAME_TYPE_CURRENCY:
        ui->nameLabel->hide();
        ui->typeLineLabel->setAlignment(Qt::AlignCenter);
        ui->horizontalWidget->setFixedSize(16777215, 34);
        ui->leftHeaderLabel->setFixedSize(29, 34);
        ui->rightHeaderLabel->setFixedSize(29, 34);
        break;
    case FRAME_TYPE_RARE:
    case FRAME_TYPE_UNIQUE:
        ui->nameLabel->show();
        ui->typeLineLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        ui->horizontalWidget->setFixedSize(16777215, 54);
        ui->leftHeaderLabel->setFixedSize(44, 54);
        ui->rightHeaderLabel->setFixedSize(44, 54);
        break;
    }

    QString style = QString("color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue());
    ui->nameLabel->setStyleSheet(style);
    ui->typeLineLabel->setStyleSheet(style);

    ui->leftHeaderLabel->setStyleSheet("border-image: url(:/headers/ItemHeader" + key + "Left.png);");
    ui->headerNameWidget->setStyleSheet("#headerNameWidget { border-image: url(:/headers/ItemHeader" + key + "Middle.png); }");
    ui->rightHeaderLabel->setStyleSheet("border-image: url(:/headers/ItemHeader" + key + "Right.png);");
}

void MainWindow::UpdateCurrentItemProperties() {
    std::vector<std::string> sections;

    std::string properties_text;
    bool first_prop = true;
    for (auto &property : current_item_->text_properties()) {
        if (!first_prop)
            properties_text += "<br>";
        first_prop = false;
        if (property.display_mode == 3) {
            QString format(property.name.c_str());
            for (auto &value : property.values)
                format = format.arg(value.c_str());
            properties_text += format.toStdString();
        } else {
            properties_text += property.name;
            if (property.values.size()) {
                if (property.name.size() > 0)
                    properties_text += ": ";
                bool first_val = true;
                for (auto &value : property.values) {
                    if (!first_val)
                        properties_text += ", ";
                    first_val = false;
                    properties_text += value;
                }
            }
        }
    }
    if (properties_text.size() > 0)
        sections.push_back(properties_text);

    std::string requirements_text;
    bool first_req = true;
    for (auto &requirement : current_item_->text_requirements()) {
        if (!first_req)
            requirements_text += ", ";
        first_req = false;
        requirements_text += requirement.name + ": " + requirement.value;
    }
    if (requirements_text.size() > 0)
        sections.push_back("Requires " + requirements_text);

    auto &mods = current_item_->text_mods();
    for (auto &mod_type : ITEM_MOD_TYPES) {
        std::string mod_list = Util::ModListAsString(mods.at(mod_type));
        if (!mod_list.empty())
            sections.push_back(mod_list);
    }

    std::string text;
    bool first = true;
    for (auto &s : sections) {
        if (!first)
            text += "<hr>";
        first = false;
        text += s;
    }
    ui->propertiesLabel->setText(text.c_str());
}

void MainWindow::UpdateCurrentItemIcon(const QImage &image) {
    int height = current_item_->h();
    int width = current_item_->w();
    int socket_rows = 0;
    int socket_columns = 0;
    // this will ensure we have enough room to draw the slots
    QPixmap pixmap(width * PIXELS_PER_SLOT, height * PIXELS_PER_SLOT);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    static const QImage link_h(":/sockets/linkH.png");
    static const QImage link_v(":/sockets/linkV.png");
    ItemSocket prev = { 255, '-' };
    size_t i = 0;

    if (current_item_->text_sockets().size() == 0) {
        // Do nothing
    }
    else if (current_item_->text_sockets().size() == 1) {
        auto &socket = current_item_->text_sockets().front();
        QImage socket_image(":/sockets/" + QString(socket.attr) + ".png");
        painter.drawImage(0, PIXELS_PER_SLOT * i, socket_image);
        socket_rows = 1;
        socket_columns = 1;
    }
    else {
        for (auto &socket : current_item_->text_sockets()) {
            bool link = socket.group == prev.group;
            QImage socket_image(":/sockets/" + QString(socket.attr) + ".png");
            if (width == 1) {
                painter.drawImage(0, PIXELS_PER_SLOT * i, socket_image);
                if (link)
                    painter.drawImage(16, PIXELS_PER_SLOT * i - 19, link_v);
                socket_columns = 1;
                socket_rows = i + 1;
            } else /* w == 2 */ {
                int row = i / 2;
                int column = i % 2;
                if (row % 2 == 1)
                    column = 1 - column;
                socket_columns = qMax(column + 1, socket_columns);
                socket_rows = qMax(row + 1, socket_rows);
                painter.drawImage(PIXELS_PER_SLOT * column, PIXELS_PER_SLOT * row, socket_image);
                if (link) {
                    if (i == 1 || i == 3 || i == 5) {
                        // horizontal link
                        painter.drawImage(
                            PIXELS_PER_SLOT - LINKH_WIDTH / 2,
                            row * PIXELS_PER_SLOT + PIXELS_PER_SLOT / 2 - LINKH_HEIGHT / 2,
                            link_h
                        );
                    } else if (i == 2) {
                        painter.drawImage(
                            PIXELS_PER_SLOT * 1.5 - LINKV_WIDTH / 2,
                            row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                            link_v
                        );
                    } else if (i == 4) {
                        painter.drawImage(
                            PIXELS_PER_SLOT / 2 - LINKV_WIDTH / 2,
                            row * PIXELS_PER_SLOT - LINKV_HEIGHT / 2,
                            link_v
                        );
                    } else {
                        QLOG_ERROR() << "No idea how to draw link for" << current_item_->PrettyName().c_str();
                    }
                }
            }

            prev = socket;
            ++i;
        }
    }

    QPixmap cropped = pixmap.copy(0, 0, PIXELS_PER_SLOT * socket_columns,
                                        PIXELS_PER_SLOT * socket_rows);

    QPixmap base(image.width(), image.height());
    base.fill(Qt::transparent);
    QPainter overlay(&base);
    overlay.drawImage(0, 0, image);

    overlay.drawPixmap((int)(0.5*(image.width() - cropped.width())),
                       (int)(0.5*(image.height() - cropped.height())), cropped);

    ui->imageLabel->setPixmap(base);
}

void MainWindow::UpdateCurrentItemMinimap() {
    QPixmap pixmap(MINIMAP_SIZE, MINIMAP_SIZE);
    pixmap.fill(QColor("transparent"));

    QPainter painter(&pixmap);
    painter.setBrush(QBrush(QColor(0x0c, 0x0b, 0x0b)));
    painter.drawRect(0, 0, MINIMAP_SIZE, MINIMAP_SIZE);
    const ItemLocation &location = current_item_->location();
    painter.setBrush(QBrush(location.socketed() ? Qt::blue : Qt::red));
    QRectF rect = current_item_->location().GetRect();
    painter.drawRect(rect);
    ui->minimapLabel->setPixmap(pixmap);
}

void MainWindow::ResetBuyoutWidgets() {
    ui->buyoutTypeComboBox->setCurrentIndex(0);
    ui->buyoutCurrencyComboBox->setEnabled(false);
    ui->buyoutValueLineEdit->setText("");
    ui->buyoutValueLineEdit->setEnabled(false);
}

void MainWindow::UpdateBuyoutWidgets(const Buyout &bo) {
    ui->buyoutCurrencyComboBox->setEnabled(true);
    ui->buyoutValueLineEdit->setEnabled(true);
    ui->buyoutTypeComboBox->setCurrentIndex(bo.type);
    ui->buyoutValueLineEdit->setText(QString::number(bo.value));

    if (ui->buyoutCurrencyComboBox->isVisible()) {
        ui->buyoutCurrencyComboBox->setCurrentIndex(bo.currency);
    }
    else {
        QString curr = QString::fromStdString(CurrencyAsTag.at(bo.currency));
        ui->buyoutValueLineEdit->setText(ui->buyoutValueLineEdit->text() + " " + curr);
    }
}

void MainWindow::UpdateCurrentBuyout() {
    if (current_item_) {
        if (!app_->buyout_manager().Exists(*current_item_))
            ResetBuyoutWidgets();
        else
            UpdateBuyoutWidgets(app_->buyout_manager().Get(*current_item_));
    } else {
        std::string tab = current_bucket_.location().GetUniqueHash();
        if (!app_->buyout_manager().ExistsTab(tab))
            ResetBuyoutWidgets();
        else
            UpdateBuyoutWidgets(app_->buyout_manager().GetTab(tab));
    }
}

void MainWindow::OnItemsRefreshed() {
    int tab = 0;
    for (auto search : searches_) {
        search->FilterItems(app_->items());
        tab_bar_->setTabText(tab++, search->GetCaption());
    }
    OnSearchFormChange();
}

MainWindow::~MainWindow() {
    delete ui;
#ifdef Q_OS_WIN32
    delete taskbar_button_;
#endif
}

void MainWindow::on_actionForum_shop_thread_triggered() {
    QString thread = QInputDialog::getText(this, "Shop thread", "Enter thread number", QLineEdit::Normal,
        app_->shop().thread().c_str());
    app_->shop().SetThread(thread.toStdString());
    UpdateShopMenu();
    UpdateSettingsBox();
}

void MainWindow::UpdateShopMenu() {
    std::string title = "Forum shop thread...";
    if (!app_->shop().thread().empty())
        title += " [" + app_->shop().thread() + "]";
    ui->actionForum_shop_thread->setText(title.c_str());
    ui->actionAutomatically_update_shop->setChecked(app_->shop().auto_update());
}

void MainWindow::UpdateOnlineGui() {
    online_label_.setVisible(auto_online_.enabled());
    ui->actionAutomatically_refresh_online_status->setChecked(auto_online_.enabled());
    std::string action_label = "control.poe.xyz.is URL...";
    if (auto_online_.IsUrlSet())
        action_label += " [******]";
    ui->actionControl_poe_xyz_is_URL->setText(action_label.c_str());
}

void MainWindow::OnUpdateAvailable() {
    update_button_.setText("An update package is available now");
    update_button_.show();
}

void MainWindow::OnOnlineUpdate(bool online) {
    if (online) {
        online_label_.setStyleSheet("color: green");
        online_label_.setText("Trade Status: Online");
    } else {
        online_label_.setStyleSheet("color: red");
        online_label_.setText("Trade Status: Offline");
    }
}

void MainWindow::on_actionCopy_shop_data_to_clipboard_triggered() {
    app_->shop().CopyToClipboard();
}

void MainWindow::on_actionItems_refresh_interval_triggered() {
    int interval = QInputDialog::getText(this, "Auto refresh items", "Refresh items every X minutes",
        QLineEdit::Normal, QString::number(app_->items_manager().auto_update_interval())).toInt();
    if (interval > 0)
        app_->items_manager().SetAutoUpdateInterval(interval);
}

void MainWindow::on_actionRefresh_triggered() {
    app_->items_manager().Update();
}

void MainWindow::on_actionAutomatically_refresh_items_triggered() {
    app_->items_manager().SetAutoUpdate(ui->actionAutomatically_refresh_items->isChecked());
}

void MainWindow::on_actionUpdate_shop_triggered() {
    app_->shop().SubmitShopToForum();
}

void MainWindow::on_actionShop_template_triggered() {
    bool ok;
    QString text = QInputDialog::getMultiLineText(this, "Shop template", "Enter shop template. [items] will be replaced with the list of items you marked for sale.",
        app_->shop().shop_template().c_str(), &ok);
    if (ok && !text.isEmpty())
        app_->shop().SetShopTemplate(text.toStdString());
}

void MainWindow::on_actionAutomatically_update_shop_triggered() {
    app_->shop().SetAutoUpdate(ui->actionAutomatically_update_shop->isChecked());
}

void MainWindow::on_actionControl_poe_xyz_is_URL_triggered() {
    bool ok;
    QString url = QInputDialog::getText(this, "control.poe.xyz.is URL",
        "Copy and paste your whole control.poe.xyz.is URL here",
        QLineEdit::Normal, "", &ok);
    if (ok && !url.isEmpty())
        auto_online_.SetUrl(url.toStdString());
    UpdateOnlineGui();
    UpdateSettingsBox();
}

void MainWindow::on_actionAutomatically_refresh_online_status_triggered() {
    auto_online_.SetEnabled(ui->actionAutomatically_refresh_online_status->isChecked());
    UpdateOnlineGui();
    UpdateSettingsBox();
}

void MainWindow::on_advancedSearchButton_toggled(bool checked) {
    ui->searchAreaWidget->setVisible(checked);
}

void MainWindow::on_darkThemeRadioButton_clicked() {
    light_palette_ = qApp->palette();
    light_style_ = qApp->styleSheet();

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15,15,15));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");


    this->setPalette(palette);
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(palette);

    qApp->setPalette(palette);
    app_->data_manager().SetBool("DarkTheme", true);
}

void MainWindow::on_lightThemeRadioButton_clicked() {
    qApp->setStyleSheet(light_style_);

    this->setPalette(light_palette_);
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(light_palette_);

    qApp->setPalette(light_palette_);
    app_->data_manager().SetBool("DarkTheme", false);
}

void MainWindow::UpdateSettingsBox() {
    // Items
    ui->refreshItemsBox->setChecked(app_->items_manager().auto_update());
    ui->refreshItemsIntervalBox->setValue(app_->items_manager().auto_update_interval());

    // Shop
    if (!app_->shop().thread().empty())
        ui->shopThreadIdBox->setValue(QString::fromStdString(app_->shop().thread()).toUInt());
    ui->updateShopBox->setChecked(app_->shop().auto_update());

    // Trade
    ui->refreshTradeBox->setChecked(auto_online_.enabled());
    ui->tradeURLLineEdit->setText(QString::fromStdString(auto_online_.GetUrl()));
    ui->showTradeURL->setChecked(false);
    ui->tradeURLLineEdit->setEchoMode(QLineEdit::Password);

    // Visual
    ui->darkThemeRadioButton->setChecked(app_->data_manager().GetBool("DarkTheme"));
    if (ui->darkThemeRadioButton->isChecked()) {
        on_darkThemeRadioButton_clicked();
    }
    bool flag = app_->data_manager().GetBool("ProcBuyoutStyle");
    ui->buyoutStyleBox->setChecked(flag);
    // Force update
    on_buyoutStyleBox_toggled(flag);

    flag = app_->data_manager().GetBool("ShowOldMenu");
    ui->showMenuBarBox->setChecked(flag);
    // Force update
    on_showMenuBarBox_toggled(flag);

}

void MainWindow::on_buyoutStyleBox_toggled(bool checked) {
    ui->buyoutCurrencyComboBox->setHidden(checked);
    app_->data_manager().SetBool("ProcBuyoutStyle", checked);
}

void MainWindow::on_showMenuBarBox_toggled(bool checked) {
    ui->menuBar->setVisible(checked);
}

void MainWindow::on_updateShopButton_clicked() {
    app_->shop().SubmitShopToForum();
}

void MainWindow::on_refreshItemsButton_clicked() {
    app_->items_manager().Update();

}

void MainWindow::on_showTradeURL_toggled(bool checked) {
    if (checked)
        ui->tradeURLLineEdit->setEchoMode(QLineEdit::Normal);
    else
        ui->tradeURLLineEdit->setEchoMode(QLineEdit::Password);
}

void MainWindow::on_refreshTradeBox_toggled(bool checked)
{
    auto_online_.SetEnabled(checked);
    QString url = ui->tradeURLLineEdit->text();
    if (!url.isEmpty())
        auto_online_.SetUrl(url.toStdString());
    UpdateOnlineGui();
}

void MainWindow::on_tradeURLLineEdit_editingFinished() {
    auto_online_.SetUrl(ui->tradeURLLineEdit->text().toStdString());
    UpdateOnlineGui();
}

void MainWindow::on_refreshItemsIntervalBox_editingFinished() {
    app_->items_manager().SetAutoUpdateInterval(ui->refreshItemsIntervalBox->value());
}

void MainWindow::on_refreshItemsBox_toggled(bool checked) {
    app_->items_manager().SetAutoUpdate(checked);
}

void MainWindow::on_shopThreadIdBox_editingFinished() {
    QString thread = QString::number(ui->shopThreadIdBox->value());
    app_->shop().SetThread(thread.toStdString());
    UpdateShopMenu();
}

void MainWindow::on_editShopTemplateButton_clicked() {
    on_actionShop_template_triggered();
}

void MainWindow::on_updateShopBox_toggled(bool checked) {
    app_->shop().SetAutoUpdate(checked);
}

void MainWindow::on_copyClipboardButton_clicked() {
    on_actionCopy_shop_data_to_clipboard_triggered();
}
