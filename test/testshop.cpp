#include "testshop.h"

#include <QNetworkAccessManager>
#include <memory>
#include "rapidjson/document.h"

#include "application.h"
#include "buyoutmanager.h"
#include "itemsmanager.h"
#include "porting.h"
#include "shop.h"
#include "testdata.h"


void TestShop::initTestCase() {
    auto null_nm = std::make_unique<QNetworkAccessManager>();
    null_nm->setNetworkAccessible(QNetworkAccessManager::NotAccessible);
    app_.InitLogin(std::move(null_nm), "TestLeague", "testuser", true);
}

void TestShop::SocketedGemsNotLinked() {
    rapidjson::Document doc;
    doc.Parse(kSocketedItem.c_str());

    Items items = { std::make_shared<Item>(doc) };
    app_.items_manager().OnItemsRefreshed(items, {}, true);

    Buyout bo;
    bo.type = BUYOUT_TYPE_FIXED;
    bo.value = 10;
    bo.currency = CURRENCY_CHAOS_ORB;
    app_.buyout_manager().Set(*items[0], bo);

    app_.shop().Update();
    std::vector<std::string> shop = app_.shop().shop_data();
    QVERIFY(shop.size() == 0);
}

void TestShop::TemplatedShopGeneration() {
    rapidjson::Document doc;
    doc.Parse(kItem1.c_str());

    Items items = { std::make_shared<Item>(doc) };
    app_.items_manager().OnItemsRefreshed(items, {}, true);

    Buyout bo;
    bo.type = BUYOUT_TYPE_FIXED;
    bo.value = 10;
    bo.currency = CURRENCY_CHAOS_ORB;
    app_.buyout_manager().Set(*items[0], bo);

    app_.shop().SetShopTemplate("My awesome shop [items]");
    app_.shop().Update();

    std::vector<std::string> shop = app_.shop().shop_data();
    QVERIFY(shop.size() == 1);
    QVERIFY(shop[0].find("~price") != std::string::npos);
    QVERIFY(shop[0].find("My awesome shop") != std::string::npos);
}
