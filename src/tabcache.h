#pragma once

#include <QObject>
#include <QAbstractNetworkCache>
#include <QNetworkDiskCache>
#include <QIODevice>

class TabCache : public QNetworkDiskCache
{
    Q_OBJECT

public:
    enum Flag {
        None = 0,
        Refresh = 1
    };
    Q_DECLARE_FLAGS(Flags, Flag)

public:
    TabCache(QObject * parent = 0);
    ~TabCache() {};

    QNetworkRequest Request(const QUrl & url, Flags flags = None);

    QIODevice *prepare(const QNetworkCacheMetaData &metaData);

private:
    const int kCacheExpireInDays{7};
    const int kMaxCacheSize{10*1024*1024}; // 10MB
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TabCache::Flags)

