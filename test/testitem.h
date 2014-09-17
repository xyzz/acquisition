#pragma once

#include <QtTest/QtTest>

class TestItem : public QObject
{
    Q_OBJECT
private slots:
    void Parse();
};
