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

#include "steamlogindialog.h"
#include "ui_steamlogindialog.h"

#include <QCloseEvent>
#include "QsLog.h"
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFile>
#include <QSettings>

#include "filesystem.h"

SteamLoginDialog::SteamLoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SteamLoginDialog)
{
    ui->setupUi(this);
}

SteamLoginDialog::~SteamLoginDialog() {
    delete ui;
}

void SteamLoginDialog::closeEvent(QCloseEvent *e) {
    if (!completed_)
        emit Closed();
    QDialog::closeEvent(e);
}

void SteamLoginDialog::Init() {
    completed_ = false;
    disconnect(this, SLOT(OnLoadFinished()));
    // clear all previous cookies
    ui->webView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
    ui->webView->setContent("Loading, please wait...");

    QByteArray data("x=0&y=0");
    QNetworkRequest request(QUrl("https://www.pathofexile.com/login/steam"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    ui->webView->load(request, QNetworkAccessManager::PostOperation, data);

    settings_path_ = Filesystem::UserDir() + "/settings.ini";
    SetSteamCookie(LoadSteamCookie());

    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished()));
}

void SteamLoginDialog::OnLoadFinished() {
    if (completed_)
        return;
    QString url = ui->webView->url().toString();
    if (url.startsWith("https://www.pathofexile.com/")) {
        QNetworkCookieJar *cookie_jar = ui->webView->page()->networkAccessManager()->cookieJar();
        QList<QNetworkCookie> cookies = cookie_jar->cookiesForUrl(QUrl("https://www.pathofexile.com/"));
        QString session_id;

        QNetworkCookieJar *steam_jar = ui->webView->page()->networkAccessManager()->cookieJar();
        QList<QNetworkCookie> steam_cookies = steam_jar->cookiesForUrl(QUrl("https://steamcommunity.com/openid/"));

        QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
        bool rembme = settings.value("remember_me_checked").toBool();

        for(auto cookie: steam_cookies) {
            if(cookie.name().startsWith("steamMachineAuth")) {
                if(rembme && !settings.contains("steamMachineAuth")) {
                    SaveSteamCookie(cookie);
                } else if(rembme && cookie != LoadSteamCookie()) {
                    SaveSteamCookie(cookie);
                }
            }
        }

        for (auto &cookie : cookies)
            if (cookie.name() == "PHPSESSID")
                session_id = QString(cookie.value());
        ui->webView->stop();

        completed_ = true;
        emit CookieReceived(session_id);

        close();
    }
}

void SteamLoginDialog::SetSteamCookie(QNetworkCookie steam_cookie) {
    // Sets a cookie on the steamcommunity.com url so that users don't have to
    // do the email verification every time that they sign in on with steam.

    QList<QNetworkCookie> cookie;
    cookie << steam_cookie;

    QNetworkCookieJar *steam_community = ui->webView->page()->networkAccessManager()->cookieJar();
    steam_community->setCookiesFromUrl(cookie, QUrl("https://steamcommunity.com/openid/"));
}

void SteamLoginDialog::SaveSteamCookie(QNetworkCookie cookie) {
    // Saves the file to a file located in the acquisition directory.

    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);
    settings.setValue("steam-id", cookie.name().remove(0, 16));
    settings.setValue(cookie.name(), cookie.value());
}

QNetworkCookie SteamLoginDialog::LoadSteamCookie() {
    // Function to load the cookie from a file located in the acquisition
    // directory.

    QString cookie_name = "steamMachineAuth";
    QString cookie_value;
    QSettings settings(settings_path_.c_str(), QSettings::IniFormat);

    cookie_name.append(settings.value("steam-id").toString());
    cookie_value = settings.value(cookie_name).toString();

    QNetworkCookie cookie(cookie_name.toUtf8(), cookie_value.toUtf8());
    cookie.setPath("/");
    cookie.setDomain("steamcommunity.com");

    return cookie;
}
