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
#include <QString>
#include <unordered_map>
#include <vector>
#include "rapidjson/document.h"

#include "itemconstants.h"
#include "itemlocation.h"

extern const std::vector<QString> ITEM_MOD_TYPES;

struct ItemSocketGroup {
    int r, g, b, w;
};

struct ItemProperty {
    QString name;
    std::vector<QString> values;
    int display_mode;
};

struct ItemRequirement {
    QString name;
    QString value;
};

struct ItemSocket {
    unsigned char group;
    char attr;
};

typedef std::vector<QString> ItemMods;
typedef std::unordered_map<std::string, double> ModTable;

class Item {
public:
    explicit Item(const rapidjson::Value &json);
    QString name() const { return name_; }
    QString typeLine() const { return typeLine_; }
    QString PrettyName() const;
    bool corrupted() const { return corrupted_; }
    int w() const { return w_; }
    int h() const { return h_; }
    int frameType() const { return frameType_; }
    const QString &icon() const { return icon_; }
    const std::map<QString, QString> &properties() const { return properties_; }
    const std::vector<ItemProperty> &text_properties() const { return text_properties_; }
    const std::vector<ItemRequirement> &text_requirements() const { return text_requirements_; }
    const std::map<QString, ItemMods> &text_mods() const { return text_mods_; }
    const std::vector<ItemSocket> &text_sockets() const { return text_sockets_; }
    const QString &hash() const { return hash_; }
    const std::vector<std::pair<QString, int>> &elemental_damage() const { return elemental_damage_; }
    const std::map<QString, int> &requirements() const { return requirements_; }
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
	QString FormatGenderNames(const QString str);

    ItemLocation location_;
    QString name_;
    QString typeLine_;
    bool corrupted_;
    int w_, h_;
    int frameType_;
    QString icon_;
    std::map<QString, QString> properties_;
    QString hash_;
    // vector of pairs [damage, type]
    std::vector<std::pair<QString, int>> elemental_damage_;
    int sockets_cnt_, links_cnt_;
    ItemSocketGroup sockets_;
    std::vector<ItemSocketGroup> socket_groups_;
    std::map<QString, int> requirements_;
    int count_;
    bool has_mtx_;
    std::vector<ItemProperty> text_properties_;
    std::vector<ItemRequirement> text_requirements_;
    std::map<QString, ItemMods> text_mods_;
    std::vector<ItemSocket> text_sockets_;
    ModTable mod_table_;
};

typedef std::vector<std::shared_ptr<Item>> Items;
