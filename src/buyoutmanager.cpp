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

const std::string Currency::currency_type_error_;

const Currency::CurrencyTypeMap Currency::currency_type_as_string_ = {
    {CURRENCY_NONE, ""},
    {CURRENCY_ORB_OF_ALTERATION, "Orb of Alteration"},
    {CURRENCY_ORB_OF_FUSING, "Orb of Fusing"},
    {CURRENCY_ORB_OF_ALCHEMY, "Orb of Alchemy"},
    {CURRENCY_CHAOS_ORB, "Chaos Orb"},
    {CURRENCY_GCP, "Gemcutter's Prism"},
    {CURRENCY_EXALTED_ORB, "Exalted Orb"},
    {CURRENCY_CHROMATIC_ORB, "Chromatic Orb"},
    {CURRENCY_JEWELLERS_ORB, "Jeweller's Orb"},
    {CURRENCY_ORB_OF_CHANCE, "Orb of Chance"},
    {CURRENCY_CARTOGRAPHERS_CHISEL, "Cartographer's Chisel"},
    {CURRENCY_ORB_OF_SCOURING, "Orb of Scouring"},
    {CURRENCY_BLESSED_ORB, "Blessed Orb"},
    {CURRENCY_ORB_OF_REGRET, "Orb of Regret"},
    {CURRENCY_REGAL_ORB, "Regal Orb"},
    {CURRENCY_DIVINE_ORB, "Divine Orb"},
    {CURRENCY_VAAL_ORB, "Vaal Orb"},
    {CURRENCY_PERANDUS_COIN, "Perandus Coin"},
    {CURRENCY_MIRROR_OF_KALANDRA, "Mirror of Kalandra"}
};

const Currency::CurrencyTypeMap Currency::currency_type_as_tag_ = {
    {CURRENCY_NONE, ""},
    {CURRENCY_ORB_OF_ALTERATION, "alt"},
    {CURRENCY_ORB_OF_FUSING, "fuse"},
    {CURRENCY_ORB_OF_ALCHEMY, "alch"},
    {CURRENCY_CHAOS_ORB, "chaos"},
    {CURRENCY_GCP, "gcp"},
    {CURRENCY_EXALTED_ORB, "exa"},
    {CURRENCY_CHROMATIC_ORB, "chrom"},
    {CURRENCY_JEWELLERS_ORB, "jew"},
    {CURRENCY_ORB_OF_CHANCE, "chance"},
    {CURRENCY_CARTOGRAPHERS_CHISEL, "chisel"},
    {CURRENCY_ORB_OF_SCOURING, "scour"},
    {CURRENCY_BLESSED_ORB, "blessed"},
    {CURRENCY_ORB_OF_REGRET, "regret"},
    {CURRENCY_REGAL_ORB, "regal"},
    {CURRENCY_DIVINE_ORB, "divine"},
    {CURRENCY_VAAL_ORB, "vaal"},
    {CURRENCY_PERANDUS_COIN, "coin"},
    {CURRENCY_MIRROR_OF_KALANDRA, "mirror"}
};

const Currency::CurrencyRankMap Currency::currency_type_as_rank_ = {
    {CURRENCY_NONE, 0},
    {CURRENCY_CHROMATIC_ORB, 1},
    {CURRENCY_ORB_OF_ALTERATION, 2},
    {CURRENCY_JEWELLERS_ORB, 3},
    {CURRENCY_ORB_OF_CHANCE, 4},
    {CURRENCY_CARTOGRAPHERS_CHISEL, 5},
    {CURRENCY_PERANDUS_COIN, 6},
    {CURRENCY_ORB_OF_FUSING, 7},
    {CURRENCY_ORB_OF_ALCHEMY, 8},
    {CURRENCY_BLESSED_ORB, 9},
    {CURRENCY_ORB_OF_SCOURING, 10},
    {CURRENCY_CHAOS_ORB, 11},
    {CURRENCY_ORB_OF_REGRET, 12},
    {CURRENCY_REGAL_ORB, 13},
    {CURRENCY_VAAL_ORB, 14},
    {CURRENCY_GCP, 15},
    {CURRENCY_DIVINE_ORB, 16},
    {CURRENCY_EXALTED_ORB, 17},
    {CURRENCY_MIRROR_OF_KALANDRA, 18}
};

const std::string Buyout::buyout_type_error_;

