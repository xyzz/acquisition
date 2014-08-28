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

#include "search.h"

#include "application.h"
#include "bucket.h"
#include "column.h"
#include "filters.h"
#include "porting.h"

#include <iostream>

Search::Search(Application *app, std::string caption, std::vector<Filter*> filters):
    app_(app),
    caption_(caption),
    model_(new ItemsModel(this))
{
    columns_ = {
        new NameColumn,
        new PriceColumn(app_->buyout_manager()),
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

    std::map<ItemLocation, std::unique_ptr<Bucket>> bucketed_tabs;
    for (const auto &item : items_) {
        ItemLocation location = item->location();
        if (!bucketed_tabs.count(location))
            bucketed_tabs[location] = std::make_unique<Bucket>(location.GetHeader());
        bucketed_tabs[location]->AddItem(item);
    }

    buckets_.clear();
    for (auto &element : bucketed_tabs)
        buckets_.push_back(std::move(element.second));
}

QString Search::GetCaption() {
    return QString("%1 [%2]").arg(caption_.c_str()).arg(GetItemsCount());
}

int Search::GetItemsCount() {
    int count = 0;
    for (auto &item : items_)
        count += item->count();
    return count;
}
