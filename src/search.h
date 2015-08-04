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

#include <QSortFilterProxyModel>
#include <memory>
#include <vector>

#include "item.h"
#include "column.h"
#include "bucket.h"

class BuyoutManager;
class Filter;
class FilterData;
class ItemsModel;
class QTreeView;

class Search {
public:
    Search(const BuyoutManager &bo, const std::string &caption, const std::vector<std::unique_ptr<Filter>> &filters);
    void FilterItems(const Items &items);
    void FromForm();
    void ToForm();
    void ResetForm();
    const std::string &caption() const { return caption_; }
    const Items &items() const { return items_; }
    const std::vector<std::unique_ptr<Column>> &columns() const { return columns_; }
    const std::vector<std::unique_ptr<Bucket>> &buckets() const { return buckets_; }
    QString GetCaption();
    int GetItemsCount();
    // Sets this search as current, will display items in passed QTreeView.
    void Activate(const Items &items, QTreeView *tree);
    QModelIndex GetIndex(const QModelIndex &index) const;
private:
    std::vector<std::unique_ptr<FilterData>> filters_;
    std::vector<std::unique_ptr<Column>> columns_;
    std::string caption_;
    Items items_;
    std::unique_ptr<ItemsModel> model_;
    std::vector<std::unique_ptr<Bucket>> buckets_;
    std::unique_ptr<QSortFilterProxyModel> sortFilter_;
};
