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
    combobox->addItems(QStringList({"No price", "Buyout", "Fixed price"}));
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

int Util::MinMaxWidth() {
    static bool calculated = false;
    static int result;

    if (!calculated) {
        calculated = true;
        QLineEdit textbox;
        QFontMetrics fm(textbox.fontMetrics());
        result = fm.width("max#");
    }
    return result;
}

int Util::LabelWidth() {
    static bool calculated = false;
    static int result;

    if (!calculated) {
        calculated = true;
        QLabel label;
        QFontMetrics fm(label.fontMetrics());
        result = fm.width("R. Level");
    }
    return result;
}

int Util::RGBWidth() {
    static bool calculated = false;
    static int result;

    if (!calculated) {
        calculated = true;
        QLineEdit textbox;
        QFontMetrics fm(textbox.fontMetrics());
        result = fm.width("R##");
    }
    return result;
}

int Util::GroupWidth() {
    static bool calculated = false;
    static int result;

    if (!calculated) {
        calculated = true;
        QLabel label;
        QFontMetrics fm(label.fontMetrics());
        result = fm.width("Defense");
    }
    return result;
}

void Util::ParseJson(QNetworkReply *reply, Json::Value *root) {
    QByteArray bytes = reply->readAll();
    std::string json(bytes.constData(), bytes.size());
    Json::Reader reader;
    reader.parse(json, *root);
}
