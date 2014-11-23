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
#include "QsLog.h"

#include "application.h"
#include "datamanager.h"
#include "util.h"

BuyoutManager::BuyoutManager(Application *app) :
    app_(app),
    save_needed_(false)
{
    Load();
}

void BuyoutManager::Set(const Item &item, const Buyout &buyout) {
    save_needed_ = true;
    buyouts_[ItemHash(item)] = buyout;
}

Buyout BuyoutManager::Get(const Item &item) {
    if (!Exists(item))
        throw std::runtime_error("Asked to get inexistant buyout.");
    return buyouts_[ItemHash(item)];
}

void BuyoutManager::Delete(const Item &item) {
    save_needed_ = true;
    buyouts_.erase(ItemHash(item));
}

bool BuyoutManager::Exists(const Item &item) {
    return buyouts_.count(ItemHash(item)) > 0;
}

std::string BuyoutManager::ItemHash(const Item &item) {
    return item.hash();
}

Buyout BuyoutManager::GetTab(const std::string &tab) {
    return tab_buyouts_[tab];
}

void BuyoutManager::SetTab(const std::string &tab, const Buyout &buyout) {
    save_needed_ = true;
    tab_buyouts_[tab] = buyout;
}

bool BuyoutManager::ExistsTab(const std::string &tab) {
    return tab_buyouts_.count(tab) > 0;
}

void BuyoutManager::DeleteTab(const std::string &tab) {
    save_needed_ = true;
    tab_buyouts_.erase(tab);
}

std::string BuyoutManager::Serialize(const std::map<std::string, Buyout> &buyouts) {
#if 0
    Json::Value root;

    for (auto &bo : buyouts) {
        if (bo.second.currency == CURRENCY_NONE || bo.second.type == BUYOUT_TYPE_NONE)
            continue;
        if (bo.second.type >= BuyoutTypeAsTag.size() || bo.second.currency >= CurrencyAsTag.size()) {
            QLOG_WARN() << "Ignoring invalid buyout, type:" << bo.second.type
                << "currency:" << bo.second.currency;
            continue;
        }
        Json::Value item;
        item["value"] = bo.second.value;
        item["type"] = BuyoutTypeAsTag[bo.second.type];
        item["currency"] = CurrencyAsTag[bo.second.currency];
        root[bo.first] = item;
    }

    Json::FastWriter writer;
    return writer.write(root);
#endif
    return "";
}

void BuyoutManager::Deserialize(const std::string &data, std::map<std::string, Buyout> *buyouts) {
#if 0
    buyouts->clear();
    Json::Value root;
    Json::Reader reader;
    // if data is empty (on first use) we shouldn't make user panic by showing ERROR messages
    if (data.empty())
        return;
    if (!reader.parse(data, root)) {
        QLOG_ERROR() << "Error while parsing buyouts.";
        QLOG_ERROR() << reader.getFormattedErrorMessages().c_str();
        return;
    }
    if (!root.isObject())
        return;
    std::vector<std::string> keys = root.getMemberNames();
    for (auto &key : keys) {
        Buyout bo;
        bo.currency = static_cast<Currency>(Util::TagAsCurrency(root[key]["currency"].asString()));
        bo.type = static_cast<BuyoutType>(Util::TagAsBuyoutType(root[key]["type"].asString()));
        bo.value = root[key]["value"].asDouble();
        (*buyouts)[key] = bo;
    }
#endif
}

void BuyoutManager::Save() {
    if (!save_needed_)
        return;
    save_needed_ = false;

    app_->data_manager()->Set("buyouts", Serialize(buyouts_));
    app_->data_manager()->Set("tab_buyouts", Serialize(tab_buyouts_));
}

void BuyoutManager::Load() {
    Deserialize(app_->data_manager()->Get("buyouts"), &buyouts_);
    Deserialize(app_->data_manager()->Get("tab_buyouts"), &tab_buyouts_);
}
