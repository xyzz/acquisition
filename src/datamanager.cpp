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

#include "sqlite/sqlite3.h"
#include <QCryptographicHash>
#include <QDir>
#include <ctime>
#include <stdexcept>

#include "currencymanager.h"

DataManager::DataManager(const std::string &filename) :
    filename_(filename)
{
    QDir dir((filename + "/..").c_str());
    if (!dir.exists())
        QDir().mkpath(dir.path());
    if (sqlite3_open(filename_.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Failed to open sqlite3 database.");
    }
    CreateTable("data", "key TEXT PRIMARY KEY, value BLOB");
    CreateTable("currency", "timestamp INTEGER PRIMARY KEY, value TEXT");
}

void DataManager::CreateTable(const std::string &name, const std::string &fields) {
    std::string query = "CREATE TABLE IF NOT EXISTS " + name + "(" + fields + ")";
    if (sqlite3_exec(db_, query.c_str(), 0, 0, 0) != SQLITE_OK) {
        throw std::runtime_error("Failed to execute creation statement for table " + name + ".");
    }
}

std::string DataManager::Get(const std::string &key, const std::string &default_value) {
    std::string query = "SELECT value FROM data WHERE key = ?";
    sqlite3_stmt *stmt;
    sqlite3_prepare(db_, query.c_str(), -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    std::string result(default_value);
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

void DataManager::InsertCurrencyUpdate(const CurrencyUpdate &update) {
    std::string query = "INSERT INTO currency (timestamp, value) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    sqlite3_prepare(db_, query.c_str(), -1, &stmt, 0);
    sqlite3_bind_int64(stmt, 1, update.timestamp);
    sqlite3_bind_text(stmt, 2, update.value.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<CurrencyUpdate> DataManager::GetAllCurrency() {
    std::string query = "SELECT timestamp, value FROM currency ORDER BY timestamp ASC";
    sqlite3_stmt* stmt;
    sqlite3_prepare(db_, query.c_str(), -1, &stmt, 0);
    std::vector<CurrencyUpdate> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CurrencyUpdate update = { 0, "" };
        update.timestamp = sqlite3_column_int64(stmt, 0);
        update.value = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        result.push_back(update);
    }
    sqlite3_finalize(stmt);
    return result;
}

void DataManager::SetBool(const std::string &key, bool value) {
    Set(key, std::to_string(static_cast<int>(value)));
}

bool DataManager::GetBool(const std::string &key, bool default_value) {
    return static_cast<bool>(std::stoi(Get(key, default_value ? "1" : "0")));
}

DataManager::~DataManager() {
    sqlite3_close(db_);
}

std::string DataManager::MakeFilename(const std::string &name, const std::string &league) {
    std::string key = name + "|" + league;
    return QString(QCryptographicHash::hash(key.c_str(), QCryptographicHash::Md5).toHex()).toStdString();
}
