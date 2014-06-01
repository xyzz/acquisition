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

const int THROTTLE_REQUESTS = 45;
const int THROTTLE_SLEEP = 60;

class QNetworkReply;
class QSignalMapper;
class MainWindow;

class ItemsManager : public QObject {
    Q_OBJECT
public:
    explicit ItemsManager(MainWindow *app);
    void Init();
    void Update();
public slots:
    void OnFirstTabReceived();
    void OnTabReceived(int index);
    /*
     * Fetches 45 tabs at once, should be called every minute.
     * These values are approximated (GGG throttles requests)
     * based on some quick testing.
     */
    void FetchSomeTabs(int limit = THROTTLE_REQUESTS);
signals:
    void ItemsRefreshed(const Items &items, const std::vector<std::string> &tabs);
    void StatusUpdate(int fetched, int total, bool throttled);
private:
    void ParseItems(const Json::Value &root, int tab);
    void LoadSavedData();
    QNetworkRequest MakeRequest(int tab_index, bool tabs);

    MainWindow *app_;
    std::vector<std::string> tabs_;
    std::queue<int> tabs_queue_;
    std::map<int, QNetworkReply*> replies_;
    Items items_;
    int tabs_received_, tabs_needed_;
    int requests_completed_, requests_needed_;
    QSignalMapper *signal_mapper_;
    Json::Value items_as_json_;
    Json::Value tabs_as_json_;
};
