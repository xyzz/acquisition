/*
    Copyright 2015 Ilya Zhuravlev

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
#include <vector>
#include <map>

#include "datastore.h"

class MemoryDataStore : public DataStore {
public:
    void Set(const std::string &key, const std::string &value);
    std::string Get(const std::string &key, const std::string &default_value = "");
    void InsertCurrencyUpdate(const CurrencyUpdate &update);
    std::vector<CurrencyUpdate> GetAllCurrency();
    void SetBool(const std::string &key, bool value);
    bool GetBool(const std::string &key, bool default_value = false);
    void SetInt(const std::string &key, int value);
    int GetInt(const std::string &key, int default_value = 0);
private:
    std::map<std::string, std::string> data_;
    std::vector<CurrencyUpdate> currency_updates_;
};
