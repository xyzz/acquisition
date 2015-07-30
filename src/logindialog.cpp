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
#include <QDateTime>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkProxyFactory>
#include <QRegExp>
#include <QVector>
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

#ifdef _GARENA
const char* POE_LEAGUE_LIST_URL = "http://api.pathofexile.com/leagues";
const char* POE_LOGIN_URL = "https://web.poe.garena.ru/login";
const char* POE_MAIN_PAGE = "https://web.poe.garena.ru/";
const char* POE_MY_ACCOUNT = "https://web.poe.garena.ru/my-account";
#else
const char* POE_LEAGUE_LIST_URL = "http://api.pathofexile.com/leagues";
const char* POE_LOGIN_URL = "https://www.pathofexile.com/login";
const char* POE_MAIN_PAGE = "https://www.pathofexile.com/";
const char* POE_MY_ACCOUNT = "https://www.pathofexile.com/my-account";
#endif
const char* POE_COOKIE_NAME = "PHPSESSID";

const char* POE_GARENA_REDIRECT_HEADER = "Location";
const char* POE_GARENA_PRELOGIN = "https://auth.garena.com/api/prelogin";
const char* POE_GARENA_LOGIN = "https://auth.garena.com/api/login";
const char* POE_GARENA_TOKEN = "https://auth.garena.com/oauth/token/grant";
const char* POE_GARENA_APPID = "200003";
const char* GARENA_MAIN_PAGE = "https://auth.garena.com";
const char* GARENA_COOKIE_NAME = "sso_key";
const char* POE_GARENA_OAUTH_PAGE = "https://web.poe.garena.ru/login/garena-oauth";

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

    settings_path_ = Filesystem::UserDir() + "/settings.ini";
    LoadSettings();

    login_manager_ = std::make_unique<QNetworkAccessManager>();
    //QNetworkReply *leagues_reply = login_manager_->get(QNetworkRequest(QUrl(QString(POE_LEAGUE_LIST_URL))));
    //connect(leagues_reply, SIGNAL(finished()), this, SLOT(OnLeaguesRequestFinished()));
	OnLeaguesRequestFinished();
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

void LoginDialog::OnLeaguesRequestFinished() {
    //QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    //QByteArray bytes = reply->readAll();
    rapidjson::Document doc;
    //doc.Parse(bytes.constData());

    leagues_.clear();
    // ignore actual response completely since it's broken anyway (at the moment of writing!)
    if (true) {
        //QLOG_ERROR() << "Failed to parse leagues. The output was:";
        //QLOG_ERROR() << QString(bytes);

        // But let's do our best and try to add at least some leagues!
        // It's in case GGG's API is broken and suddenly starts returning empty pages,
        // which of course will never happen.
#ifdef _GARENA
        leagues_ = { "Отряды", "Буря", "Стандарт", "Одна жизнь" };
#else
		leagues_ = { "Warbands", "Tempest", "Standard", "Hardcore" };
#endif
    } else {
        for (auto &league : doc)
            leagues_.push_back(QString::fromStdString(league["id"].GetString()));
    }
    ui->leagueComboBox->clear();
    for (auto &league : leagues_)
        ui->leagueComboBox->addItem(league);
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
#ifdef _GARENA
			if (reply->hasRawHeader(POE_GARENA_REDIRECT_HEADER))
			{
				qint64 date = QDateTime().toMSecsSinceEpoch();
				garena_page_ = reply->rawHeader(POE_GARENA_REDIRECT_HEADER);

				QUrlQuery query;
				query.addQueryItem("account", EncodeSpecialCharacters(ui->emailLineEdit->text()));
				query.addQueryItem("format", "json");
				query.addQueryItem("app_id", POE_GARENA_APPID);
				query.addQueryItem("id", QString::number(date).toUtf8());

				QUrl url(POE_GARENA_PRELOGIN);
				url.setQuery(query);
				QNetworkRequest request(url);

				QNetworkReply *aouth = login_manager_->get(request);
				connect(aouth, SIGNAL(finished()), this, SLOT(OnGarenaOauthPreLogin()));
			}
#else
			QByteArray bytes = reply->readAll();
			QString hash = Util::GetCsrfToken(bytes.constData(), "hash");
			if (hash.isEmpty()) {
				DisplayError("Failed to log in (can't extract form hash from page)");
				return;
			}

			QUrlQuery query;
			query.addQueryItem("login_email", EncodeSpecialCharacters(ui->emailLineEdit->text()));
			query.addQueryItem("login_password", EncodeSpecialCharacters(ui->passwordLineEdit->text()));
			query.addQueryItem("hash", hash);
			query.addQueryItem("login", "Login");

			QUrl url(POE_LOGIN_URL);
			QByteArray data(query.query().toUtf8());
			QNetworkRequest request(url);
			request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
			QNetworkReply *logged_in = login_manager_->post(request, data);
			connect(logged_in, SIGNAL(finished()), this, SLOT(OnLoggedIn()));
#endif
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

void LoginDialog::OnGarenaOauthPreLogin() {
	rapidjson::Document doc;
	qint64 date = QDateTime().toMSecsSinceEpoch();

	QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
	QByteArray bytes = reply->readAll();

	if (reply->error()) {
		DisplayError("Network error: " + reply->errorString());
		return;
	}

	doc.Parse(bytes.constData());
	if (doc.HasMember("error"))
	{
		QString error = doc["error"].GetString();
		DisplayError("Network error: " + error);
		return;
	}

	QString v1 = doc["v1"].GetString();
	QString account = doc["account"].GetString();
	QString v2 = doc["v2"].GetString();
	QString id = doc["id"].GetString();

	QString password = EncodeSpecialCharacters(ui->passwordLineEdit->text());
	QString tmp;
	QByteArray passwdhash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5);
	tmp = QString(passwdhash.toHex());
	tmp.append(v1);
	QByteArray hash1 = QCryptographicHash::hash(tmp.toUtf8(), QCryptographicHash::Sha256);
	tmp = QString(hash1.toHex());
	tmp.append(v2);
	QByteArray hash2 = QCryptographicHash::hash(tmp.toUtf8(), QCryptographicHash::Sha256);
	QString full_hash = QString(hash2.toHex());

	QUrlQuery query;
	query.addQueryItem("account", account);
	query.addQueryItem("password", full_hash);
	query.addQueryItem("format", "json");
	query.addQueryItem("app_id", POE_GARENA_APPID);
	query.addQueryItem("id", QString::number(date).toUtf8());

	QUrl url(POE_GARENA_LOGIN);
	url.setQuery(query);
	QNetworkRequest request(url);

	QNetworkReply *aouth = login_manager_->get(request);
	connect(aouth, SIGNAL(finished()), this, SLOT(OnGarenaOauthLogin()));
}

