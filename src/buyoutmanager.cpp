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
#include "QsLog.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include "application.h"
#include "datastore.h"
#include "rapidjson_util.h"
#include "util.h"
#include "itemlocation.h"

BuyoutManager::BuyoutManager(DataStore &data) :
    data_(data),
    save_needed_(false)
{
    Load();
}

void BuyoutManager::Set(const Item &item, const Buyout &buyout) {
    save_needed_ = true;
    buyouts_[item.hash()] = buyout;
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
    save_needed_ = true;
    tab_buyouts_[tab] = buyout;
}

bool BuyoutManager::ExistsTab(const std::string &tab) const {
    return tab_buyouts_.count(tab) > 0;
}

void BuyoutManager::DeleteTab(const std::string &tab) {
    save_needed_ = true;
    tab_buyouts_.erase(tab);
}

void BuyoutManager::SetRefreshChecked(const ItemLocation &loc, bool value) {
    save_needed_ = true;
    refresh_checked_[loc.GetUniqueHash()] = value;
}

bool BuyoutManager::GetRefreshChecked(const ItemLocation &loc) const {
    auto it = refresh_checked_.find(loc.GetUniqueHash());
    bool refresh_checked = (it != refresh_checked_.end()) ? it->second : (false);
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
    return std::fabs(o.value - value) < eps && o.type == type && o.currency == currency && o.inherited == inherited;
}

bool Buyout::operator!=(const Buyout &o) const {
    return !(o == *this);
}
