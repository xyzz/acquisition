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

#include "itemsmanagerworker.h"

#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QSignalMapper>
#include "QsLog.h"
#include <QTimer>
#include <QUrlQuery>
#include <algorithm>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "application.h"
#include "datastore.h"
#include "util.h"
#include "currencymanager.h"
#include "tabcache.h"
#include "mainwindow.h"
#include "buyoutmanager.h"
#include "filesystem.h"

const char *kStashItemsUrl = "https://www.pathofexile.com/character-window/get-stash-items";
const char *kCharacterItemsUrl = "https://www.pathofexile.com/character-window/get-items";
const char *kGetCharactersUrl = "https://www.pathofexile.com/character-window/get-characters";
const char *kMainPage = "https://www.pathofexile.com/";

ItemsManagerWorker::ItemsManagerWorker(Application &app, QThread *thread) :
    data_(app.data()),
    signal_mapper_(nullptr),
    league_(app.league()),
    updating_(false),
    bo_manager_(app.buyout_manager()),
    account_name_(app.email())
{
    QUrl poe(kMainPage);

    QDir cache_path{std::string{Filesystem::UserDir() + "/tabcache/" + account_name_ + "/" + league_}.c_str()};

    QLOG_DEBUG() << "Cache directory: " << cache_path.path();

    tab_cache_->setCacheDirectory(cache_path.path());
    tab_cache_->setMaximumCacheSize(kMaxCacheSize);

    // setCache takes ownership of tab_cache ptr so we don't need to destruct it
    network_manager_.setCache(tab_cache_);
    network_manager_.cookieJar()->setCookiesFromUrl(app.logged_in_nm().cookieJar()->cookiesForUrl(poe), poe);
    network_manager_.moveToThread(thread);

    connect(this, SIGNAL(ItemsRefreshed(Items, std::vector<ItemLocation>, bool)), tab_cache_, SLOT(OnItemsRefreshed()));
}

ItemsManagerWorker::~ItemsManagerWorker() {
    if (signal_mapper_)
        delete signal_mapper_;
}

void ItemsManagerWorker::Init() {
    items_.clear();
    std::string items = data_.Get("items");
    if (items.size() != 0) {
        rapidjson::Document doc;
        doc.Parse(items.c_str());
        for (auto item = doc.Begin(); item != doc.End(); ++item)
            items_.push_back(std::make_shared<Item>(*item));
    }

    tabs_.clear();
    std::string tabs = data_.Get("tabs");
    if (tabs.size() != 0) {
        rapidjson::Document doc;
        if (doc.Parse(tabs.c_str()).HasParseError()) {
            QLOG_ERROR() << "Malformed tabs data:" << tabs.c_str() << "The error was"
                << rapidjson::GetParseError_En(doc.GetParseError());
            return;
        }
        for (auto &tab : doc) {
            if (!tab.HasMember("n") || !tab["n"].IsString()) {
                QLOG_ERROR() << "Malformed tabs data:" << tabs.c_str() << "Tab doesn't contain its name (field 'n').";
                continue;
            }
            auto index = tab["i"].GetInt();
            // Index 0 is 'hidden' so we don't want to display
            if (index != 0)
                tabs_.push_back(ItemLocation(index, tab["n"].GetString()));
        }
    }
    emit ItemsRefreshed(items_, tabs_, true);
}

void ItemsManagerWorker::Update(TabCache::Policy policy, const std::vector<ItemLocation> &tab_names) {
    if (updating_) {
        QLOG_WARN() << "ItemsManagerWorker::Update called while updating";
        return;
    }

    if (policy == TabCache::ManualCache) {
        for (auto const &tab: tab_names)
            tab_cache_->AddManualRefresh(tab);
    }

    tab_cache_->OnPolicyUpdate(policy);

    QLOG_DEBUG() << "Updating stash tabs";
    updating_ = true;
    // remove all mappings (from previous requests)
    if (signal_mapper_)
        delete signal_mapper_;
    signal_mapper_ = new QSignalMapper;
    // remove all pending requests
    queue_ = std::queue<ItemsRequest>();
    queue_id_ = 0;
    replies_.clear();
    items_.clear();
    tabs_as_string_ = "";
    selected_character_ = "";

    // first, download the main page because it's the only way to know which character is selected
    QNetworkReply *main_page = network_manager_.get(Request(QUrl(kMainPage), ItemLocation(), TabCache::Refresh));
    connect(main_page, &QNetworkReply::finished, this, &ItemsManagerWorker::OnMainPageReceived);
}

