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

#include "tabbuyoutsdialog.h"
#include "ui_tabbuyoutsdialog.h"

#include <algorithm>
#include <vector>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QComboBox>
#include <QSignalMapper>

#include "application.h"
#include "buyoutmanager.h"
#include "shop.h"
#include "util.h"

TabBuyoutsDialog::TabBuyoutsDialog(QWidget *parent, Application *app) :
    QDialog(parent),
    app_(app),
    ui(new Ui::TabBuyoutsDialog),
    signal_mapper_(nullptr)
{
    ui->setupUi(this);
    ui->tableWidget->verticalHeader()->hide();
    ui->tableWidget->horizontalHeader()->hide();
}

TabBuyoutsDialog::~TabBuyoutsDialog() {
    delete ui;
}

void TabBuyoutsDialog::on_pushButton_clicked() {
    app_->buyout_manager()->Save();
    app_->shop()->ExpireShopData();
    hide();
}

enum {
    TAB_CAPTION,
    TAB_BUYOUT_TYPE,
    TAB_BUYOUT_CURRENCY,
    TAB_BUYOUT_VALUE
};

void TabBuyoutsDialog::Populate() {
    if (signal_mapper_)
        delete signal_mapper_;
    signal_mapper_ = new QSignalMapper;

    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(4);
    tab_titles_.clear();
    for (auto &tab : app_->tabs()) {
        if (!std::count(tab_titles_.begin(), tab_titles_.end(), tab))
            tab_titles_.push_back(tab);
    }

    ui->tableWidget->setRowCount(tab_titles_.size());
    for (size_t i = 0; i < tab_titles_.size(); ++i) {
        const auto &tab = tab_titles_[i];
        auto item = new QTableWidgetItem(tab.c_str());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, TAB_CAPTION, item);

        auto buyout_type_combobox = new QComboBox;
        Util::PopulateBuyoutTypeComboBox(buyout_type_combobox);
        auto buyout_currency_combobox = new QComboBox;
        Util::PopulateBuyoutCurrencyComboBox(buyout_currency_combobox);
        ui->tableWidget->setCellWidget(i, TAB_BUYOUT_TYPE, buyout_type_combobox);
        ui->tableWidget->setCellWidget(i, TAB_BUYOUT_CURRENCY, buyout_currency_combobox);

        auto buyout_value = new QLineEdit;
        ui->tableWidget->setCellWidget(i, TAB_BUYOUT_VALUE, buyout_value);

        signal_mapper_->setMapping(buyout_type_combobox, i);
        signal_mapper_->setMapping(buyout_currency_combobox, i);
        signal_mapper_->setMapping(buyout_value, i);

        connect(buyout_type_combobox, SIGNAL(currentIndexChanged(int)),
                signal_mapper_, SLOT(map()));
        connect(buyout_currency_combobox, SIGNAL(currentIndexChanged(int)),
                signal_mapper_, SLOT(map()));
        connect(buyout_value, SIGNAL(textChanged(QString)),
                signal_mapper_, SLOT(map()));

        if (app_->buyout_manager()->ExistsTab("stash:" + tab)) {
            Buyout bo = app_->buyout_manager()->GetTab("stash:" + tab);
            buyout_type_combobox->setCurrentIndex(bo.type);
            buyout_currency_combobox->setCurrentIndex(bo.currency);
            buyout_value->setText(QString::number(bo.value));
        } else {
            buyout_currency_combobox->setEnabled(false);
            buyout_value->setEnabled(false);
        }
    }
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->resizeRowsToContents();
    ui->tableWidget->setColumnWidth(3, 250);

    connect(signal_mapper_, SIGNAL(mapped(int)), this, SLOT(OnBuyoutChanged(int)));
}

void TabBuyoutsDialog::OnBuyoutChanged(int row) {
    QComboBox *buyout_type_combobox = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(row, TAB_BUYOUT_TYPE));
    QComboBox *buyout_currency_combobox = qobject_cast<QComboBox*>(ui->tableWidget->cellWidget(row, TAB_BUYOUT_CURRENCY));
    QLineEdit *buyout_value = qobject_cast<QLineEdit*>(ui->tableWidget->cellWidget(row, TAB_BUYOUT_VALUE));

    switch (buyout_type_combobox->currentIndex()) {
    case BUYOUT_TYPE_NONE:
        buyout_currency_combobox->setCurrentIndex(CURRENCY_NONE);
        buyout_currency_combobox->setEnabled(false);
        buyout_value->setText("");
        buyout_value->setEnabled(false);
        app_->buyout_manager()->DeleteTab("stash:" + tab_titles_[row]);
        break;
    default:
        buyout_currency_combobox->setEnabled(true);
        buyout_value->setEnabled(true);
        Buyout bo;
        bo.type = static_cast<BuyoutType>(buyout_type_combobox->currentIndex());
        bo.currency = static_cast<Currency>(buyout_currency_combobox->currentIndex());
        bo.value = buyout_value->text().toDouble();
        app_->buyout_manager()->SetTab("stash:" + tab_titles_[row], bo);
        break;
    }
}
