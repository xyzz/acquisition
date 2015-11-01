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
#include "datastore.h"
#include "item.h"
#include "itemsmanager.h"
#include "testdata.h"

void TestItemsManager::initTestCase() {
    app_ = nullptr;
}

void TestItemsManager::init() {
    if (app_)
        app_->deleteLater();
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

// Tests moving an item without a buyout from a tab without a buyout to a tab with a buyout
// NoBo => Bo
void TestItemsManager::MoveItemNoBoToBo() {
    ItemLocation first_tab(1, "first");
    ItemLocation second_tab(2, "second");
    auto item_before = std::make_shared<Item>("First item", first_tab);
    auto item_after = std::make_shared<Item>("First item", second_tab);

    QVERIFY2(item_before->hash() == item_after->hash(), "Before/after must have equal hashes");

    auto &bo = app_->buyout_manager();
    Buyout tab_buyout(456.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, QDateTime::currentDateTime());
    bo.SetTab(second_tab.GetUniqueHash(), tab_buyout);

    std::vector<std::string> tabs = { "first", "second" };
    // Put item into the first tab
    app_->items_manager().OnItemsRefreshed({ item_before }, tabs, true);

    QVERIFY2(!bo.Exists(*item_before), "Before: the buyout must not exist");

    // Move item to the second tab
    app_->items_manager().OnItemsRefreshed({ item_after }, tabs, true);

    QVERIFY2(bo.Exists(*item_before), "After: the buyout must exist");
    auto buyout = bo.Get(*item_before);
    QVERIFY2(buyout.inherited == true, "After: the buyout must be inherited");
    buyout.inherited = false;
    QVERIFY2(buyout == tab_buyout, "After: the buyout must equal tab buyout");
}

// Bo => NoBo, check that the buyout is reset
void TestItemsManager::MoveItemBoToNoBo() {
    ItemLocation first_tab(1, "first");
    ItemLocation second_tab(2, "second");
    auto item_before = std::make_shared<Item>("First item", first_tab);
    auto item_after = std::make_shared<Item>("First item", second_tab);

    QVERIFY2(item_before->hash() == item_after->hash(), "Before/after must have equal hashes");

    auto &bo = app_->buyout_manager();
    Buyout tab_buyout(456.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, QDateTime::currentDateTime());
    bo.SetTab(first_tab.GetUniqueHash(), tab_buyout);

    std::vector<std::string> tabs = { "first", "second" };
    // Put item into the first tab
    app_->items_manager().OnItemsRefreshed({ item_before }, tabs, true);

    QVERIFY2(bo.Exists(*item_before), "Before: the buyout must exist");
    auto buyout = bo.Get(*item_before);
    QVERIFY2(buyout.inherited == true, "Before: the buyout must be inherited");
    buyout.inherited = false;
    QVERIFY2(buyout == tab_buyout, "Before: the buyout must equal tab buyout");

    // Move item to the second tab
    app_->items_manager().OnItemsRefreshed({ item_after }, tabs, true);

    QVERIFY2(!bo.Exists(*item_before), "After: the buyout must not exist");
}

// Bo => Bo, check that the buyout is updated
void TestItemsManager::MoveItemBoToBo() {
    ItemLocation first_tab(1, "first");
    ItemLocation second_tab(2, "second");
    auto item_before = std::make_shared<Item>("First item", first_tab);
    auto item_after = std::make_shared<Item>("First item", second_tab);

    QVERIFY2(item_before->hash() == item_after->hash(), "Before/after must have equal hashes");

    auto &bo = app_->buyout_manager();
    Buyout first_buyout(456.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CHAOS_ORB, QDateTime::currentDateTime());
    Buyout second_buyout(234.0, BUYOUT_TYPE_BUYOUT, CURRENCY_CARTOGRAPHERS_CHISEL, QDateTime::currentDateTime());
    bo.SetTab(first_tab.GetUniqueHash(), first_buyout);
    bo.SetTab(second_tab.GetUniqueHash(), second_buyout);

    std::vector<std::string> tabs = { "first", "second" };
    // Put item into the first tab
    app_->items_manager().OnItemsRefreshed({ item_before }, tabs, true);

    QVERIFY2(bo.Exists(*item_before), "Before: the buyout must exist");
    auto buyout = bo.Get(*item_before);
    QVERIFY2(buyout.inherited == true, "Before: the buyout must be inherited");
    buyout.inherited = false;
    QVERIFY2(buyout == first_buyout, "Before: the buyout must equal first tab buyout");

    // Move item to the second tab
    app_->items_manager().OnItemsRefreshed({ item_after }, tabs, true);

    QVERIFY2(bo.Exists(*item_before), "After: the buyout must exist");
    buyout = bo.Get(*item_before);
    QVERIFY2(buyout.inherited == true, "After: the buyout must be inherited");
    buyout.inherited = false;
    QVERIFY2(buyout == second_buyout, "After: the buyout must equal second tab buyout");
}

// Checks that buyouts are migrated properly after item hash was changed
void TestItemsManager::ItemHashMigration() {
    rapidjson::Document doc;
    doc.Parse(kItem1.c_str());

    Item item(doc);

    auto &bo = app_->buyout_manager();

    // very hacky way to set a buyout with old itemhash
    auto buyout = Buyout(1.23, BUYOUT_TYPE_BUYOUT, CURRENCY_ORB_OF_ALTERATION, QDateTime::fromTime_t(567));
    app_->data().Set("buyouts", "{\"5f083f2f5ceb10ed720bd4c1771ed09d\": {\"currency\": \"alt\", \"type\": \"b/o\", \"value\": 1.23, \"last_update\": 567}}");
    bo.Load();

    QVERIFY2(!bo.Exists(item), "Before migration: the buyout mustn't exist");
    // trigger buyout migration by refreshing itemsmanager
    app_->items_manager().OnItemsRefreshed({ std::make_shared<Item>(item) }, {}, true);

    QVERIFY2(bo.Exists(item), "After migration: the buyout must exist");
    auto buyout_from_mgr = bo.Get(item);
    QVERIFY2(buyout_from_mgr == buyout, "After migration: the buyout must match our data");
}
