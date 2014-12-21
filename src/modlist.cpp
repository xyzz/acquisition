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

#include "modlist.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "item.h"
#include "porting.h"

// Actual list of mods is computed at runtime
std::vector<std::string> mod_list;

// These are just summed, and the mod named as the first element of a vector is generated with value equaling the sum.
// Both implicit and explicit fields are considered.
// This is pretty much the same list as poe.trade uses
const std::vector<std::vector<std::string>> simple_sum = {
    { "#% increased Block and Stun Recovery" },
    { "#% increased Mana Regeneration Rate" },
    { "#% increased Physical Damage" },
    { "#% increased Rarity of Items found" },
    { "#% increased Spell Damage" },
    { "#% increased Stun Duration on enemies" },
    { "+# to Dexterity", "+# to Strength and Dexterity", "+# to Dexterity and Intelligence", "+# to all Attributes" },
    { "+# to Strength", "+# to Strength and Dexterity", "+# to Strength and Intelligence", "+# to all Attributes" },
    { "+# to Intelligence", "+# to Dexterity and Intelligence", "+# to Strength and Intelligence", "+# to all Attributes" },
    { "+# to all Attributes" },
    { "+# to maximum Energy Shield" },
    { "+# to maximum Life" },
    { "+# to maximum Mana" },
    { "+#% to Cold Resistance", "+#% to Fire and Cold Resistances", "+#% to Cold and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to Fire Resistance", "+#% to Fire and Cold Resistances", "+#% to Fire and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to Lightning Resistance", "+#% to Cold and Lightning Resistances", "+#% to Fire and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to all Elemental Resistances" },
    { "+# to Level of Aura Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Strength Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Bow Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Cold Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Elemental Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Fire Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Lightning Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Melee Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Minion Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Movement Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Spell Gems in this item", "+# to Level of Gems in this item" },
    { "+# to Level of Vaal Gems in this item", "+# to Level of Gems in this item" },
    { "#% increased Fire Spell Damage", "#% increased Fire Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Cold Spell Damage", "#% increased Cold Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Lightning Spell Damage", "#% increased Lightning Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Fire Damage with Weapons", "#% increased Fire Damage", "#% increased Elemental Damage with Weapons", "#% increased Elemental Damage" },
    { "#% increased Cold Damage with Weapons", "#% increased Cold Damage", "#% increased Elemental Damage with Weapons", "#% increased Elemental Damage" },
    { "#% increased Lightning Damage with Weapons", "#% increased Lightning Damage", "#% increased Elemental Damage with Weapons", "#% increased Elemental Damage" },
    { "#% increased Elemental Damage with Weapons", "#% increased Elemental Damage" },
    { "#% increased Burning Damage", "#% increased Fire Damage", "#% increased Elemental Damage" },
    { "#% increased Fire Area Damage", "#% increased Fire Damage", "#% increased Elemental Damage" },
    { "#% increased Critical Strike Chance for Spells", "#% increased Global Critical Strike Chance" },
    { "#% increased Global Critical Strike Multiplier" },
    { "#% of Physical Attack Damage Leeched as Life" },
    { "Adds # Physical Damage", "Adds #-# Physical Damage" },
    { "Adds # Fire Damage", "Adds #-# Fire Damage" },
    { "Adds # Elemental Damage", "Adds #-# Fire Damage", "Adds #-# Cold Damage", "Adds #-# Lightning Damage" },
    { "Adds # Damage", "Adds #-# Fire Damage", "Adds #-# Cold Damage", "Adds #-# Lightning Damage", "Adds #-# Physical Damage", "Adds #-# Chaos Damage" },
    { "#% increased Cast Speed" },
};

std::vector<std::unique_ptr<ModGenerator>> mod_generators;

void InitModlist() {
    for (auto &list : simple_sum) {
        mod_list.push_back(list[0]);

        mod_generators.push_back(std::make_unique<SumModGenerator>(list[0], list));
    }
}

SumModGenerator::SumModGenerator(const std::string &name, const std::vector<std::string> &sum):
    name_(name)
{
    for (auto &mod_s : sum) {
        QString mod(mod_s.c_str());
        mod = mod.replace("+", "\\+").replace("#", "([\\d\\.]+)");
        regexes_.push_back(QRegExp(mod));
    }
}

bool SumModGenerator::Match(const char *mod, double *output) {
    for (auto &regex : regexes_) {
        QRegExp copy(regex);
        if (copy.exactMatch(mod)) {
            *output = 0;
            for (int i = 1; i <= copy.captureCount(); ++i)
                *output += copy.cap(i).toDouble();
            *output /= copy.captureCount();
            return true;
        }
    }

    return false;
}

void SumModGenerator::Generate(const rapidjson::Value &json, ModTable *output) {
    bool mod_present = false;
    double sum = 0;
    for (auto &type : { "implicitMods", "explicitMods" }) {
        if (!json.HasMember(type))
            continue;
        for (auto &mod : json[type]) {
            double result;
            if (Match(mod.GetString(), &result)) {
                sum += result;
                mod_present = true;
            }
        }
    }

    if (mod_present)
        (*output)[name_] = sum;
}