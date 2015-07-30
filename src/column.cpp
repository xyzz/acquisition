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

QString NameColumn::name() {
    return "Name";
}

QString NameColumn::value(const Item &item) {
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

QString CorruptedColumn::name() {
    return "Cr";
}

QString CorruptedColumn::value(const Item &item) {
    if (item.corrupted())
        return "C";
    return "";
}

PropertyColumn::PropertyColumn(const QString &name):
    name_(name),
    property_(name)
{}

PropertyColumn::PropertyColumn(const QString &name, const QString &property):
    name_(name),
    property_(property)
{}

QString PropertyColumn::name() {
    return name_;
}

QString PropertyColumn::value(const Item &item) {
    if (item.properties().count(property_))
        return item.properties().find(property_)->second;
    return "";
}

QString DPSColumn::name() {
    return "DPS";
}

QString DPSColumn::value(const Item &item) {
    double dps = item.DPS();
    if (fabs(dps) < EPS)
        return "";
    return QString::number(dps).toUtf8().constData();
}

QString pDPSColumn::name() {
    return "pDPS";
}

QString pDPSColumn::value(const Item &item) {
    double pdps = item.pDPS();
    if (fabs(pdps) < EPS)
        return "";
    return QString::number(pdps).toUtf8().constData();
}

QString eDPSColumn::name() {
    return "eDPS";
}

QString eDPSColumn::value(const Item &item) {
    double edps = item.eDPS();
    if (fabs(edps) < EPS)
        return "";
    return QString::number(edps).toUtf8().constData();
}

ElementalDamageColumn::ElementalDamageColumn(int index):
    index_(index)
{}

QString ElementalDamageColumn::name() {
    if (index_ == 0)
        return "ED";
    return "";
}

QString ElementalDamageColumn::value(const Item &item) {
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

QString PriceColumn::name() {
    return "Price";
}

QString PriceColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";
    const Buyout &bo = bo_manager_.Get(item);
    return Util::BuyoutAsText(bo);
}

DateColumn::DateColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

QString DateColumn::name() {
    return "Last Update";
}

QString DateColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";

    const Buyout &bo = bo_manager_.Get(item);
    return Util::TimeAgoInWords(bo.last_update);

}
