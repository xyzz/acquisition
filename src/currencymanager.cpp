/*
    Copyright 2015 Guillaume DUPUY <glorf@glorf.fr>

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
#include <QWidget>
#include "application.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "currencymanager.h"
#include "item.h"
CurrencyManager::CurrencyManager(Application &app) : app_(app), data_(app.data_manager())
{
    if (data_.Get("currency_base") == "")
        InitCurrency();
    LoadCurrency();
}

CurrencyManager::~CurrencyManager()
{
    SaveCurrency();
}

void CurrencyManager::UpdateBaseValue(int ind, double value)
{
    currencys_[ind].base = value;
}

void CurrencyManager::UpdateExaltedValue()
{
    for( auto &currency : currencys_)
    {
        if (currency.base != 0)
            currency.exalt = currency.count / currency.base;
    }
}

double CurrencyManager::TotalExaltedValue()
{
    double out = 0;
    for (auto currency : currencys_)
    {
        out += currency.exalt;
    }
    return out;
}

void CurrencyManager::ClearCurrency()
{
    for( auto &currency : currencys_)
    {
        currency.count = 0;
    }
}

void CurrencyManager::InitCurrency()
{
    std::string value = "";
    for (auto curr : CurrencyAsString) {
        value += "10;";
    }
    value.pop_back();//Remove the last ";"
    data_.Set("currency_base", value);
}

void CurrencyManager::LoadCurrency()
{
    //Way easier to use QT function instead of re-implementing a split() in C++
    QStringList list = QString(data_.Get("currency_base").c_str()).split(";");
    for (int i=0; i < list.size(); i++) {
        CurrencyItem curr;
        curr.count = 0;
        curr.name = CurrencyAsString[i];
        curr.base = list[i].toDouble();
        curr.exalt = 0;
        currencys_.push_back(curr);
    }
}

void CurrencyManager::SaveCurrency()
{
    std::string value = "";
    for (auto currency : currencys_) {
        value += std::to_string(currency.base) + ";";
    }
    value.pop_back();//Remove the last ";"
    data_.Set("currency_base", value);
}

void CurrencyManager::ParseSingleItem(std::shared_ptr<Item> item)
{
    for (unsigned int i=0; i < currencys_.size(); i++) {
        if (item->PrettyName() == currencys_[i].name) {
            currencys_[i].count += item->count();
        }
    }
}
