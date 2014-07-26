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

#include "logindialog.h"

#include <QApplication>
#include <QDir>
#include <QLocale>
#include "QsLog.h"
#include "QsLogDest.h"
#include <limits>

int main(int argc, char *argv[])
{
    QLocale::setDefault(QLocale::C);
    QApplication a(argc, argv);

    QsLogging::Logger& logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::InfoLevel);
    const QString sLogPath(QDir(a.applicationDirPath()).filePath("log.txt"));

    QsLogging::DestinationPtr fileDestination(
        QsLogging::DestinationFactory::MakeFileDestination(sLogPath, true, 10 * 1024 * 1024, 0) );
    QsLogging::DestinationPtr debugDestination(
        QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);

    QLOG_INFO() << "--------------------------------------------------------------------------------";
    QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();

    LoginDialog login;
    login.show();

    return a.exec();
}
