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

#include <memory>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "rapidjson/document.h"

#include "itemconstants.h"
#include "itemlocation.h"

extern const std::vector<std::string> ITEM_MOD_TYPES;

struct ItemSocketGroup {
    int r, g, b, w;
};

struct ItemPropertyValue {
    std::string str;
    int type;
};

struct ItemProperty {
    std::string name;
    std::vector<ItemPropertyValue> values;
    int display_mode;
};

struct ItemRequirement {
    std::string name;
    ItemPropertyValue value;
};

struct ItemSocket {
    unsigned char group;
    char attr;
};

typedef std::vector<std::string> ItemMods;
typedef std::unordered_map<std::string, double> ModTable;

class Item {
public:
    explicit Item(const rapidjson::Value &json);
    Item(const std::string &name, const ItemLocation &location); // used by tests
    std::string name() const { return name_; }
    std::string typeLine() const { return typeLine_; }
    std::string PrettyName() const;
    bool corrupted() const { return corrupted_; }
    bool identified() const { return identified_; }
    int w() const { return w_; }
    int h() const { return h_; }
    int frameType() const { return frameType_; }
    const std::string &icon() const { return icon_; }
    const std::map<std::string, std::string> &properties() const { return properties_; }
    const std::vector<ItemProperty> &text_properties() const { return text_properties_; }
    const std::vector<ItemRequirement> &text_requirements() const { return text_requirements_; }
    const std::map<std::string, ItemMods> &text_mods() const { return text_mods_; }
    const std::vector<ItemSocket> &text_sockets() const { return text_sockets_; }
    const std::string &hash() const { return hash_; }
    const std::string &old_hash() const { return old_hash_; }
    const std::vector<std::pair<std::string, int>> &elemental_damage() const { return elemental_damage_; }
    const std::map<std::string, int> &requirements() const { return requirements_; }
    double DPS() const;
    double pDPS() const;
    double eDPS() const;
    int sockets_cnt() const { return sockets_cnt_; }
    int links_cnt() const { return links_cnt_; }
    const ItemSocketGroup &sockets() const { return sockets_; }
    const std::vector<ItemSocketGroup> &socket_groups() const { return socket_groups_; }
    const ItemLocation &location() const { return location_; }
    int count() const { return count_; };
    bool has_mtx() const { return has_mtx_; }
    const ModTable &mod_table() const { return mod_table_; }

private:
    // The point of GenerateMods is to create combined (e.g. implicit+explicit) poe.trade-like mod map to be searched by mod filter.
    // For now it only does that for a small chosen subset of mods (think "popular" + "pseudo" sections at poe.trade)
    void GenerateMods(const rapidjson::Value &json);
    void CalculateHash(const rapidjson::Value &json);

    ItemLocation location_;
    std::string name_;
    std::string typeLine_;
    bool corrupted_;
    bool identified_;
    int w_, h_;
    int frameType_;
    std::string icon_;
    std::map<std::string, std::string> properties_;
    std::string old_hash_, hash_;
    // vector of pairs [damage, type]
    std::vector<std::pair<std::string, int>> elemental_damage_;
    int sockets_cnt_, links_cnt_;
    ItemSocketGroup sockets_;
    std::vector<ItemSocketGroup> socket_groups_;
    std::map<std::string, int> requirements_;
    int count_;
    bool has_mtx_;
    std::vector<ItemProperty> text_properties_;
    std::vector<ItemRequirement> text_requirements_;
    std::map<std::string, ItemMods> text_mods_;
    std::vector<ItemSocket> text_sockets_;
    ModTable mod_table_;
};

typedef std::vector<std::shared_ptr<Item>> Items;
