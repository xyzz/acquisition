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

#include <queue>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>

#include "item.h"
#include "mainwindow.h"

class Application;
class DataStore;
class QNetworkReply;
class QSignalMapper;
class QTimer;
class BuyoutManager;
class TabCache;

const int kThrottleRequests = 45;
const int kThrottleSleep = 60;
const int kMaxCacheSize = (10*1024*1024); // 10MB

struct ItemsRequest {
    int id;
    QNetworkRequest network_request;
    ItemLocation location;
};

struct ItemsReply {
    QNetworkReply *network_reply;
    ItemsRequest request;
};

class ItemsManagerWorker : public QObject {
    Q_OBJECT
public:
    ItemsManagerWorker(Application &app, QThread *thread);
    ~ItemsManagerWorker();
public slots:
    void Init();
    void Update(TabCache::Policy policy, const std::vector<ItemLocation> &tab_names = std::vector<ItemLocation>());
public slots:
    void OnMainPageReceived();
    void OnCharacterListReceived();
    void OnFirstTabReceived();
    void OnTabReceived(int index);
    /*
    * Makes 45 requests at once, should be called every minute.
    * These values are approximated (GGG throttles requests)
    * based on some quick testing.
    */
    void FetchItems(int limit = kThrottleRequests);
    void PreserveSelectedCharacter();
signals:
    void ItemsRefreshed(const Items &items, const std::vector<ItemLocation> &tabs, bool initial_refresh);
    void StatusUpdate(const CurrentStatusUpdate &status);
private:

    QNetworkRequest MakeTabRequest(int tab_index, const ItemLocation &location, bool tabs = false);
    QNetworkRequest MakeCharacterRequest(const std::string &name, const ItemLocation &location);
    void QueueRequest(const QNetworkRequest &request, const ItemLocation &location);
    void ParseItems(rapidjson::Value *value_ptr, const ItemLocation &base_location, rapidjson_allocator &alloc);

    QNetworkRequest Request(QUrl url, const ItemLocation &location, TabCache::Flags flags = TabCache::None);
    DataStore &data_;
    QNetworkAccessManager network_manager_;
    QSignalMapper *signal_mapper_;
    std::vector<ItemLocation> tabs_;
    std::queue<ItemsRequest> queue_;
    std::map<int, ItemsReply> replies_;
    Items items_;
    int total_completed_, total_needed_, total_cached_;
    int requests_completed_, requests_needed_;
    int cached_requests_completed_{0};
    
    std::string tabs_as_string_;
    std::string league_;
    // set to true if updating right now
    bool updating_;
    int queue_id_;
    std::string selected_character_;

    int first_fetch_tab_{1};
    TabCache *tab_cache_{new TabCache()};
    const BuyoutManager &bo_manager_;
    std::string account_name_;
};
