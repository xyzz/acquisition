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

#include "itemsmanager.h"

#include <QThread>
#include <stdexcept>

#include "application.h"
#include "buyoutmanager.h"
#include "datastore.h"
#include "itemsmanagerworker.h"
#include "porting.h"
#include "rapidjson_util.h"
#include "shop.h"
#include "util.h"
#include "mainwindow.h"

ItemsManager::ItemsManager(Application &app) :
    auto_update_timer_(std::make_unique<QTimer>()),
    data_(app.data()),
    bo_manager_(app.buyout_manager()),
    shop_(app.shop()),
    app_(app)
{
    auto_update_interval_ = std::stoi(data_.Get("autoupdate_interval", "30"));
    auto_update_ = data_.GetBool("autoupdate", true);
    SetAutoUpdateInterval(auto_update_interval_);
    connect(auto_update_timer_.get(), SIGNAL(timeout()), this, SLOT(OnAutoRefreshTimer()));
}

ItemsManager::~ItemsManager() {
    thread_->quit();
    thread_->wait();
}

void ItemsManager::Start() {
    thread_ = std::make_unique<QThread>();
    worker_ = std::make_unique<ItemsManagerWorker>(app_, thread_.get());
    connect(thread_.get(), SIGNAL(started()), worker_.get(), SLOT(Init()));
    connect(this, SIGNAL(UpdateSignal()), worker_.get(), SLOT(Update()));
    connect(worker_.get(), &ItemsManagerWorker::StatusUpdate, this, &ItemsManager::OnStatusUpdate);
    connect(worker_.get(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>, bool)), this, SLOT(OnItemsRefreshed(Items, std::vector<std::string>, bool)));
    worker_->moveToThread(thread_.get());
    thread_->start();
}

void ItemsManager::OnStatusUpdate(const CurrentStatusUpdate &status) {
    emit StatusUpdate(status);
}

void ItemsManager::PropagateTabBuyouts() {
    for (auto &item_ptr : items_) {
        auto &item = *item_ptr;
        auto &bo = app_.buyout_manager();
        std::string hash = item.location().GetUniqueHash();
        bool item_bo_exists = bo.Exists(item);
        bool tab_bo_exists = bo.ExistsTab(hash);

        Buyout item_bo, tab_bo;
        if (item_bo_exists)
            item_bo = bo.Get(item);
        if (tab_bo_exists)
            tab_bo = bo.GetTab(hash);

        // The logic below is quite complicated and probably should be simplified.
        // One day.

        // We update inherited attribute here because it later gets copied to the item if
        // its buyout doesn't match exactly the tab buyout
        tab_bo.inherited = true;

        // Don't do anything to the item if it has a personal buyout set
        if (item_bo_exists && !item_bo.inherited)
            continue;
        // If item has an inherited buyout but tab is clear, delete it
        if (item_bo_exists && !tab_bo_exists) {
            bo.Delete(item);
        // If tab buyout exists and item's buyout doesn't match it, update it
        // Only update when it doesn't match to preserve last_update
        } else if (tab_bo_exists && (!item_bo_exists || item_bo != tab_bo)) {
            tab_bo.last_update = QDateTime::currentDateTime();
            bo.Set(item, tab_bo);
        }
    }
}

void ItemsManager::OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs, bool initial_refresh) {
    items_ = items;
    tabs_ = tabs;
    MigrateBuyouts();
    PropagateTabBuyouts();

    emit ItemsRefreshed(initial_refresh);
}

void ItemsManager::Update() {
    emit UpdateSignal();
}

void ItemsManager::SetAutoUpdate(bool update) {
    data_.SetBool("autoupdate", update);
    auto_update_ = update;
    if (!auto_update_)
        auto_update_timer_->stop();
    else
        // to start timer
        SetAutoUpdateInterval(auto_update_interval_);
}

void ItemsManager::SetAutoUpdateInterval(int minutes) {
    data_.Set("autoupdate_interval", std::to_string(minutes));
    auto_update_interval_ = minutes;
    if (auto_update_)
        auto_update_timer_->start(auto_update_interval_ * 60 * 1000);
}

void ItemsManager::OnAutoRefreshTimer() {
    Update();
}

void ItemsManager::MigrateBuyouts() {
    int db_version = data_.GetInt("db_version");
    // Don't migrate twice
    if (db_version == 2)
        return;
    for (auto &item : items_)
        bo_manager_.MigrateItem(*item);
    bo_manager_.Save();
    data_.SetInt("db_version", 2);
}
