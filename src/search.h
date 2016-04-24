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

#include <memory>
#include <vector>
#include <set>

#include "item.h"
#include "column.h"
#include "bucket.h"
#include "util.h"

class BuyoutManager;
class Filter;
class FilterData;
class ItemsModel;
class QTreeView;
class QModelIndex;

class Search {
public:
    enum ViewMode {
        ByTab,
        ByItem
    };

public:
    Search(BuyoutManager &bo, const std::string &caption, const std::vector<std::unique_ptr<Filter>> &filters, QTreeView *view);
    void FilterItems(const Items &items);
    void FromForm();
    void ToForm();
    void ResetForm();
    const std::string &caption() const { return caption_; }
    const Items &items() const { return items_; }
    const std::vector<std::unique_ptr<Column>> &columns() const { return columns_; }
    const std::vector<std::unique_ptr<Bucket>> &buckets() const;
    QString GetCaption();
    int GetItemsCount();
    bool IsAnyFilterActive() const;
    // Sets this search as current, will display items in passed QTreeView.
    void Activate(const Items &items);
    void RestoreViewProperties();
    void SaveViewProperties();
    ItemLocation GetTabLocation(const QModelIndex & index) const;
    void SetViewMode(ViewMode mode);
    int GetViewMode() { return current_mode_; };
    const std::unique_ptr<Bucket> &bucket(int row) const;
    void SetRefreshReason(RefreshReason::Type reason) { refresh_reason_ = reason;};
private:
    void UpdateItemCounts(const Items &items);

    std::vector<std::unique_ptr<FilterData>> filters_;
    std::vector<std::unique_ptr<Column>> columns_;
    std::string caption_;
    Items items_;
    QTreeView *view_{nullptr};
    BuyoutManager &bo_manager_;
    std::unique_ptr<ItemsModel> model_;
    std::vector<std::unique_ptr<Bucket>> buckets_;
    std::vector<std::unique_ptr<Bucket>> bucket_;
    uint unfiltered_item_count_{0};
    uint filtered_item_count_total_{0};
    std::set<std::string> expanded_property_;
    ViewMode current_mode_{ByTab};
    RefreshReason::Type refresh_reason_{RefreshReason::Unknown};
};
