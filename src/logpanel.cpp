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

#include "logpanel.h"

#include <QObject>
#include <QPushButton>
#include <QTextEdit>
#include <QStatusBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"

LogPanel::LogPanel(MainWindow *window, Ui::MainWindow *ui):
    status_button_(new QPushButton),
    output_(new QTextEdit),
    num_errors_(0),
    num_warnings_(0),
    signal_handler_(*this)
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    output_->setReadOnly(true);
    output_->setFont(font);
    output_->setMaximumHeight(250); // TODO(xyz): remove magic numbers
    output_->insertPlainText("Errors and warnings will be printed here\n");
    output_->hide();

    status_button_->setFlat(true);
    window->statusBar()->addPermanentWidget(status_button_);
    UpdateStatusLabel();
    QObject::connect(status_button_, SIGNAL(clicked()), &signal_handler_, SLOT(OnStatusLabelClicked()));

    ui->mainLayout->addWidget(output_);
}

void LogPanel::UpdateStatusLabel() {
    QString stylesheet;
    QString text;
    if (num_errors_ > 0) {
        text = QString("%1 error").arg(num_errors_) + (num_errors_ > 1 ? "s" : "");
        stylesheet = "color: red; font-weight: bold;";
    } else if (num_warnings_ > 0) {
        text = QString("%1 warning").arg(num_warnings_) + (num_warnings_ > 1 ? "s" : "");
        stylesheet = "color: rgb(174, 141, 28); font-weight: bold;";
    } else {
        text = "Event Log";
    }

    status_button_->setStyleSheet(stylesheet);
    status_button_->setText(text);
}

void LogPanel::write(const QString& message, QsLogging::Level level) {
    QColor color;
    switch (level) {
    case QsLogging::ErrorLevel:
        color = QColor(255, 0, 0);
        ++num_errors_;
        break;
    case QsLogging::WarnLevel:
        color = QColor(174, 141, 28);
        ++num_warnings_;
        break;
    default:
        return;
    }
    output_->setTextColor(color);
    output_->insertPlainText(message + "\n");
    output_->ensureCursorVisible();

    if (output_->isVisible()) {
        num_errors_ = 0;
        num_warnings_ = 0;
    }
    UpdateStatusLabel();
}

void LogPanelSignalHandler::OnStatusLabelClicked() {
    parent_.ToggleOutputVisibility();
}

void LogPanel::ToggleOutputVisibility() {
    if (output_->isVisible()) {
        output_->hide();
    } else {
        output_->show();
        num_errors_ = 0;
        num_warnings_ = 0;
        UpdateStatusLabel();
    }
}
