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
#include "buyoutmanager.h"
#include "datastore.h"
#include "itemsmanager.h"
#include "porting.h"
#include "util.h"
#include "mainwindow.h"
#include "replytimeout.h"

const std::string kPoeEditThread = "https://www.pathofexile.com/forum/edit-thread/";
const std::string kShopTemplateItems = "[items]";
const int kMaxCharactersInPost = 50000;
const int kSpoilerOverhead = 19; // "[spoiler][/spoiler]" length

Shop::Shop(Application &app) :
    app_(app),
    shop_data_outdated_(true),
    submitting_(false)
{
    threads_ = Util::StringSplit(app_.data().Get("shop"), ';');
    auto_update_ = app_.data().GetBool("shop_update", true);
    shop_template_ = app_.data().Get("shop_template");
    if (shop_template_.empty())
        shop_template_ = kShopTemplateItems;

    connect(&app.items_manager(), &ItemsManager::ItemsRefreshed, [=]() {
        Update();
        if (auto_update_)
            SubmitShopToForum();
    });
}

void Shop::SetThread(const std::vector<std::string> &threads) {
    if (submitting_)
        return;
    threads_ = threads;
    app_.data().Set("shop", Util::StringJoin(threads, ";"));
    ExpireShopData();
    app_.data().Set("shop_hash", "");
}

void Shop::SetAutoUpdate(bool update) {
    auto_update_ = update;
    app_.data().SetBool("shop_update", update);
}

void Shop::SetShopTemplate(const std::string &shop_template) {
    shop_template_ = shop_template;
    app_.data().Set("shop_template", shop_template);
    ExpireShopData();
}
std::string Shop::SpoilerBuyout(Buyout &bo) {
    std::string out = "";
    out += "[spoiler=\"" + BuyoutTypeAsPrefix[bo.type];
    if (bo.type == BUYOUT_TYPE_BUYOUT || bo.type == BUYOUT_TYPE_FIXED)
        out += " " + QString::number(bo.value).toStdString() + " "+ CurrencyAsTag[bo.currency];
    out += "\"]";
    return out;
}

void Shop::Update() {
    if (submitting_) {
        QLOG_WARN() << "Submitting shop right now, the request to update shop data will be ignored";
        return;
    }
    shop_data_outdated_ = false;
    shop_data_.clear();
    std::string data = "";
    std::vector<AugmentedItem> aug_items;
    AugmentedItem tmp = AugmentedItem();
    //Get all buyouts to be able to sort them
    for (auto &item : app_.items_manager().items()) {
        tmp.item = item.get();
        tmp.bo.type = BUYOUT_TYPE_NONE;
        tmp.bo.type = BUYOUT_TYPE_NONE;
        std::string hash = item->location().GetUniqueHash();
        if (app_.buyout_manager().ExistsTab(hash))
            tmp.bo = app_.buyout_manager().GetTab(hash);
        if (app_.buyout_manager().Exists(*item))
            tmp.bo = app_.buyout_manager().Get(*item);
        if (tmp.bo.type == BUYOUT_TYPE_NONE)
            continue;
        if (item->location().socketed())
            continue;
        aug_items.push_back(tmp);
    }
    if (aug_items.size() == 0)
        return;
    std::sort(aug_items.begin(), aug_items.end());

    Buyout current_bo = aug_items[0].bo;
    data += SpoilerBuyout(current_bo);
    for (auto &aug : aug_items) {
        if (aug.bo.type != current_bo.type || aug.bo.currency != current_bo.currency || aug.bo.value != current_bo.value) {
            current_bo = aug.bo;
            data += "[/spoiler]";
            data += SpoilerBuyout(current_bo);
        }
        std::string item_string = aug.item->location().GetForumCode(app_.league());
        if (data.size() + item_string.size() + shop_template_.size()+ kSpoilerOverhead + QString("[/spoiler]").size() > kMaxCharactersInPost) {
            data +="[/spoiler]";
            shop_data_.push_back(data);
            data = SpoilerBuyout(current_bo);
            data += item_string;
        } else {
            data += item_string;
        }
    }
    if (!data.empty())
        shop_data_.push_back(data);

    for (size_t i = 0; i < shop_data_.size(); ++i)
        shop_data_[i] = Util::StringReplace(shop_template_, kShopTemplateItems, "[spoiler]" + shop_data_[i] + "[/spoiler]");

    shop_hash_ = Util::Md5(Util::StringJoin(shop_data_, ";"));
}

