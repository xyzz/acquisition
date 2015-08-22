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
#include <QCommandLineParser>
#include <QDir>
#include <QLocale>
#include "QsLog.h"
#include "QsLogDest.h"
#include <clocale>
#include <limits>
#include <memory>

#include "application.h"
#include "filesystem.h"
#include "itemsmanagerworker.h"
#include "modlist.h"
#include "porting.h"
#include "version.h"
#include "../test/testmain.h"

#ifdef CRASHRPT
#include "CrashRpt.h"
#endif

int main(int argc, char *argv[])
{
    qRegisterMetaType<CurrentStatusUpdate>("CurrentStatusUpdate");
    qRegisterMetaType<Items>("Items");
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
    qRegisterMetaType<QsLogging::Level>("QsLogging::Level");

    QLocale::setDefault(QLocale::C);
    std::setlocale(LC_ALL, "C");

#if defined(CRASHRPT) && !defined(DEBUG)
    CR_INSTALL_INFOW info;
    memset(&info, 0, sizeof(CR_INSTALL_INFOW));
    info.cb = sizeof(CR_INSTALL_INFOW);
    info.pszAppName = L"Acquisition";
    info.pszAppVersion = L"0.0";
    info.pszUrl = L"https://xyz.is/acquisition/utils/crashrpt.php";
    CrAutoInstallHelper cr_install_helper(&info);
#endif

    InitModlist();

    QApplication a(argc, argv);
    Filesystem::Init();

    QCommandLineParser parser;
    QCommandLineOption option_test("test"), option_data_dir("data-dir", "Where to save Acquisition data.", "data-dir");
    parser.addOption(option_test);
    parser.addOption(option_data_dir);
    parser.process(a);

    if (parser.isSet(option_test))
        return test_main();

    if (parser.isSet(option_data_dir))
        Filesystem::SetUserDir(parser.value(option_data_dir).toStdString());

    QsLogging::Logger& logger = QsLogging::Logger::instance();
    logger.setLoggingLevel(QsLogging::InfoLevel);
    const QString sLogPath(QDir(Filesystem::UserDir().c_str()).filePath("log.txt"));

    QsLogging::DestinationPtr fileDestination(
        QsLogging::DestinationFactory::MakeFileDestination(sLogPath, true, 10 * 1024 * 1024, 0) );
    QsLogging::DestinationPtr debugDestination(
        QsLogging::DestinationFactory::MakeDebugOutputDestination() );
    logger.addDestination(debugDestination);
    logger.addDestination(fileDestination);

    QLOG_INFO() << "--------------------------------------------------------------------------------";
    QLOG_INFO() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();

    LoginDialog login(std::make_unique<Application>());
    login.show();

    return a.exec();
}
