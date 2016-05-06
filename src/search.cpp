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
#include "QsLog.h"
#include <QMessageBox>

Search::Search(BuyoutManager &bo_manager, const std::string &caption,
               const std::vector<std::unique_ptr<Filter>> &filters, QTreeView *view) :
    caption_(caption),
    view_(view),
    bo_manager_(bo_manager),
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
        std::make_unique<PropertyColumn>("Lvl", "Level"),
        std::make_unique<ItemlevelColumn>()
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

const std::vector<std::unique_ptr<Bucket> > &Search::buckets() const {
    if (current_mode_ == ByTab) {
        return buckets_;
    } else {
        return bucket_;
    }
}

const std::unique_ptr<Bucket> &Search::bucket(int row) const {
    auto const &active_buckets = (current_mode_ == ByTab) ? buckets_:bucket_;

    if (row < 0 || row >= active_buckets.size()) {
        QMessageBox::critical(0, "Fatal Error", QString("Bucket row out of bounds: ") +
                              QString::number(row) + " bucket size: " + QString::number(active_buckets.size()) +
                              " mode:" + QString::number(current_mode_) +
                              ". Program will abort.");
        abort();
    }
    return active_buckets[row];
}

void Search::FilterItems(const Items &items) {
    // If we're just changing tabs we don't need to update anything
    if (refresh_reason_ == RefreshReason::TabChanged)
        return;

    QLOG_DEBUG() << "FilterItems: reason(" << refresh_reason_ << ")";
    items_.clear();
    for (const auto &item : items) {
        bool matches = true;
        for (auto &filter : filters_)
            if (!filter->Matches(item)) {
                matches = false;
                break;
            }
        if (matches)
            items_.push_back(item);
    }

    UpdateItemCounts(items);

    // Single bucket with null location is used to view all items at once
    bucket_.clear();
    bucket_.push_back(std::make_unique<Bucket>(ItemLocation()));

    std::map<ItemLocation, std::unique_ptr<Bucket>> bucketed_tabs;
    for (const auto &item : items_) {
        ItemLocation location = item->location();
        if (!bucketed_tabs.count(location))
            bucketed_tabs[location] = std::make_unique<Bucket>(location);
        bucketed_tabs[location]->AddItem(item);        
        bucket_.front()->AddItem(item);
    }

    // We need to add empty tabs here as there are no items to force their addition
    // But only do so if no filters are active as we want to hide empty tabs when
    // filtering
    if (!IsAnyFilterActive()) {
        for (auto &location: bo_manager_.GetStashTabLocations())
            if (!bucketed_tabs.count(location)) {
                bucketed_tabs[location] = std::make_unique<Bucket>(location);
            }
    }

    buckets_.clear();
    for (auto &element : bucketed_tabs)
        buckets_.push_back(std::move(element.second));

    // Let the model know that current sort order has been invalidated
    model_->SetSorted(false);
}

QString Search::GetCaption() {
    return QString("%1 [%2]").arg(caption_.c_str()).arg(GetItemsCount());
}

ItemLocation Search::GetTabLocation(const QModelIndex & index) const {
    if (!index.isValid())
        return ItemLocation();

    if (index.internalId() > 0) {
        // If index represents an item, get location from item as view may be on 'item' view
        // where bucket location doesn't match items location
        return bucket(index.parent().row())->item(index.row())->location();
    } else {
        // Otherwise index represents a tab already, get location from there
        return bucket(index.row())->location();
    }
}

void Search::SetViewMode(ViewMode mode)
{
    if (mode != current_mode_) {
        if (mode == ByItem)
            SaveViewProperties();

        current_mode_ = mode;
        // Force immediate view update
        view_->reset();
        model_->SetSorted(false);
        model_->sort();

        if (mode == ByTab)
            RestoreViewProperties();
    }
}

int Search::GetItemsCount() {
    return filtered_item_count_total_;
}

void Search::Activate(const Items &items) {
    FromForm();
    FilterItems(items);
    view_->setSortingEnabled(false);
    view_->setModel(model_.get());
    view_->header()->setSortIndicator(model_->GetSortColumn(), model_->GetSortOrder());
    view_->setSortingEnabled(true);
}

void Search::SaveViewProperties() {
    expanded_property_.clear();
    auto rowCount = model_->rowCount();
    for( int row = 0; row < rowCount; ++row ) {
        QModelIndex index = model_->index( row, 0, QModelIndex());
        if (index.isValid() && view_->isExpanded(index)) {
            expanded_property_.insert(bucket(row)->location().GetHeader());
        }
    }
}

void Search::RestoreViewProperties() {
    if (!expanded_property_.empty()) {
        auto rowCount = model_->rowCount();
        for( int row = 0; row < rowCount; ++row ) {
            QModelIndex index = model_->index( row, 0, QModelIndex());
            // Block signals else columns will be resized on every expand which can be super slow.
            view_->blockSignals(true);
            if (expanded_property_.count(bucket(row)->location().GetHeader())) {
                view_->expand(index);
            }
            view_->blockSignals(false);
        }
    }
}

bool Search::IsAnyFilterActive() const {
    return (items_.size() != unfiltered_item_count_);
}

void Search::UpdateItemCounts(const Items &items) {
    unfiltered_item_count_ = items.size();

    filtered_item_count_total_ = 0;
    for (auto &item : items_)
        filtered_item_count_total_ += item->count();
}

