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
#include <QStringList>
#include <QTabBar>
#include "jsoncpp/json.h"
#include "QsLog.h"

#include "application.h"
#include "buyoutmanager.h"
#include "column.h"
#include "filters.h"
#include "flowlayout.h"
#include "item.h"
#include "itemlocation.h"
#include "itemsmanager.h"
#include "porting.h"
#include "shop.h"
#include "util.h"

const std::string POE_WEBCDN = "http://webcdn.pathofexile.com";

MainWindow::MainWindow(Application *app):
    app_(app),
    ui(new Ui::MainWindow),
    current_search_(nullptr),
    search_count_(0)
{
#ifdef Q_OS_WIN32
    createWinId();
    taskbar_button_ = new QWinTaskbarButton(this);
    taskbar_button_->setWindow(this->windowHandle());
#endif
    image_cache_ = new ImageCache(this, porting::UserDir() + "/cache");

    InitializeUi();
    InitializeSearchForm();
    NewSearch();

    image_network_manager_ = new QNetworkAccessManager;
    connect(image_network_manager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(OnImageFetched(QNetworkReply*)));

    connect(app_->items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(OnItemsRefreshed()));
    connect(app_->items_manager(), SIGNAL(StatusUpdate(int, int, bool)),
        this, SLOT(OnItemsManagerStatusUpdate(int, int, bool)));
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

    ui->actionAutomatically_refresh_items->setChecked(app_->items_manager()->auto_update());
    UpdateShopMenu();
}

void MainWindow::ResizeTreeColumns() {
    for (int i = 0; i < ui->treeView->header()->count(); ++i)
        ui->treeView->resizeColumnToContents(i);
}

void MainWindow::OnBuyoutChange() {
    app_->shop()->ExpireShopData();

    Buyout bo;
    bo.type = static_cast<BuyoutType>(ui->buyoutTypeComboBox->currentIndex());
    bo.currency = static_cast<Currency>(ui->buyoutCurrencyComboBox->currentIndex());
    bo.value = ui->buyoutValueLineEdit->text().toDouble();

    if (bo.type == BUYOUT_TYPE_NONE) {
        ui->buyoutCurrencyComboBox->setEnabled(false);
        ui->buyoutValueLineEdit->setEnabled(false);
    } else {
        ui->buyoutCurrencyComboBox->setEnabled(true);
        ui->buyoutValueLineEdit->setEnabled(true);
    }

    if (current_item_) {
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager()->Delete(*current_item_);
        else
            app_->buyout_manager()->Set(*current_item_, bo);
    } else {
        std::string tab = current_bucket_.location().GetUniqueHash();
        if (bo.type == BUYOUT_TYPE_NONE)
            app_->buyout_manager()->DeleteTab(tab);
        else
            app_->buyout_manager()->SetTab(tab, bo);
    }
    // refresh treeView to immediately reflect price changes
    ui->treeView->model()->layoutChanged();
    ResizeTreeColumns();
}

void MainWindow::OnItemsManagerStatusUpdate(int fetched, int total, bool throttled) {
    QString status = QString("Receiving stash tabs, %1/%2").arg(fetched).arg(total);
    if (throttled)
        status += " (throttled, sleeping 60 seconds)";
    if (fetched == total)
        status = "Received all tabs";
    status_bar_label_->setText(status);

#ifdef Q_OS_WIN32
    QWinTaskbarProgress *progress = taskbar_button_->progress();
    progress->setVisible(fetched != total);
    progress->setMinimum(0);
    progress->setMaximum(total);
    progress->setValue(fetched);
    progress->setPaused(throttled);
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
                if (tab_bar_->currentIndex() == searches_.size())
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
        UpdateCurrentItemIcon(image);
}

