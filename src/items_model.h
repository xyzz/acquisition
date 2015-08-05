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

#include <QAbstractItemModel>

#include "column.h"
#include "item.h"

class BuyoutManager;
class Search;

class ItemsModel : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Role {
        SortRole = Qt::UserRole,
        HashRole = Qt::UserRole + 1
    };

    ItemsModel(const BuyoutManager &bo_manager, const Search &search);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &index) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
private:
    const BuyoutManager &bo_manager_;
    const Search &search_;
};
