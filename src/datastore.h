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
#include <vector>

#include "currencymanager.h"

class DataStore {
public:
    virtual ~DataStore() {};
    virtual void Set(const std::string &key, const std::string &value) = 0;
    virtual std::string Get(const std::string &key, const std::string &default_value = "") = 0;
    virtual void InsertCurrencyUpdate(const CurrencyUpdate &update) = 0;
    virtual std::vector<CurrencyUpdate> GetAllCurrency() = 0;
    virtual void SetBool(const std::string &key, bool value) = 0;
    virtual bool GetBool(const std::string &key, bool default_value = false) = 0;
    virtual void SetInt(const std::string &key, int value) = 0;
    virtual int GetInt(const std::string &key, int default_value = 0) = 0;
};
