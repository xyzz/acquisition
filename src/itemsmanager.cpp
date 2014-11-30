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

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSignalMapper>
#include <QTimer>
#include <QUrlQuery>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include "QsLog.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "application.h"
#include "datamanager.h"
#include "rapidjson_util.h"
#include "shop.h"
#include "util.h"

const char *POE_STASH_ITEMS_URL = "https://www.pathofexile.com/character-window/get-stash-items";
const char *POE_ITEMS_URL = "https://www.pathofexile.com/character-window/get-items";
const char *POE_GET_CHARACTERS_URL = "https://www.pathofexile.com/character-window/get-characters";

ItemsManager::ItemsManager(Application *app) :
    app_(app),
    signal_mapper_(new QSignalMapper),
    auto_update_(true),
    auto_update_timer_(new QTimer),
    updating_(false),
    queue_id_(0)
{
}

ItemsManager::~ItemsManager() {
    delete auto_update_timer_;
}

void ItemsManager::Init() {
    auto_update_interval_ = std::stoi(app_->data_manager()->Get("autoupdate_interval", "30"));
    auto_update_ = app_->data_manager()->GetBool("autoupdate", true);
    SetAutoUpdateInterval(auto_update_interval_);
    connect(auto_update_timer_, SIGNAL(timeout()), this, SLOT(OnAutoRefreshTimer()));
}

void ItemsManager::Update() {
    if (updating_) {
        QLOG_WARN() << "ItemsManager::Update called while updating";
        return;
    }
    QLOG_INFO() << "Updating stash tabs";
    updating_ = true;
    // remove all mappings (from previous requests)
    delete signal_mapper_;
    signal_mapper_ = new QSignalMapper;
    // remove all pending requests
    queue_ = std::queue<ItemsRequest>();
    queue_id_ = 0;
    replies_.clear();
    items_.clear();
    tabs_as_string_ = "";
    items_as_string_ = "[ "; // space here is important, see ParseItems and OnTabReceived when all requests are completed

    // first get character list
    if (!app_->logged_in_nm())
        return;
    QNetworkReply *characters = app_->logged_in_nm()->get(QNetworkRequest(QUrl(POE_GET_CHARACTERS_URL)));
    connect(characters, SIGNAL(finished()), this, SLOT(OnCharacterListReceived()));
}

QNetworkRequest ItemsManager::MakeTabRequest(int tab_index, bool tabs) {
    QUrlQuery query;
    query.addQueryItem("league", app_->league().c_str());
    query.addQueryItem("tabs", tabs ? "1" : "0");
    query.addQueryItem("tabIndex", QString::number(tab_index));

    QUrl url(POE_STASH_ITEMS_URL);
    url.setQuery(query);
    return QNetworkRequest(url);
}

QNetworkRequest ItemsManager::MakeCharacterRequest(const std::string &name) {
    QUrlQuery query;
    query.addQueryItem("character", name.c_str());

    QUrl url(POE_ITEMS_URL);
    url.setQuery(query);
    return QNetworkRequest(url);
}

void ItemsManager::QueueRequest(const QNetworkRequest &request, const ItemLocation &location) {
    QLOG_INFO() << "Queued" << location.GetHeader().c_str();
    ItemsRequest items_request;
    items_request.network_request = request;
    items_request.id = queue_id_++;
    items_request.location = location;
    queue_.push(items_request);
}

void ItemsManager::OnCharacterListReceived() {
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
        return;
    }

    QLOG_INFO() << "Received character list, there are" << doc.Size() << "characters";
    for (auto &character : doc) {
        if (!character.HasMember("league") || !character.HasMember("name") || !character["league"].IsString() || !character["name"].IsString()) {
            QLOG_ERROR() << "Malformed character entry, the reply is most likely invalid" << bytes.constData();
            continue;
        }
        if (character["league"].GetString() == app_->league()) {
            std::string name = character["name"].GetString();
            ItemLocation location;
            location.set_type(ItemLocationType::CHARACTER);
            location.set_character(name);
            QueueRequest(MakeCharacterRequest(name), location);
        }
    }

    // now get first tab and tab list
    QNetworkReply *first_tab = app_->logged_in_nm()->get(MakeTabRequest(0, true));
    connect(first_tab, SIGNAL(finished()), this, SLOT(OnFirstTabReceived()));
    reply->deleteLater();
}

void ItemsManager::FetchItems(int limit) {
    std::string tab_titles;
    int count = std::min(limit, static_cast<int>(queue_.size()));
    for (int i = 0; i < count; ++i) {
        ItemsRequest request = queue_.front();
        queue_.pop();

        QNetworkReply *fetched = app_->logged_in_nm()->get(request.network_request);
        signal_mapper_->setMapping(fetched, request.id);
        connect(fetched, SIGNAL(finished()), signal_mapper_, SLOT(map()));

        ItemsReply reply;
        reply.network_reply = fetched;
        reply.request = request;
        replies_[request.id] = reply;

        tab_titles += request.location.GetHeader() + " ";
    }
    QLOG_INFO() << "Created" << count << "requests:" << tab_titles.c_str();
    requests_needed_ = count;
    requests_completed_ = 0;
}

