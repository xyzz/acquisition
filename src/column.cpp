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

#include "column.h"

#include <cmath>

#include "buyoutmanager.h"
#include "util.h"

const double EPS = 1e-6;

QColor Column::color(const Item & /* item */) {
    return QColor();
}

std::string NameColumn::name() {
    return "Name";
}

std::string NameColumn::value(const Item &item) {
    return item.PrettyName();
}

QColor NameColumn::color(const Item &item) {
    switch(item.frameType()) {
    case FRAME_TYPE_NORMAL:
        return QColor();
    case FRAME_TYPE_MAGIC:
        return QColor(Qt::blue);
    case FRAME_TYPE_RARE:
        return QColor(Qt::darkYellow);
    case FRAME_TYPE_UNIQUE:
        return QColor(234, 117, 0);
    case FRAME_TYPE_GEM:
        return QColor(0x1b, 0xa2, 0x9b);
    case FRAME_TYPE_CURRENCY:
        return QColor(0x77, 0x6e, 0x59);
    }
    return QColor();
}

std::string CorruptedColumn::name() {
    return "Cr";
}

std::string CorruptedColumn::value(const Item &item) {
    if (item.corrupted())
        return "C";
    return "";
}

PropertyColumn::PropertyColumn(const std::string &name):
    name_(name),
    property_(name)
{}

PropertyColumn::PropertyColumn(const std::string &name, const std::string &property):
    name_(name),
    property_(property)
{}

std::string PropertyColumn::name() {
    return name_;
}

std::string PropertyColumn::value(const Item &item) {
    if (item.properties().count(property_))
        return item.properties().find(property_)->second;
    return "";
}

std::string DPSColumn::name() {
    return "DPS";
}

std::string DPSColumn::value(const Item &item) {
    double dps = item.DPS();
    if (fabs(dps) < EPS)
        return "";
    return QString::number(dps).toUtf8().constData();
}

std::string pDPSColumn::name() {
    return "pDPS";
}

std::string pDPSColumn::value(const Item &item) {
    double pdps = item.pDPS();
    if (fabs(pdps) < EPS)
        return "";
    return QString::number(pdps).toUtf8().constData();
}

std::string eDPSColumn::name() {
    return "eDPS";
}

std::string eDPSColumn::value(const Item &item) {
    double edps = item.eDPS();
    if (fabs(edps) < EPS)
        return "";
    return QString::number(edps).toUtf8().constData();
}

ElementalDamageColumn::ElementalDamageColumn(int index):
    index_(index)
{}

std::string ElementalDamageColumn::name() {
    if (index_ == 0)
        return "ED";
    return "";
}

std::string ElementalDamageColumn::value(const Item &item) {
    if (item.elemental_damage().size() > index_) {
        auto &ed = item.elemental_damage().at(index_);
        return ed.first;
    }
    return "";
}

QColor ElementalDamageColumn::color(const Item &item) {
    if (item.elemental_damage().size() > index_) {
        auto &ed = item.elemental_damage().at(index_);
        switch (ed.second) {
        case ED_FIRE:
            return QColor(0xc5, 0x13, 0x13);
        case ED_COLD:
            return QColor(0x36, 0x64, 0x92);
        case ED_LIGHTNING:
            return QColor(0xb9, 0x9c, 0x00);
        }
    }
    return QColor();
}

PriceColumn::PriceColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

std::string PriceColumn::name() {
    return "Price";
}

std::string PriceColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";
    const Buyout &bo = bo_manager_.Get(item);
    return Util::BuyoutAsText(bo);
}

QColor PriceColumn::color(const Item &item) {
    if (bo_manager_.Exists(item)) {
        const Buyout &bo = bo_manager_.Get(item);
        if (bo.inherited)
            return QColor(0xaa, 0xaa, 0xaa);
    }
    return QColor();
}

DateColumn::DateColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

std::string DateColumn::name() {
    return "Last Update";
}

std::string DateColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";

    const Buyout &bo = bo_manager_.Get(item);
    return Util::TimeAgoInWords(bo.last_update);

}