void ItemsManagerWorker::OnMainPageReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    std::string page(reply->readAll().constData());

    selected_character_ = Util::FindTextBetween(page, "activeCharacter\":{\"name\":\"", "\",\"league");
    if (selected_character_.empty()) {
        QLOG_WARN() << "Can't extract selected character name from the page";
    }

    // now get character list
    QNetworkReply *characters = network_manager_.get(Request(QUrl(kGetCharactersUrl), ItemLocation(), TabCache::Refresh));
    connect(characters, &QNetworkReply::finished, this, &ItemsManagerWorker::OnCharacterListReceived);

    reply->deleteLater();
}

void ItemsManagerWorker::OnCharacterListReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    if (doc.HasParseError() || !doc.IsArray()) {
        QLOG_ERROR() << "Received invalid reply instead of character list. The reply was"
            << bytes.constData();
        if (doc.HasParseError()) {
            QLOG_ERROR() << "The error was" << rapidjson::GetParseError_En(doc.GetParseError());
        }
        updating_ = false;
        return;
    }

    QLOG_DEBUG() << "Received character list, there are" << doc.Size() << "characters";
    auto char_count = 0;
    for (auto &character : doc) {
        if (!character.HasMember("league") || !character.HasMember("name") || !character["league"].IsString() || !character["name"].IsString()) {
            QLOG_ERROR() << "Malformed character entry, the reply is most likely invalid" << bytes.constData();
            continue;
        }
        if (character["league"].GetString() == league_) {
            char_count++;
            std::string name = character["name"].GetString();
            ItemLocation location;
            location.set_type(ItemLocationType::CHARACTER);
            location.set_character(name);
            QueueRequest(MakeCharacterRequest(name, location), location);
        }
    }
    CurrentStatusUpdate status;
    status.state = ProgramState::CharactersReceived;
    status.total = char_count;

    emit StatusUpdate(status);

    if (char_count == 0) {
        updating_ = false;
        return;
    }

    // Fetch a single tab and also request tabs list.  We can fetch any tab here with tabs list
    // appended, so prefer one that the user has already 'checked'.  Default to index '1' which is
    // first user visible tab.
    first_fetch_tab_ = 1;
    for (auto const &tab : tabs_) {
        if (bo_manager_.GetRefreshChecked(tab)) {
            first_fetch_tab_ = tab.get_tab_id();
            break;
        }
    }

    QNetworkReply *first_tab = network_manager_.get(MakeTabRequest(first_fetch_tab_, ItemLocation(), true));
    connect(first_tab, SIGNAL(finished()), this, SLOT(OnFirstTabReceived()));
    reply->deleteLater();
}

QNetworkRequest ItemsManagerWorker::MakeTabRequest(int tab_index, const ItemLocation &location, bool tabs) {
    QUrlQuery query;
    query.addQueryItem("league", league_.c_str());
    query.addQueryItem("tabs", tabs ? "1" : "0");
    query.addQueryItem("tabIndex", QString::number(tab_index));
    query.addQueryItem("accountName", account_name_.c_str());

    QUrl url(kStashItemsUrl);
    url.setQuery(query);

    TabCache::Flags flags;
    if (tabs) flags |= TabCache::Refresh;

    if (!location.IsValid() || bo_manager_.GetRefreshChecked(location))
        flags |= TabCache::Refresh;

    return Request(url, location, flags);
}

