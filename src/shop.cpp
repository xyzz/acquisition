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

const int POE_BUMP_DELAY = 3600; // 1 hour per Path of Exile forum rules
const QString TEMPLATE_MESSAGE = "{everything|group}\n\n[url=https://github.com/Novynn/acquisitionplus/releases]Shop made with Acquisition Plus[/url]";

Shop::Shop(Application &app)
    : app_(app)
    , templateManager(&app)
    , submitter_(&app.logged_in_nm())
    , shopsNeedUpdate(false) {
    connect(&submitter_, &ShopSubmitter::ShopSubmitted, this, &Shop::OnShopSubmitted);
    connect(&submitter_, &ShopSubmitter::ShopBumped, this, &Shop::OnShopBumped);
    connect(&submitter_, &ShopSubmitter::ShopSubmissionError, this, &Shop::OnShopError);

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
        bump_interval_ = doc.object().value("bump_interval").toInt(POE_BUMP_DELAY);
        int timeout = doc.object().value("timeout").toInt();
        if (timeout > 0)
            submitter_.SetTimeout(timeout);

        SetShareItems(doc.object().value("shared_items").toBool());

        for (QJsonValue val : doc.object().value("shops").toArray()) {
            QJsonObject obj = val.toObject();
            QString id = obj.value("thread_id").toString();
            if (id.isEmpty()) continue;
            QString temp = obj.value("template").toString();
            AddShop(id, temp, false);
            ShopData* data = shops_.value(id);
            // populate other data
            int timeT = obj.value("last_submitted").toInt();
            if (timeT >= 0)
                data->lastSubmitted = QDateTime::fromTime_t(timeT);
            else
                data->lastSubmitted = QDateTime();

            timeT = obj.value("last_bumped").toInt();
            if (timeT >= 0)
                data->lastBumped = QDateTime::fromTime_t(timeT);
            else
                data->lastBumped = QDateTime();
            data->lastSubmissionHash = obj.value("last_hash").toString();
        }
        Update();
    }

    if (shops_.count() == 0) {
        // Hmm, try to load old shops?
        QString shopId = QString::fromStdString(app_.data_manager().Get("shop"));
        bool autoUpdate = app_.data_manager().GetBool("shop_update");
        QString shopTemplate = QString::fromStdString(app_.data_manager().Get("shop_template"));

        if (!shopId.isEmpty()) {
            QLOG_INFO() << "Found old shop data! Now loading...";
            if (shopTemplate == "[items]") {
                shopTemplate = TEMPLATE_MESSAGE;
            }
            else {
                shopTemplate.replace("[items]", "{everything|group}");
            }
            AddShop(shopId, shopTemplate);

            auto_update_ = autoUpdate;

            app_.data_manager().Set("shop", "");
            app_.data_manager().SetBool("shop_update", false);
            app_.data_manager().Set("shop_template", "");
            app_.data_manager().Set("shop_hash", "");
        }
    }

    SaveShops();
}

void Shop::SaveShops() {
    Update();

    QJsonObject mainObject;
    mainObject.insert("auto_update", auto_update_);
    mainObject.insert("auto_bump", do_bump_);
    mainObject.insert("bump_interval", bump_interval_);
    mainObject.insert("timeout", submitter_.GetTimeout());
    mainObject.insert("shared_items", AreItemsShared());

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

void Shop::AddShop(const QString &threadId, QString temp, bool expire) {
    if (shops_.contains(threadId)) {
        RemoveShop(threadId);
    }

    if (temp.isEmpty()) {
        temp = TEMPLATE_MESSAGE;
    }

    ShopData* data = new ShopData;
    data->threadId = threadId;
    data->shopTemplate = temp;
    shops_.insert(threadId, data);

    if (expire)
        ExpireShopData();
    SaveShops();
}

void Shop::RemoveShop(const QString &threadId) {
    Q_ASSERT(shops_.contains(threadId));
    ShopData* data = shops_.take(threadId);

    if (data)
        delete data;

    ExpireShopData();
    SaveShops();
}

void Shop::CopyToClipboard(const QString &threadId) {
    ShopData* data = shops_.value(threadId);
    if (!data) return;

    Update();

    if (data->shopTemplate.isEmpty() || data->shopData.isEmpty())
        return;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(data->shopData);
}

void Shop::ExpireShopData() {
    Shop::shopsNeedUpdate = true;
    QLOG_INFO() << "Shop data expired.";
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
        ExpireShopData();
        SaveShops();
    }
}

void Shop::SetTimeout(int timeout) {
    submitter_.SetTimeout(timeout);
}

int Shop::GetTimeout() {
    return submitter_.GetTimeout();
}

const QString Shop::GetShopTemplate(const QString &threadId) {
    ShopData* data = shops_.value(threadId);
    if (!data) return QString();
    return data->shopTemplate;
}

