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
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QCloseEvent>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#include "autoonline.h"
#include "bucket.h"
#include "items_model.h"
#include "porting.h"
#include "updatechecker.h"


class QNetworkAccessManager;
class QNetworkReply;
class QVBoxLayout;

class Application;
class Column;
class Filter;
class FlowLayout;
class ImageCache;
class Search;

struct Buyout;

namespace Ui {
class MainWindow;
}

enum class TreeState {
    kExpand,
    kCollapse
};

enum class ProgramState {
    ItemsReceive,
    ItemsPaused,
    ItemsCompleted,
    ShopSubmitting,
    ShopCompleted
};

struct CurrentStatusUpdate {
    ProgramState state;
    int progress, total;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(std::unique_ptr<Application> app);
    ~MainWindow();
    std::vector<Column*> columns;
public slots:
    void OnTreeChange(const QModelIndex &index, const QModelIndex &prev);
    void OnSearchFormChange();
    void OnTabChange(int index);
    void OnImageFetched(QNetworkReply *reply);
    void OnItemsRefreshed();
    void OnStatusUpdate(const CurrentStatusUpdate &status);
    void OnBuyoutChange();
    void ResizeTreeColumns();
    void OnExpandAll();
    void OnCollapseAll();
    void OnUpdateAvailable();
    void OnOnlineUpdate(bool online);
    void OnUploadFinished();
private slots:
    void on_actionForum_shop_thread_triggered();
    void on_actionCopy_shop_data_to_clipboard_triggered();
    void on_actionItems_refresh_interval_triggered();
    void on_actionRefresh_triggered();
    void on_actionAutomatically_refresh_items_triggered();
    void on_actionUpdate_shop_triggered();
    void on_actionShop_template_triggered();
    void on_actionAutomatically_update_shop_triggered();
    void on_actionControl_poe_xyz_is_URL_triggered();
    void on_actionAutomatically_refresh_online_status_triggered();
    void on_actionList_currency_triggered();
    void on_actionExport_currency_triggered();
    void on_uploadTooltipButton_clicked();

private:
    void UpdateCurrentBucket();
    void UpdateCurrentItem();
    void UpdateCurrentBuyout();
    void NewSearch();
    void InitializeLogging();
    void InitializeSearchForm();
    void InitializeUi();
    void AddSearchGroup(QLayout *layout, const std::string &name);
    bool eventFilter(QObject *o, QEvent *e);
    void UpdateShopMenu();
    void ResetBuyoutWidgets();
    void UpdateBuyoutWidgets(const Buyout &bo);
    void ExpandCollapse(TreeState state);
    void UpdateOnlineGui();
    void closeEvent(QCloseEvent*);

    std::unique_ptr<Application> app_;
    Ui::MainWindow *ui;
    std::shared_ptr<Item> current_item_;
    Bucket current_bucket_;
    std::vector<Search*> searches_;
    Search *current_search_;
    QTabBar *tab_bar_;
    std::vector<std::unique_ptr<Filter>> filters_;
    int search_count_;
    QNetworkAccessManager *image_network_manager_;
    ImageCache *image_cache_;
    QLabel *status_bar_label_;
    QVBoxLayout *search_form_layout_;
    QMenu context_menu_;
    UpdateChecker update_checker_;
    QPushButton update_button_;
    AutoOnline auto_online_;
    QLabel online_label_;
    QNetworkAccessManager *network_manager_;
#ifdef Q_OS_WIN32
    QWinTaskbarButton *taskbar_button_;
#endif
};
