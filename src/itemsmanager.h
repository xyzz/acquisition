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

#include <QTimer>
#include <memory>

#include "item.h"
#include "itemsmanagerworker.h"

struct CurrentStatusUpdate;
class QThread;
class Application;
class BuyoutManager;
class DataStore;
class ItemsManagerWorker;
class Shop;

/*
 * ItemsManager manages an ItemsManagerWorker (which lives in a separate thread)
 * and glues it to the rest of Acquisition.
 */
class ItemsManager : public QObject {
    Q_OBJECT
public:
    explicit ItemsManager(Application &app);
    ~ItemsManager();
    // Creates and starts the worker
    void Start();
    void Update();
    void SetAutoUpdateInterval(int minutes);
    void SetAutoUpdate(bool update);
    int auto_update_interval() const { return auto_update_interval_; }
    bool auto_update() const { return auto_update_; }
    const Items &items() const { return items_; }
    const std::vector<std::string> &tabs() const { return tabs_; }
    void PropagateTabBuyouts();
public slots:
    // called by auto_update_timer_
    void OnAutoRefreshTimer();
    // Used to glue Worker's signals to MainWindow
    void OnStatusUpdate(const CurrentStatusUpdate &status);
    void OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs, bool initial_refresh);
signals:
    void UpdateSignal();
    void ItemsRefreshed(bool initial_refresh);
    void StatusUpdate(const CurrentStatusUpdate &status);
private:
    void MigrateBuyouts();

    // should items be automatically refreshed
    bool auto_update_;
    // items will be automatically updated every X minutes
    int auto_update_interval_;
    std::unique_ptr<QTimer> auto_update_timer_;
    std::unique_ptr<ItemsManagerWorker> worker_;
    std::unique_ptr<QThread> thread_;
    DataStore &data_;
    BuyoutManager &bo_manager_;
    Shop &shop_;
    Application &app_;
    Items items_;
    std::vector<std::string> tabs_;
};
