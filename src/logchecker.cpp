#include "logchecker.h"

#include <QEvent>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

LogChecker::LogChecker(QObject *parent, const QString &path)
    : QObject(parent)
    , path_(path) {
    logTimerId_ = 0; //this->startTimer(5000);
    logLastSize_ = 0;
}

void LogChecker::setPath(const QString &path) {
    path_ = path;
}

void LogChecker::onNewEntries(const QString &data) {
    QStringList lines = data.split("\n", QString::SkipEmptyParts);
    QRegularExpression expr = QRegularExpression("^(?<year>\\d\\d\\d\\d)/(?<month>\\d\\d)/(?<date>\\d\\d) "
                                                 "(?<hour>\\d\\d):(?<minute>\\d\\d):(?<second>\\d\\d) \\d+ \\d+ "
                                                 "\\[INFO Client \\d+\\] @(?<name>\\w+): (?<message>.*)$");
    for (QString line : lines) {
        QString trimmed = line.replace('\n', "").trimmed();
        QRegularExpressionMatch matcher = expr.match(trimmed);
        if (matcher.hasMatch()) {
            emit onCharacterMessage(matcher.captured("name"), matcher.captured("message"));
        }
    }
}

void LogChecker::timerEvent(QTimerEvent *event) {
    if (event->timerId() == logTimerId_) {
        QFileInfo info(path_);
        if (info.exists() && info.isFile()) {
            if (logLastSize_ == 0) {
                logLastSize_ = info.size();
            }
            else if (info.size() > logLastSize_) {
                // It's updated! Get the new data
                QFile f(info.absoluteFilePath());
                f.open(QFile::ReadOnly);
                f.seek(logLastSize_);
                QByteArray newData = f.read(info.size() - logLastSize_);
                logLastSize_ = info.size();
                f.close();
                onNewEntries(newData);
            }
        }
    }
    QObject::timerEvent(event);
}
