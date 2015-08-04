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

#include <QColor>
#include <QVariant>
#include <string>

#include "item.h"

class BuyoutManager;

class Column {
public:
    virtual QVariant sortValue(const Item &item) = 0;
    virtual std::string name() = 0;
    virtual std::string value(const Item &item) = 0;
    virtual QColor color(const Item &item);
    virtual ~Column() {}
    virtual std::string tooltip() {
        return name();
    }
};

class NameColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
    QColor color(const Item &item);
};

class CorruptedColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
};

class StackColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
};

// Returns values from item -> properties
class PropertyColumn : public Column {
public:
    explicit PropertyColumn(const std::string &name, const QVariant::Type type = QVariant::Invalid);
    PropertyColumn(const std::string &name, const std::string &property, const QVariant::Type type = QVariant::Invalid);
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
    std::string tooltip();
protected:
    std::string name_;
    std::string property_;
    QVariant::Type type_;
};

class PercentPropertyColumn : public PropertyColumn {
public:
    explicit PercentPropertyColumn(const std::string &name, const QVariant::Type type = QVariant::Invalid) :
        PropertyColumn(name, type) {}
    PercentPropertyColumn(const std::string &name, const std::string &property, const QVariant::Type type = QVariant::Invalid) :
        PropertyColumn(name, property, type) {}
    QVariant sortValue(const Item &item);
};

class RangePropertyColumn : public PropertyColumn {
public:
    explicit RangePropertyColumn(const std::string &name, const QVariant::Type type = QVariant::Invalid) :
        PropertyColumn(name, type) {}
    RangePropertyColumn(const std::string &name, const std::string &property, const QVariant::Type type = QVariant::Invalid) :
        PropertyColumn(name, property, type) {}
    QVariant sortValue(const Item &item);
};

class DPSColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
};

class pDPSColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
};

class eDPSColumn : public Column {
public:
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
};

class ElementalDamageColumn : public Column {
public:
    explicit ElementalDamageColumn(ELEMENTAL_DAMAGE_TYPES type);
    std::string name();
    std::string tooltip();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
    QColor color(const Item &item);
    int GetIndex(const Item &item);
private:
    ELEMENTAL_DAMAGE_TYPES type_;
    size_t index_;
};

class PriceColumn : public Column {
public:
    explicit PriceColumn(const BuyoutManager &bo_manager);
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
private:
    const BuyoutManager &bo_manager_;
};

class DateColumn : public Column {
public:
    explicit DateColumn(const BuyoutManager &bo_manager);
    std::string name();
    QVariant sortValue(const Item &item);
    std::string value(const Item &item);
private:
    const BuyoutManager &bo_manager_;
};
