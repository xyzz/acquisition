#pragma once

#include <QObject>
#include <QScrollArea>

// http://qt-project.org/forums/viewthread/13728

class VerticalScrollArea : public QScrollArea {
    Q_OBJECT
public:
    explicit VerticalScrollArea(QWidget *parent = 0);
    virtual bool eventFilter(QObject *o, QEvent *e);
};
