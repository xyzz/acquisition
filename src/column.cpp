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

#include <QVariant>
#include <QApplication>
#include <QPalette>
#include <cmath>

#include "buyoutmanager.h"
#include "util.h"

const double EPS = 1e-6;

QColor Column::color(const Item & /* item */) {
    return QColor(qApp->palette().text().color());
}

std::string NameColumn::name() {
    return "Name";
}

std::string NameColumn::value(const Item &item) {
    return item.PrettyName();
}

QVariant NameColumn::sortValue(const Item &item) {
    return QString::fromStdString(item.PrettyName());
}

QColor NameColumn::color(const Item &item) {
    switch(item.frameType()) {
    case FRAME_TYPE_NORMAL:
        break;
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
    return QColor(qApp->palette().text().color());
}

std::string CorruptedColumn::name() {
    return "Corrupted";
}

QVariant CorruptedColumn::sortValue(const Item &item) {
    return item.corrupted();
}

std::string CorruptedColumn::value(const Item &item) {
    if (item.corrupted())
        return "Yes";
    return "";
}

PropertyColumn::PropertyColumn(const std::string &name, const QVariant::Type type):
    name_(name),
    property_(name),
    type_(type)
{}

PropertyColumn::PropertyColumn(const std::string &name, const std::string &property, const QVariant::Type type):
    name_(name),
    property_(property),
    type_(type)
{}

std::string PropertyColumn::name() {
    return name_;
}

QVariant PropertyColumn::sortValue(const Item &item) {
    QVariant val = QString::fromStdString(value(item));
    if (!val.canConvert(type_) || !val.convert(type_))
        return val.toString();
    return val;
}

std::string PropertyColumn::value(const Item &item) {
    if (item.properties().count(property_))
        return item.properties().find(property_)->second;
    return "";
}

std::string PropertyColumn::tooltip() {
    return property_;
}

std::string DPSColumn::name() {
    return "DPS";
}

QVariant DPSColumn::sortValue(const Item &item) {
    double dps = item.DPS();
    if (fabs(dps) < EPS)
        return 0;
    return dps;
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

QVariant pDPSColumn::sortValue(const Item &item) {
    double pdps = item.pDPS();
    if (fabs(pdps) < EPS)
        return 0;
    return pdps;
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

QVariant eDPSColumn::sortValue(const Item &item) {
    double edps = item.eDPS();
    if (fabs(edps) < EPS)
        return 0;
    return edps;
}

std::string eDPSColumn::value(const Item &item) {
    double edps = item.eDPS();
    if (fabs(edps) < EPS)
        return "";
    return QString::number(edps).toUtf8().constData();
}

ElementalDamageColumn::ElementalDamageColumn(ELEMENTAL_DAMAGE_TYPES type):
    type_(type),
    index_(16)
{}

std::string ElementalDamageColumn::name() {
    switch (type_) {
    case ED_FIRE:
        return "FD";
    case ED_COLD:
        return "CD";
    case ED_LIGHTNING:
        return "LD";
    }
    return "";
}

std::string ElementalDamageColumn::tooltip() {
    switch (type_) {
    case ED_FIRE:
        return "Fire Damage";
    case ED_COLD:
        return "Cold Damage";
    case ED_LIGHTNING:
        return "Lightning Damage";
    }
    return "";
}

QVariant ElementalDamageColumn::sortValue(const Item &item) {
    GetIndex(item);
    if (item.elemental_damage().size() > index_) {
        auto &ed = item.elemental_damage().at(index_);
        if (ed.second == type_) {
            QString r = QString::fromStdString(ed.first);
            if (!r.isEmpty()) {
                QStringList range = r.split("-");
                if (range.size() == 2) {
                    uint low = range.first().toUInt();
                    uint high = range.last().toUInt();
                    return (high + low) / 2.0;
                }
            }
        }
    }
    return 0.0;
}

std::string ElementalDamageColumn::value(const Item &item) {
    GetIndex(item);
    if (item.elemental_damage().size() > index_) {
        auto &ed = item.elemental_damage().at(index_);
        if (ed.second == type_) {
            return ed.first;
        }
    }
    return "";
}

QColor ElementalDamageColumn::color(const Item &item) {
    Q_UNUSED(item)
    switch (type_) {
    case ED_FIRE:
        return QColor(0xc5, 0x13, 0x13);
    case ED_COLD:
        return QColor(0x36, 0x64, 0x92);
    case ED_LIGHTNING:
        return QColor(0xb9, 0x9c, 0x00);
    }
    return Column::color(item);
}

int ElementalDamageColumn::GetIndex(const Item &item)
{
    if (index_ != 16) return index_;

    for (uint i = 0; i < item.elemental_damage().size(); i++) {
        if (item.elemental_damage().at(i).second == type_) {
            index_ = i;
            return index_;
        }
    }

    index_ = 16;
    return index_;
}

PriceColumn::PriceColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

std::string PriceColumn::name() {
    return "Price";
}

QVariant PriceColumn::sortValue(const Item &item) {
    if (!bo_manager_.Exists(item))
        return 0;
    const Buyout &bo = bo_manager_.Get(item);
    // TODO(novynn): Implement some sort of currency exchange model for this...
    return QString::fromStdString(Util::BuyoutAsText(bo));
}

std::string PriceColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";
    const Buyout &bo = bo_manager_.Get(item);
    QString extra = "";
    if (!bo_manager_.IsItemManuallySet(item)) {
        extra = " (set from " + bo.set_by + ")";
    }
    return Util::BuyoutAsText(bo) + extra.toStdString();
}

QColor PriceColumn::color(const Item &item) {
    if (bo_manager_.Exists(item) && !bo_manager_.IsItemManuallySet(item)) {
        return Qt::blue;
    }
    return Column::color(item);
}

DateColumn::DateColumn(const BuyoutManager &bo_manager):
    bo_manager_(bo_manager)
{}

std::string DateColumn::name() {
    return "Last Update";
}


QVariant DateColumn::sortValue(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";

    const Buyout &bo = bo_manager_.Get(item);
    return bo.last_update;
}

std::string DateColumn::value(const Item &item) {
    if (!bo_manager_.Exists(item))
        return "";

    const Buyout &bo = bo_manager_.Get(item);
    return Util::TimeAgoInWords(bo.last_update);

}


QVariant PercentPropertyColumn::sortValue(const Item &item) {
    QString original = QString::fromStdString(value(item));
    QVariant val = original.replace("%", "").replace("+", "");
    if (!val.canConvert(type_) || !val.convert(type_))
        return val.toString();
    return val;
}


std::string StackColumn::name() {
    return "Stack";
}

QVariant StackColumn::sortValue(const Item &item) {
    QStringList fraction = QString::fromStdString(value(item)).split("/");
    if (fraction.size() != 2) {
        return 0;
    }
    uint numerator = fraction.first().toUInt();
    uint denominator = fraction.last().toUInt();
    return (float)numerator / denominator;
}

std::string StackColumn::value(const Item &item) {
    if (item.properties().count("Stack Size"))
        return item.properties().find("Stack Size")->second;
    return "";
}


QVariant RangePropertyColumn::sortValue(const Item &item) {
    QString original = QString::fromStdString(value(item));
    QStringList parts = original.split("-");
    if (parts.size() != 2) {
        return 0;
    }
    uint low = parts.first().toUInt();
    uint high = parts.last().toUInt();

    QVariant val = (low + high) / 2.0;
    if (!val.canConvert(type_) || !val.convert(type_))
        return val.toString();
    return val;
}
