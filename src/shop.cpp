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

#include "shop.h"

#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include "QsLog.h"

#include "application.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "porting.h"
#include "util.h"
#include "mainwindow.h"

const QString POE_EDIT_THREAD = "https://www.pathofexile.com/forum/edit-thread/";
const QString POE_BUMP_THREAD = "https://www.pathofexile.com/forum/post-reply/";
const QString BUMP_MESSAGE = "[url=https://github.com/Novynn/acquisitionplus/releases]Bumped with Acquisition Plus![/url]";
const QString TEMPLATE_MESSAGE = "{everything|group}\n\n[url=https://github.com/Novynn/acquisitionplus/releases]Shop made with Acquisition Plus[/url]";
const int POE_BUMP_DELAY = 3600; // 1 hour per Path of Exile forum rules
const int POE_MAX_CHAR_IN_POST = 50000;

Shop::Shop(Application &app) :
    app_(app),
    templateManager(&app) {
    LoadShops();
}

void Shop::LoadShops() {
    QByteArray data = QByteArray::fromStdString(app_.data_manager().Get("shop_data"));

    qDeleteAll(shops_);
    shops_.clear();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull() && doc.isObject()) {
        auto_update_ = doc.object().value("auto_update").toBool();
        do_bump_ = doc.object().value("auto_bump").toBool();

        for (QJsonValue val : doc.object().value("shops").toArray()) {
            QJsonObject obj = val.toObject();
            QString id = obj.value("thread_id").toString();
            if (id.isEmpty()) continue;
            QString temp = obj.value("template").toString();
            AddShop(id, temp);
            ShopData* data = shops_.value(id);
            data->lastSubmitted = QDateTime::fromTime_t(obj.value("last_submitted").toInt());
            data->lastBumped = QDateTime::fromTime_t(obj.value("last_bumped").toInt());
            data->lastSubmissionHash = obj.value("last_hash").toString();
        }
        Update();
    }

    SaveShops();
}

void Shop::SaveShops() {
    Update();

    QJsonObject mainObject;
    mainObject.insert("auto_update", auto_update_);
    mainObject.insert("auto_bump", do_bump_);

    QJsonArray array;
    for (ShopData* data : shops_) {
        if (!data || data->threadId.isEmpty()) continue;
        QJsonObject obj;
        obj.insert("thread_id", data->threadId);
        obj.insert("template", data->shopTemplate);
        obj.insert("last_submitted", (int)data->lastSubmitted.toTime_t());
        obj.insert("last_bumped", (int)data->lastBumped.toTime_t());
        obj.insert("last_hash", data->lastSubmissionHash);
        array.append(obj);
    }
    mainObject.insert("shops", array);

    QJsonDocument doc;
    doc.setObject(mainObject);
    QByteArray data = doc.toJson();
    app_.data_manager().Set("shop_data", data.toStdString());
}

void Shop::AddShop(const QString &threadId, QString temp) {
    if (shops_.contains(threadId)) {
        RemoveShop(threadId);
    }

    if (temp.isEmpty()) {
        temp = TEMPLATE_MESSAGE;
    }

    ShopData* data = new ShopData;
    data->threadId = threadId;
    data->shopTemplate = temp;
    data->requiresUpdate = true;
    data->submitting = false;

    shops_.insert(threadId, data);

    SaveShops();
}

void Shop::RemoveShop(const QString &threadId) {
    ShopData* data = shops_.take(threadId);
    if (data) {
        delete data;
    }

    SaveShops();
}

void Shop::CopyToClipboard(const QString &threadId) {
    ShopData* data = shops_.value(threadId);
    if (!data) return;

    if (data->requiresUpdate)
        Update(threadId);

    if (data->shopTemplate.isEmpty() || data->shopData.isEmpty())
        return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data->shopData);
}

void Shop::ExpireShopData() {
    for (QString thread : threadIds()) {
        ShopData* data = shops_.value(thread);
        data->requiresUpdate = true;
    }
}

void Shop::SetAutoUpdate(bool update) {
    auto_update_ = update;
    SaveShops();
}

void Shop::SetDoBump(bool bump) {
    do_bump_ = bump;
    SaveShops();
}

void Shop::SetShopTemplate(const QString &threadId, const QString &temp) {
    ShopData* data = shops_.value(threadId);
    if (!data) return;
    if (temp != data->shopTemplate) {
        data->shopTemplate = temp;
        data->requiresUpdate = true;
    }
    SaveShops();
}

