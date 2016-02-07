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

#include "item.h"
#include <QDateTime>
#include <set>

class ItemLocation;

enum Currency {
    CURRENCY_NONE,
    CURRENCY_ORB_OF_ALTERATION,
    CURRENCY_ORB_OF_FUSING,
    CURRENCY_ORB_OF_ALCHEMY,
    CURRENCY_CHAOS_ORB,
    CURRENCY_GCP,
    CURRENCY_EXALTED_ORB,
    CURRENCY_CHROMATIC_ORB,
    CURRENCY_JEWELLERS_ORB,
    CURRENCY_ORB_OF_CHANCE,
    CURRENCY_CARTOGRAPHERS_CHISEL,
    CURRENCY_ORB_OF_SCOURING,
    CURRENCY_BLESSED_ORB,
    CURRENCY_ORB_OF_REGRET,
    CURRENCY_REGAL_ORB,
    CURRENCY_DIVINE_ORB,
    CURRENCY_VAAL_ORB
};

const std::vector<std::string> CurrencyAsString({
    "",
    "Orb of Alteration",
    "Orb of Fusing",
    "Orb of Alchemy",
    "Chaos Orb",
    "Gemcutter's Prism",
    "Exalted Orb",
    "Chromatic Orb",
    "Jeweller's Orb",
    "Orb of Chance",
    "Cartographer's Chisel",
    "Orb of Scouring",
    "Blessed Orb",
    "Orb of Regret",
    "Regal Orb",
    "Divine Orb",
    "Vaal Orb"
});

const std::vector<std::string> CurrencyAsTag({
    "",
    "alt",
    "fuse",
    "alch",
    "chaos",
    "gcp",
    "exa",
    "chrom",
    "jew",
    "chance",
    "chisel",
    "scour",
    "blessed",
    "regret",
    "regal",
    "divine",
    "vaal"
});

enum BuyoutType {
    BUYOUT_TYPE_NONE,
    BUYOUT_TYPE_BUYOUT,
    BUYOUT_TYPE_FIXED,
    BUYOUT_TYPE_NO_PRICE
};

const std::vector<std::string> BuyoutTypeAsTag({
    "",
    "b/o",
    "price",
    "no price",
});

const std::vector<std::string> BuyoutTypeAsPrefix({
    "",
    " ~b/o ",
    " ~price ",
    "",
});

struct Buyout {
    double value;
    BuyoutType type;
    Currency currency;
    QDateTime last_update;
    bool inherited = false;
    bool operator==(const Buyout &o) const;
    bool operator!=(const Buyout &o) const;
    Buyout() :
        value(0),
        type(BUYOUT_TYPE_NONE),
        currency(CURRENCY_NONE)
    {}
    Buyout(double value_, BuyoutType type_, Currency currency_, QDateTime last_update_) :
        value(value_),
        type(type_),
        currency(currency_),
        last_update(last_update_)
    {}
};

class DataStore;

class BuyoutManager {
public:
    explicit BuyoutManager(DataStore &data);
    void Set(const Item &item, const Buyout &buyout);
    Buyout Get(const Item &item) const;
    void Delete(const Item &item);
    bool Exists(const Item &item) const;

    void SetTab(const std::string &tab, const Buyout &buyout);
    Buyout GetTab(const std::string &tab) const;
    void DeleteTab(const std::string &tab);
    bool ExistsTab(const std::string &tab) const;

    void SetRefreshChecked(const ItemLocation &tab, bool value);
    bool GetRefreshChecked(const ItemLocation &tab) const;

    bool GetRefreshLocked(const ItemLocation &tab) const;
    void SetRefreshLocked(const ItemLocation &tab);
    void ClearRefreshLocks();

    void SetStashTabLocations(const std::vector<ItemLocation> &tabs);
    const std::vector<ItemLocation> GetStashTabLocations() const;
    void Clear();

    void Save();
    void Load();

    void MigrateItem(const Item &item);
private:
    std::string Serialize(const std::map<std::string, Buyout> &buyouts);
    void Deserialize(const std::string &data, std::map<std::string, Buyout> *buyouts);

    std::string Serialize(const std::map<std::string, bool> &obj);
    void Deserialize(const std::string &data, std::map<std::string, bool> &obj);

    DataStore &data_;
    std::map<std::string, Buyout> buyouts_;
    std::map<std::string, Buyout> tab_buyouts_;
    std::map<std::string, bool> refresh_checked_;
    std::set<std::string> refresh_locked_;
    bool save_needed_;
    std::vector<ItemLocation> tabs_;
};