const QString Shop::GetShopTemplate() {
    if (shops_.isEmpty()) return TEMPLATE_MESSAGE;
    return shops_.first()->shopTemplate;
}

void Shop::Update() {
    QLOG_INFO() << "Update called";

    if (Shop::shopsNeedUpdate) {
        Items pool = app_.items();

        if (AreItemsShared()) {
            templateManager.SetSharedItems(true);
            templateManager.LoadTemplate(GetShopTemplate());
            QStringList results = templateManager.Generate(pool);

            for (ShopData* shop : shops_.values()) {
                if (!results.isEmpty()) shop->shopData = results.takeFirst();
                else shop->shopData = QString();
            }
            QLOG_INFO() << "Shared shop data generated!";
        }
        else {
            for (ShopData* shop : shops_.values()) {
                templateManager.SetSharedItems(false);
                templateManager.LoadTemplate(shop->shopTemplate);
                QStringList result = templateManager.Generate(pool);
                if (result.isEmpty()) {
                    QLOG_WARN() << "No data was returned from the template. Maybe it's empty?";
                    shop->shopData = QString();
                }
                else {
                    if (result.size() > 1) {
                        QLOG_WARN() << "The template exceeded the character limit. Your shop will be truncated.";
                    }
                    shop->shopData = result.first();
                    QLOG_INFO() << "Shop data generated for " << shop->threadId << " of size " << shop->shopData.length();
                }
            }
        }
    }
    Shop::shopsNeedUpdate = false;
}

void Shop::SubmitShopToForum(const QString &threadId) {
    QList<ShopData*> shopsToUpdate;
    QList<ShopData*> shopsToSubmit;

    if (threadId.isEmpty()) {
        shopsToUpdate.append(shops_.values());
        QLOG_INFO() << "Submitting shops to forum.";
    }
    else {
        shopsToUpdate.append(shops_.value(threadId));
        QLOG_INFO() << "Submitting shop " << threadId << " to forum.";
    }

    for (ShopData* shop : shopsToUpdate) {
        if (!shop) {
            QLOG_WARN() << "Attempted to submit an invalid shop.";
            continue;
        }
        if (submitter_.IsSubmitting(shop->threadId)) {
            QLOG_WARN() << "Already submitting your shop.";
            continue;
        }
        if (shop->shopTemplate.isEmpty()) {
            QLOG_ERROR() << "Cannot submit a shop with an empty template...";
            continue;
        }

        Update();

        // Don't update the shop if it hasn't changed
        std::string currentHash = Util::Md5(shop->shopData.toStdString());
        std::string previousHash = shop->lastSubmissionHash.toStdString();
        if (previousHash == currentHash)
            continue;

        shopsToSubmit.append(shop);
    }

    if (shopsToSubmit.isEmpty()) {
        QLOG_INFO() << "No shops were outdated.";
        return;
    }

    for (ShopData* shop : shopsToSubmit) {
        SubmitSingleShop(shop);
    }
    UpdateState();
}

void Shop::UpdateState() {
    int submitting = submitter_.Count();
    int count = shops_.count();
    CurrentStatusUpdate status = CurrentStatusUpdate();
    status.state = ProgramState::ShopSubmitting;
    status.progress = count - submitting;
    status.total = count;
    if (submitting == 0) {
        status.state = ProgramState::ShopCompleted;
    }
    emit StatusUpdate(status);
}

void Shop::SubmitSingleShop(ShopData* shop) {
    bool shouldBump = IsBumpEnabled();
    if (shouldBump) {
        shouldBump = shop->lastBumped.isNull() || !shop->lastBumped.isValid() ||
                     shop->lastBumped.secsTo(QDateTime::currentDateTime()) >= BumpInterval();
    }
    submitter_.BeginShopSubmission(shop->threadId, shop->shopData, shouldBump);
    UpdateState();
}

void Shop::OnShopSubmitted(const QString &threadId) {
    if (!shops_.contains(threadId)) return;
    ShopData* shop = shops_.value(threadId);
    // now let's hope that shop was submitted successfully and notify poe.trade
    QNetworkRequest request(QUrl("http://verify.xyz.is/" + threadId + "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    app_.logged_in_nm().get(request);

    QLOG_INFO() << "Shop updated successfully!";
    shop->lastSubmitted = QDateTime::currentDateTime();
    shop->lastSubmissionHash = QString::fromStdString(Util::Md5(shop->shopData.toStdString()));

    UpdateState();
    SaveShops();
}

void Shop::OnShopBumped(const QString &threadId) {
    if (!shops_.contains(threadId)) return;
    ShopData* shop = shops_.value(threadId);
    QLOG_INFO() << "Shop bumped successfully!";
    shop->lastBumped = QDateTime::currentDateTime();
    UpdateState();
}

void Shop::OnShopError(const QString &threadId, const QString &error) {
    QLOG_ERROR() << qPrintable("An error occurred with shop " + threadId + ": " + error);
    UpdateState();
}
