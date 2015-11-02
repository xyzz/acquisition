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
#include "ui_logindialog.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkProxyFactory>
#include <QRegExp>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>
#include "QsLog.h"

#include "application.h"
#include "filesystem.h"
#include "mainwindow.h"
#include "steamlogindialog.h"
#include "util.h"
#include "version.h"

const char* POE_LEAGUE_LIST_URL = "http://api.pathofexile.com/leagues";
const char* POE_LOGIN_URL = "https://www.pathofexile.com/login";
const char* POE_MAIN_PAGE = "https://www.pathofexile.com/";
const char* POE_MY_ACCOUNT = "https://www.pathofexile.com/my-account";
const char* POE_COOKIE_NAME = "PHPSESSID";

enum {
    LOGIN_PASSWORD,
    LOGIN_STEAM,
    LOGIN_SESSIONID
};

LoginDialog::LoginDialog(std::unique_ptr<Application> app) :
    app_(std::move(app)),
    ui(new Ui::LoginDialog),
    mw(0),
    asked_to_update_(false)
{
    ui->setupUi(this);
    ui->errorLabel->hide();
    ui->errorLabel->setStyleSheet("QLabel { color : red; }");
    setWindowTitle(QString("Login [") + VERSION_NAME + "]");
#if defined(Q_OS_LINUX)
    setWindowIcon(QIcon(":/icons/assets/icon.svg"));
#endif
    QStringList leagues = { "Darkshrine (IC003)", "Darkshrine HC (IC004)", "Flashback Event (IC001)", "Flashback Event HC (IC002)", "Standard", "Hardcore" };
    ui->leagueComboBox->clear();
    ui->leagueComboBox->addItems(leagues);
    ui->leagueComboBox->setEnabled(true);

    settings_path_ = Filesystem::UserDir() + "/settings.ini";
    LoadSettings();

    login_manager_ = std::make_unique<QNetworkAccessManager>();
    connect(ui->proxyCheckBox, SIGNAL(clicked(bool)), this, SLOT(OnProxyCheckBoxClicked(bool)));
    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(OnLoginButtonClicked()));
    connect(&update_checker_, &UpdateChecker::UpdateAvailable, [&](){
        // Only annoy the user once at the login dialog window, even if it's opened for more than an hour
        if (asked_to_update_)
            return;
        asked_to_update_ = true;
        UpdateChecker::AskUserToUpdate(this);
    });
}

void LoginDialog::OnLoginButtonClicked() {
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("Logging in...");
    QNetworkReply *login_page = login_manager_->get(QNetworkRequest(QUrl(POE_LOGIN_URL)));
    connect(login_page, SIGNAL(finished()), this, SLOT(OnLoginPageFinished()));
}

void LoginDialog::InitSteamDialog() {
    steam_login_dialog_ = std::make_unique<SteamLoginDialog>();
    connect(steam_login_dialog_.get(), SIGNAL(CookieReceived(const QString&)), this, SLOT(OnSteamCookieReceived(const QString&)));
    connect(steam_login_dialog_.get(), SIGNAL(Closed()), this, SLOT(OnSteamDialogClosed()), Qt::DirectConnection);
}

void LoginDialog::OnSteamDialogClosed() {
    DisplayError("Login was not completed");
}

// All characters except + should be handled by QUrlQuery, see http://doc.qt.io/qt-5/qurlquery.html#encoding
static QString EncodeSpecialCharacters(QString s) {
    s.replace("+", "%2b");
    return s;
}

void LoginDialog::OnLoginPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    if (reply->error()) {
        DisplayError("Network error: " + reply->errorString());
        return;
    }
    switch (ui->loginTabs->currentIndex()) {
        case LOGIN_PASSWORD: {
            QByteArray bytes = reply->readAll();
            std::string page(bytes.constData(), bytes.size());
            std::string hash = Util::GetCsrfToken(page, "hash");
            if (hash.empty()) {
                DisplayError("Failed to log in (can't extract form hash from page)");
                return;
            }

            QUrlQuery query;
            query.addQueryItem("login_email", EncodeSpecialCharacters(ui->emailLineEdit->text()));
            query.addQueryItem("login_password", EncodeSpecialCharacters(ui->passwordLineEdit->text()));
            query.addQueryItem("hash", QString(hash.c_str()));
            query.addQueryItem("login", "Login");

            QUrl url(POE_LOGIN_URL);
            QByteArray data(query.query().toUtf8());
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            QNetworkReply *logged_in = login_manager_->post(request, data);
            connect(logged_in, SIGNAL(finished()), this, SLOT(OnLoggedIn()));
            break;
        }
        case LOGIN_STEAM: {
            if (!steam_login_dialog_)
                InitSteamDialog();
            steam_login_dialog_->show();
            steam_login_dialog_->Init();
            break;
        }
        case LOGIN_SESSIONID: {
            LoginWithCookie(ui->sessionIDLineEdit->text());
            break;
        }
    }
}

