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
#include <QFontDatabase>
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
#include "tabcache.h"

#ifdef CRASHRPT
#include "CrashRpt.h"
#endif

int main(int argc, char *argv[])
{
    qRegisterMetaType<CurrentStatusUpdate>("CurrentStatusUpdate");
    qRegisterMetaType<Items>("Items");
    qRegisterMetaType<std::vector<std::string>>("std::vector<std::string>");
    qRegisterMetaType<std::vector<ItemLocation>>("std::vector<ItemLocation>");
    qRegisterMetaType<QsLogging::Level>("QsLogging::Level");
    qRegisterMetaType<TabCache::Policy>("TabCache::Policy");

    QLocale::setDefault(QLocale::C);
    std::setlocale(LC_ALL, "C");

#if defined(CRASHRPT) && !defined(_DEBUG)
    CR_INSTALL_INFOW info;
    memset(&info, 0, sizeof(info));
    info.cb = sizeof(info);
    info.pszAppName = L"Acquisition";
    WCHAR wversion[64];
    MultiByteToWideChar(CP_UTF8, 0, VERSION_NAME, -1, wversion, sizeof(wversion) / sizeof(wversion[0]));
    info.pszAppVersion = wversion;
    // URL for sending reports over HTTP.
    info.pszUrl = L"https://xyz.is/crashfix/index.php/crashReport/uploadExternal";
    // Define delivery transport priorities.
    info.uPriorities[CR_HTTP] = 1;                     // Use HTTP.
    info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;  // Not use SMTP.
    info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY; // Not use Simple MAPI.
    // Define flags.
    info.dwFlags = 0;
    info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS; // Install all available exception handlers.
    info.dwFlags |= CR_INST_HTTP_BINARY_ENCODING;  // Use binary encoding for HTTP uploads (recommended).
    CrAutoInstallHelper cr_install_helper(&info);
#endif

    InitModlist();

    QApplication a(argc, argv);
    Filesystem::Init();

    QFontDatabase::addApplicationFont(":/fonts/Fontin-SmallCaps.ttf");

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

    QLOG_DEBUG() << "--------------------------------------------------------------------------------";
    QLOG_DEBUG() << "Built with Qt" << QT_VERSION_STR << "running on" << qVersion();

    LoginDialog login(std::make_unique<Application>());
    login.show();

    return a.exec();
}
