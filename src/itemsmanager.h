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

#include <QNetworkRequest>
#include <QObject>
#include <string>
#include <unordered_map>
#include <queue>
#include "jsoncpp/json.h"

#include "item.h"
#include "itemlocation.h"

const int THROTTLE_REQUESTS = 45;
const int THROTTLE_SLEEP = 60;

class QNetworkReply;
class QSignalMapper;
class QTimer;
class Application;

struct ItemsRequest {
    int id;
    QNetworkRequest network_request;
    ItemLocation location;
};

struct ItemsReply {
    QNetworkReply *network_reply;
    ItemsRequest request;
};

class ItemsManager : public QObject {
    Q_OBJECT
public:
    explicit ItemsManager(Application *app);
    ~ItemsManager();
    ItemsManager(const ItemsManager&) = delete;
    ItemsManager& operator=(const ItemsManager&) = delete;
    void Init();
    void Update();
    void LoadSavedData();
    void SetAutoUpdateInterval(int minutes);
    void SetAutoUpdate(bool update);
    int auto_update_interval() const { return auto_update_interval_; }
    bool auto_update() const { return auto_update_; }
public slots:
    void OnFirstTabReceived();
    void OnTabReceived(int index);
    void OnCharacterListReceived();
    /*
     * Makes 45 requests at once, should be called every minute.
     * These values are approximated (GGG throttles requests)
     * based on some quick testing.
     */
    void FetchItems(int limit = THROTTLE_REQUESTS);
    // called by auto_update_timer_
    void OnAutoRefreshTimer();
signals:
    void ItemsRefreshed(const Items &items, const std::vector<std::string> &tabs);
    void StatusUpdate(int fetched, int total, bool throttled);
private:
    void ParseItems(const Json::Value &root, const ItemLocation &location);

    QNetworkRequest MakeTabRequest(int tab_index, bool tabs=false);
    QNetworkRequest MakeCharacterRequest(const std::string &name);
    void QueueRequest(const QNetworkRequest &request, const ItemLocation &location);

    Application *app_;
    std::vector<std::string> tabs_;
    std::queue<ItemsRequest> queue_;
    std::map<int, ItemsReply> replies_;
    Items items_;
    int total_completed_, total_needed_;
    int requests_completed_, requests_needed_;
    QSignalMapper *signal_mapper_;
    Json::Value items_as_json_;
    Json::Value tabs_as_json_;
    // should items be automatically refreshed
    bool auto_update_;
    // items will be automatically updated every X minutes
    int auto_update_interval_;
    QTimer *auto_update_timer_;
    // set to true if updating right now
    bool updating_;
    int queue_id_;
};
