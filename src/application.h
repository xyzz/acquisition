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

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>

#include "item.h"

class QNetworkReply;

class DataManager;
class ItemsManager;
class BuyoutManager;
class Shop;

class Application : QObject {
    Q_OBJECT
public:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    // Should be called by login dialog after login
    void InitLogin(std::unique_ptr<QNetworkAccessManager> login_manager, const QString &league, const QString &email);
    const QString &league() const { return league_; }
    const QString &email() const { return email_; }
    const Items &items() const { return items_; }
    ItemsManager &items_manager() { return *items_manager_; }
    DataManager &data_manager() const { return *data_manager_; }
    DataManager &sensitive_data_manager() const { return *sensitive_data_manager_; }
    BuyoutManager &buyout_manager() const { return *buyout_manager_; }
    QNetworkAccessManager &logged_in_nm() const { return *logged_in_nm_; }
    const std::vector<QString> &tabs() const { return tabs_; }
    Shop &shop() const { return *shop_; }
public slots:
    void OnItemsRefreshed(const Items &items, const std::vector<QString> &tabs);
private:
    Items items_;
    std::vector<QString> tabs_;
	QString league_;
    QString email_;
    std::unique_ptr<DataManager> data_manager_;
    // stores sensitive data that you'd rather not share, like control.poe.xyz.is secret URL
    std::unique_ptr<DataManager> sensitive_data_manager_;
    std::unique_ptr<BuyoutManager> buyout_manager_;
    std::unique_ptr<Shop> shop_;
    std::unique_ptr<QNetworkAccessManager> logged_in_nm_;
    std::unique_ptr<ItemsManager> items_manager_;
};