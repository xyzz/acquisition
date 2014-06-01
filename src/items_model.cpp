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
#include "search.h"

ItemsModel::ItemsModel(QObject *parent, Search *search) :
    QAbstractItemModel(parent),
    search_(search)
{
}

int ItemsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return search_->items().size();
}

int ItemsModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return search_->columns().size();
}

QVariant ItemsModel::headerData(int section, Qt::Orientation /* orientation */, int role) const {
    if (role == Qt::DisplayRole)
        return QString(search_->columns()[section]->name().c_str());
    return QVariant();
}

QVariant ItemsModel::data(const QModelIndex &index, int role) const {
    Column *column = search_->columns()[index.column()];
    const Item &item = *search_->items()[index.row()];
    if (role == Qt::DisplayRole)
        return QString(column->value(item).c_str());
    else if (role == Qt::ForegroundRole)
        return column->color(item);
    return QVariant();
}

QModelIndex ItemsModel::parent(const QModelIndex & /* index */) const {
    return QModelIndex();
}

QModelIndex ItemsModel::index(int row, int column, const QModelIndex & /* parent */) const {
    return createIndex(row, column);
}