void LoginDialog::OnGarenaOauthLogin() {
	qint64 date = QDateTime().toMSecsSinceEpoch();
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
	QByteArray bytes = reply->readAll();

	if (reply->error()) {
		DisplayError("Network error: " + reply->errorString());
		return;
	}
	rapidjson::Document doc;
	doc.Parse(bytes.constData());
	if (doc.HasMember("error"))
	{
		QString error = doc["error"].GetString();
		DisplayError("Network error: " + error);
		return;
	}

	QList<QNetworkCookie> cookies = reply->manager()->cookieJar()->cookiesForUrl(QUrl(GARENA_MAIN_PAGE));
	for (auto &cookie : cookies)
		if (QString(cookie.name()) == GARENA_COOKIE_NAME)
			garena_id_ = cookie.value();

	QUrlQuery q(garena_page_);
	QUrlQuery query;
	query.addQueryItem("client_id", POE_GARENA_APPID);
	query.addQueryItem("redirect_uri", POE_GARENA_OAUTH_PAGE);
	query.addQueryItem("state", q.queryItemValue("state"));
	query.addQueryItem("scope", "");
	query.addQueryItem("response_type", "code");
	query.addQueryItem("locale", "ru-RU");
	query.addQueryItem("format", "json");
	query.addQueryItem("app_id", POE_GARENA_APPID);
	query.addQueryItem("id", QString::number(date).toUtf8());

	QUrl url(POE_GARENA_TOKEN);
	QByteArray data(query.query().toUtf8());
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	QNetworkReply *logged_in = login_manager_->post(request, data);
	connect(logged_in, SIGNAL(finished()), this, SLOT(OnGarenaToken()));
}

void LoginDialog::OnGarenaToken()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
	QByteArray bytes = reply->readAll();

	if (reply->error()) {
		DisplayError("Network error: " + reply->errorString());
		return;
	}
	rapidjson::Document doc;
	doc.Parse(bytes.constData());
	if (doc.HasMember("error"))
	{
		QString error = doc["error"].GetString();
		DisplayError("Network error: " + error);
		return;
	}
	QString url = doc["redirect_uri"].GetString();
	QUrl q(url);
	QNetworkRequest request(q);
	QNetworkReply *aouth = login_manager_->get(request);
	connect(aouth, SIGNAL(finished()), this, SLOT(OnGarenaLogin()));
}

void LoginDialog::OnGarenaLogin()
{
	QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
	QByteArray bytes = reply->readAll();

	if (reply->error()) {
		DisplayError("Network error: " + reply->errorString());
		return;
	}
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
#ifdef _GARENA
    poeCookie.setDomain("web.poe.garena.ru");
#else
	poeCookie.setDomain("www.pathofexile.com");
#endif
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
    QLOG_INFO() << "Logged in as:" << account;

    QString league(ui->leagueComboBox->currentText());
    app_->InitLogin(std::move(login_manager_), league, account);
    mw = new MainWindow(std::move(app_));
    mw->setWindowTitle(QString("Acquisition - %1").arg(league));
    mw->show();
    close();
}

void LoginDialog::OnProxyCheckBoxClicked(bool checked) {
  QNetworkProxyFactory::setUseSystemConfiguration(checked);
}

void LoginDialog::LoadSettings() {
    QSettings settings(settings_path_, QSettings::IniFormat);
    session_id_ = settings.value("session_id", "").toString();
    ui->sessionIDLineEdit->setText(session_id_);
    ui->rembmeCheckBox->setChecked(settings.value("remember_me_checked").toBool());
    ui->proxyCheckBox->setChecked(settings.value("use_system_proxy_checked").toBool());

    if (ui->rembmeCheckBox->isChecked())
        ui->loginTabs->setCurrentIndex(LOGIN_SESSIONID);

    saved_league_ = settings.value("league", "").toString();
    if (saved_league_.size() > 0) {
        ui->leagueComboBox->addItem(saved_league_);
        ui->leagueComboBox->setEnabled(true);
    }

    QNetworkProxyFactory::setUseSystemConfiguration(ui->proxyCheckBox->isChecked());
}

void LoginDialog::SaveSettings() {
    QSettings settings(settings_path_, QSettings::IniFormat);
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
