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

#include "buyoutmanager.h"

#include <cassert>
#include <sstream>
#include <stdexcept>
#include <regex>
#include "QsLog.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include "application.h"
#include "datastore.h"
#include "rapidjson_util.h"
#include "util.h"
#include "itemlocation.h"
#include "QVariant"

const std::map<std::string, BuyoutType> BuyoutManager::string_to_buyout_type_ = {
    {"~gb/o", BUYOUT_TYPE_BUYOUT},
    {"~b/o", BUYOUT_TYPE_BUYOUT},
//    {"~c/o", BUYOUT_TYPE_CURRENT_OFFER},
    {"~price", BUYOUT_TYPE_FIXED},
};

const std::map<std::string, Currency> BuyoutManager::string_to_currency_type_ = {
    {"alt", CURRENCY_ORB_OF_ALTERATION},
    {"alts", CURRENCY_ORB_OF_ALTERATION},
    {"alteration", CURRENCY_ORB_OF_ALTERATION},
    {"alterations", CURRENCY_ORB_OF_ALTERATION},
    {"fuse", CURRENCY_ORB_OF_FUSING},
    {"fuses", CURRENCY_ORB_OF_FUSING},
    {"fusing", CURRENCY_ORB_OF_FUSING},
    {"fusings", CURRENCY_ORB_OF_FUSING},
    {"alch", CURRENCY_ORB_OF_ALCHEMY},
    {"alchs", CURRENCY_ORB_OF_ALCHEMY},
    {"alchemy", CURRENCY_ORB_OF_ALCHEMY},
    {"chaos", CURRENCY_CHAOS_ORB},
    {"gcp", CURRENCY_GCP},
    {"gcps", CURRENCY_GCP},
    {"gemcutter", CURRENCY_GCP},
    {"gemcutters", CURRENCY_GCP},
    {"prism", CURRENCY_GCP},
    {"prisms", CURRENCY_GCP},
    {"exa", CURRENCY_EXALTED_ORB},
    {"exalted", CURRENCY_EXALTED_ORB},
    {"chrom", CURRENCY_CHROMATIC_ORB},
    {"chrome", CURRENCY_CHROMATIC_ORB},
    {"chromes", CURRENCY_CHROMATIC_ORB},
    {"chromatic", CURRENCY_CHROMATIC_ORB},
    {"chromatics", CURRENCY_CHROMATIC_ORB},
    {"jew", CURRENCY_JEWELLERS_ORB},
    {"jews", CURRENCY_JEWELLERS_ORB},
    {"jewel", CURRENCY_JEWELLERS_ORB},
    {"jewels", CURRENCY_JEWELLERS_ORB},
    {"jeweler", CURRENCY_JEWELLERS_ORB},
    {"jewelers", CURRENCY_JEWELLERS_ORB},
    {"chance", CURRENCY_ORB_OF_CHANCE},
    {"chisel", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"chisels", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"cartographer", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"cartographers", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"scour", CURRENCY_ORB_OF_SCOURING},
    {"scours", CURRENCY_ORB_OF_SCOURING},
    {"scouring", CURRENCY_ORB_OF_SCOURING},
    {"blessed", CURRENCY_BLESSED_ORB},
    {"regret", CURRENCY_ORB_OF_REGRET},
    {"regrets", CURRENCY_ORB_OF_REGRET},
    {"regal", CURRENCY_REGAL_ORB},
    {"regals", CURRENCY_REGAL_ORB},
    {"divine", CURRENCY_DIVINE_ORB},
    {"divines", CURRENCY_DIVINE_ORB},
    {"vaal", CURRENCY_VAAL_ORB},
};

BuyoutManager::BuyoutManager(DataStore &data) :
    data_(data),
    save_needed_(false)
{
    Load();
}

void BuyoutManager::Set(const Item &item, const Buyout &buyout) {
    auto it = buyouts_.lower_bound(item.hash());
    if (it != buyouts_.end() && !(buyouts_.key_comp()(item.hash(), it->first))) {
        // Entry exists - we don't want to update if buyout is equal to existing
        if (buyout != it->second) {
            save_needed_ = true;
            it->second = buyout;
        }
    } else {
        save_needed_ = true;
        buyouts_.insert(it, {item.hash(), buyout});
    }
}

Buyout BuyoutManager::Get(const Item &item) const {
    if (!Exists(item))
        throw std::runtime_error("Asked to get inexistant buyout.");
    return buyouts_.at(item.hash());
}

void BuyoutManager::Delete(const Item &item) {
    save_needed_ = true;
    buyouts_.erase(item.hash());
}

bool BuyoutManager::Exists(const Item &item) const {
    return buyouts_.count(item.hash()) > 0;
}

Buyout BuyoutManager::GetTab(const std::string &tab) const {
    return tab_buyouts_.at(tab);
}

