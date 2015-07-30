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
#include <QVector>
#include <QRegularExpression>
#include "rapidjson/document.h"

#include "modlist.h"
#include "util.h"
#include "porting.h"
#include "itemlocation.h"

const std::vector<QString> ITEM_MOD_TYPES = {
    "implicitMods", "explicitMods", "craftedMods", "cosmeticMods"
};

static QString item_unique_properties(const rapidjson::Value &json, const QString &name) {
    const char *name_p = name.toStdString().c_str();
    if (!json.HasMember(name_p))
        return "";
    QString result;
    for (auto prop_it = json[name_p].Begin(); prop_it != json[name_p].End(); ++prop_it) {
        auto &prop = *prop_it;
        result += QString::fromStdString(prop["name"].GetString()) + "~";
        for (auto value_it = prop["values"].Begin(); value_it != prop["values"].End(); ++value_it)
            result += QString::fromStdString((*value_it)[0].GetString()) + "~";
    }
    return result;
}

Item::Item(const rapidjson::Value &json) :
    location_(ItemLocation(json)),
    name_(QString::fromStdString(json["name"].GetString())),
    typeLine_(QString::fromStdString(json["typeLine"].GetString())),
    corrupted_(json["corrupted"].GetBool()),
    w_(json["w"].GetInt()),
    h_(json["h"].GetInt()),
    frameType_(json["frameType"].GetInt()),
    icon_(QString::fromStdString(json["icon"].GetString())),
    sockets_cnt_(0),
    links_cnt_(0),
    sockets_({ 0, 0, 0, 0 }),
    has_mtx_(false)
{
	name_ = FormatGenderNames(name_);
	typeLine_ = FormatGenderNames(typeLine_);

    for (auto &mod_type : ITEM_MOD_TYPES) {
        text_mods_[mod_type] = std::vector<QString>();
        if (json.HasMember(mod_type.toStdString().c_str())) {
            auto &mods = text_mods_[mod_type];
            for (auto &mod : json[mod_type.toStdString().c_str()])
                mods.push_back(QString::fromStdString(mod.GetString()));
        }
    }

    if (json.HasMember("properties")) {
        for (auto prop_it = json["properties"].Begin(); prop_it != json["properties"].End(); ++prop_it) {
            auto &prop = *prop_it;
            QString name = QString::fromStdString(prop["name"].GetString());
            if (name == "Map Level")
                name = "Level";
            if (name == "Elemental Damage") {
                for (auto value_it = prop["values"].Begin(); value_it != prop["values"].End(); ++value_it)
                    elemental_damage_.push_back(std::make_pair(QString::fromStdString((*value_it)[0].GetString()), (*value_it)[1].GetInt()));
            }
            else {
                if (prop["values"].Size())
                    properties_[name] = QString::fromStdString(prop["values"][0][0].GetString());
            }

            ItemProperty property;
            property.name = name;
            property.display_mode = prop["displayMode"].GetInt();
            for (auto &value : prop["values"])
                property.values.push_back(QString::fromStdString(value[0].GetString()));
            text_properties_.push_back(property);
        }
    }

    if (json.HasMember("requirements")) {
        for (auto &req : json["requirements"]) {
            QString name = QString::fromStdString( req["name"].GetString() );
            QString value = QString::fromStdString( req["values"][0][0].GetString());
            requirements_[name] = value.toInt();
            text_requirements_.push_back({ name, value });
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

    QString unique(QString::fromStdString(json["name"].GetString()) + "~" + QString::fromStdString(json["typeLine"].GetString()) + "~");

    if (json.HasMember("explicitMods"))
        for (auto mod_it = json["explicitMods"].Begin(); mod_it != json["explicitMods"].End(); ++mod_it)
            unique += QString::fromStdString(mod_it->GetString()) + "~";

    if (json.HasMember("implicitMods"))
        for (auto mod_it = json["implicitMods"].Begin(); mod_it != json["implicitMods"].End(); ++mod_it)
            unique += QString::fromStdString(mod_it->GetString()) + "~";

    unique += item_unique_properties(json, "properties") + "~";
    unique += item_unique_properties(json, "additionalProperties") + "~";

    if (json.HasMember("sockets"))
        for (auto socket_it = json["sockets"].Begin(); socket_it != json["sockets"].End(); ++socket_it)
            unique += QString::number((*socket_it)["group"].GetInt()) + "~" + QString::fromStdString((*socket_it)["attr"].GetString()) + "~";

    hash_ = Util::Md5(unique);

    count_ = 1;
    if (properties_.find("Stack Size") != properties_.end()) {
        QString size = properties_["Stack Size"];
        if (size.indexOf("/") != -1) {
            size = size.mid(0, size.indexOf("/"));
            count_ = size.toInt();
        }
    }

    has_mtx_ = json.HasMember("cosmeticMods");

    GenerateMods(json);
}

QString Item::FormatGenderNames(const QString str)
{
	QString gendername(str);
	QRegularExpression rx("<<set:(\\w+)>>");
	QRegularExpressionMatch rxm = rx.match(gendername);

	QVector<QString> genders;
	genders.clear();

	if (rxm.hasMatch())
	{
		for (int i = 1; i <= rxm.lastCapturedIndex(); i = +2)
		{
			genders.push_back(rxm.captured(i));
		}
		gendername.replace(QRegularExpression("<<set:\\w+>>"), "");

		QRegularExpression rxx("<([elif]+):(\\w+)>{([\\w\\dа-яА-ЯёЁ ]+)}");
		QRegularExpressionMatch rxxm = rxx.match(gendername);
		QString tmp = "";
		if (rxxm.hasMatch())
		{
			for (int i = 0; i <= rxxm.lastCapturedIndex(); i = i + 4)
			{
				if (genders.contains(rxxm.captured(i + 2)))
				{
					tmp += rxxm.captured(i + 3);
				}
			}
			gendername.replace(
				QRegularExpression("(<[elif]+:\\w+>{[\\w\\dа-яА-ЯёЁ ]+})", QRegularExpression::CaseInsensitiveOption),
				""
				);
			gendername = tmp + gendername;
		}
	}
	return gendername;
}

QString Item::PrettyName() const {
    if (!name_.isEmpty())
        return name_ + " " + typeLine_;
    return typeLine_;
}

double Item::DPS() const {
    return pDPS() + eDPS();
}

double Item::pDPS() const {
    if (!properties_.count("Physical Damage") || !properties_.count("Attacks per Second"))
        return 0;
    double aps = properties_.at("Attacks per Second").toDouble();
    QString pd = properties_.at("Physical Damage");

    return aps * Util::AverageDamage(pd);
}

double Item::eDPS() const {
    if (elemental_damage_.empty() || !properties_.count("Attacks per Second"))
        return 0;
    double damage = 0;
    for (auto &x : elemental_damage_)
        damage += Util::AverageDamage(x.first);
    double aps = properties_.at("Attacks per Second").toDouble();
    return aps * damage;
}

void Item::GenerateMods(const rapidjson::Value &json) {
    for (auto &generator : mod_generators)
        generator->Generate(json, &mod_table_);
}
