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

const int kEditThreadTimeout = 300000; // 5 minutes, pathofexile.com can be very slow
const int kImgurUploadTimeout = 10000; // 10s
const int kPoeApiTimeout = 3000;