const Buyout::BuyoutTypeMap Buyout::buyout_type_as_tag_ = {
    {BUYOUT_TYPE_IGNORE, "[ignore]"},
    {BUYOUT_TYPE_BUYOUT, "b/o"},
    {BUYOUT_TYPE_FIXED, "price"},
    {BUYOUT_TYPE_NO_PRICE, "no price"},
    {BUYOUT_TYPE_CURRENT_OFFER, "c/o"},
    {BUYOUT_TYPE_INHERIT, ""}
};

const Buyout::BuyoutTypeMap Buyout::buyout_type_as_prefix_ = {
    {BUYOUT_TYPE_IGNORE, ""},
    {BUYOUT_TYPE_BUYOUT, " ~b/o "},
    {BUYOUT_TYPE_FIXED, " ~price "},
    {BUYOUT_TYPE_CURRENT_OFFER, " ~c/o "},
    {BUYOUT_TYPE_NO_PRICE, ""},
    {BUYOUT_TYPE_INHERIT, ""}
};

const Buyout::BuyoutSourceMap Buyout::buyout_source_as_tag_ = {
    {BUYOUT_SOURCE_NONE, ""},
    {BUYOUT_SOURCE_MANUAL, "manual"},
    {BUYOUT_SOURCE_GAME, "game"},
    {BUYOUT_SOURCE_AUTO, "auto"}
};

const std::map<std::string, BuyoutType> BuyoutManager::string_to_buyout_type_ = {
    {"~gb/o", BUYOUT_TYPE_BUYOUT},
    {"~b/o", BUYOUT_TYPE_BUYOUT},
    {"~c/o", BUYOUT_TYPE_CURRENT_OFFER},
    {"~price", BUYOUT_TYPE_FIXED},
};

const std::map<std::string, Currency> BuyoutManager::string_to_currency_type_ = {
    {"alch", CURRENCY_ORB_OF_ALCHEMY},
    {"alchs", CURRENCY_ORB_OF_ALCHEMY},
    {"alchemy", CURRENCY_ORB_OF_ALCHEMY},
    {"alt", CURRENCY_ORB_OF_ALTERATION},
    {"alts", CURRENCY_ORB_OF_ALTERATION},
    {"alteration", CURRENCY_ORB_OF_ALTERATION},
    {"alterations", CURRENCY_ORB_OF_ALTERATION},   
    {"blessed", CURRENCY_BLESSED_ORB},
    {"cartographer", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"cartographers", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"chance", CURRENCY_ORB_OF_CHANCE},
    {"chaos", CURRENCY_CHAOS_ORB},
    {"chisel", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"chisels", CURRENCY_CARTOGRAPHERS_CHISEL},
    {"chrom", CURRENCY_CHROMATIC_ORB},
    {"chrome", CURRENCY_CHROMATIC_ORB},
    {"chromes", CURRENCY_CHROMATIC_ORB},
    {"chromatic", CURRENCY_CHROMATIC_ORB},
    {"chromatics", CURRENCY_CHROMATIC_ORB},
    {"coin", CURRENCY_PERANDUS_COIN},
    {"coins", CURRENCY_PERANDUS_COIN},
    {"divine", CURRENCY_DIVINE_ORB},
    {"divines", CURRENCY_DIVINE_ORB},
    {"exa", CURRENCY_EXALTED_ORB},
    {"exalted", CURRENCY_EXALTED_ORB},
    {"fuse", CURRENCY_ORB_OF_FUSING},
    {"fuses", CURRENCY_ORB_OF_FUSING},
    {"fusing", CURRENCY_ORB_OF_FUSING},
    {"fusings", CURRENCY_ORB_OF_FUSING},
    {"gcp", CURRENCY_GCP},
    {"gcps", CURRENCY_GCP},
    {"gemcutter", CURRENCY_GCP},
    {"gemcutters", CURRENCY_GCP},
    {"jew", CURRENCY_JEWELLERS_ORB},
    {"jewel", CURRENCY_JEWELLERS_ORB},
    {"jewels", CURRENCY_JEWELLERS_ORB},
    {"jeweler", CURRENCY_JEWELLERS_ORB},
    {"jewelers", CURRENCY_JEWELLERS_ORB},
    {"mir", CURRENCY_MIRROR_OF_KALANDRA},
    {"mirror", CURRENCY_MIRROR_OF_KALANDRA},
    {"p", CURRENCY_PERANDUS_COIN},
    {"perandus", CURRENCY_PERANDUS_COIN},
    {"regal", CURRENCY_REGAL_ORB},
    {"regals", CURRENCY_REGAL_ORB},
    {"regret", CURRENCY_ORB_OF_REGRET},
    {"regrets", CURRENCY_ORB_OF_REGRET},
    {"scour", CURRENCY_ORB_OF_SCOURING},
    {"scours", CURRENCY_ORB_OF_SCOURING},
    {"scouring", CURRENCY_ORB_OF_SCOURING},
    {"shekel", CURRENCY_PERANDUS_COIN},
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
    auto const it = buyouts_.find(item.hash());
    if (it != buyouts_.end()) {
        return it->second;
    }
    return Buyout();
}

