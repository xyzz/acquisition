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

#include "item.h"

#include <utility>
#include <QString>
#include "rapidjson/document.h"

#include "modlist.h"
#include "util.h"
#include "porting.h"
#include "itemlocation.h"

const std::vector<std::string> ITEM_MOD_TYPES = {
    "implicitMods", "explicitMods", "craftedMods", "cosmeticMods"
};

static std::string item_unique_properties(const rapidjson::Value &json, const std::string &name) {
    const char *name_p = name.c_str();
    if (!json.HasMember(name_p))
        return "";
    std::string result;
    for (auto &prop : json[name_p]) {
        result += std::string(prop["name"].GetString()) + "~";
        for (auto &value : prop["values"])
            result += std::string(value[0].GetString()) + "~";
    }
    return result;
}

// Fix up names, remove all <<set:X>> modifiers
static std::string fixup_name(const std::string &name) {
    std::string::size_type right_shift = name.rfind(">>");
    if (right_shift != std::string::npos) {
        return name.substr(right_shift + 2);
    }
    return name;
}

Item::Item(const std::string &name, const ItemLocation &location) :
    name_(name),
    location_(location),
    hash_(Util::Md5(name)) // Unique enough for tests
{}

Item::Item(const rapidjson::Value &json) :
    location_(ItemLocation(json)),
    name_(fixup_name(json["name"].GetString())),
    typeLine_(fixup_name(json["typeLine"].GetString())),
    corrupted_(json["corrupted"].GetBool()),
    identified_(json["identified"].GetBool()),
    w_(json["w"].GetInt()),
    h_(json["h"].GetInt()),
    frameType_(json["frameType"].GetInt()),
    icon_(json["icon"].GetString()),
    sockets_cnt_(0),
    links_cnt_(0),
    sockets_({ 0, 0, 0, 0 }),
    has_mtx_(false)
{
    for (auto &mod_type : ITEM_MOD_TYPES) {
        text_mods_[mod_type] = std::vector<std::string>();
        if (json.HasMember(mod_type.c_str())) {
            auto &mods = text_mods_[mod_type];
            for (auto &mod : json[mod_type.c_str()])
                mods.push_back(mod.GetString());
        }
    }

    if (json.HasMember("properties")) {
        for (auto prop_it = json["properties"].Begin(); prop_it != json["properties"].End(); ++prop_it) {
            auto &prop = *prop_it;
            std::string name = prop["name"].GetString();
            if (name == "Map Level")
                name = "Level";
            if (name == "Elemental Damage") {
                for (auto value_it = prop["values"].Begin(); value_it != prop["values"].End(); ++value_it)
                    elemental_damage_.push_back(std::make_pair((*value_it)[0].GetString(), (*value_it)[1].GetInt()));
            }
            else {
                if (prop["values"].Size())
                    properties_[name] = prop["values"][0][0].GetString();
            }

            ItemProperty property;
            property.name = name;
            property.display_mode = prop["displayMode"].GetInt();
            for (auto &value : prop["values"]) {
                ItemPropertyValue v;
                v.str = value[0].GetString();
                v.type = value[1].GetInt();
                property.values.push_back(v);
            }
            text_properties_.push_back(property);
        }
    }

    if (json.HasMember("requirements")) {
        for (auto &req : json["requirements"]) {
            std::string name = req["name"].GetString();
            std::string value = req["values"][0][0].GetString();
            requirements_[name] = std::atoi(value.c_str());
            ItemPropertyValue v;
            v.str = value;
            v.type = req["values"][0][1].GetInt();
            text_requirements_.push_back({ name, v });
        }
    }

    if (json.HasMember("sockets")) {
        ItemSocketGroup current_group = { 0, 0, 0, 0 };
        sockets_cnt_ = json["sockets"].Size();
        int counter = 0, prev_group = -1;
        for (auto &socket : json["sockets"]) {
            ItemSocket current_socket = { static_cast<unsigned char>(socket["group"].GetInt()), socket["attr"].GetString()[0] };
            text_sockets_.push_back(current_socket);
            if (prev_group != current_socket.group) {
                counter = 0;
                socket_groups_.push_back(current_group);
                current_group = { 0, 0, 0, 0 };
            }
            prev_group = current_socket.group;
            ++counter;
            links_cnt_ = std::max(links_cnt_, counter);
            switch (current_socket.attr) {
            case 'S':
                sockets_.r++;
                current_group.r++;
                break;
            case 'D':
                sockets_.g++;
                current_group.g++;
                break;
            case 'I':
                sockets_.b++;
                current_group.b++;
                break;
            case 'G':
                sockets_.w++;
                current_group.w++;
                break;
            }
        }
        socket_groups_.push_back(current_group);
    }

    CalculateHash(json);

    count_ = 1;
    if (properties_.find("Stack Size") != properties_.end()) {
        std::string size = properties_["Stack Size"];
        if (size.find("/") != std::string::npos) {
            size = size.substr(0, size.find("/"));
            count_ = std::stoi(size);
        }
    }

    has_mtx_ = json.HasMember("cosmeticMods");

    GenerateMods(json);
}

std::string Item::PrettyName() const {
    if (!name_.empty())
        return name_ + " " + typeLine_;
    return typeLine_;
}

double Item::DPS() const {
    return pDPS() + eDPS();
}

double Item::pDPS() const {
    if (!properties_.count("Physical Damage") || !properties_.count("Attacks per Second"))
        return 0;
    double aps = std::stod(properties_.at("Attacks per Second"));
    std::string pd = properties_.at("Physical Damage");

    return aps * Util::AverageDamage(pd);
}

double Item::eDPS() const {
    if (elemental_damage_.empty() || !properties_.count("Attacks per Second"))
        return 0;
    double damage = 0;
    for (auto &x : elemental_damage_)
        damage += Util::AverageDamage(x.first);
    double aps = std::stod(properties_.at("Attacks per Second"));
    return aps * damage;
}

void Item::GenerateMods(const rapidjson::Value &json) {
    for (auto &generator : mod_generators)
        generator->Generate(json, &mod_table_);
}

void Item::CalculateHash(const rapidjson::Value &json) {
    std::string unique_old(name_ + "~" + typeLine_ + "~");
    std::string unique_new(std::string(json["name"].GetString()) + "~" + json["typeLine"].GetString() + "~");

    std::string unique_common;

    if (json.HasMember("explicitMods"))
        for (auto &mod : json["explicitMods"])
            unique_common += std::string(mod.GetString()) + "~";

    if (json.HasMember("implicitMods"))
        for (auto &mod : json["implicitMods"])
            unique_common += std::string(mod.GetString()) + "~";

    unique_common += item_unique_properties(json, "properties") + "~";
    unique_common += item_unique_properties(json, "additionalProperties") + "~";

    if (json.HasMember("sockets"))
        for (auto &socket : json["sockets"])
            unique_common += std::to_string(socket["group"].GetInt()) + "~" + socket["attr"].GetString() + "~";

    unique_old += unique_common;
    unique_new += unique_common;

    old_hash_ = Util::Md5(unique_old);
    unique_new += "~" + location_.GetUniqueHash();
    hash_ = Util::Md5(unique_new);
}