void MainWindow::OnSearchFormChange() {
    app_->buyout_manager()->Save();
    current_search_->FromForm();
    current_search_->FilterItems(app_->items());
    ui->treeView->setModel(current_search_->model());
    connect(ui->treeView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(OnTreeChange(const QModelIndex&, const QModelIndex&)));
    ui->treeView->reset();

    // if it's connected during the call it's called for every item apparently, which is damn slow!
    disconnect(ui->treeView, SIGNAL(expanded(QModelIndex)), this, SLOT(ResizeTreeColumns()));
    disconnect(ui->treeView, SIGNAL(collapsed(QModelIndex)), this, SLOT(ResizeTreeColumns()));
    if (current_search_->items().size() <= MAX_EXPANDABLE_ITEMS)
        ui->treeView->expandAll();
    connect(ui->treeView, SIGNAL(expanded(QModelIndex)), this, SLOT(ResizeTreeColumns()));
    connect(ui->treeView, SIGNAL(collapsed(QModelIndex)), this, SLOT(ResizeTreeColumns()));

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
    if (index == searches_.size()) {
        NewSearch();
    } else {
        current_search_ = searches_[index];
        current_search_->ToForm();
    }
    OnSearchFormChange();
}

void MainWindow::AddSearchGroup(FlowLayout *layout, std::string name) {
    ui->searchFormLayout->addLayout(layout);
    QLabel *label = new QLabel(("<h3>" + name + "</h3>").c_str());
    label->setFixedWidth(Util::GroupWidth());
    layout->addWidget(label);
}

void MainWindow::InitializeSearchForm() {
    NameSearchFilter *name_search = new NameSearchFilter(ui->searchFormLayout);
    FlowLayout *offense_layout = new FlowLayout;
    FlowLayout *defense_layout = new FlowLayout;
    FlowLayout *sockets_layout = new FlowLayout;
    FlowLayout *requirements_layout = new FlowLayout;
    FlowLayout *misc_layout = new FlowLayout;

    AddSearchGroup(offense_layout, "Offense");
    AddSearchGroup(defense_layout, "Defense");
    AddSearchGroup(sockets_layout, "Sockets");
    AddSearchGroup(requirements_layout, "Reqs.");
    AddSearchGroup(misc_layout, "Misc");

    filters_ = {
        name_search,
        // Offense
        // new DamageFilter(offense_layout, "Damage"),
        new SimplePropertyFilter(offense_layout, "Critical Strike Chance", "Crit."),
        new ItemMethodFilter(offense_layout, [](Item* item) { return item->DPS(); }, "DPS"),
        new ItemMethodFilter(offense_layout, [](Item* item) { return item->pDPS(); }, "pDPS"),
        new ItemMethodFilter(offense_layout, [](Item* item) { return item->eDPS(); }, "eDPS"),
        new SimplePropertyFilter(offense_layout, "Attacks per Second", "APS"),
        // Defense
        new SimplePropertyFilter(defense_layout, "Armour"),
        new SimplePropertyFilter(defense_layout, "Evasion"),
        new SimplePropertyFilter(defense_layout, "Energy Shield", "Shield"),
        new SimplePropertyFilter(defense_layout, "Chance to Block", "Block"),
        // Sockets
        new SocketsFilter(sockets_layout, "Sockets"),
        new LinksFilter(sockets_layout, "Links"),
        new SocketsColorsFilter(sockets_layout),
        new LinksColorsFilter(sockets_layout),
        // Requirements
        new RequiredStatFilter(requirements_layout, "Level", "R. Level"),
        new RequiredStatFilter(requirements_layout, "Str", "R. Str"),
        new RequiredStatFilter(requirements_layout, "Dex", "R. Dex"),
        new RequiredStatFilter(requirements_layout, "Int", "R. Int"),
        // Misc
        new SimplePropertyFilter(misc_layout, "Quality"),
        new SimplePropertyFilter(misc_layout, "Level"),
        new MTXFilter(misc_layout, "", "MTX"),
        new AltartFilter(misc_layout, "", "Alt. art"),
    };
}

void MainWindow::NewSearch() {
    current_search_ = new Search(app_, QString("Search %1").arg(++search_count_).toStdString(), filters_);
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

    ui->nameLabel->setText(current_bucket_.location().GetHeader().c_str());
    ui->nameLabel->show();
}

void MainWindow::UpdateCurrentItem() {
    ui->typeLineLabel->show();
    ui->imageLabel->show();
    ui->minimapLabel->show();
    ui->locationLabel->show();
    ui->propertiesLabel->show();

    app_->buyout_manager()->Save();
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

    ui->locationLabel->setText(current_item_->location().GetHeader().c_str());
}

