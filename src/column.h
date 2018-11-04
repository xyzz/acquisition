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
#include <string>
#include <QVariant>

#include "item.h"

class BuyoutManager;

class Column {
public:
    virtual std::string name() const = 0;
    virtual QVariant value(const Item &item) const = 0;
    virtual QColor color(const Item &item) const;
    virtual bool lt(const Item* lhs, const Item* rhs) const;
    virtual ~Column() {}
private:
    std::tuple<QVariant, QVariant, const Item&> multivalue(const Item *item) const;
};

class NameColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
    QColor color(const Item &item) const;
};

class CorruptedColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

class WarColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

// Returns values from item -> properties
class PropertyColumn : public Column {
public:
    explicit PropertyColumn(const std::string &name);
    PropertyColumn(const std::string &name, const std::string &property);
    std::string name() const;
    QVariant value(const Item &item) const;
private:
    std::string name_;
    std::string property_;
};

class DPSColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

class pDPSColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

class eDPSColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

class ElementalDamageColumn : public Column {
public:
    explicit ElementalDamageColumn(int index);
    std::string name() const;
    QVariant value(const Item &item) const;
    QColor color(const Item &item) const;
private:
    size_t index_;
};

class ChaosDamageColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
    QColor color(const Item &item) const;
};

class cDPSColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};

class PriceColumn : public Column {
public:
    explicit PriceColumn(const BuyoutManager &bo_manager);
    std::string name() const;
    QVariant value(const Item &item) const;
    QColor color(const Item &item) const;
    bool lt(const Item *lhs, const Item *rhs) const;
private:
    std::tuple<int, double, const Item&> multivalue(const Item *item) const;
    const BuyoutManager &bo_manager_;
};

class DateColumn : public Column {
public:
    explicit DateColumn(const BuyoutManager &bo_manager);
    std::string name() const;
    QVariant value(const Item &item) const;
    bool lt(const Item *lhs, const Item *rhs) const;
private:
    const BuyoutManager &bo_manager_;
};

class ItemlevelColumn : public Column {
public:
    std::string name() const;
    QVariant value(const Item &item) const;
};
