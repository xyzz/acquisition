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

#pragma once

#include <QObject>
#include <QString>

extern const QString kShopTemplateItems;

class Application;

class Shop : public QObject {
    Q_OBJECT
public:
    explicit Shop(Application &app);
    void SetThread(const QString &thread);
    void SetAutoUpdate(bool update);
    void SetShopTemplate(const QString &shop_template);
    void Update();
    void CopyToClipboard();
    void ExpireShopData();
    void SubmitShopToForum();
    bool auto_update() const { return auto_update_; }
    const QString &thread() const { return thread_; }
    const QString &shop_data() const { return shop_data_; }
    const QString &shop_template() const { return shop_template_; }
public slots:
    void OnEditPageFinished();
    void OnShopSubmitted();
private:
    QString ShopEditUrl();
    Application &app_;
    QString thread_;
    QString shop_data_;
    QString shop_hash_;
    QString shop_template_;
    bool shop_data_outdated_;
    bool auto_update_;
};
