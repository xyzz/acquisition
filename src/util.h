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
#include "jsoncpp/json-forwards.h"

class QComboBox;
class QNetworkReply;

namespace Util {
std::string Md5(const std::string &value);
double AverageDamage(const std::string &s);
void PopulateBuyoutTypeComboBox(QComboBox *combobox);
void PopulateBuyoutCurrencyComboBox(QComboBox *combobox);
int TagAsCurrency(const std::string &tag);
int TagAsBuyoutType(const std::string &tag);

int MinMaxWidth();
int LabelWidth();
int RGBWidth();
int GroupWidth();

void ParseJson(QNetworkReply *reply, Json::Value *root);
std::string GetCsrfToken(const std::string &page, const std::string &name);
std::string FindTextBetween(const std::string &page, const std::string &left, const std::string &right);
}
