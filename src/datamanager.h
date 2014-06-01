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

#include "sqlite/sqlite3.h"
#include <string>

class MainWindow;

class DataManager {
public:
    DataManager(MainWindow *app, const std::string &directory_);
    ~DataManager();
    void Set(const std::string &key, const std::string &value);
    std::string Get(const std::string &key);
private:
    MainWindow *app_;
    std::string email_;
    std::string league_;
    std::string filename_;
    sqlite3 *db_;
};