void Shop::ExpireShopData() {
    shop_data_outdated_ = true;
}

void Shop::SubmitShopToForum(bool force) {
    if (submitting_) {
        QLOG_WARN() << "Already submitting your shop.";
        return;
    }
    if (threads_.empty()) {
        QLOG_ERROR() << "Asked to update a shop with no shop ID defined.";
        return;
    }

    if (shop_data_outdated_)
        Update();

    std::string previous_hash = app_.data().Get("shop_hash");
    // Don't update the shop if it hasn't changed
    if (previous_hash == shop_hash_ && !force)
        return;

    if (threads_.size() < shop_data_.size()) {
        QLOG_WARN() << "Need" << shop_data_.size() - threads_.size() << "more shops defined to fit all your items.";
    }

    requests_completed_ = 0;
    submitting_ = true;
    SubmitSingleShop();
}

std::string Shop::ShopEditUrl(int idx) {
    return kPoeEditThread + threads_[idx];
}

void Shop::SubmitSingleShop() {
    CurrentStatusUpdate status = CurrentStatusUpdate();
    status.state = ProgramState::ShopSubmitting;
    status.progress = requests_completed_;
    status.total = threads_.size();
    if (requests_completed_ == threads_.size()) {
        status.state = ProgramState::ShopCompleted;
        submitting_ = false;
        app_.data().Set("shop_hash", shop_hash_);
    } else {
        // first, get to the edit-thread page to grab CSRF token
        QNetworkReply *fetched = app_.logged_in_nm().get(QNetworkRequest(QUrl(ShopEditUrl(requests_completed_).c_str())));
        new QReplyTimeout(fetched, kEditThreadTimeout);
        connect(fetched, SIGNAL(finished()), this, SLOT(OnEditPageFinished()));
    }
    emit StatusUpdate(status);
}

void Shop::OnEditPageFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string hash = Util::GetCsrfToken(page, "forum_thread");
    if (hash.empty()) {
        QLOG_ERROR() << "Can't update shop -- cannot extract CSRF token from the page. Check if thread ID is valid.";
        submitting_ = false;
        return;
    }

    // now submit our edit

    // holy shit give me some html parser library please
    std::string title = Util::FindTextBetween(page, "<input type=\"text\" name=\"title\" id=\"title\" value=\"", "\" class=\"textInput\">");
    if (title.empty()) {
        QLOG_ERROR() << "Can't update shop -- title is empty. Check if thread ID is valid.";
        submitting_ = false;
        return;
    }

    QUrlQuery query;
    query.addQueryItem("forum_thread", hash.c_str());
    query.addQueryItem("title", title.c_str());
    query.addQueryItem("content", requests_completed_ < shop_data_.size() ? shop_data_[requests_completed_].c_str() : "Empty");
    query.addQueryItem("submit", "Submit");

    QByteArray data(query.query().toUtf8());
    QNetworkRequest request((QUrl(ShopEditUrl(requests_completed_).c_str())));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *submitted = app_.logged_in_nm().post(request, data);
    new QReplyTimeout(submitted, kEditThreadTimeout);
    connect(submitted, SIGNAL(finished()), this, SLOT(OnShopSubmitted()));
}

void Shop::OnShopSubmitted() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    QByteArray bytes = reply->readAll();
    std::string page(bytes.constData(), bytes.size());
    std::string error = Util::FindTextBetween(page, "<ul class=\"errors\"><li>", "</li></ul>");
    if (!error.empty()) {
        QLOG_ERROR() << "Error while submitting shop to forums:" << error.c_str();
        submitting_ = false;
        return;
    }

    // now let's hope that shop was submitted successfully and notify poe.trade
    QNetworkRequest request(QUrl(("http://verify.xyz.is/" + threads_[requests_completed_] + "/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str()));
    app_.logged_in_nm().get(request);

    ++requests_completed_;
    SubmitSingleShop();
}

void Shop::CopyToClipboard() {
    if (shop_data_outdated_)
        Update();

    if (shop_data_.empty())
        return;

    if (shop_data_.size() > 1) {
        QLOG_WARN() << "You have more than one shop, only the first one will be copied.";
    }

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString(shop_data_[0].c_str()));
}
