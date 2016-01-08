#pragma once

#include <functional>
#include <QString>
#include <QObject>

//Adapted from https://github.com/caetanus/lambda-connect-qt4

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

class lambda_connector : public QObject {
    Q_OBJECT
public:
    lambda_connector(QObject *parent, const char* signal,
        const std::function<void()>& f,
        Qt::ConnectionType type = Qt::AutoConnection);
    bool disconnect();
    bool connected();

public slots:
    void trigger();

private:
    bool m_result;
    const QString m_signal;
    std::function<void()> m_lambda;
};

lambda_connector* lambda_connect(QObject *sender, const char *signal, const std::function<void()> &lambda, Qt::ConnectionType type = Qt::AutoConnection);

#else

#define lambda_connect connect

#endif
