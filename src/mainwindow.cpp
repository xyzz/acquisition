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
#include "currencymanager.h"
#include "filesystem.h"
#include "filters.h"
#include "flowlayout.h"
#include "imagecache.h"
#include "item.h"
#include "itemlocation.h"
#include "itemtooltip.h"
#include "itemsmanager.h"
#include "logpanel.h"
#include "modsfilter.h"
#include "replytimeout.h"
#include "search.h"
#include "selfdestructingreply.h"
#include "shop.h"
#include "util.h"
#include "verticalscrollarea.h"

const std::string POE_WEBCDN = "http://webcdn.pathofexile.com";

MainWindow::MainWindow(std::unique_ptr<Application> app):
    app_(std::move(app)),
    ui(new Ui::MainWindow),
    current_search_(nullptr),
    search_count_(0),
    auto_online_(app_->data(), app_->sensitive_data()),
    network_manager_(new QNetworkAccessManager)
{
#ifdef Q_OS_WIN32
    createWinId();
    taskbar_button_ = new QWinTaskbarButton(this);
    taskbar_button_->setWindow(this->windowHandle());
#elif defined(Q_OS_LINUX)
    setWindowIcon(QIcon(":/icons/assets/icon.svg"));
#endif

    image_cache_ = new ImageCache(Filesystem::UserDir() + "/cache");

    InitializeUi();
    InitializeLogging();
    InitializeSearchForm();
    NewSearch();

    image_network_manager_ = new QNetworkAccessManager;
    connect(image_network_manager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(OnImageFetched(QNetworkReply*)));

    connect(&app_->items_manager(), &ItemsManager::ItemsRefreshed, this, &MainWindow::OnItemsRefreshed);
    connect(&app_->items_manager(), &ItemsManager::StatusUpdate, this, &MainWindow::OnStatusUpdate);
    connect(&app_->shop(), &Shop::StatusUpdate, this, &MainWindow::OnStatusUpdate);
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
    // resize columns when a tab is expanded/collapsed
    connect(ui->treeView, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(ResizeTreeColumns()));
    connect(ui->treeView, SIGNAL(expanded(const QModelIndex&)), this, SLOT(ResizeTreeColumns()));

    ui->propertiesLabel->setStyleSheet("QLabel { background-color: black; color: #7f7f7f; padding: 10px; font-size: 17px; }");
    ui->propertiesLabel->setFont(QFont("Fontin SmallCaps"));
    ui->itemNameFirstLine->setFont(QFont("Fontin SmallCaps"));
    ui->itemNameSecondLine->setFont(QFont("Fontin SmallCaps"));
    ui->itemNameFirstLine->setAlignment(Qt::AlignCenter);
    ui->itemNameSecondLine->setAlignment(Qt::AlignCenter);

    ui->itemTooltipWidget->hide();
    ui->uploadTooltipButton->hide();
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

    // Don't assign a zero buyout if nothing is entered in the value textbox
    if (ui->buyoutValueLineEdit->text().isEmpty() && (bo.type == BUYOUT_TYPE_BUYOUT || bo.type == BUYOUT_TYPE_FIXED))
        return;

    if (current_item_) {
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager().Delete(*current_item_);
        else
            app_->buyout_manager().Set(*current_item_, bo);
    } else {
        std::string tab = current_bucket_.location().GetUniqueHash();
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager().DeleteTab(tab);
        else
            app_->buyout_manager().SetTab(tab, bo);
    }
    app_->items_manager().PropagateTabBuyouts();
    // refresh treeView to immediately reflect price changes
    ui->treeView->model()->layoutChanged();
    ResizeTreeColumns();
}

void MainWindow::OnStatusUpdate(const CurrentStatusUpdate &status) {
    QString title;
    bool need_progress = false;
    switch (status.state) {
    case ProgramState::ItemsReceive:
    case ProgramState::ItemsPaused:
        title = QString("Receiving stash data, %1/%2").arg(status.progress).arg(status.total);
        if (status.state == ProgramState::ItemsPaused)
            title += " (throttled, sleeping 60 seconds)";
        need_progress = true;
        break;
    case ProgramState::ItemsCompleted:
        title = "Received all tabs";
        break;
    case ProgramState::ShopSubmitting:
        title = QString("Sending your shops to the forum, %1/%2").arg(status.progress).arg(status.total);
        need_progress = true;
        break;
    case ProgramState::ShopCompleted:
        title = QString("Shop threads updated");
        break;
    default:
        title = "Unknown";
    }

    status_bar_label_->setText(title);

#ifdef Q_OS_WIN32
    QWinTaskbarProgress *progress = taskbar_button_->progress();
    progress->setVisible(need_progress);
    progress->setMinimum(0);
    progress->setMaximum(status.total);
    progress->setValue(status.progress);
    progress->setPaused(status.state == ProgramState::ItemsPaused);
#else
    (void)need_progress;//Fix compilation warning(unused var on non-windows)
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
    std::string url = reply->url().toString().toStdString();
    if (reply->error()) {
        QLOG_WARN() << "Failed to download item image," << url.c_str();
        return;
    }
    QImageReader image_reader(reply);
    QImage image = image_reader.read();

    image_cache_->Set(url, image);

    if (current_item_ && (url == current_item_->icon() || url == POE_WEBCDN + current_item_->icon()))
        GenerateItemIcon(*current_item_, image, ui);
}

