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

#include "item.h"
#include "column.h"
#include "flowlayout.h"
#include "filters.h"
#include "itemsmanager.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "shop.h"
#include "tabbuyoutsdialog.h"
#include "util.h"
#include "porting.h"

MainWindow::MainWindow(QWidget *parent, QNetworkAccessManager *login_manager,
                       const std::string &league, const std::string &email) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    current_search_(nullptr),
    search_count_(0),
    league_(league),
    email_(email),
    logged_in_nm_(login_manager),
    tab_buyouts_dialog_(new TabBuyoutsDialog(0, this))
{
#ifdef Q_OS_WIN32
    createWinId();
    taskbar_button_ = new QWinTaskbarButton(this);
    taskbar_button_->setWindow(this->windowHandle());
#endif
    std::string root_dir(porting::UserDir().toUtf8().constData());
    data_manager_ = new DataManager(this, root_dir + "/data");
    image_cache_ = new ImageCache(this, root_dir + "/cache");
    buyout_manager_ = new BuyoutManager(this);
    shop_ = new Shop(this);

    InitializeUi();
    InitializeSearchForm();
    NewSearch();

    image_network_manager_ = new QNetworkAccessManager;
    connect(image_network_manager_, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(OnImageFetched(QNetworkReply*)));

    items_manager_ = new ItemsManager(this);
    connect(items_manager_, SIGNAL(ItemsRefreshed(Items,std::vector<std::string>)),
            this, SLOT(OnItemsRefreshed(Items,std::vector<std::string>)));
    connect(items_manager_, SIGNAL(StatusUpdate(int, int, bool)),
            this, SLOT(OnItemsManagerStatusUpdate(int, int, bool)));
    items_manager_->Init();
    items_manager_->Update();
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

    connect(ui->buyoutCurrencyComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(OnBuyoutChange()));
    connect(ui->buyoutValueLineEdit, SIGNAL(textChanged(QString)), this, SLOT(OnBuyoutChange()));
}

void MainWindow::ResizeTreeColumns() {
    for (int i = 0; i < ui->treeView->header()->count(); ++i)
        ui->treeView->resizeColumnToContents(i);
}

void MainWindow::OnBuyoutChange() {
    shop_->ExpireShopData();
    Buyout bo;
    bo.type = static_cast<BuyoutType>(ui->buyoutTypeComboBox->currentIndex());
    bo.currency = static_cast<Currency>(ui->buyoutCurrencyComboBox->currentIndex());
    bo.value = ui->buyoutValueLineEdit->text().toDouble();
    if (bo.type == BUYOUT_TYPE_NONE) {
        buyout_manager_->Delete(*current_item_);
        ui->buyoutCurrencyComboBox->setEnabled(false);
        ui->buyoutValueLineEdit->setEnabled(false);
    } else {
        buyout_manager_->Set(*current_item_, bo);
        ui->buyoutCurrencyComboBox->setEnabled(true);
        ui->buyoutValueLineEdit->setEnabled(true);
    }
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
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(o, e);
}

void MainWindow::OnImageFetched(QNetworkReply *reply) {
    std::string url = reply->url().toString().toUtf8().constData();
    if (reply->error())
        return;
    QImageReader image_reader(reply);
    QImage image = image_reader.read();

    image_cache_->Set(url, image);

    if (url == current_item_->icon())
        UpdateCurrentItemIcon(image);
}

void MainWindow::OnSearchFormChange() {
    buyout_manager_->Save();
    current_search_->FromForm();
    current_search_->FilterItems(items_);
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
}

void MainWindow::OnTreeChange(const QModelIndex &current, const QModelIndex & /* previous */) {
    // clicked on a bucket
    if (!current.parent().isValid())
        return;
    current_item_ = current_search_->buckets()[current.parent().row()]->items()[current.row()];
    UpdateCurrentItem();
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
        new ItemMethodFilter(offense_layout, &Item::DPS, "DPS"),
        new ItemMethodFilter(offense_layout, &Item::pDPS, "pDPS"),
        new ItemMethodFilter(offense_layout, &Item::eDPS, "eDPS"),
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
    };
}

