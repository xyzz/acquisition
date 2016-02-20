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
#include "datastore.h"
#include "version.h"

namespace {

bool is_poe_running_locally() {
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

bool is_poe_running_remotely(const std::string& script) {
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
    std::string poe_check_script = "/bin/sh -c \"" + script + "\"";

    QProcess process;
    process.start(poe_check_script.c_str());
    process.waitForFinished(-1);

    return process.exitCode() == 0;
#elif defined(Q_OS_WIN)
    QLOG_ERROR() << "Script handling has not been implemented on Windows";
    return false;
#endif
}


} //end of anonymous namespace

// check for PoE running every minute
const int kOnlineCheckInterval = 60 * 1000;

AutoOnline::AutoOnline(DataStore &data, DataStore &sensitive_data) :
    data_(data),
    sensitive_data_(sensitive_data),
    enabled_(data_.GetBool("online_enabled")),
    url_(sensitive_data_.Get("online_url")),
    process_script_(data.Get("process_script")),
    previous_status_(true)  // set to true to force first refresh
{
    timer_.setInterval(kOnlineCheckInterval);
    connect(&timer_, &QTimer::timeout, this, &AutoOnline::Check);
    if (enabled_) {
        timer_.start();
        Check();
    }
}

void AutoOnline::SendOnlineUpdate(bool online) {
    // online: true  -> Go online
    // online: false -> Go offline
    std::string url = url_;
    if(!online) {
        url += "/offline";
    }
    QNetworkRequest request(QUrl(url.c_str()));
    QByteArray data;
    request.setHeader(QNetworkRequest::UserAgentHeader, (std::string("Acquisition ") + VERSION_NAME).c_str());
    nm_.post(request, data);
}

void AutoOnline::Check() {
    bool running = is_poe_running_locally();
    if(IsRemoteScriptSet()){
        running = is_poe_running_remotely(process_script_);
    } else {
        running = is_poe_running_locally();
    }

    if (running || previous_status_) {
        SendOnlineUpdate(running);
    }

    previous_status_ = running;

    emit Update(running);
}

const std::vector<std::string> url_valid_prefixes = { "http://control.poe.xyz.is/", "http://control.poe.trade/" };

void AutoOnline::SetUrl(const std::string &url) {
    bool prefix_is_valid = false;
    for (auto &valid_prefix : url_valid_prefixes) {
        prefix_is_valid |= url.compare(0, valid_prefix.size(), valid_prefix) == 0;
    }
    if (!prefix_is_valid) {
        QLOG_WARN() << "Online URL is probably invalid.";
    }
    url_ = url;
    // remove trailing '/'s
    while (url_.size() > 0 && url_[url_.size() - 1] == '/')
        url_.erase(url_.size() - 1);
    sensitive_data_.Set("online_url", url_);
}

void AutoOnline::SetRemoteScript(const std::string& path) {
    process_script_ = path;
    data_.Set("process_script", path);
}

void AutoOnline::SetEnabled(bool enabled) {
    enabled_ = enabled;
    if (enabled)
        timer_.start();
    else
        timer_.stop();
    data_.SetBool("online_enabled", enabled_);
}
