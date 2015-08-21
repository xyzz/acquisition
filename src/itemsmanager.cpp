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

#include <QNetworkReply>
#include <QSignalMapper>
#include <QTimer>
#include <QUrlQuery>
#include <QTimer>
#include <QThread>
#include <iostream>
#include <stdexcept>
#include "QsLog.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include "application.h"
#include "datamanager.h"
#include "itemsmanagerworker.h"
#include "porting.h"
#include "rapidjson_util.h"
#include "shop.h"
#include "util.h"
#include "mainwindow.h"

ItemsManager::ItemsManager(Application &app) :
    auto_update_timer_(std::make_unique<QTimer>()),
    data_manager_(app.data_manager()),
    shop_(app.shop()),
    app_(app)
{
    auto_update_interval_ = std::stoi(data_manager_.Get("autoupdate_interval", "30"));
    auto_update_ = data_manager_.GetBool("autoupdate", true);
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
    connect(worker_.get(), &ItemsManagerWorker::SelectedCharacterUpdate, &app_, &Application::OnActiveCharacterChange);
    connect(worker_.get(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>, bool)), this, SLOT(OnItemsRefreshed(Items, std::vector<std::string>, bool)));
    worker_->moveToThread(thread_.get());
    thread_->start();
}

void ItemsManager::OnStatusUpdate(const CurrentStatusUpdate &status) {
    emit StatusUpdate(status);
}

void ItemsManager::OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs, bool initial_refresh) {
    emit ItemsRefreshed(items, tabs, initial_refresh);
}

void ItemsManager::Update() {
    emit UpdateSignal();
}

QString ItemsManager::GetObjectNote(const QString &hash) {
    return data_manager_.Get(QString("note_" + hash).toStdString()).c_str();
}

void ItemsManager::SetObjectNote(const QString &hash, const QString &notes) {
    if (hash.isEmpty()) return;
    data_manager_.Set(QString("note_" + hash).toStdString(), notes.toStdString());
}

void ItemsManager::SetAutoUpdate(bool update) {
    data_manager_.SetBool("autoupdate", update);
    auto_update_ = update;
    if (!auto_update_)
        auto_update_timer_->stop();
    else
        // to start timer
        SetAutoUpdateInterval(auto_update_interval_);
}

void ItemsManager::SetAutoUpdateInterval(int minutes) {
    data_manager_.Set("autoupdate_interval", std::to_string(minutes));
    auto_update_interval_ = minutes;
    if (auto_update_)
        auto_update_timer_->start(auto_update_interval_ * 60 * 1000);
}

void ItemsManager::OnAutoRefreshTimer() {
    Update();
}
