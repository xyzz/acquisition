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
#include <memory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#ifndef NO_WEBENGINE
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#endif
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QFile>

#include "filesystem.h"

extern const char* POE_COOKIE_NAME;

SteamLoginDialog::SteamLoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SteamLoginDialog)
{
    ui->setupUi(this);
#ifndef NO_WEBENGINE
    webView = new QWebEngineView();
    ui->verticalLayout->addWidget(webView);

    connect(webView, &QWebEngineView::loadProgress, [this](int progress) {
        if (progress > 0 && progress < 100) {
            setWindowTitle(QString("Loading...  (%2%)").arg(progress));
        } else {
            setWindowTitle(webView->title());
        }
    });

    connect(QWebEngineProfile::defaultProfile()->cookieStore(),
            &QWebEngineCookieStore::cookieAdded, [this](const QNetworkCookie& cookie) {
        if (cookie.name() == POE_COOKIE_NAME) {
            QString session_id = QString(cookie.value());
            webView->stop();
            emit CookieReceived(session_id);
            completed_ = true;
            close();
        }
    });
#endif
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
#ifndef NO_WEBENGINE
    QByteArray data("x=0&y=0");
    QNetworkRequest request(QUrl("https://www.pathofexile.com/login/steam"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = network_manager_.post(request, data);

    auto conn = std::shared_ptr<QMetaObject::Connection>(new QMetaObject::Connection());
    *conn = connect(reply, &QNetworkReply::finished, [this, reply, conn] {
        auto attr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        webView->load(attr.toUrl());
        reply->deleteLater();
        disconnect(*conn);
    });
#endif
}
