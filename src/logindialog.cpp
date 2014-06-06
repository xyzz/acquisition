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
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <iostream>
#include "jsoncpp/json.h"
#include "QsLog.h"

#include "mainwindow.h"
#include "version.h"

const char* POE_LEAGUE_LIST_URL = "http://api.pathofexile.com/leagues";
const char* POE_LOGIN_URL = "https://www.pathofexile.com/login";

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setWindowTitle(QString("Login [") + VERSION_NAME + "]");

    settingsFile_ = "./settings.ini";
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
}

void LoginDialog::OnLoginButtonClicked() {
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("Logging in...");
    QNetworkReply *login_page = login_manager_->get(QNetworkRequest(QUrl(POE_LOGIN_URL)));
    connect(login_page, SIGNAL(finished()), this, SLOT(OnLoginPageFinished()));
}

void LoginDialog::OnLoginPageFinished() {
    if(ui->sessIDcheckBox->isChecked()) {
        QNetworkCookie poeCookie("PHPSESSID", ui->emailLineEdit->text().toUtf8());
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
    std::string needle = "name=\"hash\" value=\"";
    std::string hash = page.substr(page.find(needle) + needle.size(), 32);

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
    std::string league(ui->leagueComboBox->currentText().toUtf8().constData());
    std::string email(ui->emailLineEdit->text().toUtf8().constData());
    MainWindow *w = new MainWindow(0, login_manager_, league, email);
    w->show();
    close();
}

void LoginDialog::LoadSettings() {
    QSettings settings(settingsFile_, QSettings::NativeFormat);
    ui->emailLineEdit->setText(settings.value("email", "").toString());
    ui->sessIDcheckBox->setChecked(settings.value("sessIDBox").toBool());
    ui->remEmailcheckBox->setChecked(settings.value("emailBox").toBool());
}

void LoginDialog::SaveSettings() {
    QSettings settings(settingsFile_, QSettings::NativeFormat);
    if(ui->remEmailcheckBox->isChecked())
        settings.setValue("email", ui->emailLineEdit->text());
    else
        settings.setValue("email", "");
    settings.setValue("sessIDBox", ui->sessIDcheckBox->isChecked());
    settings.setValue("emailBox", ui->remEmailcheckBox->isChecked());
}

LoginDialog::~LoginDialog() {
    SaveSettings();
    delete ui;
}