void ItemsManager::OnFirstTabReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    int index = 0;
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

    QLOG_INFO() << "Received tabs list, there are" << doc["tabs"].Size() << "tabs";
    tabs_.clear();
    for (auto &tab : doc["tabs"]) {
        std::string label = tab["n"].GetString();
        tabs_.push_back(label);
        if (index > 0) {
            ItemLocation location;
            location.set_type(ItemLocationType::STASH);
            location.set_tab_id(index);
            location.set_tab_label(label);
            if (!tab.HasMember("hidden") || !tab["hidden"].GetBool())
                QueueRequest(MakeTabRequest(index), location);
        }
        ++index;
    }

    ItemLocation first_tab_location;
    first_tab_location.set_type(ItemLocationType::STASH);
    first_tab_location.set_tab_id(0);
    first_tab_location.set_tab_label(tabs_[0]);
    if (!doc["tabs"][0].HasMember("hidden") || !doc["tabs"][0]["hidden"].GetBool())
        ParseItems(&doc["items"], first_tab_location, doc.GetAllocator());

    total_needed_ = queue_.size() + 1;
    total_completed_ = 1;
    FetchItems(THROTTLE_REQUESTS - 1);

    connect(signal_mapper_, SIGNAL(mapped(int)), this, SLOT(OnTabReceived(int)));
    reply->deleteLater();
}

void ItemsManager::ParseItems(rapidjson::Value *value_ptr, const ItemLocation &base_location, rapidjson_allocator &alloc) {
    auto &value = *value_ptr;
    for (auto &item : value) {
        ItemLocation location(base_location);
        location.FromItemJson(item);
        location.ToItemJson(&item, alloc);
        items_as_string_ += Util::RapidjsonSerialize(item) + ",";
        items_.push_back(std::make_shared<Item>(item));
        location.set_socketed(true);
        ParseItems(&item["socketedItems"], location, alloc);
    }
}

void ItemsManager::LoadSavedData() {
    items_.clear();
    std::string items = app_->data_manager()->Get("items");
    if (items.size() != 0) {
        rapidjson::Document doc;
        doc.Parse(items.c_str());
        for (auto item = doc.Begin(); item != doc.End(); ++item)
            items_.push_back(std::make_shared<Item>(*item));
    }

    tabs_.clear();
    std::string tabs = app_->data_manager()->Get("tabs");
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
            tabs_.push_back(tab["n"].GetString());
        }
    }
    emit ItemsRefreshed(items_, tabs_);
}

void ItemsManager::OnTabReceived(int request_id) {
    if (!replies_.count(request_id)) {
        QLOG_WARN() << "Received a reply for request" << request_id << "that was not requested.";
        return;
    }

    ItemsReply reply = replies_[request_id];
    QLOG_INFO() << "Received a reply for" << reply.request.location.GetHeader().c_str();
    QByteArray bytes = reply.network_reply->readAll();
    rapidjson::Document doc;
    doc.Parse(bytes.constData());

    bool error = false;
    if (doc.HasMember("error")) {
        // this can happen if user is browsing stash in background and we can't know about it
        QLOG_WARN() << request_id << "got 'error' instead of stash tab contents";
        QueueRequest(reply.request.network_request, reply.request.location);
        error = true;
    }

    ++requests_completed_;

    if (!error)
        ++total_completed_;

    bool throttled = false;
    if (requests_completed_ == requests_needed_ && queue_.size() > 0) {
        throttled = true;
        QLOG_INFO() << "Sleeping one minute to prevent throttling.";
        QTimer::singleShot(THROTTLE_SLEEP * 1000, this, SLOT(FetchItems()));
    }
    emit StatusUpdate(total_completed_, total_needed_, throttled);

    if (error)
        return;

    ParseItems(&doc["items"], reply.request.location, doc.GetAllocator());

    if (total_completed_ == total_needed_) {
        // all requests completed
        emit ItemsRefreshed(items_, tabs_);

        // since we build items_as_string_ in a hackish way inside ParseItems last character will either be
        // ' ' when no items were parsed or ',' when at least one item is parsed, and the first character is '['
        items_as_string_[items_as_string_.size() - 1] = ']';
        app_->data_manager()->Set("items", items_as_string_);
        app_->data_manager()->Set("tabs", tabs_as_string_);

        updating_ = false;
        QLOG_INFO() << "Finished updating stash.";

        if (app_->shop()->auto_update())
            app_->shop()->SubmitShopToForum();
    }

    reply.network_reply->deleteLater();
}

void ItemsManager::SetAutoUpdate(bool update) {
    app_->data_manager()->SetBool("autoupdate", update);
    auto_update_ = update;
    if (!auto_update_)
        auto_update_timer_->stop();
    else
        // to start timer
        SetAutoUpdateInterval(auto_update_interval_);
}

void ItemsManager::SetAutoUpdateInterval(int minutes) {
    app_->data_manager()->Set("autoupdate_interval", std::to_string(minutes));
    auto_update_interval_ = minutes;
    if (auto_update_)
        auto_update_timer_->start(auto_update_interval_ * 60 * 1000);
}

void ItemsManager::OnAutoRefreshTimer() {
    Update();
}
