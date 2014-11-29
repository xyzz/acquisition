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

#include "util.h"
#include "porting.h"
#include "itemlocation.h"

static std::string item_unique_properties(const rapidjson::Value &json, const std::string &name) {
    const char *name_p = name.c_str();
    if (!json.HasMember(name_p))
        return "";
    std::string result;
    for (auto prop_it = json[name_p].Begin(); prop_it != json[name_p].End(); ++prop_it) {
        auto &prop = *prop_it;
        result += std::string(prop["name"].GetString()) + "~";
        for (auto value_it = prop["values"].Begin(); value_it != prop["values"].End(); ++value_it)
            result += std::string((*value_it)[0].GetString()) + "~";
    }
    return result;
}

Item::Item(const rapidjson::Value &json) :
    location_(ItemLocation(json)),
    name_(json["name"].GetString()),
    typeLine_(json["typeLine"].GetString()),
    corrupted_(json["corrupted"].GetBool()),
    w_(json["w"].GetInt()),
    h_(json["h"].GetInt()),
    frameType_(json["frameType"].GetInt()),
    icon_(json["icon"].GetString()),
    sockets_cnt_(0),
    links_cnt_(0),
    sockets_({ 0, 0, 0, 0 })
{
    if (json.HasMember("explicitMods"))
        for (auto mod = json["explicitMods"].Begin(); mod != json["explicitMods"].End(); ++mod)
            explicitMods_.push_back(mod->GetString());

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
        }
    }

    if (json.HasMember("requirements")) {
        for (auto req_it = json["requirements"].Begin(); req_it != json["requirements"].End(); ++req_it) {
            auto &req = *req_it;
            int value = QString(req["values"][0][0].GetString()).toInt();
            requirements_[req["name"].GetString()] = value;
        }
    }

    if (json.HasMember("sockets")) {
        ItemSocketGroup current_group = { 0, 0, 0, 0 };
        sockets_cnt_ = json["sockets"].Size();
        int counter = 0, prev = -1;
        for (auto socket_it = json["sockets"].Begin(); socket_it != json["sockets"].End(); ++socket_it) {
            int cur = (*socket_it)["group"].GetInt();
            if (prev != cur) {
                counter = 0;
                socket_groups_.push_back(current_group);
                current_group = { 0, 0, 0, 0 };
            }
            prev = cur;
            ++counter;
            links_cnt_ = std::max(links_cnt_, counter);
            switch ((*socket_it)["attr"].GetString()[0]) {
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

    std::string unique(std::string(json["name"].GetString()) + "~" + json["typeLine"].GetString() + "~");

    if (json.HasMember("explicitMods"))
        for (auto mod_it = json["explicitMods"].Begin(); mod_it != json["explicitMods"].End(); ++mod_it)
            unique += std::string(mod_it->GetString()) + "~";

    if (json.HasMember("implicitMods"))
        for (auto mod_it = json["implicitMods"].Begin(); mod_it != json["implicitMods"].End(); ++mod_it)
            unique += std::string(mod_it->GetString()) + "~";

    unique += item_unique_properties(json, "properties") + "~";
    unique += item_unique_properties(json, "additionalProperties") + "~";

    if (json.HasMember("sockets"))
        for (auto socket_it = json["sockets"].Begin(); socket_it != json["sockets"].End(); ++socket_it)
            unique += std::to_string((*socket_it)["group"].GetInt()) + "~" + (*socket_it)["attr"].GetString() + "~";

    hash_ = Util::Md5(unique);

    count_ = 1;
    if (properties_.find("Stack Size") != properties_.end()) {
        std::string size = properties_["Stack Size"];
        if (size.find("/") != std::string::npos) {
            size = size.substr(0, size.find("/"));
            count_ = std::stoi(size);
        }
    }
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
