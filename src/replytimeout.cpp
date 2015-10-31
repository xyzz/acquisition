// https://codereview.stackexchange.com/questions/30031/qnetworkreply-network-reply-timeout-helper

#include "replytimeout.h"

QReplyTimeout::QReplyTimeout(QNetworkReply* reply, const int timeout):
    QObject(reply) 
{
    if (reply) {
        QTimer::singleShot(timeout, this, SLOT(timeout()));
    }
}

void QReplyTimeout::timeout() {
    QNetworkReply* reply = static_cast<QNetworkReply*>(parent());
    if (reply->isRunning()) {
        reply->close();
    }
}
