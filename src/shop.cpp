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

#include "mainwindow.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "porting.h"

#include <QApplication>
#include <QClipboard>

Shop::Shop(MainWindow *app):
    app_(app),
    shop_data_outdated_(true)
{
    thread_ = app_->data_manager()->Get("shop");
}

void Shop::SetThread(const std::string &thread) {
    thread_ = thread;
    app_->data_manager()->Set("shop", thread);
}

void Shop::Update(bool submit) {
    shop_data_outdated_ = false;
    std::string data;
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

    shop_data_ = data;
    if (submit)
        SubmitShopToForum();
}

void Shop::ExpireShopData() {
    shop_data_outdated_ = true;
}

void Shop::SubmitShopToForum() {
    if (thread_.size() == 0)
        return;

    // TODO
}

void Shop::CopyToClipboard() {
    if (shop_data_outdated_)
        Update();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString(shop_data_.c_str()));
}
