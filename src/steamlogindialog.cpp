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

        for(auto cookie: steam_cookies) {
            if(cookie.name().startsWith("steamMachineAuth")) {
                if(!QFile::exists("cookies.txt")) {
                    SaveSteamCookie(cookie);
                } else if(QFile::exists("cookies.txt") && cookie != LoadSteamCookie()) {
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

    QFile cookie_file("cookies.txt");

    if(cookie_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
         cookie_file.write(cookie.name());
         cookie_file.write("\n");
         cookie_file.write(cookie.value());
    } else {
        qWarning() << "Failed to open file";
    }

}

QNetworkCookie SteamLoginDialog::LoadSteamCookie() {
    // Function to load the cookie from a file located in the acquisition
    // directory.

    QByteArray cookie_name("");
    QByteArray cookie_value("");
    QFile cookie_file("cookies.txt");

    if(cookie_file.open(QIODevice::ReadOnly)) {
        QTextStream read(&cookie_file);
        while(!read.atEnd()) {
            QString line = read.readLine();
            if(line.startsWith("steamMachineAuth"))
                cookie_name.append(line);
            else
                cookie_value.append(line);
        }
        cookie_file.close();
    } else {
        // Failed to open the file. Probably doesn't exist.
        qWarning() << "Failed to load cookies file";
    }

    QNetworkCookie cookie(cookie_name, cookie_value);
    cookie.setPath("/");
    cookie.setDomain("steamcommunity.com");

    return cookie;
}
