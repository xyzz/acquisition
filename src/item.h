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
#include "rapidjson/document.h"

#include "itemconstants.h"
#include "itemlocation.h"

class Item {
public:
    explicit Item(const rapidjson::Value &json);
    std::string name() const { return name_; }
    std::string typeLine() const { return typeLine_; }
    std::string PrettyName() const;
    bool corrupted() const { return corrupted_; }
    int w() const { return w_; }
    int h() const { return h_; }
    int frameType() const { return frameType_; }
    const std::string &icon() const { return icon_; }
    const std::vector<std::string>& explicitMods() const { return explicitMods_; }
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
    const ItemLocation &location() const { return location_; }
    int count() const { return count_; };

private:
    ItemLocation location_;
    std::string name_;
    std::string typeLine_;
    bool corrupted_;
    int w_, h_;
    int frameType_;
    std::string icon_;
    std::vector<std::string> explicitMods_;
    std::map<std::string, std::string> properties_;
    std::string hash_;
    // vector of pairs [damage, type]
    std::vector<std::pair<std::string, int>> elemental_damage_;
    int sockets_, links_;
    int sockets_r_, sockets_g_, sockets_b_, sockets_w_;
    std::map<std::string, int> requirements_;
    int count_;
};

typedef std::vector<std::shared_ptr<Item>> Items;