QString Shop::GetShopTemplate(const QString &threadId) {
    ShopData* data = shops_.value(threadId);
    if (!data) return QString();
    return data->shopTemplate;
}

void Shop::Update(const QString &threadId, bool force) {
    QList<ShopData*> shopsToUpdate;
    if (threadId.isEmpty())
        shopsToUpdate.append(shops_.values());
    else
        shopsToUpdate.append(shops_.value(threadId));

    for (ShopData* shop : shopsToUpdate) {
        if (!shop) {
            continue;
        }
        if (!shop->requiresUpdate && !force) {
            continue;
        }
        if (shop->submitting) {
            continue;
        }
        templateManager.LoadTemplate(shop->shopTemplate);
        QString result = templateManager.Generate(app_.items());
        shop->requiresUpdate = false;
        shop->shopData = result;

        if (auto_update_) {
            SubmitShopToForum(threadId);
        }
    }


}

void Shop::SubmitShopToForum(const QString &threadId) {
    QList<ShopData*> shopsToUpdate;
    QList<ShopData*> shopsToSubmit;

    if (threadId.isEmpty())
        shopsToUpdate.append(shops_.values());
    else
        shopsToUpdate.append(shops_.value(threadId));

    for (ShopData* shop : shopsToUpdate) {
        if (!shop) {
            QLOG_WARN() << "Attempted to submit an invalid shop.";
            continue;
        }
        if (shop->submitting) {
            QLOG_WARN() << "Already submitting your shop.";
            continue;
        }
        if (shop->shopTemplate.isEmpty()) {
            QLOG_ERROR() << "Cannot submit a shop with an empty template...";
            continue;
        }

        if (shop->requiresUpdate)
            Update();

        // Don't update the shop if it hasn't changed
        std::string currentHash = Util::Md5(shop->shopData.toStdString());
        std::string previousHash = shop->lastSubmissionHash.toStdString();
        if (previousHash == currentHash)
            continue;

        shop->submitting = true;

        shopsToSubmit.append(shop);
    }

    if (shopsToSubmit.isEmpty()) {
        QLOG_WARN() << "No shops are ready or need to submit.";
        return;
    }

    for (ShopData* shop : shopsToSubmit) {
        SubmitSingleShop(shop->threadId);
    }
    UpdateState();
}

void Shop::UpdateState() {
    CurrentStatusUpdate status = CurrentStatusUpdate();
    status.state = ProgramState::ShopSubmitting;
    status.progress = ShopsIdle();
    status.total = shops_.count();
    if (status.progress == status.total) {
        status.state = ProgramState::ShopCompleted;
    }
    emit StatusUpdate(status);
}

void Shop::SubmitSingleShop(const QString &threadId) {
    QNetworkRequest request(QUrl(ShopEditUrl(threadId)));
    // Embed our requested ID just in case!
    request.setAttribute(QNetworkRequest::User, threadId);
    QNetworkReply *fetched = app_.logged_in_nm().get(request);
    connect(fetched, SIGNAL(finished()), this, SLOT(OnEditPageFinished()));
}

void Shop::OnEditPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QString threadId = reply->request().attribute(QNetworkRequest::User).toString();
    QByteArray bytes = reply->readAll();
    reply->disconnect();
    reply->deleteLater();

    if (threadId.isEmpty() || !shops_.contains(threadId)) {
        QLOG_ERROR() << "Shop was deleted while submitting, ignoring reply from GGG.";
        UpdateState();
        return;
    }
    ShopData* shop = shops_.value(threadId);

    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_thread");
    if (hash.empty()) {
        QLOG_ERROR() << "Can't update shop -- cannot extract CSRF token from the page. Check if thread ID is valid.";
        shop->submitting = false;
        UpdateState();
        return;
    }

    std::string title = Util::FindTextBetween(page, "<input type=\"text\" name=\"title\" id=\"title\" value=\"", "\" class=\"textInput\">");
    if (title.empty()) {
        QLOG_ERROR() << "Can't update shop -- title is empty. Check if thread ID is valid.";
        shop->submitting = false;
        UpdateState();
        return;
    }

    QUrlQuery query;
    query.addQueryItem("forum_thread", hash.c_str());
    query.addQueryItem("title", title.c_str());
    query.addQueryItem("content", shop->shopData);
    query.addQueryItem("submit", "Submit");

    QByteArray data(query.query().toUtf8());
    QNetworkRequest request(QUrl(ShopEditUrl(threadId)));
    request.setAttribute(QNetworkRequest::User, threadId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = app_.logged_in_nm().post(request, data);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnShopSubmitted()));

}

