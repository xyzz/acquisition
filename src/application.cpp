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

#include "application.h"

#include <QNetworkAccessManager>

#include "buyoutmanager.h"
#include "datamanager.h"
#include "filesystem.h"
#include "itemsmanager.h"
#include "porting.h"
#include "shop.h"

Application::Application() {}

Application::~Application() {
    if (buyout_manager_)
        buyout_manager_->Save();
}

void Application::InitLogin(std::unique_ptr<QNetworkAccessManager> login_manager, const QString &league, const QString &email) {
    league_ = league;
    email_ = email;
    logged_in_nm_ = std::move(login_manager);

    QString data_file = DataManager::MakeFilename(email, league);
    data_manager_ = std::make_unique<DataManager>(Filesystem::UserDir() + "/data/" + data_file, "data_manager");
    sensitive_data_manager_ = std::make_unique<DataManager>(Filesystem::UserDir() + "/sensitive_data/" + data_file, "sensitive_data_manager");
    buyout_manager_ = std::make_unique<BuyoutManager>(*data_manager_);
    shop_ = std::make_unique<Shop>(*this);
    items_manager_ = std::make_unique<ItemsManager>(*this);
    connect(items_manager_.get(), SIGNAL(ItemsRefreshed(Items, std::vector<QString>)),
        this, SLOT(OnItemsRefreshed(Items, std::vector<QString>)));

    items_manager_->Start();
    items_manager_->Update();
}

void Application::OnItemsRefreshed(const Items &items, const std::vector<QString> &tabs) {
    items_ = items;
    tabs_ = tabs;
    shop_->Update();
}
