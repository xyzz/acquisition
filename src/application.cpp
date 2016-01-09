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
#include "sqlitedatastore.h"
#include "memorydatastore.h"
#include "filesystem.h"
#include "itemsmanager.h"
#include "currencymanager.h"
#include "porting.h"
#include "shop.h"

Application::Application() {}

Application::~Application() {
    if (buyout_manager_)
        buyout_manager_->Save();
}

void Application::InitLogin(std::unique_ptr<QNetworkAccessManager> login_manager, const std::string &league, const std::string &email,
        bool mock_data) {
    league_ = league;
    email_ = email;
    logged_in_nm_ = std::move(login_manager);

    if (mock_data) {
        // This is used in tests
        data_ = std::make_unique<MemoryDataStore>();
        sensitive_data_ = std::make_unique<MemoryDataStore>();
    } else {
        std::string data_file = SqliteDataStore::MakeFilename(email, league);
        data_ = std::make_unique<SqliteDataStore>(Filesystem::UserDir() + "/data/" + data_file);
        sensitive_data_ = std::make_unique<SqliteDataStore>(Filesystem::UserDir() + "/sensitive_data/" + data_file);
    }

    buyout_manager_ = std::make_unique<BuyoutManager>(*data_);
    shop_ = std::make_unique<Shop>(*this);
    items_manager_ = std::make_unique<ItemsManager>(*this);
    currency_manager_ = std::make_unique<CurrencyManager>(*this);
    connect(items_manager_.get(), SIGNAL(ItemsRefreshed(bool)), this, SLOT(OnItemsRefreshed(bool)));
    items_manager_->Start();
    items_manager_->Update();
}

void Application::OnItemsRefreshed(bool initial_refresh) {
    currency_manager_->Update();
    shop_->Update();
    if (!initial_refresh && shop_->auto_update())
        shop_->SubmitShopToForum();
}
