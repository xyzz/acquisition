#ifndef LOGCHECKER_H
#define LOGCHECKER_H

#include <QObject>

class LogChecker : public QObject
{
    Q_OBJECT
public:
    explicit LogChecker(QObject *parent = 0, const QString &path = QString());
    void setPath(const QString &path);
protected:
    void timerEvent(QTimerEvent *event);
private:
    QString path_;
    int logTimerId_;
    qint64 logLastSize_;
    void onNewEntries(const QString &data);
signals:
    void onCharacterMessage(const QString &name, const QString &message);
};

#endif // LOGCHECKER_H
