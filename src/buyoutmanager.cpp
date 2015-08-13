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
#include "datamanager.h"
#include "rapidjson_util.h"
#include "util.h"

BuyoutManager::BuyoutManager(DataManager &data_manager) :
    data_manager_(data_manager),
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
        (*buyouts)[name] = bo;
    }
}

void BuyoutManager::Save() {
    if (!save_needed_)
        return;
    save_needed_ = false;
    data_manager_.Set("buyouts", Serialize(buyouts_));
    data_manager_.Set("tab_buyouts", Serialize(tab_buyouts_));
}

void BuyoutManager::Load() {
    Deserialize(data_manager_.Get("buyouts"), &buyouts_);
    Deserialize(data_manager_.Get("tab_buyouts"), &tab_buyouts_);
}
