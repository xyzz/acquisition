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
#include "itemsmanager.h"
#include "porting.h"
#include "shop.h"

Application::Application() :
    data_manager_(nullptr),
    buyout_manager_(nullptr),
    shop_(nullptr),
    logged_in_nm_(nullptr),
    items_manager_(nullptr)
{}

Application::~Application() {
    delete items_manager_;
    delete shop_;
    delete buyout_manager_;
    delete data_manager_;
    delete logged_in_nm_;
}

void Application::InitLogin(QNetworkAccessManager *login_manager, const std::string &league, const std::string &email) {
    league_ = league;
    email_ = email;
    logged_in_nm_ = login_manager;

    data_manager_ = new DataManager(this, porting::UserDir() + "/data");
    buyout_manager_ = new BuyoutManager(this);
    shop_ = new Shop(this);
    items_manager_ = new ItemsManager(this);
    items_manager_->Init();
    connect(items_manager_, SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(OnItemsRefreshed(Items, std::vector<std::string>)));

    items_manager_->LoadSavedData();
    items_manager_->Update();
}

void Application::OnItemsRefreshed(const Items &items, const std::vector<std::string> &tabs) {
    items_ = items;
    tabs_ = tabs;
    shop_->Update();
}