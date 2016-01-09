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
#include "porting.h"

#include <QApplication>
#include <QDir>
#include <QString>
#ifndef PORTABLE
#include <QStandardPaths>
#endif

// This namespace should contain platform-dependant functions.

namespace porting {
std::string DefaultUserDir() {
#ifdef PORTABLE
    return qApp->applicationDirPath().toStdString();
#else
    return QStandardPaths::writableLocation(QStandardPaths::DataLocation).toStdString();
#endif
}
}

#ifdef __ANDROID__
namespace std {
double stod(const std::string& str) {
    std::istringstream is(str);
    double result;
    is >> result;
    return result;
}
}
#endif
