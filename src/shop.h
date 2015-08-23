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

#include "shoptemplatemanager.h"

#include <QObject>
#include <QDateTime>
#include <string>
#include <vector>
#include <QQueue>

#include <shops/shopsubmitter.h>

struct CurrentStatusUpdate;
extern const std::string kShopTemplateItems;

class Application;

struct ShopData {
    QString threadId;
    QString shopData;

    QString shopTemplate;
    bool requiresUpdate;

    QDateTime lastSubmitted;
    QDateTime lastBumped;
    QString lastSubmissionHash;
};

class Shop : public QObject {
    Q_OBJECT
public:
    explicit Shop(Application &app);
    void AddShop(const QString &threadId, QString temp = QString());
    void RemoveShop(const QString &threadId);
    void CopyToClipboard(const QString &threadId);
    void ExpireShopData();
    void SetShopTemplate(const QString &threadId, const QString &temp);

    void SetTimeout(int timeout);
    int GetTimeout();

    const QList<QString> threadIds() const { return shops_.uniqueKeys(); }
    QString GetShopTemplate(const QString &threadId);

    void Update(const QString &threadId = QString(), bool force = false);
    void SubmitShopToForum(const QString &threadId = QString());

    void SetAutoUpdate(bool update);
    void SetDoBump(bool bump);

    bool IsAutoUpdateEnabled() const { return auto_update_; }
    bool IsBumpEnabled() const { return do_bump_; }
    void SaveShops();
    void LoadShops();
public slots:
    void OnShopSubmitted(const QString &threadId);
    void OnShopBumped(const QString &threadId);
    void OnShopError(const QString &threadId, const QString &error);
signals:
    void StatusUpdate(const CurrentStatusUpdate &status);
private:
    void SubmitSingleShop(ShopData* shop);

    Application &app_;
    ShopTemplateManager templateManager;
    ShopSubmitter submitter_;

    QHash<QString, ShopData*> shops_;

    bool auto_update_;
    bool do_bump_;

    void UpdateState();
};
