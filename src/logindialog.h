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
#include <memory>
#include <string>
#include <vector>

#include "updatechecker.h"

class QNetworkAccessManager;
class QNetworkReply;
class QString;

class Application;
class SteamLoginDialog;
class MainWindow;

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(std::unique_ptr<Application> app);
    ~LoginDialog();
public slots:
    void OnLoginButtonClicked();
    void OnLoginPageFinished();
    void OnLoggedIn();
    void OnMainPageFinished();
    void OnProxyCheckBoxClicked(bool);
    void OnSteamCookieReceived(const QString &cookie);
    void OnSteamDialogClosed();
protected:
    bool event(QEvent *e);
private:
    void SaveSettings();
    void LoadSettings();
    void DisplayError(const QString &error);
    void LoginWithCookie(const QString &cookie);
    void InitSteamDialog();
    std::unique_ptr<Application> app_;
    Ui::LoginDialog *ui;
    MainWindow *mw;
    std::string settings_path_;
    QString saved_league_;
    QString session_id_;
    std::unique_ptr<QNetworkAccessManager> login_manager_;
    std::vector<std::string> leagues_;
    std::unique_ptr<SteamLoginDialog> steam_login_dialog_;
    UpdateChecker update_checker_;
    bool asked_to_update_;
};
