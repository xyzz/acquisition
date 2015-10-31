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

#include "memorydatastore.h"

#include "currencymanager.h"

std::string MemoryDataStore::Get(const std::string &key, const std::string &default_value) {
    auto i = data_.find(key);
    if (i == data_.end())
        return default_value;
    return i->second;
}

void MemoryDataStore::Set(const std::string &key, const std::string &value) {
    data_[key] = value;
}

void MemoryDataStore::InsertCurrencyUpdate(const CurrencyUpdate &update) {
    currency_updates_.push_back(update);
}

std::vector<CurrencyUpdate> MemoryDataStore::GetAllCurrency() {
    return currency_updates_;
}

void MemoryDataStore::SetBool(const std::string &key, bool value) {
    SetInt(key, static_cast<int>(value));
}

bool MemoryDataStore::GetBool(const std::string &key, bool default_value) {
    return static_cast<bool>(GetInt(key, static_cast<int>(default_value)));
}

void MemoryDataStore::SetInt(const std::string &key, int value) {
    Set(key, std::to_string(value));
}

int MemoryDataStore::GetInt(const std::string &key, int default_value) {
    return std::stoi(Get(key, std::to_string(default_value)));
}