void LoginDialog::OnLoggedIn() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 302) {
        DisplayError("Failed to log in (invalid password?)");
        return;
    }

    QList<QNetworkCookie> cookies = reply->manager()->cookieJar()->cookiesForUrl(QUrl(POE_MAIN_PAGE));
    for (auto &cookie : cookies)
        if (QString(cookie.name()) == POE_COOKIE_NAME)
            session_id_ = cookie.value();

    // we need one more request to get account name
    QNetworkReply *main_page = login_manager_->get(QNetworkRequest(QUrl(POE_MY_ACCOUNT)));
    connect(main_page, SIGNAL(finished()), this, SLOT(OnMainPageFinished()));
}

void LoginDialog::OnSteamCookieReceived(const QString &cookie) {
    if (cookie.isEmpty()) {
        DisplayError("Failed to log in, the received cookie is empty.");
        return;
    }
    LoginWithCookie(cookie);
}

void LoginDialog::LoginWithCookie(const QString &cookie) {
    QNetworkCookie poeCookie(POE_COOKIE_NAME, cookie.toUtf8());
    poeCookie.setPath("/");
    poeCookie.setDomain("www.pathofexile.com");

    login_manager_->cookieJar()->insertCookie(poeCookie);

    QNetworkReply *login_page = login_manager_->get(QNetworkRequest(QUrl(POE_LOGIN_URL)));
    connect(login_page, SIGNAL(finished()), this, SLOT(OnLoggedIn()));
}

void LoginDialog::OnMainPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QString html(reply->readAll());
    QRegExp regexp("/account/view-profile/(.*)\"");
    regexp.setMinimal(true);
    int pos = regexp.indexIn(html);
    if (pos == -1) {
        DisplayError("Failed to find account name.");
        return;
    }
    QString account = regexp.cap(1);
    QLOG_DEBUG() << "Logged in as:" << account;

    std::string league(ui->leagueComboBox->currentText().toStdString());
    app_->InitLogin(std::move(login_manager_), league, account.toStdString());
    mw = new MainWindow(std::move(app_));
    mw->setWindowTitle(QString("Acquisition - %1").arg(league.c_str()));
    mw->show();
    close();
}

void LoginDialog::OnProxyCheckBoxClicked(bool checked) {
  QNetworkProxyFactory::setUseSystemConfiguration(checked);
}

void LoginDialog::LoadSettings() {
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    session_id_ = settings.value("session_id", "").toString();
    ui->sessionIDLineEdit->setText(session_id_);
    ui->rembmeCheckBox->setChecked(settings.value("remember_me_checked").toBool());
    ui->proxyCheckBox->setChecked(settings.value("use_system_proxy_checked").toBool());

    if (ui->rembmeCheckBox->isChecked())
        ui->loginTabs->setCurrentIndex(LOGIN_SESSIONID);

    saved_league_ = settings.value("league", "").toString();
    if (saved_league_.size() > 0)
        ui->leagueComboBox->setCurrentText(saved_league_);

    QNetworkProxyFactory::setUseSystemConfiguration(ui->proxyCheckBox->isChecked());
}

void LoginDialog::SaveSettings() {
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    if(ui->rembmeCheckBox->isChecked()) {
        settings.setValue("session_id", session_id_);
        settings.setValue("league", ui->leagueComboBox->currentText());
    } else {
        settings.setValue("session_id", "");
        settings.setValue("league", "");
    }
    settings.setValue("remember_me_checked", ui->rembmeCheckBox->isChecked() && !session_id_.isEmpty());
    settings.setValue("use_system_proxy_checked", ui->proxyCheckBox->isChecked());
}

void LoginDialog::DisplayError(const QString &error) {
    ui->errorLabel->setText(error);
    ui->errorLabel->show();
    ui->loginButton->setEnabled(true);
    ui->loginButton->setText("Login");
}

LoginDialog::~LoginDialog() {
    SaveSettings();
    delete ui;

    if (mw)
        delete mw;
}

bool LoginDialog::event(QEvent *e) {
    if (e->type() == QEvent::LayoutRequest)
        setFixedSize(sizeHint());
    return QDialog::event(e);
}