QNetworkRequest ItemsManagerWorker::MakeCharacterRequest(const std::string &name, const ItemLocation &location) {
    QUrlQuery query;
    query.addQueryItem("character", name.c_str());
    query.addQueryItem("accountName", account_name_.c_str());

    QUrl url(kCharacterItemsUrl);
    url.setQuery(query);

    TabCache::Flags flags;
    if (!location.IsValid() || bo_manager_.GetRefreshChecked(location))
        flags |= TabCache::Refresh;

    return Request(url, location, flags);
}

QNetworkRequest ItemsManagerWorker::Request(QUrl url, const ItemLocation &location, TabCache::Flags flags) {
    return tab_cache_->Request(url, location, flags);
}

void ItemsManagerWorker::QueueRequest(const QNetworkRequest &request, const ItemLocation &location) {
    QLOG_DEBUG() << "Queued" << location.GetHeader().c_str();
    ItemsRequest items_request;
    items_request.network_request = request;
    items_request.id = queue_id_++;
    items_request.location = location;
    queue_.push(items_request);
}

void ItemsManagerWorker::FetchItems(int limit) {
    std::string tab_titles;
    int count = std::min(limit, static_cast<int>(queue_.size()));
    for (int i = 0; i < count; ++i) {
        ItemsRequest request = queue_.front();
        queue_.pop();

        QNetworkReply *fetched = network_manager_.get(request.network_request);
        signal_mapper_->setMapping(fetched, request.id);
        connect(fetched, SIGNAL(finished()), signal_mapper_, SLOT(map()));

        ItemsReply reply;
        reply.network_reply = fetched;
        reply.request = request;
        replies_[request.id] = reply;

        tab_titles += request.location.GetHeader() + " ";
    }
    QLOG_DEBUG() << "Created" << count << "requests:" << tab_titles.c_str();
    requests_needed_ = count;
    requests_completed_ = 0;
    cached_requests_completed_ = 0;
}

void ItemsManagerWorker::OnFirstTabReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    if (!doc.IsObject()) {
        QLOG_ERROR() << "Can't even fetch first tab. Failed to update items.";
        updating_ = false;
        return;
    }
    if (!doc.HasMember("tabs") || doc["tabs"].Size() == 0) {
        QLOG_WARN() << "There are no tabs, this should not happen, bailing out.";
        updating_ = false;
        return;
    }

    tabs_as_string_ = Util::RapidjsonSerialize(doc["tabs"]);

    QLOG_DEBUG() << "Received tabs list, there are" << doc["tabs"].Size() << "tabs";
    tabs_.clear();

    // Create tab location objects
    for (auto &tab : doc["tabs"]) {
        std::string label = tab["n"].GetString();
        auto index = tab["i"].GetInt();
        // Ignore hidden locations
        if (!doc["tabs"][index].HasMember("hidden") || !doc["tabs"][index]["hidden"].GetBool())
            tabs_.push_back(ItemLocation(index, label, ItemLocationType::STASH));
    }

    // Immediately parse items received from this tab (first_fetch_tab_) and Queue requests for the others
    for (auto const &tab: tabs_) {
        auto index = tab.get_tab_id();
        if (index == first_fetch_tab_) {
            ParseItems(&doc["items"], tab, doc.GetAllocator());
        } else {
            QueueRequest(MakeTabRequest(index, tab), tab);
        }
    }

    total_needed_ = queue_.size() + 1;
    total_completed_ = 1;
    total_cached_ = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool() ? 1:0;

    FetchItems(kThrottleRequests - 1);

    connect(signal_mapper_, SIGNAL(mapped(int)), this, SLOT(OnTabReceived(int)));
    reply->deleteLater();
}

void ItemsManagerWorker::ParseItems(rapidjson::Value *value_ptr, const ItemLocation &base_location, rapidjson_allocator &alloc) {
    auto &value = *value_ptr;
    for (auto &item : value) {
        ItemLocation location(base_location);
        location.FromItemJson(item);
        location.ToItemJson(&item, alloc);
        items_.push_back(std::make_shared<Item>(item));
        location.set_socketed(true);
        ParseItems(&item["socketedItems"], location, alloc);
    }
}

