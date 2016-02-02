TARGET = acquisitionplus
TEMPLATE = app

QT += core gui network

# Enable below for steam support
#CONFIG += steam

steam {
    QT += webkitwidgets
    SOURCES += src/steamlogindialog.cpp
    HEADERS += src/steamlogindialog.h
    FORMS += forms/steamlogindialog.ui
    DEFINES += WITH_STEAM
}

win32 {
    QT += winextras
}

unix {
    LIBS += -ldl
    QMAKE_CXXFLAGS += -Wno-inconsistent-missing-override
}

!msvc {
    QMAKE_CXXFLAGS += -Wno-missing-field-initializers
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

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
    src/modsfilter.cpp \
    src/modlist.cpp \
    src/logpanel.cpp \
    src/itemsmanagerworker.cpp \
    src/updatechecker.cpp \
    src/autoonline.cpp \
    src/filesystem.cpp \
    src/logchecker.cpp \
    src/shoptemplatemanager.cpp \
    forms/settingspane.cpp \
    forms/recipepane.cpp \
    forms/currencypane.cpp \
    src/external/qcustomplot.cpp \
    forms/templatedialog.cpp \
    src/shops/shopsubmitter.cpp

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
    src/rapidjson_util.h \
    src/modsfilter.h \
    src/modlist.h \
    src/logpanel.h \
    src/itemsmanagerworker.h \
    src/updatechecker.h \
    src/autoonline.h \
    src/filesystem.h \
    src/logchecker.h \
    src/shoptemplatemanager.h \
    forms/settingspane.h \
    forms/recipepane.h \
    forms/currencypane.h \
    src/external/boolinq.h \
    src/external/qcustomplot.h \
    forms/templatedialog.h \
    src/external/syntaxhighlighter.h \
    src/shops/shopsubmitter.h

FORMS += \
    forms/mainwindow.ui \
    forms/logindialog.ui \
    forms/settingspane.ui \
    forms/recipepane.ui \
    forms/currencypane.ui \
    forms/templatedialog.ui

CONFIG += c++11

DEPENDPATH *= $${INCLUDEPATH}

RESOURCES += resources.qrc

RC_FILE = resources.rc
