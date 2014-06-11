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

#include <QDialog>
#include <string>
#include <vector>

class QNetworkAccessManager;
class QNetworkReply;

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = 0);
    ~LoginDialog();
public slots:
    void OnLeaguesRequestFinished();
    void OnLoginButtonClicked();
    void OnLoginPageFinished();
    void OnLoggedIn();
    void OnUpdateCheckCompleted();

private:
    void SaveSettings();
    void LoadSettings();
    Ui::LoginDialog *ui;
    QString settings_path_;
    QNetworkAccessManager *login_manager_;
    std::vector<std::string> leagues_;
};
