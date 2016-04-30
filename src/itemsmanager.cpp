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
    connect(this, SIGNAL(UpdateSignal(TabCache::Policy, const std::vector<ItemLocation> &)), worker_.get(), SLOT(Update(TabCache::Policy, const std::vector<ItemLocation> &)));
    connect(worker_.get(), &ItemsManagerWorker::StatusUpdate, this, &ItemsManager::OnStatusUpdate);
    connect(worker_.get(), SIGNAL(ItemsRefreshed(Items, std::vector<ItemLocation>, bool)), this, SLOT(OnItemsRefreshed(Items, std::vector<ItemLocation>, bool)));
    worker_->moveToThread(thread_.get());
    thread_->start();
}

void ItemsManager::OnStatusUpdate(const CurrentStatusUpdate &status) {
    emit StatusUpdate(status);
}

void ItemsManager::ApplyAutoTabBuyouts() {
    // Can handle everything related to auto-tab pricing here.
    // 1. First format we need to honor is ascendency pricing formats which is top priority and overrides other types
    // 2. Second priority is to honor manual user pricing
    // 3. Third priority it to apply pricing based on ideally user specified formats (doesn't exist yet)

    // Loop over all tabs, create buyout based on tab name which applies auto-pricing policies
    auto &bo = app_.buyout_manager();
    for (auto const &loc: bo_manager_.GetStashTabLocations()) {
        auto tab_label = loc.get_tab_label();
        Buyout buyout = bo.StringToBuyout(tab_label);
        if (buyout.IsActive()) {
            bo.SetTab(loc.GetUniqueHash(), buyout);
        }
    }

    // Need to compress tab buyouts here, as the tab names change we accumulate and save BO's
    // for tabs that no longer exist I think.
    bo.CompressTabBuyouts();
}

void ItemsManager::ApplyAutoItemBuyouts() {
    // Loop over all items, check for note field with pricing and apply
    auto &bo = app_.buyout_manager();
    for (auto const& item: items_) {
        auto const &note = item->note();
        if (!note.empty()) {
            Buyout buyout = bo.StringToBuyout(note);
            // This line may look confusing, buyout returns an active buyout if game
            // pricing was found or a default buyout (inherit) if it was not.
            // If there is a currently valid note we want to apply OR if
            // old note no longer is valid (so basically clear pricing)
            if (buyout.IsActive() || bo.Get(*item).IsGameSet()) {
                bo.Set(*item, buyout);
            }
        }
    }

    bo.CompressItemBuyouts(items_);
}

void ItemsManager::PropagateTabBuyouts() {
    auto &bo = app_.buyout_manager();
    bo.ClearRefreshLocks();
    for (auto &item_ptr : items_) {
        Item &item = *item_ptr;
        std::string hash = item.location().GetUniqueHash();
        auto item_bo = bo.Get(item);
        auto tab_bo = bo.GetTab(hash);

        if (item_bo.IsInherited()) {
            if (tab_bo.IsActive()) {
                // Any propagation from tab price to item price should include this bit set
                tab_bo.inherited = true;
                tab_bo.last_update = QDateTime::currentDateTime();
                bo.Set(item, tab_bo);
            } else {
                // This effectively 'clears' buyout by setting back to 'inherit' state.
                bo.Set(item, Buyout());
            }
        }

        // If any savable bo's are set on an item or the tab then lock
        // the refresh state.
        if (bo.Get(item).RequiresRefresh() || tab_bo.RequiresRefresh()) {
            bo.SetRefreshLocked(item.location());
        }
    }
}

void ItemsManager::OnItemsRefreshed(const Items &items, const std::vector<ItemLocation> &tabs, bool initial_refresh) {
    items_ = items;

    bo_manager_.SetStashTabLocations(tabs);
    MigrateBuyouts();
    ApplyAutoTabBuyouts();
    ApplyAutoItemBuyouts();
    PropagateTabBuyouts();

    emit ItemsRefreshed(initial_refresh);
}

void ItemsManager::Update(TabCache::Policy policy, const std::vector<ItemLocation> &locations) {
    emit UpdateSignal(policy, locations);
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
