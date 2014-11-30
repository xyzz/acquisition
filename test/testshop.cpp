#include "test/testshop.h"

#include "rapidjson/document.h"

#include "application.h"
#include "buyoutmanager.h"
#include "itemsmanager.h"
#include "shop.h"

const std::string item_string =
   "{\"verified\":false,\"w\":2,\"h\":2,\"icon\":\"http://webcdn.pathofexile.com/image/Art/2DItems/Armours/Helmets/Helm"
   "etStrDex10.png?scale=1&w=2&h=2&v=0a540f285248cdb64d4607186e348e3d3\",\"support\":true,\"league\":\"Rampage\",\"sock"
   "ets\":[{\"group\":0,\"attr\":\"D\"}],\"name\":\"Demon Ward\",\"typeLine\":\"Nightmare Bascinet\",\"identified\":tru"
   "e,\"corrupted\":false,\"properties\":[{\"name\":\"Quality\",\"values\":[[\"+20%\",1]],\"displayMode\":0},{\"name\":"
   "\"Armour\",\"values\":[[\"310\",1]],\"displayMode\":0},{\"name\":\"Evasion Rating\",\"values\":[[\"447\",1]],\"disp"
   "layMode\":0}],\"requirements\":[{\"name\":\"Level\",\"values\":[[\"67\",0]],\"displayMode\":0},{\"name\":\"Str\",\""
   "values\":[[\"62\",0]],\"displayMode\":1},{\"name\":\"Dex\",\"values\":[[\"85\",0]],\"displayMode\":1}],\"explicitMo"
   "ds\":[\"+93 to Accuracy Rating\",\"100% increased Armour and Evasion\",\"+24% to Fire Resistance\",\"+32% to Lightn"
   "ing Resistance\",\"15% increased Block and Stun Recovery\"],\"frameType\":2,\"x\":0,\"y\":0,\"inventoryId\": \"\", "
   "\"socketedItems\":[], \"_socketed\":true}";
    
void TestShop::SocketedGemsNotLinked() {
    Application app;
    app.InitLogin(nullptr, "TestLeague", "testuser");

    rapidjson::Document doc;
    doc.Parse(item_string.c_str());

    Items items = { std::make_shared<Item>(doc) };
    app.OnItemsRefreshed(items, {});

    Buyout bo;
    bo.type = BUYOUT_TYPE_FIXED;
    bo.value = 10;
    bo.currency = CURRENCY_CHAOS_ORB;
    app.buyout_manager()->Set(*items[0], bo);

    app.shop()->Update();
    std::string shop = app.shop()->shop_data();
    QVERIFY(shop.find("~price") == std::string::npos);
}
