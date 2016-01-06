#ifndef _LAMBDA_CONNECT_H_
#define _LAMBDA_CONNECT_H_

//Adapted from https://github.com/caetanus/lambda-connect-qt4

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

#include <functional>
#include <QString>
#include <QObject>

template<typename Functor>
class LambdaConnectorHelper : public QObject {
    Q_OBJECT
public:
    LambdaConnectorHelper(QObject *parent, const char* signal, Functor f, Qt::ConnectionType type = Qt::AutoConnection)
            : QObject(parent), m_signal(signal), m_lambda(f){
        m_result = QObject::connect(parent, signal, this, SLOT(trigger()), type);
    }

    bool disconnect(){
        if (m_result){
            m_result = QObject::disconnect(parent(),m_signal.toAscii().data(), this, SLOT(trigger()));
        }

        return m_result;
    }

    bool connected(){
        return m_result;
    }

public slots:
    void trigger(){
        m_lambda();
    }

private:
    bool m_result;
    const QString m_signal;
    Functor m_lambda;
};

template<typename Functor>
LambdaConnectorHelper<Functor>* lambda_connect(QObject *sender, const char *signal, Functor lambda, Qt::ConnectionType type = Qt::AutoConnection){
    return new LambdaConnectorHelper<Functor>(sender, signal, lambda, type);
}

#else

#define lambda_connect connect

#endif

#endif // _LAMBDA_CONNECT_H_
