#include "lambda_connect.h"

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)

lambda_connector::lambda_connector(QObject *parent, const char* signal, const std::function<void()>& f, Qt::ConnectionType type)
            : QObject(parent), m_signal(signal), m_lambda(f) {
    m_result = QObject::connect(parent, signal, this, SLOT(trigger()), type);
}

void lambda_connector::trigger(){
    m_lambda();
}

bool lambda_connector::connected(){
    return m_result;
}

bool lambda_connector::disconnect(){
    if (m_result){
        m_result = QObject::disconnect(parent(), m_signal.toAscii().data(), this, SLOT(trigger()));
    }

    return m_result;
}

lambda_connector* lambda_connect(QObject *sender, const char *signal, const std::function<void()> &lambda, Qt::ConnectionType type){
    return new lambda_connector(sender, signal, lambda, type);
}

#endif
