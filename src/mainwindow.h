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

#pragma once

#include <memory>
#include <QMainWindow>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#include "column.h"
#include "items_model.h"
#include "search.h"
#include "imagecache.h"

class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

class DataManager;
class Filter;
class ItemsManager;
class BuyoutManager;
class Shop;
class FlowLayout;
class TabBuyoutsDialog;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent, QNetworkAccessManager *login_manager,
               const std::string &league, const std::string &email);
    ~MainWindow();
    std::vector<Column*> columns;
    const std::string &league() const { return league_; }
    const std::string &email() const { return email_; }
    const Items &items() const { return items_; }
    DataManager *data_manager() const { return data_manager_; }
    BuyoutManager *buyout_manager() const { return buyout_manager_; }
    QNetworkAccessManager *logged_in_nm() const { return logged_in_nm_; }
    const std::vector<std::string> &tabs() const { return tabs_; }
    Shop *shop() const { return shop_; }
public slots:
    void OnTreeChange(const QModelIndex &index, const QModelIndex &prev);
    void OnSearchFormChange();
    void OnTabChange(int index);
    void OnImageFetched(QNetworkReply *reply);
    void OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs);
    void OnItemsManagerStatusUpdate(int fetched, int total, bool throttled);
    void OnBuyoutChange();
    void ResizeTreeColumns();
private slots:
    void on_actionForum_shop_thread_triggered();
    void on_actionCopy_shop_data_to_clipboard_triggered();
    void on_actionTab_buyouts_triggered();
    void on_actionItems_refresh_interval_triggered();
    void on_actionRefresh_triggered();
    void on_actionAutomatically_refresh_items_triggered();
    void on_actionUpdate_shop_triggered();

private:
    void UpdateCurrentItem();
    void UpdateCurrentItemMinimap();
    void UpdateCurrentItemIcon(const QImage &image);
    void UpdateCurrentItemProperties();
    void UpdateCurrentItemBuyout();
    void NewSearch();
    void InitializeSearchForm();
    void InitializeUi();
    void AddSearchGroup(FlowLayout *layout, std::string name);
    bool eventFilter(QObject *o, QEvent *e);
    void UpdateShopMenu();
    Ui::MainWindow *ui;
    Items items_;
    std::vector<std::string> tabs_;
    std::shared_ptr<Item> current_item_;
    std::vector<Search*> searches_;
    Search *current_search_;
    QTabBar *tab_bar_;
    std::vector<Filter*> filters_;
    int search_count_;
    QNetworkAccessManager *image_network_manager_;
    ImageCache *image_cache_;
    ItemsManager *items_manager_;
    QLabel *status_bar_label_;
    DataManager *data_manager_;
    std::string league_;
    std::string email_;
    BuyoutManager *buyout_manager_;
    Shop *shop_;
    QNetworkAccessManager *logged_in_nm_;
    TabBuyoutsDialog *tab_buyouts_dialog_;
#ifdef Q_OS_WIN32
    QWinTaskbarButton *taskbar_button_;
#endif
};