void ItemsManagerWorker::OnTabReceived(int request_id) {
    if (!replies_.count(request_id)) {
        QLOG_WARN() << "Received a reply for request" << request_id << "that was not requested.";
        return;
    }

    ItemsReply reply = replies_[request_id];

    bool cache_status = reply.network_reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute).toBool();

    if (cache_status) {
        QLOG_DEBUG() << "Received a cached reply for" << reply.request.location.GetHeader().c_str();
        ++cached_requests_completed_;
        ++total_cached_;
    } else {
        QLOG_DEBUG() << "Received a reply for" << reply.request.location.GetHeader().c_str();
    }

    QByteArray bytes = reply.network_reply->readAll();
    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    bool error = false;
    if (!doc.IsObject()) {
        QLOG_WARN() << request_id << "got a non-object response";
        error = true;
    } else if (doc.HasMember("error")) {
        // this can happen if user is browsing stash in background and we can't know about it
        QLOG_WARN() << request_id << "got 'error' instead of stash tab contents";
        error = true;
    }

    // re-queue a failed request
    if (error) {
        // We can 'cache' error response document so make sure we remove it
        // before reque
        tab_cache_->remove(reply.request.network_request.url());
        QueueRequest(reply.request.network_request, reply.request.location);
    }

    ++requests_completed_;

    if (!error)
        ++total_completed_;

    bool throttled = false;
    if (requests_completed_ == requests_needed_ && queue_.size() > 0) {
        if (cached_requests_completed_ > 0) {
            // We basically don't want cached requests to count against throttle limit
            // so if we did get any cached requests fetch up to that number without a
            // large delay
            QTimer::singleShot(1, [&]() { FetchItems(cached_requests_completed_); });
        } else {
            throttled = true;
            QLOG_DEBUG() << "Sleeping one minute to prevent throttling.";
            QTimer::singleShot(kThrottleSleep * 1000, this, SLOT(FetchItems()));
        }
    }
    CurrentStatusUpdate status = CurrentStatusUpdate();
    status.state = throttled ? ProgramState::ItemsPaused : ProgramState::ItemsReceive;
    status.progress = total_completed_;
    status.total = total_needed_;
    status.cached = total_cached_;
    if (total_completed_ == total_needed_)
        status.state = ProgramState::ItemsCompleted;
    emit StatusUpdate(status);

    if (error)
        return;

    ParseItems(&doc["items"], reply.request.location, doc.GetAllocator());

    if (total_completed_ == total_needed_) {
        // It's possible that we receive character vs stash tabs out of order, or users
        // move items around in a tab and we get them in a different order. For
        // consistency we want to present the tab data in a deterministic way to the rest
        // of the application.  Especially so we don't try to update shop when nothing actually
        // changed.  So sort items_ here before emitting and then generate
        // item list as strings.

        std::sort(begin(items_), end(items_), [](const std::shared_ptr<Item> &a, const std::shared_ptr<Item> &b){
            return *a < *b;
        });

        QStringList tmp;
        for (auto const &item: items_) {
            tmp.push_back(item->json().c_str());
        }
        auto items_as_string = std::string("[") + tmp.join(",").toStdString() + "]";

        // all requests completed
        emit ItemsRefreshed(items_, tabs_, false);

        // DataStore is thread safe so it's ok to call it here
        data_.Set("items", items_as_string);
        data_.Set("tabs", tabs_as_string_);

        updating_ = false;
        QLOG_DEBUG() << "Finished updating stash.";

        // if we're at the verge of getting throttled, sleep so we don't
        if (requests_completed_ == kThrottleRequests)
            QTimer::singleShot(kThrottleSleep, this, SLOT(PreserveSelectedCharacter()));
        else
            PreserveSelectedCharacter();
    }

    reply.network_reply->deleteLater();
}

void ItemsManagerWorker::PreserveSelectedCharacter() {
    if (selected_character_.empty())
        return;
    tab_cache_->OnPolicyUpdate(TabCache::DefaultCache);
    network_manager_.get(MakeCharacterRequest(selected_character_, ItemLocation()));
}
