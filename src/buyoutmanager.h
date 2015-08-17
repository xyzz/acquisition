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
#include "bucket.h"
#include <QDateTime>

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
    CURRENCY_DIVINE_ORB
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
    "Divine Orb"
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
    "divine"
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

class BuyoutManager;

struct Buyout {
    double value;
    BuyoutType type;
    Currency currency;
    QString set_by;
    QDateTime last_update;

    inline bool operator == (const Buyout &rhs) const {
        return (currency == rhs.currency &&
                type == rhs.type &&
                value == rhs.value);
    }

    inline bool operator<(const Buyout &rhs) const {
        if (type != rhs.type)
            return type < rhs.type;
        if (currency != rhs.currency)
            return currency < rhs.currency;
        return value < rhs.value;
    }
};

class DataManager;

class BuyoutManager {
public:
    explicit BuyoutManager(DataManager &data_manager);
    void Set(const Item &item, Buyout &buyout, QString setter = "");
    Buyout Get(const Item &item) const;
    void Delete(const Item &item);
    bool Exists(const Item &item) const;

    static bool Equal(const Buyout &buyout1, const Buyout &buyout2);
    static QString Generate(const Buyout &buyout);

    void SetTab(const Bucket &tab, const Buyout &buyout);
    Buyout GetTab(const std::string &tab) const;
    void DeleteTab(const Bucket &tab);
    bool ExistsTab(const std::string &tab) const;

    void Save();
    void Load();
    bool IsItemManuallySet(const Item &item) const;
    void UpdateTabItems(const Bucket &tab);
private:
    std::string ItemHash(const Item &item) const;
    std::string Serialize(const std::map<std::string, Buyout> &buyouts);
    void Deserialize(const std::string &data, std::map<std::string, Buyout> *buyouts);

    DataManager &data_manager_;
    std::map<std::string, Buyout> buyouts_;
    std::map<std::string, Buyout> tab_buyouts_;
    bool save_needed_;
};

