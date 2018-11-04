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

#include <QStringList>

#include "item.h"
#include "porting.h"
#include "util.h"

// Actual list of mods is computed at runtime
QStringList mod_string_list;

// These are just summed, and the mod named as the first element of a vector is generated with value equaling the sum.
// Both implicit and explicit fields are considered.
// This is pretty much the same list as poe.trade uses
const std::vector<std::vector<std::string>> simple_sum = {
    { "#% increased Quantity of Items found" },
    { "#% increased Rarity of Items found" },
    { "+# to maximum Life" },
    { "#% increased maximum Life" },
    { "+# to maximum Energy Shield" },
    { "+# to maximum Mana" },
    { "#% increased Mana Regeneration Rate" },
    { "+#% to Fire Resistance", "+#% to Fire and Cold Resistances", "+#% to Fire and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to Cold Resistance", "+#% to Fire and Cold Resistances", "+#% to Cold and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to Lightning Resistance", "+#% to Cold and Lightning Resistances", "+#% to Fire and Lightning Resistances", "+#% to all Elemental Resistances" },
    { "+#% to all Elemental Resistances" },
    { "+#% to Chaos Resistance" },
    { "+# to Level of Socketed Aura Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Fire Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Cold Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Lightning Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Chaos Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Elemental Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Spell Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Bow Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Minion Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Vaal Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Melee Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Movement Gems", "+# to Level of Socketed Gems" },
    { "+# to Level of Socketed Strength Gems", "+# to Level of Socketed Gems" },
    { "#% increased Physical Damage", "#% increased Global Physical Damage" },
    { "#% increased Spell Damage" },
    { "#% increased Fire Damage", "#% increased Elemental Damage" },
    { "#% increased Cold Damage", "#% increased Elemental Damage" },
    { "#% increased Lightning Damage", "#% increased Elemental Damage" },
    { "#% increased Chaos Damage" },
    { "#% increased Fire Damage with Attack Skills", "#% increased Fire Damage", "#% increased Elemental Damage with Attack Skills", "#% increased Elemental Damage" },
    { "#% increased Cold Damage with Attack Skills", "#% increased Cold Damage", "#% increased Elemental Damage with Attack Skills", "#% increased Elemental Damage" },
    { "#% increased Lightning Damage with Attack Skills", "#% increased Lightning Damage", "#% increased Elemental Damage with Attack Skills", "#% increased Elemental Damage" },
    { "#% increased Elemental Damage with Attack Skills", "#% increased Elemental Damage" },
    { "#% increased Fire Spell Damage", "#% increased Fire Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Cold Spell Damage", "#% increased Cold Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Lightning Spell Damage", "#% increased Lightning Damage", "#% increased Elemental Damage", "#% increased Spell Damage" },
    { "#% increased Global Critical Strike Chance" },
    { "#% increased Critical Strike Chance for Spells", "#% increased Global Critical Strike Chance", "Spells have +#% to Critical Strike Chance " },
    { "+#% to Global Critical Strike Multiplier" },
    { "+# to Accuracy Rating" },
    { "#% increased Accuracy Rating" },
    { "#% increased Area Damage" },
    { "#% increased Damage over Time" },
    { "#% increased Burning Damage", "#% increased Fire Damage", "#% increased Elemental Damage", "#% increased Damage over Time" },
    { "#% of Physical Attack Damage Leeched as Life" },
    { "#% of Physical Attack Damage Leeched as Mana" },
    { "Adds # Physical Damage to Attacks", "Adds # to # Physical Damage", "Adds # to # Physical Damage to Attacks" },
    { "Adds # Elemental Damage to Attacks", "Adds # to # Fire Damage", "Adds # to # Cold Damage", "Adds # to # Lightning Damage", "Adds # to # Fire Damage to Attacks", "Adds # to # Cold Damage to Attacks", "Adds # to # Lightning Damage to Attacks" },
    { "Adds # Chaos Damage to Attacks", "Adds # to # Chaos Damage to Attacks" },
    { "Adds # Damage to Attacks", "Adds # to # Fire Damage", "Adds # to # Cold Damage", "Adds # to # Lightning Damage", "Adds # to # Physical Damage", "Adds # to # Chaos Damage", "Adds # to # Fire Damage to Attacks", "Adds # to # Cold Damage to Attacks", "Adds # to # Lightning Damage to Attacks", "Adds # to # Physical Damage to Attacks", "Adds # to # Chaos Damage to Attacks" },
    { "Adds # Elemental Damage to Spells", "Adds # to # Fire Damage to Spells", "Adds # to # Cold Damage to Spells", "Adds # to # Lightning Damage to Spells" },
    { "Adds # Chaos Damage to Spells", "Adds # to # Chaos Damage to Spells" },
    { "Adds # Damage to Spells", "Adds # to # Fire Damage to Spells", "Adds # to # Cold Damage to Spells", "Adds # to # Lightning Damage to Spells", "Adds # to # Chaos Damage to Spells" },
    { "#% increased Attack Speed", "#% increased Attack and Cast Speed" },
    { "#% increased Cast Speed", "#% increased Attack and Cast Speed" },
    { "#% increased Movement Speed" },
    { "+# to Dexterity", "+# to Strength and Dexterity", "+# to Dexterity and Intelligence", "+# to all Attributes" },
    { "+# to Strength", "+# to Strength and Dexterity", "+# to Strength and Intelligence", "+# to all Attributes" },
    { "+# to Intelligence", "+# to Dexterity and Intelligence", "+# to Strength and Intelligence", "+# to all Attributes" },
    { "+# to all Attributes" },
    { "#% increased Stun Duration on enemies" },
    { "#% increased Block and Stun Recovery" },
    // Master mods
    { "+#% to Quality of Socketed Support Gems" },
    { "-# to Total Mana Cost of Skills" },
    { "+# to Level of Socketed Support Gems", "+# to Level of Socketed Gems" },
    { "Causes Bleeding on Hit" },
    { "#% increased Life Leeched per second" },
    { "Hits can't be Evaded" },
    { "#% increased Damage" },
    { "* Zana's legacy Map Invasion Boss mod", "Area is inhabited by # additional Invasion Boss" },
    { "* Zana's Map Quantity and Rarity mod", "This Map's Modifiers to Quantity of Items found also apply to Rarity" },
    // Master meta-crafting
    { "Prefixes Cannot Be Changed" },
    { "Suffixes Cannot Be Changed" },
    { "Cannot roll Attack Mods" },
    { "Cannot roll Caster Mods" },
    { "Can have multiple Crafted Mods" },
    { "* Leo's Level-28-capped-rolls mod", "Cannot roll Mods with Required Level above #" },
};

std::vector<std::unique_ptr<ModGenerator>> mod_generators;

void InitModlist() {
    for (auto &list : simple_sum) {
        mod_string_list.push_back(list[0].c_str());

        mod_generators.push_back(std::make_unique<SumModGenerator>(list[0], list));
    }
}

SumModGenerator::SumModGenerator(const std::string &name, const std::vector<std::string> &sum):
    name_(name),
    matches_(sum)
{}

bool SumModGenerator::Match(const char *mod, double *output) {
    bool found = false;
    *output = 0.0;
    for (auto &match : matches_) {
        double result = 0.0;
        if (Util::MatchMod(match.c_str(), mod, &result)) {
            *output += result;
            found = true;
        }
    }

    return found;
}

void SumModGenerator::Generate(const rapidjson::Value &json, ModTable *output) {
    bool mod_present = false;
    double sum = 0;
    for (auto &type : { "implicitMods", "explicitMods" }) {
        if (!json.HasMember(type) || !json[type].IsArray())
            continue;
        for (auto &mod : json[type]) {
            if (!mod.IsString())
                continue;

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
