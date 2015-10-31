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
    app_ = nullptr;
}

void TestItemsManager::init() {
    if (app_)
        delete app_;
    app_ = new Application();
    auto null_nm = std::make_unique<QNetworkAccessManager>();
    null_nm->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    app_->InitLogin(std::move(null_nm), "TestLeague", "testuser", true);
}

void TestItemsManager::BuyoutForNewItem() {
    rapidjson::Document doc;
    doc.Parse(kItem1.c_str());

    Items items = { std::make_shared<Item>(doc) };
    app_->items_manager().OnItemsRefreshed(items, {}, true);

    BuyoutManager &bo = app_->buyout_manager();
    Item &item = *items[0];
    QVERIFY2(!bo.Exists(item), "Buyout for a newly created item shouldn't exist.");
}

// Tests that buyouts are propagated when a user sets tab buyout
void TestItemsManager::BuyoutPropagation() {
    ItemLocation first_tab(1, "first");
    ItemLocation second_tab(2, "second");
    auto first = std::make_shared<Item>("First item", first_tab);
    auto second = std::make_shared<Item>("Second item", second_tab);

    auto &bo = app_->buyout_manager();
    Buyout buyout(123.0, BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_ALTERATION, QDateTime::currentDateTime());
    bo.SetTab(first_tab.GetUniqueHash(), buyout);

    Items items = { first, second };
    std::vector<std::string> tabs = { "first", "second" };
    app_->items_manager().OnItemsRefreshed(items, tabs, true);

    QVERIFY2(bo.Exists(*first), "Item buyout for the first item must exist");
    QVERIFY2(!bo.Exists(*second), "Item buyout for the second item must not exist");

    Buyout from_mgr = bo.Get(*first);
    QVERIFY2(from_mgr.inherited == true, "Buyout for the first item must be marked as inherited");
    from_mgr.inherited = false;
    QVERIFY2(from_mgr == buyout, "Buyout for the first item must match the tab buyout");
}

// Tests that a user-set buyout has priority over a tab buyout
void TestItemsManager::UserSetBuyoutPropagation() {
    ItemLocation first_tab(1, "first");
    auto first = std::make_shared<Item>("First item", first_tab);
    auto second = std::make_shared<Item>("Second item", first_tab);

    auto &bo = app_->buyout_manager();
    Buyout item_buyout(123.0, BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_ALTERATION, QDateTime::currentDateTime());
    Buyout tab_buyout(456.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, QDateTime::currentDateTime());
    bo.Set(*first, item_buyout);
    bo.SetTab(first_tab.GetUniqueHash(), tab_buyout);

    Items items = { first, second };
    std::vector<std::string> tabs = { "first" };
    app_->items_manager().OnItemsRefreshed(items, tabs, true);

    QVERIFY2(bo.Exists(*first), "Item buyout for the first item must exist");
    QVERIFY2(bo.Exists(*second), "Item buyout for the second item must exist");

    Buyout from_mgr_first = bo.Get(*first);
    Buyout from_mgr_second = bo.Get(*second);
    QVERIFY2(from_mgr_first.inherited == false, "First item must have a non-inherited buyout");
    QVERIFY2(from_mgr_second.inherited == true, "Second item must have an inherited buyout");
    from_mgr_second.inherited = false;
    QVERIFY2(from_mgr_first == item_buyout, "First item must have its own buyout");
    QVERIFY2(from_mgr_second == tab_buyout, "Second item must have a tabwide buyout");
}
