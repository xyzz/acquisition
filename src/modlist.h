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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <QStringList>
#include "rapidjson/document.h"

class Item;
typedef std::unordered_map<std::string, double> ModTable;

// This generates regular expressions for mods and does other setup, should be called when the app starts, perhaps in main()
// Maybe this is not needed and constexpr could do the trick, but VS doesn't support it right now.
void InitModlist();

class ModGenerator {
public:
    virtual void Generate(const rapidjson::Value &json, ModTable *output) = 0;
};

class SumModGenerator : public ModGenerator {
public:
    SumModGenerator(const std::string &name, const std::vector<std::string> &sum);
    virtual void Generate(const rapidjson::Value &json, ModTable *output);
private:
    bool Match(const char *mod, double *output);

    std::string name_;
    std::vector<std::string> matches_;
};

extern QStringList mod_string_list;
extern std::vector<std::unique_ptr<ModGenerator>> mod_generators;
