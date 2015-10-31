/*
    Copyright 2015 Ilya Zhuravlev

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

#include "testitemsmanager.h"

#include "rapidjson/document.h"

#include "buyoutmanager.h"
#include "item.h"
#include "itemsmanager.h"
#include "testdata.h"

void TestItemsManager::initTestCase() {
    auto null_nm = std::make_unique<QNetworkAccessManager>();
    null_nm->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    app_.InitLogin(std::move(null_nm), "TestLeague", "testuser", true);
}

void TestItemsManager::BuyoutPropagation() {
    rapidjson::Document doc;
    doc.Parse(kItem1.c_str());

    Items items = { std::make_shared<Item>(doc) };
    app_.items_manager().OnItemsRefreshed(items, {}, true);

    BuyoutManager &bo = app_.buyout_manager();
    Item &item = *items[0];
    QVERIFY2(!bo.Exists(item), "Buyout for a newly created item shouldn't exist.");
}
