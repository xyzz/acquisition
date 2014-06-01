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
#include <vector>
#include "jsoncpp/json.h"

const int PIXELS_PER_SLOT = 47;
const int INVENTORY_SLOTS = 12;

enum FRAME_TYPES {
    FRAME_TYPE_NORMAL = 0,
    FRAME_TYPE_MAGIC = 1,
    FRAME_TYPE_RARE = 2,
    FRAME_TYPE_UNIQUE = 3,
    FRAME_TYPE_GEM = 4,
    FRAME_TYPE_CURRENCY = 5,
};

enum ELEMENTAL_DAMAGE_TYPES {
    ED_FIRE = 4,
    ED_COLD = 5,
    ED_LIGHTNING = 6,
};

class Item {
public:
    Item(const Json::Value &json, int tab, std::string tab_caption);
    std::string name() const { return name_; }
    std::string typeLine() const { return typeLine_; }
    std::string PrettyName() const;
    const Json::Value &json() const { return json_; }
    bool corrupted() const { return corrupted_; }
    int w() const { return w_; }
    int h() const { return h_; }
    int x() const { return x_; }
    int y() const { return y_; }
    int frameType() const { return frameType_; }
    const std::string &icon() const { return icon_; }
    const std::vector<std::string>& explicitMods() const { return explicitMods_; }
    int tab() const { return tab_; }
    const std::string &tab_caption() const { return tab_caption_; }
    const std::map<std::string, std::string> &properties() const { return properties_; }
    const std::string &hash() const { return hash_; }
    const std::vector<std::pair<std::string, int>> &elemental_damage() const { return elemental_damage_; }
    const std::map<std::string, int> &requirements() const { return requirements_; }
    double DPS() const;
    double pDPS() const;
    double eDPS() const;
    int sockets() const { return sockets_; }
    int links() const { return links_; }
    int sockets_r() const { return sockets_r_; }
    int sockets_g() const { return sockets_g_; }
    int sockets_b() const { return sockets_b_; }
    int sockets_w() const { return sockets_w_; }
private:
    std::string UniqueProperties(const std::string &name);

    Json::Value json_;
    std::string name_;
    std::string typeLine_;
    bool corrupted_;
    int w_, h_;
    int x_, y_;
    int frameType_;
    std::string icon_;
    std::vector<std::string> explicitMods_;
    int tab_;
    std::string tab_caption_;
    std::map<std::string, std::string> properties_;
    std::string hash_;
    // vector of pairs [damage, type]
    std::vector<std::pair<std::string, int>> elemental_damage_;
    int sockets_, links_;
    int sockets_r_, sockets_g_, sockets_b_, sockets_w_;
    std::map<std::string, int> requirements_;
};

typedef std::vector<std::shared_ptr<Item>> Items;
