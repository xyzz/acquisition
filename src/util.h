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

#include <string>
#include <QDateTime>
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
std::string Md5(const std::string &value);
double AverageDamage(const std::string &s);
void PopulateBuyoutTypeComboBox(QComboBox *combobox);
void PopulateBuyoutCurrencyComboBox(QComboBox *combobox);
int TagAsCurrency(const std::string &tag);
int TagAsBuyoutType(const std::string &tag);

int TextWidth(TextWidthId id);

void ParseJson(QNetworkReply *reply, rapidjson::Document *doc);
std::string GetCsrfToken(const std::string &page, const std::string &name);
std::string FindTextBetween(const std::string &page, const std::string &left, const std::string &right);

std::string BuyoutAsText(const Buyout &bo);

std::string RapidjsonSerialize(const rapidjson::Value &val);
void RapidjsonAddConstString(rapidjson::Value *object, const char *const name, const std::string &value, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> &alloc);

std::string StringReplace(const std::string &haystack, const std::string &needle, const std::string &replace);
std::string StringJoin(const std::vector<std::string> &array, const std::string &separator);
std::vector<std::string> StringSplit(const std::string &str, char delim);

/*
    Example usage:
        MatchMod("+# to Life", "+12.3 to Life", &result);
    Will return true if matches and save average value to output.
*/
bool MatchMod(const char *match, const char *mod, double *output);

std::string TimeAgoInWords(const QDateTime buyout_time);
}
