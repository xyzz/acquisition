/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QObject>
#include "QsLogDest.h"

#include <vector>

class MainWindow;
class QPushButton;
class QTextEdit;

namespace Ui {
    class MainWindow;
}

class LogPanel;

class LogPanelSignalHandler : public QObject {
    Q_OBJECT
public:
    LogPanelSignalHandler(LogPanel &parent) :
        parent_(parent)
    {}
public slots:
    void OnStatusLabelClicked();
    void OnMessage(const QString &message, QsLogging::Level level);
private:
    LogPanel &parent_;
};

class LogPanel : public QsLogging::Destination {
    friend class LogPanelSignalHandler;
public:
    LogPanel(MainWindow *window, Ui::MainWindow *ui);
    virtual void write(const QString& message, QsLogging::Level level);
    virtual bool isValid() { return true; }
private:
    void AddLine(const QString &message, QsLogging::Level level);
    void UpdateStatusLabel();
    void ToggleOutputVisibility();

    QPushButton *status_button_;
    QTextEdit *output_;
    std::vector<int> num_messages_;
    LogPanelSignalHandler signal_handler_;
};
