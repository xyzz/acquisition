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

const QString POE_WEBCDN = "http://webcdn.pathofexile.com";

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

    connect(&app_->items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<QString>)),
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
    statusBar()->addWidget(status_bar_label_);
    ui->itemLayout->setAlignment(Qt::AlignTop);
    ui->itemLayout->setAlignment(ui->minimapLabel, Qt::AlignHCenter);
    ui->itemLayout->setAlignment(ui->typeLineLabel, Qt::AlignHCenter);
    ui->itemLayout->setAlignment(ui->nameLabel, Qt::AlignHCenter);
    ui->itemLayout->setAlignment(ui->imageLabel, Qt::AlignHCenter);
    ui->itemLayout->setAlignment(ui->locationLabel, Qt::AlignHCenter);

    tab_bar_ = new QTabBar;
    tab_bar_->installEventFilter(this);
    tab_bar_->setExpanding(false);
    ui->mainLayout->insertWidget(0, tab_bar_);
    tab_bar_->addTab("+");
    connect(tab_bar_, SIGNAL(currentChanged(int)), this, SLOT(OnTabChange(int)));

    Util::PopulateBuyoutTypeComboBox(ui->buyoutTypeComboBox);
    Util::PopulateBuyoutCurrencyComboBox(ui->buyoutCurrencyComboBox);

    connect(ui->buyoutCurrencyComboBox, SIGNAL(activated(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutTypeComboBox, SIGNAL(activated(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutValueLineEdit, SIGNAL(textEdited(QString)), this, SLOT(OnBuyoutChange()));

    ui->actionAutomatically_refresh_items->setChecked(app_->items_manager().auto_update());
    UpdateShopMenu();

    search_form_layout_ = new QVBoxLayout;
    search_form_layout_->setAlignment(Qt::AlignTop);
    search_form_layout_->setContentsMargins(0, 0, 0, 0);

    auto search_form_container = new QWidget;
    search_form_container->setLayout(search_form_layout_);

    auto scroll_area = new VerticalScrollArea;
    scroll_area->setFrameShape(QFrame::NoFrame);
    scroll_area->setWidgetResizable(true);
    scroll_area->setWidget(search_form_container);
    scroll_area->setMinimumWidth(150); // TODO(xyz): remove magic numbers
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->horizontalLayout_2->insertWidget(0, scroll_area);
    search_form_container->show();

    ui->horizontalLayout_2->setStretchFactor(scroll_area, 1);
    ui->horizontalLayout_2->setStretchFactor(ui->itemListAndSearchFormLayout, 4);
    ui->horizontalLayout_2->setStretchFactor(ui->itemLayout, 1);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    context_menu_.addAction("Expand All", this, SLOT(OnExpandAll()));
    context_menu_.addAction("Collapse All", this, SLOT(OnCollapseAll()));

    connect(ui->treeView, &QTreeView::customContextMenuRequested, [&](const QPoint &pos) {
        context_menu_.popup(ui->treeView->viewport()->mapToGlobal(pos));
    });

    statusBar()->addPermanentWidget(&online_label_);
    UpdateOnlineGui();

    update_button_.setStyleSheet("color: blue; font-weight: bold;");
    update_button_.setFlat(true);
    update_button_.hide();
    statusBar()->addPermanentWidget(&update_button_);
    connect(&update_button_, &QPushButton::clicked, [=](){
        UpdateChecker::AskUserToUpdate(this);
    });
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
    bo.currency = static_cast<Currency>(ui->buyoutCurrencyComboBox->currentIndex());
    bo.value = ui->buyoutValueLineEdit->text().replace(',', ".").toDouble();
    bo.last_update = QDateTime::currentDateTime();

    if (bo.type == BUYOUT_TYPE_NONE) {
        ui->buyoutCurrencyComboBox->setEnabled(false);
        ui->buyoutValueLineEdit->setEnabled(false);
    } else {
        ui->buyoutCurrencyComboBox->setEnabled(true);
        ui->buyoutValueLineEdit->setEnabled(true);
    }

    if (current_item_) {
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager().Delete(*current_item_);
        else
            app_->buyout_manager().Set(*current_item_, bo);
    } else {
        QString tab = current_bucket_.location().GetUniqueHash();
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager().DeleteTab(tab);
        else
            app_->buyout_manager().SetTab(tab, bo);
    }
    // refresh treeView to immediately reflect price changes
    ui->treeView->model()->layoutChanged();
    ResizeTreeColumns();
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
                tab_bar_->removeTab(index);
                delete searches_[index];
                searches_.erase(searches_.begin() + index);
                if (static_cast<size_t>(tab_bar_->currentIndex()) == searches_.size())
                    tab_bar_->setCurrentIndex(searches_.size() - 1);
                OnTabChange(tab_bar_->currentIndex());
                // that's because after removeTab text will be set to previous search's caption
                // which is because my way of dealing with "+" tab is hacky and should be replaced by something sane
                tab_bar_->setTabText(tab_bar_->count() - 1, "+");
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}

void MainWindow::OnImageFetched(QNetworkReply *reply) {
    QString url = reply->url().toString();
    if (reply->error()) {
        QLOG_WARN() << "Failed to download item image," << url;
        return;
    }
    QImageReader image_reader(reply);
    QImage image = image_reader.read();

    image_cache_->Set(url, image);

    if (current_item_ && (url == current_item_->icon() || url == POE_WEBCDN + current_item_->icon()))
        UpdateCurrentItemIcon(image);
}

void MainWindow::OnSearchFormChange() {
    app_->buyout_manager().Save();
    current_search_->Activate(app_->items(), ui->treeView);
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(OnTreeChange(const QModelIndex&, const QModelIndex&)));
    ui->treeView->reset();

    if (current_search_->items().size() <= MAX_EXPANDABLE_ITEMS)
        ExpandCollapse(TreeState::kExpand);
    ResizeTreeColumns();

    tab_bar_->setTabText(tab_bar_->currentIndex(), current_search_->GetCaption());
}

void MainWindow::OnTreeChange(const QModelIndex &current, const QModelIndex & /* previous */) {
    if (!current.parent().isValid()) {
        // clicked on a bucket
        current_item_ = nullptr;
        current_bucket_ = *current_search_->buckets()[current.row()];
        UpdateCurrentBucket();
    } else {
        current_item_ = current_search_->buckets()[current.parent().row()]->items()[current.row()];
        UpdateCurrentItem();
    }
    UpdateCurrentBuyout();
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

void MainWindow::AddSearchGroup(QLayout *layout, const QString &name="") {
    if (!name.isEmpty()) {
        auto label = new QLabel("<h3>" + name + "</h3>");
        search_form_layout_->addWidget(label);
    }
    layout->setContentsMargins(0, 0, 0, 0);
    auto layout_container = new QWidget;
    layout_container->setLayout(layout);
    search_form_layout_->addWidget(layout_container);
}

void MainWindow::InitializeSearchForm() {
    auto name_search = std::make_unique<NameSearchFilter>(search_form_layout_);
    auto offense_layout = new FlowLayout;
    auto defense_layout = new FlowLayout;
    auto sockets_layout = new FlowLayout;
    auto requirements_layout = new FlowLayout;
    auto misc_layout = new FlowLayout;
    auto misc_flags_layout = new FlowLayout;
    auto mods_layout = new QHBoxLayout;

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
}

void MainWindow::NewSearch() {
    current_search_ = new Search(app_->buyout_manager(), QString("Search %1").arg(++search_count_), filters_);
    tab_bar_->setTabText(tab_bar_->count() - 1, current_search_->GetCaption());
    tab_bar_->addTab("+");
    // this can't be done in ctor because it'll call OnSearchFormChange slot
    // and remove all previous search data
    current_search_->ResetForm();
    searches_.push_back(current_search_);
    OnSearchFormChange();
}

void MainWindow::UpdateCurrentBucket() {
    ui->typeLineLabel->hide();
    ui->imageLabel->hide();
    ui->minimapLabel->hide();
    ui->locationLabel->hide();
    ui->propertiesLabel->hide();

    ui->nameLabel->setText(current_bucket_.location().GetHeader());
    ui->nameLabel->show();
}

void MainWindow::UpdateCurrentItem() {
    ui->typeLineLabel->show();
    ui->imageLabel->show();
    ui->minimapLabel->show();
    ui->locationLabel->show();
    ui->propertiesLabel->show();

    app_->buyout_manager().Save();
    ui->typeLineLabel->setText(current_item_->typeLine());
    if (current_item_->name().isEmpty())
        ui->nameLabel->hide();
    else {
        ui->nameLabel->setText(current_item_->name());
        ui->nameLabel->show();
    }
    ui->imageLabel->setText("Loading...");
    ui->imageLabel->setStyleSheet("QLabel { background-color : rgb(12, 12, 43); color: white }");
    ui->imageLabel->setFixedSize(QSize(current_item_->w(), current_item_->h()) * PIXELS_PER_SLOT);

    UpdateCurrentItemProperties();
    UpdateCurrentItemMinimap();

    QString icon = current_item_->icon();
    if (icon.size() && icon[0] == '/')
        icon = POE_WEBCDN + icon;
    if (!image_cache_->Exists(icon))
        image_network_manager_->get(QNetworkRequest(QUrl(icon)));
    else
        UpdateCurrentItemIcon(image_cache_->Get(icon));

    ui->locationLabel->setText(current_item_->location().GetHeader());
}

void MainWindow::UpdateCurrentItemProperties() {
    std::vector<QString> sections;

    QString properties_text;
    bool first_prop = true;
    for (auto &property : current_item_->text_properties()) {
        if (!first_prop)
            properties_text += "<br>";
        first_prop = false;
        if (property.display_mode == 3) {
            QString format(property.name);
            for (auto &value : property.values)
                format = format.arg(value);
            properties_text += format;
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

    QString requirements_text;
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
        QString mod_list = Util::ModListAsString(mods.at(mod_type));
        if (!mod_list.isEmpty())
            sections.push_back(mod_list);
    }

    QString text;
    bool first = true;
    for (auto &s : sections) {
        if (!first)
            text += "<hr>";
        first = false;
        text += s;
    }
    ui->propertiesLabel->setText(text);
}

void MainWindow::UpdateCurrentItemIcon(const QImage &image) {
    QPixmap pixmap = QPixmap::fromImage(image);
    QPainter painter(&pixmap);

    static const QImage link_h(":/sockets/linkH.png");
    static const QImage link_v(":/sockets/linkV.png");
    ItemSocket prev = { 255, '-' };
    size_t i = 0;
    for (auto &socket : current_item_->text_sockets()) {
        bool link = socket.group == prev.group;
        QImage socket_image(":/sockets/" + QString(socket.attr) + ".png");
        if (current_item_->w() == 1) {
            painter.drawImage(0, PIXELS_PER_SLOT * i, socket_image);
            if (link)
                painter.drawImage(16, PIXELS_PER_SLOT * i - 19, link_v);
        } else /* w == 2 */ {
            int row = i / 2;
            int column = i % 2;
            if (row % 2 == 1)
                column = 1 - column;
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
                    QLOG_ERROR() << "No idea how to draw link for" << current_item_->PrettyName();
                }
            }
        }

        prev = socket;
        ++i;
    }

    ui->imageLabel->setPixmap(pixmap);
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
    ui->buyoutCurrencyComboBox->setCurrentIndex(bo.currency);
    ui->buyoutValueLineEdit->setText(QString::number(bo.value));
}

void MainWindow::UpdateCurrentBuyout() {
    if (current_item_) {
        if (!app_->buyout_manager().Exists(*current_item_))
            ResetBuyoutWidgets();
        else
            UpdateBuyoutWidgets(app_->buyout_manager().Get(*current_item_));
    } else {
        QString tab = current_bucket_.location().GetUniqueHash();
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
        app_->shop().thread());
    app_->shop().SetThread(thread);
    UpdateShopMenu();
}

void MainWindow::UpdateShopMenu() {
    QString title = "Forum shop thread...";
    if (!app_->shop().thread().isEmpty())
        title += " [" + app_->shop().thread() + "]";
    ui->actionForum_shop_thread->setText(title);
    ui->actionAutomatically_update_shop->setChecked(app_->shop().auto_update());
}

void MainWindow::UpdateOnlineGui() {
    online_label_.setVisible(auto_online_.enabled());
    ui->actionAutomatically_refresh_online_status->setChecked(auto_online_.enabled());
    QString action_label = "control.poe.xyz.is URL...";
    if (auto_online_.IsUrlSet())
        action_label += " [******]";
    ui->actionControl_poe_xyz_is_URL->setText(action_label);
}

void MainWindow::OnUpdateAvailable() {
    update_button_.setText("An update package is available now");
    update_button_.show();
}

void MainWindow::OnOnlineUpdate(bool online) {
    if (online) {
        online_label_.setStyleSheet("color: green");
        online_label_.setText("Online");
    } else {
        online_label_.setStyleSheet("color: red");
        online_label_.setText("Offline");
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
        app_->shop().shop_template(), &ok);
    if (ok && !text.isEmpty())
        app_->shop().SetShopTemplate(text);
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
        auto_online_.SetUrl(url);
    UpdateOnlineGui();
}

void MainWindow::on_actionAutomatically_refresh_online_status_triggered() {
    auto_online_.SetEnabled(ui->actionAutomatically_refresh_online_status->isChecked());
    UpdateOnlineGui();
}
