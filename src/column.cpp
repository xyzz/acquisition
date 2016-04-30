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
#include <QVector>
#include <QRegularExpression>

#include "buyoutmanager.h"
#include "util.h"

const double EPS = 1e-6;
const QRegularExpression sort_double_match("^\\+?([\\d.]+)%?$");
const QRegularExpression sort_two_values("^(\\d+)([-/])(\\d+)$");

QColor Column::color(const Item & /* item */) const {
    return QColor();
}

std::tuple<QVariant, QVariant, const Item&> Column::multivalue(const Item* item) const {
    // Transform values into something optimal for sorting
    // Possibilities: 12, 12.12, 10%, 10.13%, +16%, 12-14, 10/20
    QVariant sort_first_by;
    QVariant sort_second_by;

    QString str = value(*item).toString();
    QRegularExpressionMatch match;

    if (str.contains(sort_double_match, &match)) {
        sort_first_by = match.captured(1).toDouble();
    } else if (str.contains(sort_two_values, &match)) {
        if (match.captured(2).startsWith("-")) {
            sort_first_by = 0.5 * (match.captured(1).toDouble() + match.captured(3).toDouble());
        } else {
            sort_first_by = item->PrettyName().c_str();
            sort_second_by = match.captured(1).toDouble();
        }
    } else {
        sort_first_by = str;
        sort_second_by = item->PrettyName().c_str();
    }

    return std::forward_as_tuple(sort_first_by, sort_second_by, *item);
}

bool Column::lt(const Item* lhs, const Item* rhs) const {
    return multivalue(lhs) < multivalue(rhs);
}

std::string NameColumn::name() const {
    return "Name";
}

QVariant NameColumn::value(const Item &item) const {
    return item.PrettyName().c_str();
}

QColor NameColumn::color(const Item &item) const {
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

std::string CorruptedColumn::name() const {
    return "Cr";
}

QVariant CorruptedColumn::value(const Item &item) const {
    if (item.corrupted())
        return "C";
    return QVariant();
}

PropertyColumn::PropertyColumn(const std::string &name):
    name_(name),
    property_(name)
{}

PropertyColumn::PropertyColumn(const std::string &name, const std::string &property):
    name_(name),
    property_(property)
{}

std::string PropertyColumn::name() const {
    return name_;
}

QVariant PropertyColumn::value(const Item &item) const {
    if (item.properties().count(property_))
        return item.properties().find(property_)->second.c_str();
    return QVariant();
}

std::string DPSColumn::name() const {
    return "DPS";
}

QVariant DPSColumn::value(const Item &item) const {
    double dps = item.DPS();
    if (fabs(dps) < EPS)
        return QVariant();
    return dps;
}

std::string pDPSColumn::name() const {
    return "pDPS";
}

QVariant pDPSColumn::value(const Item &item) const {
    double pdps = item.pDPS();
    if (fabs(pdps) < EPS)
        return QVariant();
    return pdps;
}

std::string eDPSColumn::name() const {
    return "eDPS";
}

QVariant eDPSColumn::value(const Item &item) const {
    double edps = item.eDPS();
    if (fabs(edps) < EPS)
        return QVariant();
    return edps;
}

ElementalDamageColumn::ElementalDamageColumn(int index):
    index_(index)
{}

std::string ElementalDamageColumn::name() const {
    if (index_ == 0)
        return "ED";
    return "";
}

QVariant ElementalDamageColumn::value(const Item &item) const {
    if (item.elemental_damage().size() > index_) {
        auto &ed = item.elemental_damage().at(index_);
        return ed.first.c_str();
    }
    return QVariant();
}

QColor ElementalDamageColumn::color(const Item &item) const {
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

std::string PriceColumn::name() const {
    return "Price";
}

QVariant PriceColumn::value(const Item &item) const {
    const Buyout &bo = bo_manager_.Get(item);
    return bo.AsText().c_str();
}

QColor PriceColumn::color(const Item &item) const {
    const Buyout &bo = bo_manager_.Get(item);
    return bo.IsInherited() ? QColor(0xaa, 0xaa, 0xaa):QColor();
}

std::tuple<int, double, const Item&> PriceColumn::multivalue(const Item* item) const {
    const Buyout &bo = bo_manager_.Get(*item);
    // forward_as_tuple used to forward item reference properly and avoid ref to temporary
    // that will be destroyed.  We want item reference because we want to sort based on item
    // object itself and not the pointer.  I'm not entirely sure I fully understand
    // this mechanism, but basically want to avoid a ton of Item copies during sorting
    // so trying to avoid pass by value (ericsium).
    return std::forward_as_tuple(bo.currency.AsRank(), bo.value, *item);
}

bool PriceColumn::lt(const Item* lhs, const Item* rhs) const {
    return multivalue(lhs) < multivalue(rhs);
}

DateColumn::DateColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

std::string DateColumn::name() const {
    return "Last Update";
}

QVariant DateColumn::value(const Item &item) const {
    const Buyout &bo = bo_manager_.Get(item);
    return bo.IsActive() ? Util::TimeAgoInWords(bo.last_update).c_str():QVariant();
}

bool DateColumn::lt(const Item* lhs, const Item* rhs) const {
    auto lhs_update_time = bo_manager_.Get(*lhs).last_update;
    auto rhs_update_time = bo_manager_.Get(*rhs).last_update;
    return (std::tie(lhs_update_time, *lhs) <
            std::tie(rhs_update_time, *rhs)) ;
}

std::string ItemlevelColumn::name() const {
    return "ilvl";
}

QVariant ItemlevelColumn::value(const Item &item) const {
    if (item.ilvl() > 0)
        return item.ilvl();
    return QVariant();
}