void MainWindow::OnSearchFormChange() {
    app_->buyout_manager().Save();
    current_search_->Activate(app_->items_manager().items(), ui->treeView);
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

void MainWindow::AddSearchGroup(QLayout *layout, const std::string &name="") {
    if (!name.empty()) {
        auto label = new QLabel(("<h3>" + name + "</h3>").c_str());
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
    current_search_ = new Search(app_->buyout_manager(), QString("Search %1").arg(++search_count_).toStdString(), filters_);
    tab_bar_->setTabText(tab_bar_->count() - 1, current_search_->GetCaption());
    tab_bar_->addTab("+");
    // this can't be done in ctor because it'll call OnSearchFormChange slot
    // and remove all previous search data
    current_search_->ResetForm();
    searches_.push_back(current_search_);
    OnSearchFormChange();
}

void MainWindow::UpdateCurrentBucket() {
    ui->imageLabel->hide();
    ui->minimapLabel->hide();
    ui->locationLabel->hide();
    ui->itemTooltipWidget->hide();
    ui->uploadTooltipButton->hide();

    ui->nameLabel->setText(current_bucket_.location().GetHeader().c_str());
    ui->nameLabel->show();
}

void MainWindow::UpdateCurrentItem() {
    app_->buyout_manager().Save();

    ui->imageLabel->show();
    ui->minimapLabel->show();
    ui->locationLabel->show();
    ui->itemTooltipWidget->show();
    ui->uploadTooltipButton->show();
    ui->nameLabel->hide();

    ui->imageLabel->setText("Loading...");
    ui->imageLabel->setStyleSheet("QLabel { background-color : rgb(12, 12, 43); color: white }");
    ui->imageLabel->setFixedSize(QSize(current_item_->w(), current_item_->h()) * PIXELS_PER_SLOT);

    // Everything except item image now lives in itemtooltip.cpp
    // in future should move everything tooltip-related there
    GenerateItemTooltip(*current_item_, ui);

    std::string icon = current_item_->icon();
    if (icon.size() && icon[0] == '/')
        icon = POE_WEBCDN + icon;
    if (!image_cache_->Exists(icon))
        image_network_manager_->get(QNetworkRequest(QUrl(icon.c_str())));
    else
        GenerateItemIcon(*current_item_, image_cache_->Get(icon), ui);

    ui->locationLabel->setText(current_item_->location().GetHeader().c_str());
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
        search->FilterItems(app_->items_manager().items());
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
    bool ok;
    QString thread = QInputDialog::getText(this, "Shop thread",
        "Enter thread number. You can enter multiple shops by separating them with a comma. More than one shop may be needed if you have a lot of items.",
        QLineEdit::Normal, Util::StringJoin(app_->shop().threads(), ",").c_str(), &ok);
    if (ok && !thread.isEmpty())
        app_->shop().SetThread(Util::StringSplit(thread.toStdString(), ','));
    UpdateShopMenu();
}

void MainWindow::UpdateShopMenu() {
    std::string title = "Forum shop thread...";
    if (!app_->shop().threads().empty())
        title += " [" + Util::StringJoin(app_->shop().threads(), ",") + "]";
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
    app_->shop().SubmitShopToForum(true);
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
}

void MainWindow::on_actionAutomatically_refresh_online_status_triggered() {
    auto_online_.SetEnabled(ui->actionAutomatically_refresh_online_status->isChecked());
    UpdateOnlineGui();
}

void MainWindow::on_actionList_currency_triggered() {
    app_->currency_manager().DisplayCurrency();
}
void MainWindow::on_actionExport_currency_triggered() {
    app_->currency_manager().ExportCurrency();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    auto_online_.SendOnlineUpdate(false);
}

void MainWindow::on_uploadTooltipButton_clicked() {
    ui->uploadTooltipButton->setDisabled(true);
    ui->uploadTooltipButton->setText("Uploading...");

    QPixmap pixmap(ui->itemTooltipWidget->size());
    ui->itemTooltipWidget->render(&pixmap);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format

    QNetworkRequest request(QUrl("https://api.imgur.com/3/upload/"));
    request.setRawHeader("Authorization", "Client-ID d6d2d8a0437a90f");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray data = "image=" + QUrl::toPercentEncoding(bytes.toBase64());
    QNetworkReply *reply = network_manager_->post(request, data);
    new QReplyTimeout(reply, kImgurUploadTimeout);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::OnUploadFinished);
}

void MainWindow::OnUploadFinished() {
    ui->uploadTooltipButton->setDisabled(false);
    ui->uploadTooltipButton->setText("Upload to imgur");

    SelfDestructingReply reply(qobject_cast<QNetworkReply *>(QObject::sender()));
    QByteArray bytes = reply->readAll();

    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    if (doc.HasParseError() || !doc.IsObject() || !doc.HasMember("status") || !doc["status"].IsNumber()) {
        QLOG_ERROR() << "Imgur API returned invalid data (or timed out): " << bytes;
        reply->deleteLater();
        return;
    }
    if (doc["status"].GetInt() != 200) {
        QLOG_ERROR() << "Imgur API returned status!=200: " << bytes;
        reply->deleteLater();
        return;
    }
    if (!doc.HasMember("data") || !doc["data"].HasMember("link") || !doc["data"]["link"].IsString()) {
        QLOG_ERROR() << "Imgur API returned malformed reply: " << bytes;
        reply->deleteLater();
        return;
    }
    std::string url = doc["data"]["link"].GetString();
    QApplication::clipboard()->setText(url.c_str());
    QLOG_INFO() << "Image successfully uploaded, the URL is" << url.c_str() << "It also was copied to your clipboard.";

    reply->deleteLater();
}
