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
#include <QTextDocument>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include <sstream>

#include "buyoutmanager.h"
#include "porting.h"

namespace Util {
std::map<std::string, BuyoutType> string_to_buyout_type_ = {
    {"~gb/o", BUYOUT_TYPE_BUYOUT},
    {"~b/o", BUYOUT_TYPE_BUYOUT},
//    {"~c/o", BUYOUT_TYPE_CURRENT_OFFER},
    {"~price", BUYOUT_TYPE_FIXED},
};

std::map<std::string, Currency> string_to_currency_type_ = {
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
}

Currency Util::StringToCurrencyType(std::string currency) {
    auto const &it = string_to_currency_type_.find(currency);
    if (it != string_to_currency_type_.end()) {
        return it->second;
    }
    return CURRENCY_NONE;
}

BuyoutType Util::StringToBuyoutType(std::string bo_str) {
    auto const &it = string_to_buyout_type_.find(bo_str);
    if (it != string_to_buyout_type_.end()) {
        return it->second;
    }
    return BUYOUT_TYPE_NONE;
}

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
    size_t first = page.find(left);
    size_t last = page.find(right, first);
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

std::string Util::StringReplace(const std::string &haystack, const std::string &needle, const std::string &replace) {
    std::string out = haystack;
    for (size_t pos = 0; ; pos += replace.length()) {
        pos = out.find(needle, pos);
        if (pos == std::string::npos)
            break;
        out.erase(pos, needle.length());
        out.insert(pos, replace);
    }
    return out;
}

std::string Util::StringJoin(const std::vector<std::string> &arr, const std::string &separator) {
    std::string result;
    for (size_t i = 0; i < arr.size(); ++i) {
        if (i != 0)
            result += separator;
        result += arr[i];
    }
    return result;
}

std::vector<std::string> Util::StringSplit(const std::string &str, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

bool Util::MatchMod(const char *match, const char *mod, double *output) {
    double result = 0.0;
    auto pmatch = match;
    auto pmod = mod;
    int cnt = 0;

    while (*pmatch && *pmod) {
        if (*pmatch == '#') {
            ++cnt;
            auto prev = pmod;
            while ((*pmod >= '0' && *pmod <= '9') || *pmod == '.')
                ++pmod;
            result += std::strtod(prev, NULL);
            ++pmatch;
        } else if (*pmatch == *pmod) {
            ++pmatch;
            ++pmod;
        } else {
            return false;
        }
    }
    *output = result / cnt;
    return !*pmatch && !*pmod;
}

std::string Util::TimeAgoInWords(const QDateTime buyout_time){
    QDateTime current_date = QDateTime::currentDateTime();
    qint64 secs = buyout_time.secsTo(current_date);
    qint64 days = secs / 60 / 60 / 24;
    qint64 hours = (secs / 60 / 60) % 24;
    qint64 minutes = (secs / 60) % 60;

    // YEARS
    if (days > 365){
        int years = (days / 365);
        if (days % 365 != 0)
            years++;
        return QString("%1 %2 ago").arg(years).arg(years == 1 ? "year" : "years").toStdString();
    }
    // MONTHS
    if (days > 30){
        int months = (days / 365);
        if (days % 30 != 0)
            months++;
        return QString("%1 %2 ago").arg(months).arg(months == 1 ? "month" : "months").toStdString();
    // DAYS
    }else if (days > 0){
        return QString("%1 %2 ago").arg(days).arg(days == 1 ? "day" : "days").toStdString();
    // HOURS
    }else if (hours > 0){
        return QString("%1 %2 ago").arg(hours).arg(hours == 1 ? "hour" : "hours").toStdString();
    //MINUTES
    }else if (minutes > 0){
        return QString("%1 %2 ago").arg(minutes).arg(minutes == 1 ? "minute" : "minutes").toStdString();
    // SECONDS
    }else if (secs > 5){
        return QString("%1 %2 ago").arg(secs).arg("seconds").toStdString();
    }else if (secs < 5){
        return QString("just now").toStdString();
    }else{
        return "";
    }
}

std::string Util::Decode(const std::string &entity) {
    QTextDocument text;
    text.setHtml(entity.c_str());
    return text.toPlainText().toStdString();
}
