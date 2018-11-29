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
#include <boost/algorithm/string.hpp>
#include <regex>
#include "rapidjson/document.h"

#include "QsLog.h"
#include "modlist.h"
#include "util.h"
#include "porting.h"
#include "itemlocation.h"

const std::vector<std::string> ITEM_MOD_TYPES = {
    "implicitMods", "enchantMods", "explicitMods", "craftedMods"
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
    identified_(true),
    corrupted_(false),
    crafted_(false),
    enchanted_(false),
    baseType_(BASE_NORMAL),
    w_(0),
    h_(0),
    frameType_(0),
    sockets_cnt_(0),
    links_cnt_(0),
    sockets_({ 0, 0, 0, 0 }),
    json_(Util::RapidjsonSerialize(json)),
    ilvl_(0)
{
    if (json.HasMember("name") && json["name"].IsString())
        name_ = fixup_name(json["name"].GetString());
    if (json.HasMember("typeLine") && json["typeLine"].IsString())
        typeLine_ = fixup_name(json["typeLine"].GetString());

    if (json.HasMember("identified") && json["identified"].IsBool())
        identified_ = json["identified"].GetBool();
    if (json.HasMember("corrupted") && json["corrupted"].IsBool())
        corrupted_ = json["corrupted"].GetBool();

    if (json.HasMember("craftedMods") && json["craftedMods"].IsArray() && !json["craftedMods"].Empty())
        crafted_ = true;
    if (json.HasMember("enchantMods") && json["enchantMods"].IsArray() && !json["enchantMods"].Empty())
        enchanted_ = true;
	
    if (json.HasMember("shaper") && json["shaper"].IsBool() && json["shaper"].GetBool())
        baseType_ = BASE_SHAPER;
    if (json.HasMember("elder") && json["elder"].IsBool() && json["elder"].GetBool()) {
        if(baseType_ != BASE_NORMAL) {
            QLOG_WARN() << PrettyName().c_str() << " has multiple conflicting base type attributes.";
        }
        baseType_ = BASE_ELDER;
    }

    if (json.HasMember("w") && json["w"].IsInt())
        w_ = json["w"].GetInt();
    if (json.HasMember("h") && json["h"].IsInt())
        h_ = json["h"].GetInt();

    if (json.HasMember("frameType") && json["frameType"].IsInt())
        frameType_ = json["frameType"].GetInt();

    if (json.HasMember("icon") && json["icon"].IsString())
        icon_ = json["icon"].GetString();

    for (auto &mod_type : ITEM_MOD_TYPES) {
        text_mods_[mod_type] = std::vector<std::string>();
        const char *mod_type_s = mod_type.c_str();
        if (json.HasMember(mod_type_s) && json[mod_type_s].IsArray()) {
            auto &mods = text_mods_[mod_type];
            for (auto &mod : json[mod_type_s])
                if (mod.IsString())
                    mods.push_back(mod.GetString());
        }
    }

    // Other code assumes icon is proper size so force quad=1 to quad=0 here as it's clunky
    // to handle elsewhere
    boost::replace_last(icon_, "quad=1", "quad=0");

    CalculateCategories(json);

    if (json.HasMember("talismanTier") && json["talismanTier"].IsUint()) {
       talisman_tier_ = json["talismanTier"].GetUint();
    }

    if (json.HasMember("id") && json["id"].IsString()) {
        uid_ = json["id"].GetString();
    }

    if (json.HasMember("note") && json["note"].IsString()) {
        note_ = json["note"].GetString();
    }

    if (json.HasMember("properties") && json["properties"].IsArray()) {
        for (auto &prop : json["properties"]) {
            if (!prop.HasMember("name") || !prop["name"].IsString() || !prop.HasMember("values") || !prop["values"].IsArray())
                continue;
            std::string name = prop["name"].GetString();
            if (name == "Elemental Damage") {
                for (auto &value : prop["values"]) {
                    if (value.IsArray() && value.Size() >= 2 && value[0].IsString() && value[1].IsInt())
                        elemental_damage_.push_back(std::make_pair(value[0].GetString(), value[1].GetInt()));
                }
            } else {
                if (prop["values"].Size() > 0 && prop["values"][0].IsArray() && prop["values"][0].Size() > 0 &&
                        prop["values"][0][0].IsString())
                    properties_[name] = prop["values"][0][0].GetString();
            }

            ItemProperty property;
            property.name = name;
            property.display_mode = prop["displayMode"].GetInt();
            for (auto &value : prop["values"]) {
                if (value.IsArray() && value.Size() >= 2 && value[0].IsString() && value[1].IsInt()) {
                    ItemPropertyValue v;
                    v.str = value[0].GetString();
                    v.type = value[1].GetInt();
                    property.values.push_back(v);
                }
            }
            text_properties_.push_back(property);
        }
    }

    if (json.HasMember("requirements") && json["requirements"].IsArray()) {
        for (auto &req : json["requirements"]) {
            if (req.IsObject() && req.HasMember("name") && req["name"].IsString() &&
                    req.HasMember("values") && req["values"].IsArray() && req["values"].Size() >= 1 &&
                    req["values"][0].IsArray() && req["values"][0].Size() >= 2 &&
                    req["values"][0][0].IsString() && req["values"][0][1].IsInt()) {
                std::string name = req["name"].GetString();
                std::string value = req["values"][0][0].GetString();
                requirements_[name] = std::atoi(value.c_str());
                ItemPropertyValue v;
                v.str = value;
                v.type = req["values"][0][1].GetInt();
                text_requirements_.push_back({ name, v });
            }
        }
    }

    if (json.HasMember("sockets") && json["sockets"].IsArray()) {
        ItemSocketGroup current_group = { 0, 0, 0, 0 };
        sockets_cnt_ = json["sockets"].Size();
        int counter = 0, prev_group = -1;
        for (auto &socket : json["sockets"]) {
            if (!socket.IsObject() || !socket.HasMember("group") || !socket["group"].IsInt())
                continue;

            char attr = '\0';
            if (socket["attr"].IsString())
                attr = socket["attr"].GetString()[0];
            else if (socket["sColour"].IsString())
                attr = socket["sColour"].GetString()[0];

            if (!attr)
                continue;

            ItemSocket current_socket = { static_cast<unsigned char>(socket["group"].GetInt()), attr };
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

    if (json.HasMember("ilvl") && json["ilvl"].IsInt())
        ilvl_ = json["ilvl"].GetInt();

    GenerateMods(json);
}

std::string Item::PrettyName() const {
    if (!name_.empty())
        return name_ + " " + typeLine_;
    return typeLine_;
}

void Item::CalculateCategories(const rapidjson::Value &json) {
    category_ = "";
    if (json.HasMember("category") && json["category"].IsObject()) {
        // This object contains a single array who's name is the item's category. The array may contain the item's sub-category
        rapidjson::Value::ConstMemberIterator itr = json["category"].MemberBegin();
        category_ = itr->name.GetString();
        category_ = Util::Capitalise(category_);

        if(category_ == "Cards") {
            category_ = "Divination cards";
        }

        category_vector_.push_back(category_);
        if( itr->value.IsArray() && !itr->value.Empty() ) {
            // Handle sub-categories

            if(category_ == "Accessories") {    // Elevate accessories sub-categories to their own top level categories
                category_vector_.pop_back();
                // If amulet and .HasMember("talismanTier") (which is also checked after CalculateCategories()), add Talisman sub-category?
            }

            /*
            If Armour.Chests, rename Armour.BodyArmour?
            If Flasks, use Base64 encoded JSON within icon path to determine sub-category?
            */

            category_ = itr->value[0].GetString();
            category_ = Util::Capitalise(category_);

            if(category_ == "Activegem") {
                // Rename these sub-categories
                category_ = "Skill";
                if(icon_.find("/Art/2DItems/Gems/VaalGems/") != std::string::npos) {
                    category_vector_.push_back(category_);
                    category_ = "Vaal";
                }
            } else if(category_ == "Supportgem") {
                category_ = "Support";
            } else if(category_ == "Bow" || category_ == "Staff") {
                // Sub-categories these categories under handedness
                category_vector_.push_back("TwoHand");
            } else if(category_ == "Claw" || category_ == "Dagger" || category_ == "Sceptre" || category_ == "Wand") {
                category_vector_.push_back("OneHand");
            } else if(category_ == "Oneaxe") {
                // Sub-categories these categories under handedness and rename them
                category_vector_.push_back("OneHand");
                category_ = "Axe";
            } else if(category_ == "Onemace") {
                category_vector_.push_back("OneHand");
                category_ = "Mace";
            } else if(category_ == "Onesword") {
                category_vector_.push_back("OneHand");
                category_ = "Sword";
            } else if(category_ == "Twoaxe") {
                category_vector_.push_back("TwoHand");
                category_ = "Axe";
            } else if(category_ == "Twomace") {
                category_vector_.push_back("TwoHand");
                category_ = "Mace";
            } else if(category_ == "Twosword") {
                category_vector_.push_back("TwoHand");
                category_ = "Sword";
            }

            category_vector_.push_back(category_);
        } else if(category_ == "Maps") {
            // Use icon path to determine possible expansion sub-category
            std::smatch sm;
            if(std::regex_search(icon_, sm, std::regex("/Art/2DItems/Maps/(.*)/"))) {
                std::string match = sm.str(1);
                std::vector<std::string> iconsubs;
                boost::split(iconsubs, match, boost::is_any_of("/"));
                std::string sub = iconsubs[0];
                if(sub == "Atlas2Maps") {
                    category_vector_.push_back("3.1");
                    if(iconsubs.size() > 1) {
                        sub = iconsubs[1];  // Typically "New"
                        sub = Util::Capitalise(sub);
                        category_vector_.push_back(sub);
                    }
                } else if(sub == "AtlasMaps") {
                    category_vector_.push_back("2.4");
                } else if(sub == "act4maps") {
                    category_vector_.push_back("2.0");
                } else {
                    Util::Capitalise(sub);
                    category_vector_.push_back(sub);
                }
            } else {
                if(icon_.find("/Art/2DItems/Maps/Map") != std::string::npos) {
                    // Doesn't find all because of some maps like FairgravesMap01.png, olmec.png, etc. They'll end up in Maps.Misc, then moved to Maps.Older Uniques
                    category_vector_.push_back("Original");
                } else if(icon_.find("/Art/2DItems/Currency/Breach/") != std::string::npos) {
                    // Isn't really a map, has no property named Map Tier
                    category_vector_.pop_back();
                    category_vector_.push_back("Currency");
                    category_vector_.push_back("Breach");
                } else {
                    category_vector_.push_back("Misc");

                    if(json.HasMember("properties") && json["properties"].IsArray()) {
                        for(auto &prop : json["properties"]) {
                            if(!prop.HasMember("name") || !prop["name"].IsString() || !prop.HasMember("values") || !prop["values"].IsArray())
                                continue;
                            std::string name = prop["name"].GetString();
                            if(name == "Map Tier") {
                                category_vector_.pop_back();
                                category_vector_.push_back("Older Uniques");    // Future ones might fall under a new /Maps/ icon path
                                break;
                            }
                            // Else determine if Sacrifice/Mortal/Offering/etc fragment?
                        }
                    }
                }
            }
        } else if(category_ == "Currency") {
            if(icon_.find("/Art/2DItems/Currency/Breach/") != std::string::npos) {
                category_vector_.push_back("Breach");
            } else if(icon_.find("/Art/2DItems/Currency/Essence/") != std::string::npos) {
                category_vector_.push_back("Essence");
            }
        }
    }
    category_ = boost::join(category_vector_, ".");
    boost::to_lower(category_);

}

double Item::DPS() const {
    return pDPS() + eDPS() + cDPS();
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

double Item::cDPS() const {
    if (!properties_.count("Chaos Damage") || !properties_.count("Attacks per Second"))
        return 0;
    double aps = std::stod(properties_.at("Attacks per Second"));
    std::string cd = properties_.at("Chaos Damage");

    return aps * Util::AverageDamage(cd);
}

void Item::GenerateMods(const rapidjson::Value &json) {
    for (auto &generator : mod_generators)
        generator->Generate(json, &mod_table_);
}

void Item::CalculateHash(const rapidjson::Value &json) {
    std::string unique_new = name_ + "~" + typeLine_ + "~";
    // GGG removed the <<set>> things in patch 3.4.3e but our hashes all include them, oops
    std::string unique_old = "<<set:MS>><<set:M>><<set:S>>" + unique_new;

    std::string unique_common;

    if (json.HasMember("explicitMods") && json["explicitMods"].IsArray())
        for (auto &mod : json["explicitMods"])
            if (mod.IsString())
                unique_common += std::string(mod.GetString()) + "~";

    if (json.HasMember("implicitMods") && json["implicitMods"].IsArray())
        for (auto &mod : json["implicitMods"])
            if (mod.IsString())
                unique_common += std::string(mod.GetString()) + "~";

    unique_common += item_unique_properties(json, "properties") + "~";
    unique_common += item_unique_properties(json, "additionalProperties") + "~";

    if (json.HasMember("sockets") && json["sockets"].IsArray())
        for (auto &socket : json["sockets"]) {
            if (!socket.HasMember("group") || !socket.HasMember("attr") || !socket["group"].IsInt() || !socket["attr"].IsString())
                continue;
            unique_common += std::to_string(socket["group"].GetInt()) + "~" + socket["attr"].GetString() + "~";
        }

    unique_common += "~" + location_.GetUniqueHash();

    unique_old += unique_common;
    unique_new += unique_common;

    old_hash_ = Util::Md5(unique_old);
    hash_ = Util::Md5(unique_new);
}

bool Item::operator<(const Item &rhs) const {
    std::string name = PrettyName();
    std::string rhs_name = rhs.PrettyName();
    return std::tie(name,uid_,hash_) < std::tie(rhs_name, rhs.uid_, hash_);
}

bool Item::Wearable() const {
    return (category_ == "flasks"
            || category_ == "amulet" || category_ == "ring" || category_ == "belt"
            || category_.find("armour") != std::string::npos
            || category_.find("weapons") != std::string::npos
            || category_.find("jewels") != std::string::npos
    );
}
