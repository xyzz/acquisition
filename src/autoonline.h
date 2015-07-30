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

class DataManager;

class AutoOnline : public QObject {
    Q_OBJECT
public:
    AutoOnline(DataManager &data, DataManager &sensitive_data);
    void SetUrl(const QString &url);
    void SetEnabled(bool enabled);
    bool enabled() { return enabled_; }
    bool IsUrlSet() { return !url_.isEmpty(); }
public slots:
    void Check();
signals:
    void Update(bool running);
private:
    DataManager &data_;
    DataManager &sensitive_data_;
    bool enabled_;
    QString url_;
    bool previous_status_;
    QTimer timer_;
    QNetworkAccessManager nm_;
};