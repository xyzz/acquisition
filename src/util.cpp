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

QString Util::Md5(const QString &value) {
    QString hash = QString(QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Md5).toHex());
    return hash.toUtf8().constData();
}

double Util::AverageDamage(const QString &s) {
    size_t x = s.indexOf("-");
    if (x == -1)
        return 0;
    return (s.mid(0, x).toDouble() + s.mid(x + 1).toDouble() ) / 2;
}

void Util::PopulateBuyoutTypeComboBox(QComboBox *combobox) {
    combobox->addItems(QStringList({"Ignore", "Buyout", "Fixed price", "No price"}));
}

void Util::PopulateBuyoutCurrencyComboBox(QComboBox *combobox) {
    for (auto &currency : CurrencyAsString)
        combobox->addItem(currency);
}

int Util::TagAsBuyoutType(const QString &tag) {
    return std::find(BuyoutTypeAsTag.begin(), BuyoutTypeAsTag.end(), tag) - BuyoutTypeAsTag.begin();
}

int Util::TagAsCurrency(const QString &tag) {
    return std::find(CurrencyAsTag.begin(), CurrencyAsTag.end(), tag) - CurrencyAsTag.begin();
}

static std::vector<QString> width_strings = {
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
            result[i] = fm.width(width_strings[i]);
    }
    return result[static_cast<int>(id)];
}

void Util::ParseJson(QNetworkReply *reply, rapidjson::Document *doc) {
    QByteArray bytes = reply->readAll();
    doc->Parse(bytes.constData());
}

QString Util::GetCsrfToken(const QString &page, const QString &name) {
    QString needle = "name=\"" + name + "\" value=\"";
    if (page.indexOf(needle) == -1)
        return "";
    return page.mid(page.indexOf(needle) + needle.length(), 32);
}

QString Util::FindTextBetween(const QString &page, const QString &left, const QString &right) {
    size_t first = page.indexOf(left);
    size_t last = page.indexOf(right, first);
    if (first == -1 || last == -1 || first > last)
        return "";
    return page.mid(first + left.length(), last - first - left.length());
}

QString Util::BuyoutAsText(const Buyout &bo) {
    if (bo.type != BUYOUT_TYPE_NO_PRICE) {
        return BuyoutTypeAsTag[bo.type] + " " + QString::number(bo.value) + " " + CurrencyAsTag[bo.currency];
    } else {
        return BuyoutTypeAsTag[bo.type];
    }
}

QString Util::ModListAsString(const ItemMods &list) {
    QString mods;
    bool first = true;
    for (auto &mod : list) {
        mods += (first ? "" : "<br>") + mod;
        first = false;
    }
    return mods;
}

QString Util::RapidjsonSerialize(const rapidjson::Value &val) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    val.Accept(writer);
    return buffer.GetString();
}

void Util::RapidjsonAddConstString(rapidjson::Value *object, const char *const name, const QString &value, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc) {
    rapidjson::Value rjson_name;
    rjson_name.SetString(name, strlen(name));
    rapidjson::Value rjson_val;
    rjson_val.SetString(value.toStdString().c_str(), value.toStdString().size());
    object->AddMember(rjson_name, rjson_val, alloc);
}

QString Util::StringReplace(const QString &haystack, const QString &needle, const QString &replace) {
    QString out = haystack;
    for (size_t pos = 0; ; pos += replace.length()) {
        pos = out.indexOf(needle, pos);
        if (pos == -1)
            break;
        out.remove(pos, needle.length());
        out.insert(pos, replace);
    }
    return out;
}

bool Util::MatchMod(const QString match, const QString mod, double *output) {
    double result = 0.0;
	QString pmatch = QString(match).replace(QString("#"),QString("([\\d\\.]+)"));
	QRegExp rx(pmatch);
	if (!rx.isValid() || rx.indexIn(mod) == -1)
		return false;
	result = rx.cap(1).toDouble();
    *output = result;
	return true;
}

QString Util::TimeAgoInWords(const QDateTime buyout_time){
    QDateTime current_date = QDateTime::currentDateTime();
    qint64 days = buyout_time.daysTo(current_date);
    qint64 secs = buyout_time.secsTo(current_date);
    qint64 hours = (secs / 60 / 60) % 24;
    qint64 minutes = (secs / 60) % 60;

    // YEARS
    if (days > 365){
        int years = (days / 365);
        if (days % 365 != 0)
            years++;
        return QString("%1 %2 ago").arg(years).arg(years == 1 ? "year" : "years");
    }
    // MONTHS
    if (days > 30){
        int months = (days / 365);
        if (days % 30 != 0)
            months++;
        return QString("%1 %2 ago").arg(months).arg(months == 1 ? "month" : "months");
    // DAYS
    }else if (days > 0){
        return QString("%1 %2 ago").arg(days).arg(days == 1 ? "day" : "days");
    // HOURS
    }else if (hours > 0){
        return QString("%1 %2 ago").arg(hours).arg(hours == 1 ? "hour" : "hours");
    //MINUTES
    }else if (minutes > 0){
        return QString("%1 %2 ago").arg(minutes).arg(minutes == 1 ? "minute" : "minutes");
    // SECONDS
    }else if (secs > 5){
        return QString("%1 %2 ago").arg(secs).arg("seconds");
    }else if (secs < 5){
        return QString("just now");
    }else{
        return "";
    }
}