void BuyoutManager::SetTab(const std::string &tab, const Buyout &buyout) {
    auto it = tab_buyouts_.lower_bound(tab);
    if (it != tab_buyouts_.end() && !(tab_buyouts_.key_comp()(tab, it->first))) {
        // Entry exists - we don't want to update if buyout is equal to existing
        if (buyout != it->second) {
            save_needed_ = true;
            it->second = buyout;
        }
    } else {
        save_needed_ = true;
        tab_buyouts_.insert(it, {tab, buyout});
    }
}

bool BuyoutManager::ExistsTab(const std::string &tab) const {
    return tab_buyouts_.count(tab) > 0;
}

void BuyoutManager::DeleteTab(const std::string &tab) {
    save_needed_ = true;
    tab_buyouts_.erase(tab);
}

void BuyoutManager::CompressTabBuyouts() {
    // When tabs are renamed we end up with stale tab buyouts that aren't deleted.
    // This function is to remove buyouts associated with tab names that don't
    // currently exist.
    std::set<std::string> tmp;
    for (auto const& loc: tabs_)
        tmp.insert(loc.GetUniqueHash());

    for (auto it = tab_buyouts_.begin(), ite = tab_buyouts_.end(); it != ite;) {
        if(tmp.count(it->first) == 0) {
            save_needed_ = true;
            it = tab_buyouts_.erase(it);
        } else {
            ++it;
        }
    }
}

void BuyoutManager::SetRefreshChecked(const ItemLocation &loc, bool value) {
    save_needed_ = true;
    refresh_checked_[loc.GetUniqueHash()] = value;
}

bool BuyoutManager::GetRefreshChecked(const ItemLocation &loc) const {
    auto it = refresh_checked_.find(loc.GetUniqueHash());
    bool refresh_checked = (it != refresh_checked_.end()) ? it->second : true;
    return (refresh_checked || GetRefreshLocked(loc));
}

bool BuyoutManager::GetRefreshLocked(const ItemLocation &loc) const {
    return refresh_locked_.count(loc.GetUniqueHash());
}

void BuyoutManager::SetRefreshLocked(const ItemLocation &loc) {
    refresh_locked_.insert(loc.GetUniqueHash());
}

void BuyoutManager::ClearRefreshLocks() {
    refresh_locked_.clear();
}

bool BuyoutManager::IsGamePriced(const Item &item) {
    auto it = buyouts_.find(item.hash());
    if (it != buyouts_.end()) {
        return it->second.source == BUYOUT_SOURCE_GAME;
    }
    return false;
}

bool BuyoutManager::IsGamePriced(const std::string &tab) {
    auto it = tab_buyouts_.find(tab);
    if (it != tab_buyouts_.end()) {
        return it->second.source == BUYOUT_SOURCE_GAME;
    }
    return false;
}

void BuyoutManager::Clear() {
    save_needed_ = true;
    buyouts_.clear();
    tab_buyouts_.clear();
    refresh_locked_.clear();
    refresh_checked_.clear();
    tabs_.clear();
}

std::string BuyoutManager::Serialize(const std::map<std::string, Buyout> &buyouts) {
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();

    for (auto &bo : buyouts) {
        const Buyout &buyout = bo.second;
        if (buyout.type != BUYOUT_TYPE_NO_PRICE && (buyout.currency == CURRENCY_NONE || buyout.type == BUYOUT_TYPE_NONE))
            continue;
        if (buyout.type >= BuyoutTypeAsTag.size() || buyout.currency >= CurrencyAsTag.size()) {
            QLOG_WARN() << "Ignoring invalid buyout, type:" << buyout.type
                << "currency:" << buyout.currency;
            continue;
        }
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("value", buyout.value, alloc);

        if (!buyout.last_update.isNull()){
            item.AddMember("last_update", buyout.last_update.toTime_t(), alloc);
        }else{
            // If last_update is null, set as the actual time
            item.AddMember("last_update", QDateTime::currentDateTime().toTime_t(), alloc);
        }

        Util::RapidjsonAddConstString(&item, "type", BuyoutTypeAsTag[buyout.type], alloc);
        Util::RapidjsonAddConstString(&item, "currency", CurrencyAsTag[buyout.currency], alloc);
        Util::RapidjsonAddConstString(&item, "source", BuyoutSourceAsTag[buyout.source], alloc);

        item.AddMember("inherited", buyout.inherited, alloc);

        rapidjson::Value name(bo.first.c_str(), alloc);
        doc.AddMember(name, item, alloc);
    }

    return Util::RapidjsonSerialize(doc);
}

