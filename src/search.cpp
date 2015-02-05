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

#include <iostream>
#include <memory>
#include <QTreeView>

#include "buyoutmanager.h"
#include "bucket.h"
#include "column.h"
#include "filters.h"
#include "porting.h"

Search::Search(const BuyoutManager &bo_manager, const std::string &caption, const std::vector<std::unique_ptr<Filter>> &filters) :
    caption_(caption),
    model_(std::make_unique<ItemsModel>(bo_manager, *this))
{
    using move_only = std::unique_ptr<Column>;
    move_only init[] = {
        std::make_unique<NameColumn>(),
        std::make_unique<PriceColumn>(bo_manager),
        std::make_unique<DateColumn>(bo_manager),
        std::make_unique<PropertyColumn>("Q", "Quality"),
        std::make_unique<PropertyColumn>("Stack", "Stack Size"),
        std::make_unique<CorruptedColumn>(),
        std::make_unique<PropertyColumn>("PD", "Physical Damage"),
        std::make_unique<ElementalDamageColumn>(0),
        std::make_unique<ElementalDamageColumn>(1),
        std::make_unique<ElementalDamageColumn>(2),
        std::make_unique<PropertyColumn>("APS", "Attacks per Second"),
        std::make_unique<DPSColumn>(),
        std::make_unique<pDPSColumn>(),
        std::make_unique<eDPSColumn>(),
        std::make_unique<PropertyColumn>("Crit", "Critical Strike Chance"),
        std::make_unique<PropertyColumn>("Ar", "Armour"),
        std::make_unique<PropertyColumn>("Ev", "Evasion Rating"),
        std::make_unique<PropertyColumn>("ES", "Energy Shield"),
        std::make_unique<PropertyColumn>("B", "Chance to Block"),
        std::make_unique<PropertyColumn>("Lvl", "Level")
    };
    columns_ = std::vector<move_only>(std::make_move_iterator(std::begin(init)), std::make_move_iterator(std::end(init)));

    for (auto &filter : filters)
        filters_.push_back(std::move(filter->CreateData()));
}

void Search::FromForm() {
    for (auto &filter : filters_)
        filter->FromForm();
}

void Search::ToForm() {
    for (auto &filter : filters_)
        filter->ToForm();
}

void Search::ResetForm() {
    for (auto &filter : filters_)
        filter->filter()->ResetForm();
}

void Search::FilterItems(const Items &items) {
    items_.clear();
    for (auto item : items) {
        bool matches = true;
        for (auto &filter : filters_)
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
            bucketed_tabs[location] = std::make_unique<Bucket>(location);
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

void Search::Activate(const Items &items, QTreeView *tree) {
    FromForm();
    FilterItems(items);
    tree->setModel(model_.get());
}
