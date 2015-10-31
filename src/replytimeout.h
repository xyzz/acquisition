#pragma once

#include <QObject>
#include <QNetworkReply>
#include <QtCore/QTimer>

class QReplyTimeout : public QObject {
    Q_OBJECT
public:
    QReplyTimeout(QNetworkReply* reply, const int timeout);
private slots:
    void timeout();
};

const int kEditThreadTimeout = 30000; // 30s, pathofexile.com can be slow
