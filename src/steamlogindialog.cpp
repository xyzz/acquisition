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

#if defined(USE_WEBENGINE)
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <memory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#elif defined(USE_WEBKIT)
#include <QSettings>
#include <QWebView>
#endif

#include "filesystem.h"

extern const char* POE_COOKIE_NAME;

SteamLoginDialog::SteamLoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SteamLoginDialog)
{
    ui->setupUi(this);

#if defined(USE_WEBENGINE)
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
#elif defined(USE_WEBKIT)
    webkitView = new QWebView();
    ui->verticalLayout->addWidget(webkitView);
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

#ifdef USE_WEBKIT
    disconnect(this, SLOT(OnLoadFinished()));
    // clear all previous cookies
    webkitView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
    webkitView->setContent("Loading, please wait...");
#endif

#if defined(USE_WEBKIT) || defined(USE_WEBENGINE)
    QByteArray data("x=0&y=0");
    QNetworkRequest request(QUrl("https://www.pathofexile.com/login/steam"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
#endif

#ifdef USE_WEBENGINE
    QNetworkReply *reply = network_manager_.post(request, data);

    auto conn = std::shared_ptr<QMetaObject::Connection>(new QMetaObject::Connection());
    *conn = connect(reply, &QNetworkReply::finished, [this, reply, conn] {
        auto attr = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        webView->load(attr.toUrl());
        reply->deleteLater();
        disconnect(*conn);
    });
#endif

#ifdef USE_WEBKIT
    webkitView->load(request, QNetworkAccessManager::PostOperation, data);

    settings_path_ = Filesystem::UserDir() + "/settings.ini";
    SetSteamCookie(LoadSteamCookie());

    connect(webkitView, SIGNAL(loadFinished(bool)), this, SLOT(OnLoadFinished()));
#endif
}

#ifdef USE_WEBKIT
void SteamLoginDialog::OnLoadFinished() {
    if (completed_)
        return;
    QString url = webkitView->url().toString();
    if (url.startsWith("https://www.pathofexile.com/")) {
        QNetworkCookieJar *cookie_jar = webkitView->page()->networkAccessManager()->cookieJar();
        QList<QNetworkCookie> cookies = cookie_jar->cookiesForUrl(QUrl("https://www.pathofexile.com/"));
        QString session_id;

        QNetworkCookieJar *steam_jar = webkitView->page()->networkAccessManager()->cookieJar();
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
            if (cookie.name() == POE_COOKIE_NAME)
                session_id = QString(cookie.value());
        webkitView->stop();

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

    QNetworkCookieJar *steam_community = webkitView->page()->networkAccessManager()->cookieJar();
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
#endif
