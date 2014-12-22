QT += core gui network webkitwidgets testlib

win32 {
    QT += winextras
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = acquisition
TEMPLATE = app

unix {
    LIBS += -ldl
}

include(deps/QsLog/QsLog.pri)

INCLUDEPATH += src deps

SOURCES += \
    src/main.cpp\
    src/mainwindow.cpp \
    src/item.cpp \
    src/column.cpp \
    src/filters.cpp \
    src/search.cpp \
    src/items_model.cpp \
    src/flowlayout.cpp \
    src/imagecache.cpp \
    src/logindialog.cpp \
    src/itemsmanager.cpp \
    src/datamanager.cpp \
    deps/sqlite/sqlite3.c \
    src/buyoutmanager.cpp \
    src/util.cpp \
    src/shop.cpp \
    src/porting.cpp \
    src/bucket.cpp \
    src/itemlocation.cpp \
    src/version.cpp \
    src/application.cpp \
    src/steamlogindialog.cpp \
    src/modsfilter.cpp \
    src/modlist.cpp \
    src/logpanel.cpp \
    src/itemsmanagerworker.cpp \
    test/testmain.cpp \
    test/testdata.cpp \
    test/testitem.cpp \
    test/testshop.cpp \
    test/testutil.cpp

HEADERS += \
    src/item.h \
    src/column.h \
    src/filters.h \
    src/search.h \
    src/mainwindow.h \
    src/items_model.h \
    src/flowlayout.h \
    src/imagecache.h \
    src/logindialog.h \
    src/itemsmanager.h \
    src/datamanager.h \
    deps/sqlite/sqlite3.h \
    src/buyoutmanager.h \
    src/util.h \
    src/shop.h \
    src/version.h \
    src/porting.h \
    src/bucket.h \
    src/itemlocation.h \
    src/itemconstants.h \
    src/version_defines.h \
    src/application.h \
    src/steamlogindialog.h \
    src/rapidjson_util.h \
    src/modsfilter.h \
    src/modlist.h \
    src/logpanel.h \
    src/itemsmanagerworker.h \
    test/testmain.h \
    test/testdata.h \
    test/testitem.h \
    test/testshop.h \
    test/testutil.h

FORMS += \
    forms/mainwindow.ui \
    forms/logindialog.ui \
    forms/steamlogindialog.ui

CONFIG += c++11

DEPENDPATH *= $${INCLUDEPATH}

RESOURCES += resources.qrc

RC_FILE = resources.rc