void Shop::OnShopSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QString threadId = reply->request().attribute(QNetworkRequest::User).toString();
    QByteArray bytes = reply->readAll();
    reply->disconnect();
    reply->deleteLater();

    if (threadId.isEmpty() || !shops_.contains(threadId)) {
        QLOG_ERROR() << "Shop was deleted while submitting...";
        UpdateState();
        return;
    }
    ShopData* shop = shops_.value(threadId);

    std::string page(bytes.constData(), bytes.size());
    std::string error = Util::FindTextBetween(page, "<ul class=\"errors\"><li>", "</li></ul>");
    if (!error.empty()) {
        QLOG_ERROR() << "Error while submitting shop to forums:" << error.c_str();
        shop->submitting = false;
        UpdateState();
        return;
    }

    // now let's hope that shop was submitted successfully and notify poe.trade
    QNetworkRequest request(QUrl("http://verify.xyz.is/" + threadId + "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    app_.logged_in_nm().get(request);

    QLOG_INFO() << "Shop updated successfully!";
    shop->submitting = false;
    shop->lastSubmitted = QDateTime::currentDateTime();
    shop->lastSubmissionHash = QString::fromStdString(Util::Md5(shop->shopData.toStdString()));

    if (do_bump_) {
        SubmitShopBumpToForum(threadId);
    }
    UpdateState();
    SaveShops();
}

// BUMPING

void Shop::SubmitShopBumpToForum(const QString &threadId) {
    if (threadId.isEmpty() || !shops_.contains(threadId)) {
        QLOG_WARN() << "Asked to bump a shop with invalid thread ID.";
        return;
    }
    ShopData* shop = shops_.value(threadId);
    if (!shop->lastBumped.isNull() && shop->lastBumped.secsTo(QDateTime::currentDateTime()) < POE_BUMP_DELAY) {
        QLOG_WARN() << "Asked to bump too often! Ignoring request.";
        return;
    }

    // first, get to the post-reply page
    QNetworkRequest request(QUrl(ShopBumpUrl(threadId)));
    request.setAttribute(QNetworkRequest::User, threadId);
    QNetworkReply *fetched = app_.logged_in_nm().get(request);
    connect(fetched, SIGNAL(finished()), this, SLOT(OnBumpPageFinished()));
}

void Shop::OnBumpPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QString threadId = reply->request().attribute(QNetworkRequest::User).toString();
    QByteArray bytes = reply->readAll();
    reply->disconnect();
    reply->deleteLater();

    if (threadId.isEmpty() || !shops_.contains(threadId)) {
        QLOG_ERROR() << "Shop was removed while bumping.";
        return;
    }

    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_post");
    if (hash.empty()) {
        QLOG_ERROR() << "Can't bump shop -- cannot extract CSRF token from the page. Check if thread ID is valid.";
        return;
    }

    // now submit our bump
    QUrlQuery query;
    query.addQueryItem("forum_post", hash.c_str());
    query.addQueryItem("content", BUMP_MESSAGE);
    query.addQueryItem("post_submit", "Submit");

    QByteArray data(query.query().toUtf8());
    QNetworkRequest request(QUrl(ShopBumpUrl(threadId)));
    request.setAttribute(QNetworkRequest::User, threadId);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = app_.logged_in_nm().post(request, data);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnBumpSubmitted()));
}

void Shop::OnBumpSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QString threadId = reply->request().attribute(QNetworkRequest::User).toString();
    QByteArray bytes = reply->readAll();
    reply->disconnect();
    reply->deleteLater();

    if (threadId.isEmpty() || !shops_.contains(threadId)) {
        QLOG_ERROR() << "Shop was deleted while bumping...";
        return;
    }
    ShopData* shop = shops_.value(threadId);

    std::string page(bytes.constData(), bytes.size());
    std::string error = Util::FindTextBetween(page, "<ul class=\"errors\"><li>", "</li></ul>");
    if (!error.empty()) {
        QLOG_ERROR() << "Error while bumping shop on forums:" << error.c_str();
        return;
    }

    QLOG_INFO() << "Shop bumped successfully!";
    shop->lastBumped = QDateTime::currentDateTime();
}


QString Shop::ShopEditUrl(const QString &threadId) {
    return POE_EDIT_THREAD + threadId;
}

QString Shop::ShopBumpUrl(const QString &threadId) {
    return POE_BUMP_THREAD + threadId;
}

