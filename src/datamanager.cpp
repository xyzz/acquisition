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

#include "mainwindow.h"

DataManager::DataManager(MainWindow *app, const std::string &directory):
    app_(app)
{
    if (!QDir(directory.c_str()).exists())
        QDir().mkpath(directory.c_str());
    std::string key = app_->email() + "|" + app->league();
    filename_ = QString(QCryptographicHash::hash(key.c_str(), QCryptographicHash::Md5).toHex()).toUtf8().constData();
    filename_ = directory + "/" + filename_;
    if (sqlite3_open(filename_.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open sqlite3 database.");
    }
    std::string query = "CREATE TABLE IF NOT EXISTS data(key TEXT PRIMARY KEY, value BLOB)";
    if (sqlite3_exec(db_, query.c_str(), 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("Failed to execute creation statement.");
    }
}

std::string DataManager::Get(const std::string &key) {
    std::string query = "SELECT value FROM data WHERE key = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare(db_, query.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    std::string result;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        result = std::string(static_cast<const char*>(sqlite3_column_blob(stmt, 0)), sqlite3_column_bytes(stmt, 0));
    sqlite3_finalize(stmt);
    return result;
}

void DataManager::Set(const std::string &key, const std::string &value) {
    std::string query = "INSERT OR REPLACE INTO data (key, value) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    sqlite3_prepare(db_, query.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, value.c_str(), value.size(), SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

DataManager::~DataManager() {
    sqlite3_close(db_);
}
