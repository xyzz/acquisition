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

#include "filters.h"
#include "search.h"
#include "column.h"

#include <iostream>

Search::Search(std::string caption, std::vector<Filter*> filters):
    caption_(caption),
    model_(new ItemsModel(0, this))
{
    columns_ = {
        new NameColumn,
        new PropertyColumn("Q", "Quality"),
        new PropertyColumn("Stack", "Stack Size"),
        new CorruptedColumn,
        new PropertyColumn("PD", "Physical Damage"),
        new ElementalDamageColumn(0),
        new ElementalDamageColumn(1),
        new ElementalDamageColumn(2),
        new PropertyColumn("APS", "Attacks per Second"),
        new DPSColumn,
        new pDPSColumn,
        new eDPSColumn,
        new PropertyColumn("Crit", "Critical Strike Chance"),
        new PropertyColumn("Ar", "Armour"),
        new PropertyColumn("Ev", "Evasion"),
        new PropertyColumn("ES", "Energy Shield"),
        new PropertyColumn("B", "Chance to Block"),
        new PropertyColumn("Lvl", "Level"),
    };
    for (auto filter : filters)
        filters_.push_back(filter->CreateData());
}

Search::~Search() {
    for (auto filter : filters_)
        delete filter;
    for (auto column : columns_)
        delete column;
    delete model_;
}

void Search::FromForm() {
    for (auto filter : filters_)
        filter->FromForm();
}

void Search::ToForm() {
    for (auto filter : filters_)
        filter->ToForm();
}

void Search::ResetForm() {
    for (auto filter : filters_)
        filter->filter()->ResetForm();
}

void Search::FilterItems(const Items &items) {
    items_.clear();
    for (auto item : items) {
        bool matches = true;
        for (auto filter : filters_)
            if (!filter->Matches(item)) {
                matches = false;
                break;
            }
        if (matches)
            items_.push_back(item);
    }
}
