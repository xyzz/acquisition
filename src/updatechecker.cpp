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

#include "updatechecker.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "version.h"

// Check for updates every hour
const int kUpdateCheckInterval = 60 * 60 * 1000;

UpdateChecker::UpdateChecker() {
    timer_.setInterval(kUpdateCheckInterval);
    timer_.start();
    connect(&timer_, &QTimer::timeout, this, &UpdateChecker::CheckForUpdates);
    CheckForUpdates();
}

void UpdateChecker::CheckForUpdates() {
    QNetworkReply *version_check = nm_.get(QNetworkRequest(QUrl(UPDATE_CHECK_URL)));
    connect(version_check, &QNetworkReply::finished, this, &UpdateChecker::OnUpdateCheckCompleted);
}

void UpdateChecker::OnUpdateCheckCompleted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    int available_version = QString(bytes).toInt();
    if (available_version > VERSION_CODE) {
        emit UpdateAvailable();
    }
}

void UpdateChecker::AskUserToUpdate(QWidget *parent) {
    QMessageBox::StandardButton result = QMessageBox::information(parent, "Update",
        "A newer version of Acquisition is available. "
        "Would you like to navigate to GitHub to download it?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);
    if (result == QMessageBox::Yes)
        QDesktopServices::openUrl(QUrl(UPDATE_DOWNLOAD_LOCATION));
}