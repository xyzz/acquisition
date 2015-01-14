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

#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

class QWidget;

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    UpdateChecker();
    static void AskUserToUpdate(QWidget *parent);
public slots:
    void CheckForUpdates();
    void OnUpdateCheckCompleted();
signals:
    void UpdateAvailable();
private:
    QNetworkAccessManager nm_;
    QTimer timer_;
};