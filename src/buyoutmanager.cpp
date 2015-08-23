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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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

void BuyoutManager::Set(const Item &item, Buyout &buyout, QString setter) {
    save_needed_ = true;
    buyout.set_by = setter;
    buyouts_.insert(ItemHash(item), buyout);
}

Buyout BuyoutManager::Get(const Item &item) const {
    if (!Exists(item))
        throw std::runtime_error("Asked to get inexistant buyout.");
    return buyouts_.value(ItemHash(item));
}

void BuyoutManager::Delete(const Item &item) {
    save_needed_ = true;
    buyouts_.remove(ItemHash(item));
}

bool BuyoutManager::Exists(const Item &item) const {
    return buyouts_.count(ItemHash(item)) > 0;
}

bool BuyoutManager::Equal(const Buyout &buyout1, const Buyout &buyout2) {
    return buyout1 == buyout2;
}

QString BuyoutManager::Generate(const Buyout &buyout) {
    QString result;
    if (buyout.type == BUYOUT_TYPE_NONE) return result;
    result = QString::fromStdString(BuyoutTypeAsPrefix[buyout.type]);
    if (buyout.type == BUYOUT_TYPE_BUYOUT || buyout.type == BUYOUT_TYPE_FIXED) {
        result += QString::number(buyout.value);
        result += " " + QString::fromStdString(CurrencyAsTag[buyout.currency]);
    }
    return result;
}

bool BuyoutManager::IsItemManuallySet(const Item &item) const {
    return Exists(item) ? buyouts_.value(ItemHash(item)).set_by == "" : false;
}

QString BuyoutManager::ItemHash(const Item &item) const {
    if (!use_broken_) {
        return QString::fromStdString(item.hash());
    }
    return QString::fromStdString(item.broken_hash());
}

Buyout BuyoutManager::GetTab(const std::string &tab) const {
    return tab_buyouts_.value(QString::fromStdString(tab));
}

void BuyoutManager::SetTab(const Bucket &tab, const Buyout &buyout) {
    QString hash = QString::fromStdString(tab.location().GetGeneralHash());
    save_needed_ = true;
    tab_buyouts_.insert(hash, buyout);

    UpdateTabItems(tab);
}

bool BuyoutManager::ExistsTab(const std::string &tab) const {
    return tab_buyouts_.count(QString::fromStdString(tab)) > 0;
}

void BuyoutManager::DeleteTab(const Bucket &tab) {
    QString hash = QString::fromStdString(tab.location().GetGeneralHash());
    save_needed_ = true;
    tab_buyouts_.remove(hash);

    UpdateTabItems(tab);
}

void BuyoutManager::UpdateTabItems(const Bucket &tab) {
    std::string hash = tab.location().GetGeneralHash();
    bool set = ExistsTab(hash);
    Buyout buyout;
    if (set) buyout = GetTab(hash);
    // Set buyouts on items with the "set_by" flag set to the tab
    for (auto &item : tab.items()) {
        if (set) {
            Buyout b;
            b.currency = buyout.currency;
            b.type = buyout.type;
            b.value = buyout.value;
            if (!Exists(*item)) {
                // It's new!
                b.last_update = QDateTime::currentDateTime();
            }
            else if (!IsItemManuallySet(*item)) {
                Buyout curr = Get(*item);
                if (!Equal(buyout, curr)) {
                    b.last_update = buyout.last_update;
                }
                else {
                    continue;
                }
            }
            else {
                continue;
            }
            Set(*item, b, QString::fromStdString(hash));
        }
        else if (!set && Exists(*item) && !IsItemManuallySet(*item)) {
            Delete(*item);
        }
    }
    Save();
}

std::string BuyoutManager::Serialize(const QMap<QString, Buyout> &buyouts) {
    QJsonDocument doc;
    QJsonObject root;
    for (QString hash : buyouts.uniqueKeys()) {
        Buyout buyout = buyouts.value(hash);
        if (buyout.type != BUYOUT_TYPE_NO_PRICE && (buyout.currency == CURRENCY_NONE || buyout.type == BUYOUT_TYPE_NONE))
            continue;
        if (buyout.type >= BuyoutTypeAsTag.size() || buyout.currency >= CurrencyAsTag.size()) {
            QLOG_WARN() << "Ignoring invalid buyout, type:" << buyout.type
                << "currency:" << buyout.currency;
            continue;
        }
        QJsonObject value;
        value.insert("value", buyout.value);
        value.insert("set_by", buyout.set_by);

        if (!buyout.last_update.isNull()){
            value.insert("last_update", (int) buyout.last_update.toTime_t());
        } else {
            // If last_update is null, set as the actual time
            value.insert("last_update", (int) QDateTime::currentDateTime().toTime_t());
        }

        value.insert("type", QString::fromStdString(BuyoutTypeAsTag[buyout.type]));
        value.insert("currency", QString::fromStdString(CurrencyAsTag[buyout.currency]));

        root.insert(hash, value);
    }
    doc.setObject(root);
    QByteArray result = doc.toJson();
    return result.toStdString();
}

void BuyoutManager::Deserialize(const std::string &data, QMap<QString, Buyout> *buyouts) {
    buyouts->clear();

    // if data is empty (on first use) we shouldn't make user panic by showing ERROR messages
    if (data.empty())
        return;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(data), &err);
    if (err.error != QJsonParseError::NoError) {
        QLOG_ERROR() << "Error while parsing buyouts.";
        QLOG_ERROR() << err.errorString();
        return;
    }
    if (!doc.isObject()) {
        QLOG_ERROR() << "Error while parsing buyouts.";
        QLOG_ERROR() << "Value is not an object.";
        return;
    }

    for (QString key : doc.object().keys()) {
        QJsonObject object = doc.object().value(key).toObject();
        Buyout bo;

        bo.currency = static_cast<Currency>(Util::TagAsCurrency(object.value("currency").toString().toStdString()));
        bo.type = static_cast<BuyoutType>(Util::TagAsBuyoutType(object.value("type").toString().toStdString()));
        bo.value = object.value("value").toDouble();
        bo.last_update = QDateTime();
        bo.set_by = "";

        if (object.contains("last_update")) {
            bo.last_update = QDateTime::fromTime_t(object.value("last_update").toInt());
        }
        if (object.contains("set_by")) {
            bo.set_by = object.value("set_by").toString();
        }

        buyouts->insert(key, bo);
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
