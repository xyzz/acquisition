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

#include "QsLog.h"
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrlQuery>

SteamLoginDialog::SteamLoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SteamLoginDialog)
{
    ui->setupUi(this);
}

SteamLoginDialog::~SteamLoginDialog() {
    delete ui;
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
    connect(ui->webView, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished()));
}

void SteamLoginDialog::OnLoadFinished() {
    if (completed_)
        return;
    QString url = ui->webView->url().toString();
    if (url.startsWith("http://www.pathofexile.com/")) {
        QNetworkCookieJar *cookie_jar = ui->webView->page()->networkAccessManager()->cookieJar();
        QList<QNetworkCookie> cookies = cookie_jar->cookiesForUrl(QUrl("http://www.pathofexile.com/"));
        QString session_id;
        for (auto &cookie : cookies)
            if (cookie.name() == "PHPSESSID")
                session_id = QString(cookie.value());
        ui->webView->stop();

        completed_ = true;
        emit CookieReceived(session_id);

        close();
    }
}
