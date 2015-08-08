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

const std::string POE_EDIT_THREAD = "https://www.pathofexile.com/forum/edit-thread/";
const std::string POE_BUMP_THREAD = "https://www.pathofexile.com/forum/post-reply/";
const std::string POE_BUMP_MESSAGE = "[url=https://github.com/Novynn/acquisitionplus/releases]Bumped with Acquisition Plus![/url]";
const std::string kShopTemplateItems = "[items]";
const int POE_BUMP_DELAY = 3600; // 1 hour per Path of Exile forum rules

Shop::Shop(Application &app) :
    app_(app),
    shop_data_outdated_(true)
{
    thread_ = app_.data_manager().Get("shop");
    auto_update_ = app_.data_manager().GetBool("shop_update", true);
    shop_template_ = app_.data_manager().Get("shop_template");
    if (shop_template_.empty())
        shop_template_ = kShopTemplateItems;
}

void Shop::SetThread(const std::string &thread) {
    thread_ = thread;
    app_.data_manager().Set("shop", thread);
}

void Shop::SetAutoUpdate(bool update) {
    auto_update_ = update;
    app_.data_manager().SetBool("shop_update", update);
}

void Shop::SetShopTemplate(const std::string &shop_template) {
    shop_template_ = shop_template;
    app_.data_manager().Set("shop_template", shop_template);
    ExpireShopData();
}

void Shop::Update() {
    shop_data_outdated_ = false;
    std::string data = "[spoiler]";
    for (auto &item : app_.items()) {
        if (item->location().socketed())
            continue;
        Buyout bo;
        bo.type = BUYOUT_TYPE_NONE;

        std::string hash = item->location().GetUniqueHash();
        if (app_.buyout_manager().ExistsTab(hash))
            bo = app_.buyout_manager().GetTab(hash);
        if (app_.buyout_manager().Exists(*item))
            bo = app_.buyout_manager().Get(*item);
        if (bo.type == BUYOUT_TYPE_NONE)
            continue;

        data += item->location().GetForumCode(app_.league());

        data += BuyoutTypeAsPrefix[bo.type];

        if (bo.type == BUYOUT_TYPE_BUYOUT || bo.type == BUYOUT_TYPE_FIXED) {
            data += QString::number(bo.value).toUtf8().constData();
            data += " " + CurrencyAsTag[bo.currency];
        }
    }
    data += "[/spoiler]";

    shop_data_ = Util::StringReplace(shop_template_, kShopTemplateItems, data);
    shop_hash_ = Util::Md5(shop_data_);
}

void Shop::ExpireShopData() {
    shop_data_outdated_ = true;
}

std::string Shop::ShopEditUrl() {
    return POE_EDIT_THREAD + thread_;
}

std::string Shop::ShopBumpUrl() {
    return POE_BUMP_THREAD + thread_;
}

void Shop::SubmitShopToForum() {
    if (thread_.empty()) {
        QLOG_WARN() << "Asked to update a shop with empty thread ID.";
        return;
    }

    if (shop_data_outdated_)
        Update();

    std::string previous_hash = app_.data_manager().Get("shop_hash");
    // Don't update the shop if it hasn't changed
    if (previous_hash == shop_hash_)
        return;
    // first, get to the edit-thread page
    QNetworkReply *fetched = app_.logged_in_nm().get(QNetworkRequest(QUrl(ShopEditUrl().c_str())));
    connect(fetched, SIGNAL(finished()), this, SLOT(OnEditPageFinished()));

    emit ShopUpdateBegin();
    QLOG_INFO() << "Updating shop...";
}

void Shop::SubmitShopBumpToForum() {
    if (thread_.empty()) {
        QLOG_WARN() << "Asked to bump a shop with empty thread ID.";
        return;
    }

    if (!lastBumped_.isNull() && lastBumped_.secsTo(QDateTime::currentDateTime()) < POE_BUMP_DELAY) {
        QLOG_WARN() << "Asked to bump too often! Ignoring request.";
        return;
    }

    // first, get to the post-reply page
    QNetworkReply *fetched = app_.logged_in_nm().get(QNetworkRequest(QUrl(ShopBumpUrl().c_str())));
    connect(fetched, SIGNAL(finished()), this, SLOT(OnBumpPageFinished()));
}



void Shop::OnEditPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_thread");
    if (hash.empty()) {
        QLOG_ERROR() << "Can't update shop -- cannot extract CSRF token from the page. Check if thread ID is valid.";
        emit ShopUpdateFinished();
        return;
    }

    // now submit our edit

    // holy shit give me some html parser library please
    std::string title = Util::FindTextBetween(page, "<input type=\"text\" name=\"title\" id=\"title\" value=\"", "\" class=\"textInput\">");
    if (title.empty()) {
        QLOG_ERROR() << "Can't update shop -- title is empty. Check if thread ID is valid.";
        emit ShopUpdateFinished();
        return;
    }

    QUrlQuery query;
    query.addQueryItem("forum_thread", hash.c_str());
    query.addQueryItem("title", title.c_str());
    query.addQueryItem("content", shop_data_.c_str());
    query.addQueryItem("submit", "Submit");

    QByteArray data(query.query().toUtf8());
    QNetworkRequest request((QUrl(ShopEditUrl().c_str())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = app_.logged_in_nm().post(request, data);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnShopSubmitted()));
}

void Shop::OnBumpPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_post");
    if (hash.empty()) {
        QLOG_ERROR() << "Can't bump shop -- cannot extract CSRF token from the page. Check if thread ID is valid.";
        return;
    }

    // now submit our bump

    QUrlQuery query;
    query.addQueryItem("forum_post", hash.c_str());
    query.addQueryItem("content", POE_BUMP_MESSAGE.c_str());
    query.addQueryItem("post_submit", "Submit");

    QByteArray data(query.query().toUtf8());
    QNetworkRequest request((QUrl(ShopBumpUrl().c_str())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = app_.logged_in_nm().post(request, data);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnBumpSubmitted()));
}

void Shop::OnBumpSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string error = Util::FindTextBetween(page, "<ul class=\"errors\"><li>", "</li></ul>");
    if (!error.empty()) {
        QLOG_ERROR() << "Error while bumping shop on forums:" << error.c_str();
        return;
    }

    QLOG_INFO() << "Shop bumped successfully!";
    lastBumped_ = QDateTime::currentDateTime();
}

void Shop::OnShopSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string error = Util::FindTextBetween(page, "<ul class=\"errors\"><li>", "</li></ul>");
    if (!error.empty()) {
        QLOG_ERROR() << "Error while submitting shop to forums:" << error.c_str();
        emit ShopUpdateFinished();
        return;
    }

    // now let's hope that shop was submitted successfully and notify poe.xyz.is
    QNetworkRequest request(QUrl(("http://verify.xyz.is/" + thread_ + "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str()));
    app_.logged_in_nm().get(request);

    app_.data_manager().Set("shop_hash", shop_hash_);

    QLOG_INFO() << "Shop updated successfully!";

    if (app_.data_manager().GetBool("shop_bump")) {
        SubmitShopBumpToForum();
    }
    emit ShopUpdateFinished();
}

void Shop::CopyToClipboard() {
    if (shop_data_outdated_)
        Update();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString(shop_data_.c_str()));
}
