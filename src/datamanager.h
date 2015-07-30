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

#include <QString>
#include <QString>
#include <QtSql/QtSql>

class Application;

class DataManager {
public:
    DataManager(const QString &filename_, const QString &connectionName);
    ~DataManager();
    void Set(const QString &key, const QString &value);
    QString Get(const QString &key, const QString &default_value = "");
    void SetBool(const QString &key, bool value);
    bool GetBool(const QString &key, bool default_value = false);
    static QString MakeFilename(const QString &name, const QString &league);
private:
	QString filename_;
	QString connectionName_;
	QSqlDatabase db_;
};
