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
#include <QString>

#include "item.h"

class BuyoutManager;

class Column {
public:
    virtual QString name() = 0;
    virtual QString value(const Item &item) = 0;
    virtual QColor color(const Item &item);
    virtual ~Column() {}
};

class NameColumn : public Column {
public:
    QString name();
    QString value(const Item &item);
    QColor color(const Item &item);
};

class CorruptedColumn : public Column {
public:
    QString name();
    QString value(const Item &item);
};

// Returns values from item -> properties
class PropertyColumn : public Column {
public:
    explicit PropertyColumn(const QString &name);
    PropertyColumn(const QString &name, const QString &property);
    QString name();
    QString value(const Item &item);
private:
    QString name_;
    QString property_;
};

class DPSColumn : public Column {
public:
    QString name();
    QString value(const Item &item);
};

class pDPSColumn : public Column {
public:
    QString name();
    QString value(const Item &item);
};

class eDPSColumn : public Column {
public:
    QString name();
    QString value(const Item &item);
};

class ElementalDamageColumn : public Column {
public:
    explicit ElementalDamageColumn(int index);
    QString name();
    QString value(const Item &item);
    QColor color(const Item &item);
private:
    size_t index_;
};

class PriceColumn : public Column {
public:
    explicit PriceColumn(const BuyoutManager &bo_manager);
    QString name();
    QString value(const Item &item);
private:
    const BuyoutManager &bo_manager_;
};

class DateColumn : public Column {
public:
    explicit DateColumn(const BuyoutManager &bo_manager);
    QString name();
    QString value(const Item &item);
private:
    const BuyoutManager &bo_manager_;
};
