/*
    Copyright 2015 Ilya Zhuravlev

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

#include "autoonline.h"

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <tlhelp32.h>
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <QProcess>
#endif

#include <QNetworkRequest>
#include "QsLog.h"
#include "datamanager.h"
#include "version.h"

// check for PoE running every minute
const int kOnlineCheckInterval = 60 * 1000;

AutoOnline::AutoOnline(DataManager &data, DataManager &sensitive_data):
    data_(data),
    sensitive_data_(sensitive_data),
    enabled_(data_.GetBool("online_enabled")),
    url_(sensitive_data_.Get("online_url")),
    previous_status_(true)  // set to true to force first refresh
{
    timer_.setInterval(kOnlineCheckInterval);
    connect(&timer_, &QTimer::timeout, this, &AutoOnline::Check);
    if (enabled_) {
        timer_.start();
        Check();
    }
}

static bool IsPoeRunning() {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    QProcess process;
    process.start("/bin/sh -c \"ps -ax|grep PathOfExile.exe|grep -v grep|wc -l\"");
    process.waitForFinished(-1);
    QString i = process.readAllStandardOutput();
    process.start("/bin/sh -c \"ps -ax|grep PathOfExileSteam.exe|grep -v grep|wc -l\"");
    process.waitForFinished(-1);
    QString j = process.readAllStandardOutput();
    return i.toInt() > 0 || j.toInt() > 0;
#elif defined(Q_OS_WIN)
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms686701%28v=vs.85%29.aspx
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        QLOG_ERROR() << "CreateToolhelp32Snapshot (of processes)";
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(hProcessSnap, &pe32)) {
        QLOG_ERROR() << "Process32First";
        CloseHandle(hProcessSnap);
        return false;
    }

    bool found = false;
    do {
        QString s = QString::fromUtf16((char16_t*)pe32.szExeFile);
        if (s == "PathOfExile.exe" || s == "PathOfExileSteam.exe")
            found = true;
    } while(Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return found;
#endif
}

void AutoOnline::Check() {
    bool running = IsPoeRunning();

    QString url = url_;
    if (!running)
        url += "/offline";

    if (running || previous_status_) {
		QUrl u(url);
        QNetworkRequest request(u);
        QByteArray data;
        request.setHeader(QNetworkRequest::UserAgentHeader, QString("Acquisition ") + QString(VERSION_NAME));
        nm_.post(request, data);
    }

    previous_status_ = running;

    emit Update(running);
}

const QString url_valid_prefix = "http://control.poe.xyz.is/";

void AutoOnline::SetUrl(const QString &url) {
    if (url.indexOf(url_valid_prefix)) {
        QLOG_WARN() << "Online URL is probably invalid.";
    }
    url_ = url;
    // remove trailing '/'s
	while (url_.endsWith('/')) url_.chop(1);
    sensitive_data_.Set("online_url", url_);
}

void AutoOnline::SetEnabled(bool enabled) {
    enabled_ = enabled;
    if (enabled)
        timer_.start();
    else
        timer_.stop();
    data_.SetBool("online_enabled", enabled_);
}