void MainWindow::UpdateCurrentItemProperties() {
    const auto &json = current_item_->json();
    std::vector<std::string> sections;

    std::string properties_text;
    bool first_prop = true;
    for (auto &property : json["properties"]) {
        if (!first_prop)
            properties_text += "<br>";
        first_prop = false;
        if (property["displayMode"].asInt() == 3) {
            QString format(property["name"].asString().c_str());
            for (int i = 0; i < property["values"].size(); ++i) {
                const auto &value = property["values"][i];
                format = format.arg(value[0].asString().c_str());
            }
            properties_text += format.toUtf8().constData();
        } else {
            std::string name = property["name"].asString();
            properties_text += name;
            if (property["values"].size() > 0) {
                if (name.size() > 0)
                    properties_text += ": ";
                bool first_val = true;
                for (auto &value : property["values"]) {
                    if (!first_val)
                        properties_text += ", ";
                    first_val = false;
                    properties_text += value[0].asString();
                }
            }
        }
    }
    if (properties_text.size() > 0)
        sections.push_back(properties_text);

    std::string requirements_text;
    bool first_req = true;
    for (auto &requirement : current_item_->json()["requirements"]) {
        if (!first_req)
            requirements_text += ", ";
        first_req = false;
        requirements_text += requirement["name"].asString() + ": " + requirement["values"][0][0].asString();
    }
    if (requirements_text.size() > 0)
        sections.push_back("Requires " + requirements_text);

    for (auto mod_type : { "implicitMods", "explicitMods", "craftedMods", "cosmeticMods" }) {
        std::string mod_list = Util::ModListAsString(current_item_->json()[mod_type]);
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
    QPixmap pixmap = QPixmap::fromImage(image);
    QPainter painter(&pixmap);

    QImage link_h(":/sockets/linkH.png");
    QImage link_v(":/sockets/linkV.png");

    auto &json = current_item_->json();
    for (int i = 0; i < json["sockets"].size(); ++i) {
        auto &socket = json["sockets"][i];
        bool link = (i > 0) && (socket["group"].asInt() == json["sockets"][i - 1]["group"].asInt());
        QImage socket_image(":/sockets/" + QString(socket["attr"].asString().c_str()) + ".png");
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
                    QLOG_ERROR() << "No idea how to draw link for" << current_item_->PrettyName().c_str();
                }
            }
        }
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
        if (!app_->buyout_manager()->Exists(*current_item_))
            ResetBuyoutWidgets();
        else
            UpdateBuyoutWidgets(app_->buyout_manager()->Get(*current_item_));
    } else {
        std::string tab = current_bucket_.location().GetUniqueHash();
        if (!app_->buyout_manager()->ExistsTab(tab))
            ResetBuyoutWidgets();
        else
            UpdateBuyoutWidgets(app_->buyout_manager()->GetTab(tab));
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
    app_->buyout_manager()->Save();
    delete ui;
    delete app_;
#ifdef Q_OS_WIN32
    delete taskbar_button_;
#endif
}

void MainWindow::on_actionForum_shop_thread_triggered() {
    QString thread = QInputDialog::getText(this, "Shop thread", "Enter thread number", QLineEdit::Normal,
        app_->shop()->thread().c_str());
    app_->shop()->SetThread(thread.toStdString());
    UpdateShopMenu();
}

void MainWindow::UpdateShopMenu() {
    std::string title = "Forum shop thread...";
    if (!app_->shop()->thread().empty())
        title += " [" + app_->shop()->thread() + "]";
    ui->actionForum_shop_thread->setText(title.c_str());
    ui->actionAutomatically_update_shop->setChecked(app_->shop()->auto_update());
}

void MainWindow::on_actionCopy_shop_data_to_clipboard_triggered() {
    app_->shop()->CopyToClipboard();
}

void MainWindow::on_actionItems_refresh_interval_triggered() {
    int interval = QInputDialog::getText(this, "Auto refresh items", "Refresh items every X minutes",
        QLineEdit::Normal, QString::number(app_->items_manager()->auto_update_interval())).toInt();
    if (interval > 0)
        app_->items_manager()->SetAutoUpdateInterval(interval);
}

void MainWindow::on_actionRefresh_triggered() {
    app_->items_manager()->Update();
}

void MainWindow::on_actionAutomatically_refresh_items_triggered() {
    app_->items_manager()->SetAutoUpdate(ui->actionAutomatically_refresh_items->isChecked());
}

void MainWindow::on_actionUpdate_shop_triggered() {
    app_->shop()->SubmitShopToForum();
}
