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
#include "jsoncpp/json.h"
#include "QsLog.h"

#include "mainwindow.h"
#include "datamanager.h"
#include "util.h"

const char *POE_STASH_ITEMS_URL = "https://www.pathofexile.com/character-window/get-stash-items";
const char *POE_ITEMS_URL = "https://www.pathofexile.com/character-window/get-items";
const char *POE_GET_CHARACTERS_URL = "https://www.pathofexile.com/character-window/get-characters";
const int DEFAULT_AUTO_UPDATE_INTERVAL = 30;

ItemsManager::ItemsManager(MainWindow *app):
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
    SetAutoUpdateInterval(DEFAULT_AUTO_UPDATE_INTERVAL);
    LoadSavedData();
    connect(auto_update_timer_, SIGNAL(timeout()), this, SLOT(OnAutoRefreshTimer()));
}

void ItemsManager::Update() {
    if (updating_) {
        QLOG_WARN() << "ItemsManager::Update called while updating";
        return;
    }
    updating_ = true;
    // remove all mappings (from previous requests)
    delete signal_mapper_;
    signal_mapper_ = new QSignalMapper;
    // remove all pending requests
    queue_ = std::queue<ItemsRequest>();
    queue_id_ = 0;
    replies_.clear();
    items_as_json_.clear();
    items_.clear();

    // first get character list
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
    ItemsRequest items_request;
    items_request.network_request = request;
    items_request.id = queue_id_++;
    items_request.location = location;
    queue_.push(items_request);
}

void ItemsManager::OnCharacterListReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    Json::Value root;
    Util::ParseJson(reply, &root);

    for (auto &character : root)
        if (character["league"].asString() == app_->league()) {
            std::string name = character["name"].asString();
            ItemLocation location;
            location.set_type(ItemLocationType::CHARACTER);
            location.set_character(name);
            QueueRequest(MakeCharacterRequest(name), location);
        }

    // now get first tab and tab list
    QNetworkReply *first_tab = app_->logged_in_nm()->get(MakeTabRequest(0, true));
    connect(first_tab, SIGNAL(finished()), this, SLOT(OnFirstTabReceived()));
    reply->deleteLater();
}

void ItemsManager::FetchItems(int limit) {
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
    }
    requests_needed_ = count;
    requests_completed_ = 0;
}

void ItemsManager::OnFirstTabReceived() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    Json::Value root;
    Util::ParseJson(reply, &root);

    int index = 0;
    if (!root.isObject()) {
        QLOG_ERROR() << "Can't even fetch first tab. Failed to update items.";
        updating_ = false;
        return;
    }
    tabs_.clear();
    tabs_as_json_ = root["tabs"];
    for (auto &tab : root["tabs"]) {
        std::string label = tab["n"].asString();
        tabs_.push_back(label);
        if (index > 0) {
            ItemLocation location;
            location.set_type(ItemLocationType::STASH);
            location.set_tab_id(index);
            location.set_tab_label(label);
            QueueRequest(MakeTabRequest(index), location);
        }
        ++index;
    }

    ItemLocation first_tab_location;
    first_tab_location.set_type(ItemLocationType::STASH);
    first_tab_location.set_tab_id(0);
    first_tab_location.set_tab_label(tabs_[0]);
    ParseItems(root["items"], first_tab_location);

    total_needed_ = queue_.size() + 1;
    total_completed_ = 1;
    FetchItems(THROTTLE_REQUESTS - 1);

    connect(signal_mapper_, SIGNAL(mapped(int)), this, SLOT(OnTabReceived(int)));
    reply->deleteLater();
}

void ItemsManager::ParseItems(const Json::Value &root, const ItemLocation &base_location) {
    for (auto item : root) {
        ItemLocation location(base_location);
        location.FromItemJson(item);
        location.ToItemJson(&item);
        items_as_json_.append(item);
        items_.push_back(std::make_shared<Item>(item));
        location.set_socketed(true);
        ParseItems(item["socketedItems"], location);
    }
}

void ItemsManager::LoadSavedData() {
    items_.clear();
    std::string items = app_->data_manager()->Get("items");
    if (items.size() != 0) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(items, root);
        for (auto &item : root)
            items_.push_back(std::make_shared<Item>(item));
    }

    tabs_.clear();
    std::string tabs = app_->data_manager()->Get("tabs");
    if (tabs.size() != 0) {
        Json::Value root;
        Json::Reader reader;
        reader.parse(tabs, root);
        for (auto &tab : root)
            tabs_.push_back(tab["n"].asString());
    }
    emit ItemsRefreshed(items_, tabs_);
}

void ItemsManager::OnTabReceived(int request_id) {
    if (!replies_.count(request_id)) {
        QLOG_WARN() << "Received a reply for request" << request_id << "that was not requested.";
        return;
    }

    ItemsReply reply = replies_[request_id];
    Json::Value root;
    Util::ParseJson(reply.network_reply, &root);

    if (root.isMember("error")) {
        // this can happen if user is browsing stash in background and we can't know about it
        QLOG_WARN() << request_id << "got 'error' instead of stash tab contents";
        QueueRequest(reply.request.network_request, reply.request.location);
        return;
    }

    ++requests_completed_;
    ++total_completed_;

    bool throttled = false;
    if (requests_completed_ == requests_needed_ && queue_.size() > 0) {
        throttled = true;
        QLOG_INFO() << "Sleeping one minute to prevent throttling.";
        QTimer::singleShot(THROTTLE_SLEEP * 1000, this, SLOT(FetchItems()));
    }
    emit StatusUpdate(total_completed_, total_needed_, throttled);

    ParseItems(root["items"], reply.request.location);

    if (total_completed_ == total_needed_) {
        // all requests completed
        emit ItemsRefreshed(items_, tabs_);

        Json::FastWriter writer;
        app_->data_manager()->Set("items", writer.write(items_as_json_));
        app_->data_manager()->Set("tabs", writer.write(tabs_as_json_));

        updating_ = false;
    }

    reply.network_reply->deleteLater();
}

void ItemsManager::SetAutoUpdate(bool update) {
    auto_update_ = update;
    if (!auto_update_)
        auto_update_timer_->stop();
    else
        // to start timer
        SetAutoUpdateInterval(auto_update_interval_);
}

void ItemsManager::SetAutoUpdateInterval(int minutes) {
    auto_update_interval_ = minutes;
    if (auto_update_)
        auto_update_timer_->start(auto_update_interval_ * 60 * 1000);
}

void ItemsManager::OnAutoRefreshTimer() {
    Update();
}