void BuyoutManager::Deserialize(const std::string &data, std::map<std::string, Buyout> *buyouts) {
    buyouts->clear();

    // if data is empty (on first use) we shouldn't make user panic by showing ERROR messages
    if (data.empty())
        return;

    rapidjson::Document doc;
    if (doc.Parse(data.c_str()).HasParseError()) {
        QLOG_ERROR() << "Error while parsing buyouts.";
        QLOG_ERROR() << rapidjson::GetParseError_En(doc.GetParseError());
        return;
    }
    if (!doc.IsObject())
        return;
    for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
        auto &object = itr->value;
        const std::string &name = itr->name.GetString();
        Buyout bo;

        bo.currency = static_cast<Currency>(Util::TagAsCurrency(object["currency"].GetString()));
        bo.type = static_cast<BuyoutType>(Util::TagAsBuyoutType(object["type"].GetString()));
        bo.value = object["value"].GetDouble();
        if (object.HasMember("last_update")){
            bo.last_update = QDateTime::fromTime_t(object["last_update"].GetInt());
        }
        if (object.HasMember("source")){
            bo.source = static_cast<BuyoutSource>(Util::TagAsBuyoutSource(object["source"].GetString()));
        }
        bo.inherited = false;
        if (object.HasMember("inherited"))
            bo.inherited = object["inherited"].GetBool();
        (*buyouts)[name] = bo;
    }
}


std::string BuyoutManager::Serialize(const std::map<std::string, bool> &obj) {
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();

    for (auto &pair : obj) {
        rapidjson::Value key(pair.first.c_str(), alloc);
        rapidjson::Value val(pair.second);
        doc.AddMember(key, val, alloc);
    }
    return Util::RapidjsonSerialize(doc);
}

void BuyoutManager::Deserialize(const std::string &data, std::map<std::string, bool> &obj) {
    // if data is empty (on first use) we shouldn't make user panic by showing ERROR messages
    if (data.empty())
        return;

    rapidjson::Document doc;
    if (doc.Parse(data.c_str()).HasParseError()) {
        QLOG_ERROR() << rapidjson::GetParseError_En(doc.GetParseError());
        return;
    }

    if (!doc.IsObject())
        return;

    for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
        const auto &val = itr->value.GetBool();
        const auto &name = itr->name.GetString();
        obj[name] = val;
    }
}

void BuyoutManager::Save() {
    if (!save_needed_)
        return;
    save_needed_ = false;
    data_.Set("buyouts", Serialize(buyouts_));
    data_.Set("tab_buyouts", Serialize(tab_buyouts_));
    data_.Set("refresh_checked_state", Serialize(refresh_checked_));
}

void BuyoutManager::Load() {
    Deserialize(data_.Get("buyouts"), &buyouts_);
    Deserialize(data_.Get("tab_buyouts"), &tab_buyouts_);
    Deserialize(data_.Get("refresh_checked_state"), refresh_checked_);
}
void BuyoutManager::SetStashTabLocations(const std::vector<ItemLocation> &tabs) {
    tabs_ = tabs;
}

const std::vector<ItemLocation> BuyoutManager::GetStashTabLocations() const {
    return tabs_;
}


Currency BuyoutManager::StringToCurrencyType(std::string currency) const {
    auto const &it = string_to_currency_type_.find(currency);
    if (it != string_to_currency_type_.end()) {
        return it->second;
    }
    return CURRENCY_NONE;
}

BuyoutType BuyoutManager::StringToBuyoutType(std::string bo_str) const {
    auto const &it = string_to_buyout_type_.find(bo_str);
    if (it != string_to_buyout_type_.end()) {
        return it->second;
    }
    return BUYOUT_TYPE_NONE;
}

Buyout BuyoutManager::StringToBuyout(std::string format) {
    // Parse format string and initialize buyout object, if string does not match any known format
    // then the buyout object will not be valid (IsValid will return false).
    Buyout tmp;
    try {
        std::regex exp("(~\\S+)\\s+(\\d+\\.?\\d*)\\s+(\\S+)");

        std::smatch sm;

        // regex_search allows for stuff before ~ and after currency type.  We only want to honor the formats
        // that POE trade also accept so this may need to change if it's too generous
        if (std::regex_search(format,sm,exp)) {
            tmp.type = StringToBuyoutType(sm[1]);
            tmp.value = QVariant(sm[2].str().c_str()).toDouble();
            tmp.currency = StringToCurrencyType(sm[3]);
            tmp.source = BUYOUT_SOURCE_GAME;
            tmp.last_update = QDateTime::currentDateTime();
        }
    } catch(std::regex_error) {
        // In case exp is not a valid regex according to the std::regex implementation,
        // just fall through to returning the non-valid object.
    }
    return tmp;
}

void BuyoutManager::MigrateItem(const Item &item) {
    std::string old_hash = item.old_hash();
    std::string hash = item.hash();
    auto it = buyouts_.find(old_hash);
    if (it != buyouts_.end()) {
        buyouts_[hash] = it->second;
        buyouts_.erase(it);
        save_needed_ = true;
    }
}

bool Buyout::operator==(const Buyout&o) const {
    static const double eps = 1e-6;
    return std::fabs(o.value - value) < eps && o.type == type
            && o.currency == currency && o.inherited == inherited
            && o.source == source;
}

bool Buyout::operator!=(const Buyout &o) const {
    return !(o == *this);
}
