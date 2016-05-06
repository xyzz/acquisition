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

#include "items_model.h"

#include "application.h"
#include "bucket.h"
#include "buyoutmanager.h"
#include "itemlocation.h"
#include "search.h"
#include "util.h"
#include "QsLog.h"

ItemsModel::ItemsModel(BuyoutManager &bo_manager, const Search &search) :
    bo_manager_(bo_manager),
    search_(search)
{
}

/*
    Tree structure:

    + stash tab title (called "bucket" elsewhere)
    |- item
    |- item
      ...
    + another stash tab or character
    |- item
    |- item

    and so on
*/

int ItemsModel::rowCount(const QModelIndex &parent) const {
    // Root element, contains buckets
    if (!parent.isValid())
        return search_.buckets().size();
    // Bucket, contains elements
    if (parent.isValid() && !parent.parent().isValid()) {
        return search_.bucket(parent.row())->items().size();
    }
    // Element, contains nothing
    return 0;
}

int ItemsModel::columnCount(const QModelIndex &parent) const {
    // Root element, contains buckets
    if (!parent.isValid())
        return search_.columns().size();
    // Bucket, contains elements
    if (parent.isValid() && !parent.parent().isValid())
        return search_.columns().size();
    // Element, contains nothing
    return 0;
}

QVariant ItemsModel::headerData(int section, Qt::Orientation /* orientation */, int role) const {
    if (role == Qt::DisplayRole)
        return QString(search_.columns()[section]->name().c_str());
    return QVariant();
}

QVariant ItemsModel::data(const QModelIndex &index, int role) const {
    // Bucket title
    if (!index.isValid())
        return QVariant();

    if (index.internalId() == 0) {
        if (index.column() > 0)
            return QVariant();

        const ItemLocation &location = search_.GetTabLocation(index);
        if ( role == Qt::CheckStateRole) {
            if (!location.IsValid())
                return QVariant();
            if (bo_manager_.GetRefreshLocked(location))
                return Qt::PartiallyChecked;
            return ( bo_manager_.GetRefreshChecked(location) ? Qt::Checked : Qt::Unchecked );
        }
        if (role == Qt::DisplayRole) {
            if (!location.IsValid())
                return "All Items";
            QString title(location.GetHeader().c_str());
            auto const &bo = bo_manager_.GetTab(location.GetUniqueHash());
            if (bo.IsActive())
                title += QString(" [%1]").arg(bo.AsText().c_str());
            return title;
        }
        return QVariant();
    }
    auto &column = search_.columns()[index.column()];
    const Item &item = *search_.bucket(index.parent().row())->item(index.row());
    if (role == Qt::DisplayRole)
        return column->value(item);
    else if (role == Qt::ForegroundRole)
        return column->color(item);
    return QVariant();
}

Qt::ItemFlags ItemsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    if ( index.column() == 0 && index.internalId() == 0) {
        const ItemLocation &location = search_.GetTabLocation(index);
        if (location.IsValid() && !bo_manager_.GetRefreshLocked(location))
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
    }
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

bool ItemsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (!index.isValid())
        return false;

    if (role == Qt::CheckStateRole) {
        const ItemLocation &location = search_.GetTabLocation(index);
        bo_manager_.SetRefreshChecked(location, value.toBool());

        // It's possible that our tabs can have the same name.  Right now we don't have a
        // way to differentiate these tabs so indicate dataChanged event for each tab with
        // the same name as the current checked tab so the 'check' is properly updated in
        // the layout
        std::string target_hash = location.GetUniqueHash();
        auto row_count = rowCount();
        for (int i = 0; i < row_count; ++i) {
            auto match_index = this->index(i);
            if (search_.GetTabLocation(match_index).GetUniqueHash() == target_hash)
                emit dataChanged(match_index,match_index);
        }

        return true;
    }
    return false;
}

void ItemsModel::sort(int column, Qt::SortOrder order)
{
    // Ignore sort requests if we're already sorted
    if (sorted_ && (sort_column_ == column) && (sort_order_ == order))
        return;

    QLOG_DEBUG() << "Sorting";
    sort_order_ = order;
    sort_column_ = column;

    auto &column_obj = search_.columns()[column];
    for (const auto &bucket: search_.buckets()) {
        bucket->Sort(*column_obj, order);
    }
    layoutChanged();
    SetSorted(true);
}

void ItemsModel::sort()
{
    sort(sort_column_, sort_order_);
}

QModelIndex ItemsModel::parent(const QModelIndex &index) const {
    // bucket
    if (!index.isValid() || index.internalId() == 0) {
        return QModelIndex();
    }
    // item
    return createIndex(index.internalId() - 1, 0, static_cast<quintptr>(0));
}

QModelIndex ItemsModel::index(int row, int column, const QModelIndex &parent) const {
    if (parent.isValid()) {
        if (parent.row() >= (signed)search_.buckets().size()) {
            QLOG_WARN() << "Should not happen: Index request parent contains invalid row";
            return QModelIndex();
        }
        // item, we pass parent's (bucket's) row through ID parameter
        return createIndex(row, column, static_cast<quintptr>(parent.row() + 1));
    } else {
        if (row >= (signed)search_.buckets().size()) {
            QLOG_WARN() << "Index request asking for invalid row";
            return QModelIndex();
        }
        return createIndex(row, column, static_cast<quintptr>(0));
    }
}
