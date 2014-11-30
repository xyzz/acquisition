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

#include "util.h"

#include <QComboBox>
#include <QCryptographicHash>
#include <QString>
#include <QStringList>
#include <QLineEdit>
#include <QLabel>
#include <QFontMetrics>
#include <QNetworkReply>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

#include "buyoutmanager.h"
#include "porting.h"

std::string Util::Md5(const std::string &value) {
    QString hash = QString(QCryptographicHash::hash(value.c_str(), QCryptographicHash::Md5).toHex());
    return hash.toUtf8().constData();
}

double Util::AverageDamage(const std::string &s) {
    size_t x = s.find("-");
    if (x == std::string::npos)
        return 0;
    return (std::stod(s.substr(0, x)) + std::stod(s.substr(x + 1))) / 2;
}

void Util::PopulateBuyoutTypeComboBox(QComboBox *combobox) {
    combobox->addItems(QStringList({"Ignore", "Buyout", "Fixed price", "No price"}));
}

void Util::PopulateBuyoutCurrencyComboBox(QComboBox *combobox) {
    for (auto &currency : CurrencyAsString)
        combobox->addItem(QString(currency.c_str()));
}

int Util::TagAsBuyoutType(const std::string &tag) {
    return std::find(BuyoutTypeAsTag.begin(), BuyoutTypeAsTag.end(), tag) - BuyoutTypeAsTag.begin();
}

int Util::TagAsCurrency(const std::string &tag) {
    return std::find(CurrencyAsTag.begin(), CurrencyAsTag.end(), tag) - CurrencyAsTag.begin();
}

static std::vector<std::string> width_strings = {
    "max#",
    "R. Level",
    "R##",
    "Defense"
};

int Util::TextWidth(TextWidthId id) {
    static bool calculated = false;
    static std::vector<int> result;

    if (!calculated) {
        calculated = true;
        result.resize(width_strings.size());
        QLineEdit textbox;
        QFontMetrics fm(textbox.fontMetrics());
        for (size_t i = 0; i < width_strings.size(); ++i)
            result[i] = fm.width(width_strings[i].c_str());
    }
    return result[static_cast<int>(id)];
}

void Util::ParseJson(QNetworkReply *reply, rapidjson::Document *doc) {
    QByteArray bytes = reply->readAll();
    doc->Parse(bytes.constData());
}

std::string Util::GetCsrfToken(const std::string &page, const std::string &name) {
    std::string needle = "name=\"" + name + "\" value=\"";
    if (page.find(needle) == std::string::npos)
        return "";
    return page.substr(page.find(needle) + needle.size(), 32);
}

std::string Util::FindTextBetween(const std::string &page, const std::string &left, const std::string &right) {
    int first = page.find(left);
    int last = page.find(right);
    if (first == std::string::npos || last == std::string::npos || first > last)
        return "";
    return page.substr(first + left.size(), last - first - left.size());
}

std::string Util::BuyoutAsText(const Buyout &bo) {
    if (bo.type != BUYOUT_TYPE_NO_PRICE) {
        return BuyoutTypeAsTag[bo.type] + " " + QString::number(bo.value).toStdString() + " " + CurrencyAsTag[bo.currency];
    } else {
        return BuyoutTypeAsTag[bo.type];
    }
}

std::string Util::ModListAsString(const ItemMods &list) {
    std::string mods;
    bool first = true;
    for (auto &mod : list) {
        mods += (first ? "" : "<br>") + mod;
        first = false;
    }
    return mods;
}

std::string Util::RapidjsonSerialize(const rapidjson::Value &val) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    val.Accept(writer);
    return buffer.GetString();
}

void Util::RapidjsonAddConstString(rapidjson::Value *object, const char *const name, const std::string &value, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc) {
    rapidjson::Value rjson_name;
    rjson_name.SetString(name, strlen(name));
    rapidjson::Value rjson_val;
    rjson_val.SetString(value.c_str(), value.size());
    object->AddMember(rjson_name, rjson_val, alloc);
}
