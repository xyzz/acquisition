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

Shop::Shop(Application *app) :
    app_(app),
    shop_data_outdated_(true)
{
    thread_ = app_->data_manager()->Get("shop");
    auto_update_ = app_->data_manager()->GetBool("shop_update", true);
}

void Shop::SetThread(const std::string &thread) {
    thread_ = thread;
    app_->data_manager()->Set("shop", thread);
}

void Shop::SetAutoUpdate(bool update) {
    auto_update_ = update;
    app_->data_manager()->SetBool("shop_update", update);
}

void Shop::Update() {
    shop_data_outdated_ = false;
    std::string data = "[spoiler]";
    for (auto &item : app_->items()) {
        Buyout bo;
        bo.type = BUYOUT_TYPE_NONE;

        std::string hash = item->location().GetUniqueHash();
        if (app_->buyout_manager()->ExistsTab(hash))
            bo = app_->buyout_manager()->GetTab(hash);
        if (app_->buyout_manager()->Exists(*item))
            bo = app_->buyout_manager()->Get(*item);
        if (bo.type == BUYOUT_TYPE_NONE)
            continue;

        data += item->location().GetForumCode(app_->league());

        if (bo.type == BUYOUT_TYPE_BUYOUT)
            data += " ~b/o ";
        else if (bo.type == BUYOUT_TYPE_FIXED)
            data += " ~price ";
        data += QString::number(bo.value).toUtf8().constData();
        data += " " + CurrencyAsTag[bo.currency];
    }
    data += "[/spoiler]";

    shop_data_ = data;
}

void Shop::ExpireShopData() {
    shop_data_outdated_ = true;
}

std::string Shop::ShopEditUrl() {
    return POE_EDIT_THREAD + thread_;
}

void Shop::SubmitShopToForum() {
    if (thread_.empty()) {
        QLOG_WARN() << "Asked to update a shop with empty thread ID.";
        return;
    }

    if (shop_data_outdated_)
        Update();

    // first, get to the edit-thread page
    QNetworkReply *fetched = app_->logged_in_nm()->get(QNetworkRequest(QUrl(ShopEditUrl().c_str())));
    connect(fetched, SIGNAL(finished()), this, SLOT(OnEditPageFinished()));
}

void Shop::OnEditPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_thread");
    if (hash.empty()) {
        QLOG_WARN() << "Can't update shop -- cannot extract CSRF token from page.";
        return;
    }

    // now submit our edit

    // holy shit give me some html parser library please
    std::string title = Util::FindTextBetween(page, "<input type=\"text\" name=\"title\" id=\"title\" value=\"", "\" class=\"textInput\">");
    if (title.empty()) {
        QLOG_WARN() << "Can't update shop -- title is empty. Check if thread ID is valid.";
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
    QNetworkReply *submitted = app_->logged_in_nm()->post(request, data);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnShopSubmitted()));
}

void Shop::OnShopSubmitted() {
    // now let's hope that shop was submitted successfully and notify poe.xyz.is
    QNetworkRequest request(QUrl(("http://verify.xyz.is/" + thread_ + "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str()));
    app_->logged_in_nm()->get(request);
}

void Shop::CopyToClipboard() {
    if (shop_data_outdated_)
        Update();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString(shop_data_.c_str()));
}