void MainWindow::NewSearch() {
    tab_bar_->setTabText(tab_bar_->count() - 1, QString("Search %1").arg(++search_count_));
    tab_bar_->addTab("+");
    current_search_ = new Search("Search 1", filters_);
    // this can't be done in ctor because it'll call OnSearchFormChange slot
    // and remove all previous search data
    current_search_->ResetForm();
    searches_.push_back(current_search_);
    OnSearchFormChange();
}

void MainWindow::UpdateCurrentItem() {
    buyout_manager_->Save();
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

    if (!image_cache_->Exists(current_item_->icon())) {
        image_network_manager_->get(QNetworkRequest(QUrl(current_item_->icon().c_str())));
    } else {
        UpdateCurrentItemIcon(image_cache_->Get(current_item_->icon()));
    }

    ui->locationLabel->setText(current_item_->location().GetHeader().c_str());

    UpdateCurrentItemBuyout();
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

    if (json["implicitMods"].size() > 0) {
        std::string implicit;
        bool first_impl = true;
        for (auto &mod : json["implicitMods"]) {
            if (!first_impl)
                implicit += "<br>";
            first_impl = false;
            implicit += mod.asString();
        }
        sections.push_back(implicit);
    }

    std::string explicit_text;
    for (auto mod : current_item_->explicitMods())
        explicit_text += mod + "<br>";
    if (explicit_text.size() > 0)
        sections.push_back(explicit_text);

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

void MainWindow::UpdateCurrentItemBuyout() {
    if (!buyout_manager_->Exists(*current_item_)) {
        ui->buyoutTypeComboBox->setCurrentIndex(0);
        ui->buyoutCurrencyComboBox->setEnabled(false);
        ui->buyoutValueLineEdit->setText("");
        ui->buyoutValueLineEdit->setEnabled(false);
    } else {
        ui->buyoutCurrencyComboBox->setEnabled(true);
        ui->buyoutValueLineEdit->setEnabled(true);
        Buyout buyout = buyout_manager_->Get(*current_item_);
        ui->buyoutTypeComboBox->setCurrentIndex(buyout.type);
        ui->buyoutCurrencyComboBox->setCurrentIndex(buyout.currency);
        ui->buyoutValueLineEdit->setText(QString::number(buyout.value));
    }
}

void MainWindow::OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs) {
    items_ = items;
    tabs_ = tabs;
    for (auto search : searches_)
        search->FilterItems(items_);
    OnSearchFormChange();
    shop_->Update();
}

MainWindow::~MainWindow() {
    buyout_manager_->Save();
    delete ui;
    delete data_manager_;
    delete items_manager_;
    delete buyout_manager_;
#ifdef Q_OS_WIN32
    delete taskbar_button_;
#endif
}

void MainWindow::on_actionForum_shop_thread_triggered() {
    QString thread = QInputDialog::getText(this, "[NOT IMPLEMENTED] Shop thread", "Enter thread number");
    shop_->SetThread(thread.toUtf8().constData());
}

void MainWindow::on_actionCopy_shop_data_to_clipboard_triggered() {
    shop_->CopyToClipboard();
}

void MainWindow::on_actionTab_buyouts_triggered() {
    tab_buyouts_dialog_->Populate();
    tab_buyouts_dialog_->show();
}

void MainWindow::on_actionItems_refresh_interval_triggered() {
    int interval = QInputDialog::getText(this, "Auto refresh items", "Refresh items every X minutes",
        QLineEdit::Normal, QString::number(items_manager_->auto_update_interval())).toInt();
    if (interval > 0)
        items_manager_->SetAutoUpdateInterval(interval);
}

void MainWindow::on_actionRefresh_triggered() {
    items_manager_->Update();
}

void MainWindow::on_actionAutomatically_refresh_items_triggered() {
    items_manager_->SetAutoUpdate(ui->actionAutomatically_refresh_items->isChecked());
}
