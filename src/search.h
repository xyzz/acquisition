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

#include "item.h"
#include "column.h"
#include "bucket.h"

class Filter;
class FilterData;
class ItemsModel;
class MainWindow;

class Search {
public:
    explicit Search(MainWindow *app, std::string caption, std::vector<Filter*> filters);
    ~Search();
    void FilterItems(const Items &items);
    void FromForm();
    void ToForm();
    void ResetForm();
    const std::string &caption() const { return caption_; }
    const Items &items() const { return items_; }
    const std::vector<Column*> &columns() const { return columns_; }
    ItemsModel *model() const { return model_; }
    const std::vector<std::unique_ptr<Bucket>> &buckets() const { return buckets_; }
    QString GetCaption();
    int GetItemsCount();
private:
    MainWindow *app_;
    std::vector<FilterData*> filters_;
    std::vector<Column*> columns_;
    std::string caption_;
    Items items_;
    ItemsModel *model_;
    std::vector<std::unique_ptr<Bucket>> buckets_;
};