Buyout BuyoutManager::GetTab(const std::string &tab) const {
    auto const it = tab_buyouts_.find(tab);
    if (it != tab_buyouts_.end()) {
        return it->second;
    }
    return Buyout();
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

void BuyoutManager::CompressItemBuyouts(const Items &items) {
    // When items are moved between tabs or deleted their buyouts entries remain
    // This function looks at buyouts and makes sure there is an associated item
    // that exists
    std::set<std::string> tmp;
    for (auto const &item_sp: items) {
        const Item & item= *item_sp;
        tmp.insert(item.hash());
    }

    for (auto it = buyouts_.cbegin(); it != buyouts_.cend();) {
        if (tmp.count(it->first) == 0) {

            buyouts_.erase(it++);
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
        if (!buyout.IsSavable())
            continue;
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("value", buyout.value, alloc);

        if (!buyout.last_update.isNull()){
            item.AddMember("last_update", buyout.last_update.toTime_t(), alloc);
        }else{
            // If last_update is null, set as the actual time
            item.AddMember("last_update", QDateTime::currentDateTime().toTime_t(), alloc);
        }

        Util::RapidjsonAddConstString(&item, "type", buyout.BuyoutTypeAsTag(), alloc);
        Util::RapidjsonAddConstString(&item, "currency", buyout.CurrencyAsTag(), alloc);
        Util::RapidjsonAddConstString(&item, "source", buyout.BuyoutSourceAsTag(), alloc);

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

        bo.currency = Currency::FromTag(object["currency"].GetString());
        bo.type = Buyout::TagAsBuyoutType(object["type"].GetString());
        bo.value = object["value"].GetDouble();
        if (object.HasMember("last_update")){
            bo.last_update = QDateTime::fromTime_t(object["last_update"].GetInt());
        }
        if (object.HasMember("source")){
            bo.source = Buyout::TagAsBuyoutSource(object["source"].GetString());
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
    return BUYOUT_TYPE_INHERIT;
}

Buyout BuyoutManager::StringToBuyout(std::string format) {
    // Parse format string and initialize buyout object, if string does not match any known format
    // then the buyout object will not be valid (IsValid will return false).
    std::regex exp("(~\\S+)\\s+(\\d+\\.?\\d*)\\s+(\\w+)");

    std::smatch sm;

    Buyout tmp;
    // regex_search allows for stuff before ~ and after currency type.  We only want to honor the formats
    // that POE trade also accept so this may need to change if it's too generous
    if (std::regex_search(format,sm,exp)) {
        tmp.type = StringToBuyoutType(sm[1]);
        tmp.value = QVariant(sm[2].str().c_str()).toDouble();
        tmp.currency = StringToCurrencyType(sm[3]);
        tmp.source = BUYOUT_SOURCE_GAME;
        tmp.last_update = QDateTime::currentDateTime();
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

bool Buyout::IsValid() const {
    switch (type) {
    case BUYOUT_TYPE_IGNORE:
    case BUYOUT_TYPE_INHERIT:
    case BUYOUT_TYPE_NO_PRICE:
        return true;
        break;
    default:
        return (currency != CURRENCY_NONE) && (source != BUYOUT_SOURCE_NONE);
    }
}

bool Buyout::IsActive() const {
    return IsValid() && type != BUYOUT_TYPE_INHERIT;
}

bool Buyout::IsPostable() const {
    return ((source != BUYOUT_SOURCE_GAME) && (IsPriced() || (type == BUYOUT_TYPE_NO_PRICE)));
}

bool Buyout::IsPriced() const {
    return (type == BUYOUT_TYPE_BUYOUT || type == BUYOUT_TYPE_FIXED || type == BUYOUT_TYPE_CURRENT_OFFER);
}

bool Buyout::IsGameSet() const {
    return (source == BUYOUT_SOURCE_GAME);
}

bool Buyout::RequiresRefresh() const
{
    return !(type == BUYOUT_TYPE_IGNORE || type == BUYOUT_TYPE_INHERIT);
}

BuyoutSource Buyout::TagAsBuyoutSource(std::string tag) {
    auto &m = buyout_source_as_tag_;
    auto const &it = std::find_if(m.begin(), m.end(), [&](BuyoutSourceMap::value_type const &x){ return x.second == tag;});
    return (it != m.end()) ? it->first:BUYOUT_SOURCE_NONE;
}

BuyoutType Buyout::TagAsBuyoutType(std::string tag) {
    auto &m = buyout_type_as_tag_;
    auto const &it = std::find_if(m.begin(), m.end(), [&](BuyoutTypeMap::value_type const &x){ return x.second == tag;});
    return (it != m.end()) ? it->first:BUYOUT_TYPE_INHERIT;
}

BuyoutType Buyout::IndexAsBuyoutType(int index) {
    if (index >= buyout_type_as_tag_.size()) {
        QLOG_WARN() << "Buyout type index out of bounds: " << index << ". This should never happen - please report.";
        return BUYOUT_TYPE_INHERIT;
    } else {
        return static_cast<BuyoutType>(index);
    }
}

std::string Buyout::AsText() const {
    if (IsPriced()) {
        return BuyoutTypeAsTag() + " " + QString::number(value).toStdString() + " " + CurrencyAsTag();
    } else {
        return BuyoutTypeAsTag();
    }
}

const std::string &Buyout::BuyoutTypeAsTag() const {
    auto const &it = buyout_type_as_tag_.find(type);
    if (it != buyout_type_as_tag_.end()) {
        return it->second;
    } else {
        QLOG_WARN() << "No mapping from buyout type: " << type << " to tag. This should never happen - please report.";
        return buyout_type_error_;
    }
}

const std::string &Buyout::BuyoutTypeAsPrefix() const {
    auto const &it = buyout_type_as_prefix_.find(type);
    if (it != buyout_type_as_prefix_.end()) {
        return it->second;
    } else {
        QLOG_WARN() << "No mapping from buyout type: " << type << " to prefix. This should never happen - please report.";
        return buyout_type_error_;
    }
}

const std::string &Buyout::BuyoutSourceAsTag() const {
    auto const &it = buyout_source_as_tag_.find(source);
    if (it != buyout_source_as_tag_.end()) {
        return it->second;
    } else {
        QLOG_WARN() << "No mapping from buyout source: " << source << " to tag. This should never happen - please report.";
        return buyout_type_error_;
    }
}

const std::string &Buyout::CurrencyAsTag() const
{
    return currency.AsTag();
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

Currency Currency::FromTag(std::string tag)
{
    auto &m = currency_type_as_tag_;
    auto const &it = std::find_if(m.begin(), m.end(), [&](CurrencyTypeMap::value_type const &x){ return x.second == tag;});
    return Currency((it != m.end()) ? it->first:CURRENCY_NONE);
}

std::vector<CurrencyType> Currency::Types() {
    std::vector<CurrencyType> tmp;
    for (unsigned int i = 0; i < currency_type_as_tag_.size();i++) {
       tmp.push_back(static_cast<CurrencyType>(i));
    }
    return tmp;
}

Currency Currency::FromIndex(int index) {
    if (index >= currency_type_as_tag_.size()) {
        QLOG_WARN() << "Currency type index out of bounds: " << index << ". This should never happen - please report.";
        return CURRENCY_NONE;
    } else {
        return Currency(static_cast<CurrencyType>(index));
    }
}

const std::string &Currency::AsString() const {
    auto const &it = currency_type_as_string_.find(type);
    if (it != currency_type_as_string_.end()) {
        return it->second;
    } else {
        QLOG_WARN() << "No mapping from currency type: " << type << " to string. This should never happen - please report.";
        return currency_type_error_;
    }
}

const std::string &Currency::AsTag() const {
    auto const &it = currency_type_as_tag_.find(type);
    if (it != currency_type_as_tag_.end()) {
        return it->second;
    } else {
        QLOG_WARN() << "No mapping from currency type: " << type << " to tag. This should never happen - please report.";
        return currency_type_error_;
    }
}

const int &Currency::AsRank() const
{
    return currency_type_as_rank_.at(type);
}
