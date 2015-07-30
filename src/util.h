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

#pragma once

#include <QString>
#include <QDateTime>
#include <QString>
#include "rapidjson/document.h"

#include "item.h"

class QComboBox;
class QNetworkReply;

struct Buyout;

enum class TextWidthId {
    WIDTH_MIN_MAX,
    WIDTH_LABEL,
    WIDTH_RGB,
    WIDTH_GROUP
};

namespace Util {
QString Md5(const QString &value);
double AverageDamage(const QString &s);
void PopulateBuyoutTypeComboBox(QComboBox *combobox);
void PopulateBuyoutCurrencyComboBox(QComboBox *combobox);
int TagAsCurrency(const QString &tag);
int TagAsBuyoutType(const QString &tag);

int TextWidth(TextWidthId id);

void ParseJson(QNetworkReply *reply, rapidjson::Document *doc);
QString GetCsrfToken(const QString &page, const QString &name);
QString FindTextBetween(const QString &page, const QString &left, const QString &right);

QString BuyoutAsText(const Buyout &bo);

QString ModListAsString(const ItemMods &list);

QString RapidjsonSerialize(const rapidjson::Value &val);
void RapidjsonAddConstString(rapidjson::Value *object, const char *const name, const QString &value, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc);

QString StringReplace(const QString &haystack, const QString &needle, const QString &replace);

/*
    Example usage:
        MatchMod("+# to Life", "+12.3 to Life", &result);
    Will return true if matches and save average value to output.
*/
bool MatchMod(const QString match, const QString mod, double *output);

QString TimeAgoInWords(const QDateTime buyout_time);
}
