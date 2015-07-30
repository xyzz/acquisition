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

#include "datamanager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QString>
#include <stdexcept>

#include "application.h"

DataManager::DataManager(const QString &filename, const QString &connectionName) :
    filename_(filename),
	connectionName_(connectionName)
{

    QDir dir(filename_ + "/..");
    if (!dir.exists())
        QDir().mkpath(dir.path());

	db_ = QSqlDatabase::addDatabase("QSQLITE",connectionName);
	db_.setDatabaseName(filename_);

	if (!db_.open())
	{
		throw std::runtime_error("Failed to open sqlite3 database. "+ db_.lastError().text().toStdString());
	}

	QSqlQuery query("CREATE TABLE IF NOT EXISTS data(key VARCHAR(255) PRIMARY KEY, value TEXT)", db_);
    if (!query.exec()) {
        throw std::runtime_error("Failed to execute creation statement.");
    }
}

QString DataManager::Get(const QString &key, const QString &default_value) {
	QString result(default_value);
	QSqlQuery query("", db_); 
	query.prepare("SELECT value FROM data WHERE key = :key");
	query.bindValue(":key", key);
	if (query.exec() && query.size())
	{
		query.next();
		result = query.value(0).toString();
	}
    return result;
}

void DataManager::Set(const QString &key, const QString &value) {
	QSqlQuery query("", db_);
	query.prepare("INSERT OR REPLACE INTO data (key, value) VALUES (:key, :value)");
	query.bindValue(":key", key);
	query.bindValue(":value", value);
	query.exec();
}

void DataManager::SetBool(const QString &key, bool value) {
    Set(key, value?"1":"0");
}

bool DataManager::GetBool(const QString &key, bool default_value) {
    return Get(key, default_value ? "1" : "0")=="1";
}

DataManager::~DataManager() {
	db_.close();
}

QString DataManager::MakeFilename(const QString &name, const QString &league) {
    QString key = name + "|" + league;
    return QString(QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex());
}
