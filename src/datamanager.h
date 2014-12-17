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

class Application;

class DataManager {
public:
    DataManager(const std::string &filename_);
    ~DataManager();
    void Set(const std::string &key, const std::string &value);
    std::string Get(const std::string &key, const std::string &default_value = "");
    void SetBool(const std::string &key, bool value);
    bool GetBool(const std::string &key, bool default_value = false);
    static std::string MakeFilename(const std::string &name, const std::string &league);
private:
    std::string filename_;
    sqlite3 *db_;
};
