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
#include <QRegExp>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>
#include "jsoncpp/json.h"
#include "QsLog.h"

#include "application.h"
#include "mainwindow.h"
#include "porting.h"
#include "util.h"
#include "version.h"

const char* POE_LEAGUE_LIST_URL = "http://api.pathofexile.com/leagues";
const char* POE_LOGIN_URL = "https://www.pathofexile.com/login";
const char* POE_MAIN_PAGE = "https://www.pathofexile.com/";

LoginDialog::LoginDialog(Application *app) :
    app_(app),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->errorLabel->hide();
    ui->errorLabel->setStyleSheet("QLabel { color : red; }");
    setWindowTitle(QString("Login [") + VERSION_NAME + "]");

    settings_path_ = porting::UserDir() + "/settings.ini";
    LoadSettings();

    login_manager_ = new QNetworkAccessManager;
    QNetworkReply *leagues_reply = login_manager_->get(QNetworkRequest(QUrl(QString(POE_LEAGUE_LIST_URL))));
    connect(leagues_reply, SIGNAL(finished()), this, SLOT(OnLeaguesRequestFinished()));

    QNetworkReply *version_check = login_manager_->get(QNetworkRequest(QUrl(UPDATE_CHECK_URL)));
    connect(version_check, SIGNAL(finished()), this, SLOT(OnUpdateCheckCompleted()));

    connect(ui->loginButton, SIGNAL(clicked()), this, SLOT(OnLoginButtonClicked()));
}

void LoginDialog::OnUpdateCheckCompleted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    int available_version = QString(bytes).toInt();
    if (available_version > VERSION_CODE) {
        QMessageBox::StandardButton result = QMessageBox::information(this, "Update",
            "A newer version of Acquisition is available. "
            "Would you like to navigate to GitHub to download it?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes);
        if (result == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(UPDATE_DOWNLOAD_LOCATION));
        }
    }
}

void LoginDialog::OnLeaguesRequestFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string json(bytes.constData(), bytes.size());
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json, root)) {
        QLOG_ERROR() << "Failed to parse leagues. The output was:";
        QLOG_ERROR() << QString(bytes);
        return;
    }
    ui->leagueComboBox->clear();
    for (auto league : root) {
        std::string name = league["id"].asString();
        leagues_.push_back(name);
        ui->leagueComboBox->addItem(name.c_str());
    }
    ui->leagueComboBox->setEnabled(true);

    if (saved_league_.size() > 0)
        ui->leagueComboBox->setCurrentText(saved_league_);
}

void LoginDialog::OnLoginButtonClicked() {
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("Logging in...");
    QNetworkReply *login_page = login_manager_->get(QNetworkRequest(QUrl(POE_LOGIN_URL)));
    connect(login_page, SIGNAL(finished()), this, SLOT(OnLoginPageFinished()));
}

void LoginDialog::OnLoginPageFinished() {
    if(ui->sessIDCheckBox->isChecked()) {
        QNetworkCookie poeCookie("PHPSESSID", ui->sessionIDLineEdit->text().toUtf8());
        poeCookie.setPath("/");
        poeCookie.setDomain("www.pathofexile.com");

        login_manager_->cookieJar()->insertCookie(poeCookie);

        QNetworkReply *login_page = login_manager_->get(QNetworkRequest(QUrl(POE_LOGIN_URL)));
        connect(login_page, SIGNAL(finished()), this, SLOT(OnLoggedIn()));

        return;
    }
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "hash");
    if (hash.empty()) {
        DisplayError("Failed to log in (can't extract form hash from page)");
        return;
    }

    QUrlQuery query;
    query.addQueryItem("login_email", ui->emailLineEdit->text());
    query.addQueryItem("login_password", ui->passwordLineEdit->text());
    query.addQueryItem("hash", QString(hash.c_str()));
    query.addQueryItem("login", "Login");

    QUrl url(POE_LOGIN_URL);
    QByteArray data(query.query().toUtf8());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *logged_in = login_manager_->post(request, data);
    connect(logged_in, SIGNAL(finished()), this, SLOT(OnLoggedIn()));
}

void LoginDialog::OnLoggedIn() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 302) {
        DisplayError("Failed to log in (invalid password?)");
        return;
    }

    // we need one more request to get account name
    QNetworkReply *main_page = login_manager_->get(QNetworkRequest(QUrl(POE_MAIN_PAGE)));
    connect(main_page, SIGNAL(finished()), this, SLOT(OnMainPageFinished()));
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
    QLOG_INFO() << "Logged in as:" << account;

    std::string league(ui->leagueComboBox->currentText().toStdString());
    app_->InitLogin(login_manager_, league, account.toStdString());
    MainWindow *w = new MainWindow(app_);
    w->show();
    close();
}

void LoginDialog::LoadSettings() {
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    ui->emailLineEdit->setText(settings.value("email", "").toString());
    ui->sessionIDLineEdit->setText(settings.value("session_id", "").toString());
    ui->sessIDCheckBox->setChecked(settings.value("session_id_checked").toBool());
    ui->rembmeCheckBox->setChecked(settings.value("remember_me_checked").toBool());
    ui->sessionIDLineEdit->setVisible(ui->sessIDCheckBox->isChecked());
    ui->sessIDLabel->setVisible(ui->sessIDCheckBox->isChecked());

    saved_league_ = settings.value("league", "").toString();
    if (saved_league_.size() > 0) {
        ui->leagueComboBox->addItem(saved_league_);
        ui->leagueComboBox->setEnabled(true);
    }
}

void LoginDialog::SaveSettings() {
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    if(ui->rembmeCheckBox->isChecked()) {
        settings.setValue("email", ui->emailLineEdit->text());
        settings.setValue("session_id", ui->sessionIDLineEdit->text());
        settings.setValue("league", ui->leagueComboBox->currentText());
    } else {
        settings.setValue("email", "");
        settings.setValue("session_id", "");
        settings.setValue("league", "");
    }
    settings.setValue("session_id_checked", ui->sessIDCheckBox->isChecked());
    settings.setValue("remember_me_checked", ui->rembmeCheckBox->isChecked());
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
}
