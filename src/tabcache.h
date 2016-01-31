#pragma once

#include <QObject>
#include <QAbstractNetworkCache>
#include <QNetworkDiskCache>
#include <QIODevice>
#include <set>

class ItemLocation;

class TabCache : public QNetworkDiskCache
{
    Q_OBJECT

public:
    enum Flag {
        None = 0,
        Refresh = 1
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    enum Policy {
        DefaultCache,
        ManualCache,
        AlwaysCache,
        NeverCache
    };

public:
    TabCache(QObject * parent = 0);

    ~TabCache() {};

    QNetworkRequest Request(const QUrl & url, const ItemLocation &loc, Flags flags = None);
    void AddManualRefresh(const ItemLocation &loc);

    QIODevice *prepare(const QNetworkCacheMetaData &metaData);

private:
    const int kCacheExpireInDays{7};

    std::set<std::string> manual_refresh_;
    Policy cache_policy_{DefaultCache};

public slots:
    void OnPolicyUpdate(Policy policy);
    void OnItemsRefreshed();
};

Q_DECLARE_METATYPE(TabCache::Policy)
Q_DECLARE_OPERATORS_FOR_FLAGS(TabCache::Flags)

