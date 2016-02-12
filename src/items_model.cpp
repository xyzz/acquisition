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
        return search_.buckets()[parent.row()]->items().size();
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
    if (index.internalId() == 0) {
        if (index.column() > 0)
            return QVariant();

        const ItemLocation &location = search_.GetTabLocation(index);
        if ( role == Qt::CheckStateRole) {
            if (bo_manager_.GetRefreshLocked(location))
                return Qt::PartiallyChecked;
            return ( bo_manager_.GetRefreshChecked(location) ? Qt::Checked : Qt::Unchecked );
        }
        if (role == Qt::DisplayRole) {
            QString title(location.GetHeader().c_str());
            if (bo_manager_.ExistsTab(location.GetUniqueHash()))
                title += QString(" [%1]").arg(Util::BuyoutAsText(bo_manager_.GetTab(location.GetUniqueHash())).c_str());
            return title;
        }
        return QVariant();
    }
    auto &column = search_.columns()[index.column()];
    const Item &item = *search_.buckets()[index.parent().row()]->items()[index.row()];
    if (role == Qt::DisplayRole)
        return QString(column->value(item).c_str());
    else if (role == Qt::ForegroundRole)
        return column->color(item);
    return QVariant();
}

Qt::ItemFlags ItemsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    if ( index.column() == 0 && index.internalId() == 0) {
        const ItemLocation &location = search_.GetTabLocation(index);
        flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        if (!bo_manager_.GetRefreshLocked(location)) {
            flags |= Qt::ItemIsUserCheckable;
        }
    }
    return flags;
}

bool ItemsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::CheckStateRole) {
        const ItemLocation &location = search_.GetTabLocation(index);
        bo_manager_.SetRefreshChecked(location, value.toBool());

        // It's possible that our tabs can have the same name.  Right now we don't have a
        // way to differentiate these tabs so indicate dataChanged event for each tab with
        // the same name as the current checked tab so the 'check' is properly updated in
        // the layout
        std::string target_hash = location.GetUniqueHash();
        for (int i = 0; i < rowCount(); ++i) {
            auto match_index = this->index(i);
            if (search_.GetTabLocation(match_index).GetUniqueHash() == target_hash)
                emit dataChanged(match_index,match_index);
        }

        return true;
    }
    return false;
}

QModelIndex ItemsModel::parent(const QModelIndex &index) const {
    // bucket
    if (index.internalId() == 0)
        return QModelIndex();
    // item
    return createIndex(index.internalId() - 1, 0, static_cast<quintptr>(0));
}

QModelIndex ItemsModel::index(int row, int column, const QModelIndex &parent) const {
    // bucket
    if (!parent.isValid())
        return createIndex(row, column, static_cast<quintptr>(0));
    // item, we pass parent's (bucket's) row through ID parameter
    return createIndex(row, column, static_cast<quintptr>(parent.row() + 1));
}
