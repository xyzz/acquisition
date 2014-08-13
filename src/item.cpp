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
#include "jsoncpp/json.h"

#include "util.h"
#include "porting.h"
#include "itemlocation.h"

Item::Item(const Json::Value &json) :
    json_(json),
    location_(ItemLocation(json)),
    name_(json["name"].asString()),
    typeLine_(json["typeLine"].asString()),
    corrupted_(json["corrupted"].asBool()),
    w_(json["w"].asInt()),
    h_(json["h"].asInt()),
    frameType_(json["frameType"].asInt()),
    icon_(json["icon"].asString()),
    sockets_(0),
    links_(0),
    sockets_r_(0),
    sockets_g_(0),
    sockets_b_(0),
    sockets_w_(0)
{
    for (auto mod : json["explicitMods"])
        explicitMods_.push_back(mod.asString());

    for (auto &property : json["properties"]) {
        std::string name = property["name"].asString();
        if (name == "Map Level")
            name = "Level";
        if (name == "Elemental Damage") {
            for (auto &value : property["values"])
                elemental_damage_.push_back(std::make_pair(value[0].asString(), value[1].asInt()));
        } else {
            properties_[name] = property["values"][0][0].asString();
        }
    }

    for (auto &requirement : json["requirements"]) {
        int value = QString(requirement["values"][0][0].asString().c_str()).toInt();
        requirements_[requirement["name"].asString()] = value;
    }

    sockets_ = json["sockets"].size();
    int counter = 0, prev = -1;
    for (auto &socket : json["sockets"]) {
        int cur = socket["group"].asInt();
        if (prev != cur)
            counter = 0;
        prev = cur;
        ++counter;
        links_ = std::max(links_, counter);
        switch (socket["attr"].asString()[0]) {
        case 'S':
            sockets_r_++;
            break;
        case 'D':
            sockets_g_++;
            break;
        case 'I':
            sockets_b_++;
            break;
        case 'G':
            sockets_w_++;
            break;
        }
    }

    std::string unique;
    unique += json["name"].asString() + "~" + json["typeLine"].asString() + "~";
    for (auto &mod : json["explicitMods"])
        unique += mod.asString() + "~";
    for (auto &mod : json["implicitMods"])
        unique += mod.asString() + "~";
    unique += UniqueProperties("properties") + "~";
    unique += UniqueProperties("additionalProperties") + "~";

    for (auto &socket : json["sockets"])
        unique += std::to_string(socket["group"].asInt()) + "~" + socket["attr"].asString() + "~";

    hash_ = Util::Md5(unique);
}

std::string Item::UniqueProperties(const std::string &name) {
    std::string result;
    for (auto &property : json_[name]) {
        result += property["name"].asString() + "~";
        for (auto value : property["values"])
            result += value[0].asString() + "~";
    }
    return result;
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
