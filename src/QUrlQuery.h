/*!
 * \file
 * \brief This file allows usage of QUrlQuery to be supported in QT4
 */

#pragma once

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)

#include <QUrlQuery>

inline QByteArray query_to_data(const QUrlQuery& query){
    return {query.query().toUtf8()};
}

inline QUrlQuery prepare_query(QUrl& /*url*/){
    return {};
}

inline void set_query(QUrlQuery& query, QUrl& url){
    url.setQuery(query);
}

#else

using QUrlQuery = QUrl;

inline QByteArray query_to_data(const QUrlQuery& query){
    return query.encodedQuery();
}

inline QUrlQuery& prepare_query(QUrl& url){
    return url;
}

inline void set_query(QUrlQuery& /*query*/, QUrl& /*url*/){
    //NOP
}

#endif
