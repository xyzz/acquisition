/*
    Copyright 2015 Guillaume DUPUY <glorf@glorf.fr>

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
#include <QWidget>
#include <QtGui>
#include <QDialog>
#include "application.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "currencymanager.h"
#include "itemsmanager.h"
#include "item.h"
CurrencyManager::CurrencyManager(Application &app) : app_(app), data_(app.data_manager())
{
    if (data_.Get("currency_base") == "")
        InitCurrency();
    LoadCurrency();
    connect(&app_.items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(UpdateExaltedValue()));
}

CurrencyManager::~CurrencyManager()
{
    SaveCurrency();
}

void CurrencyManager::UpdateBaseValue(int ind, double value)
{
    currencys_[ind].base = value;
}

void CurrencyManager::UpdateExaltedValue()
{
    for( auto &currency : currencys_) {
        if (currency.base != 0)
            currency.exalt = currency.count / currency.base;
    }
}

double CurrencyManager::TotalExaltedValue()
{
    double out = 0;
    for (auto currency : currencys_) {
        out += currency.exalt;
    }
    return out;
}

void CurrencyManager::ClearCurrency()
{
    for( auto &currency : currencys_) {
        currency.count = 0;
    }
}

void CurrencyManager::InitCurrency()
{
    std::string value = "";
    for (auto curr : CurrencyAsString) {
        value += "10;";
    }
    value.pop_back();//Remove the last ";"
    data_.Set("currency_base", value);
}

void CurrencyManager::LoadCurrency()
{
    //Way easier to use QT function instead of re-implementing a split() in C++
    QStringList list = QString(data_.Get("currency_base").c_str()).split(";");
    for (int i=0; i < list.size(); i++) {
        CurrencyItem curr;
        curr.count = 0;
        curr.name = CurrencyAsString[i];
        //We can't use the toDouble function from QT, because it encodes a double with a ","
        //Might be related to localisation issue, but anyway it's safer this way
        curr.base = std::stod(list[i].toStdString());
        curr.exalt = 0;
        currencys_.push_back(curr);
    }

}

void CurrencyManager::SaveCurrency()
{
    std::string value = "";
    for (auto currency : currencys_) {
        value += std::to_string(currency.base) + ";";
    }
    value.pop_back();//Remove the last ";"
    data_.Set("currency_base", value);
}

void CurrencyManager::ParseSingleItem(std::shared_ptr<Item> item)
{
    for (unsigned int i=0; i < currencys_.size(); i++) {
        if (item->PrettyName() == currencys_[i].name) {
            currencys_[i].count += item->count();
        }
    }
}

void CurrencyManager::DisplayCurrency()
{
    dialog_ = new DialogCurrency(*this);
    connect(&app_.items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        dialog_, SLOT(Update()));
}

DialogCurrency::DialogCurrency(CurrencyManager &manager) : m_(manager)
{
    mapper = new QSignalMapper;
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(OnBaseValueChanged(int)));
    layout_ = new QGridLayout;
    layout_->addWidget(new QLabel("Name"), 0, 0);
    layout_->addWidget(new QLabel("Exalted value"), 0, 1);
    layout_->addWidget(new QLabel("Base exalted value"), 0, 2);
    for (auto curr : m_.currencys()) {
        std::string text = curr.name + " (" + std::to_string(curr.count) + ")";
        QLabel *tmpName = new QLabel(text.c_str());
        QDoubleSpinBox *tmpValue = new QDoubleSpinBox;
        tmpValue->setMaximum(100000);
        tmpValue->setValue(curr.exalt);
        tmpValue->setEnabled(false);
        QDoubleSpinBox *tmpBaseValue = new QDoubleSpinBox;
        tmpBaseValue->setMaximum(100000);
        tmpBaseValue->setValue(curr.base);
        names_.push_back(tmpName);
        values_.push_back(tmpValue);
        base_values_.push_back(tmpBaseValue);
        int curr_row = layout_->rowCount() +1;
        //To keep every vector the same size, we DO create spinboxes for the empty currency, just don't display them
        if (curr.name == "")
            continue;
        layout_->addWidget(names_.back(),curr_row , 0);
        layout_->addWidget(values_.back(), curr_row, 1);
        layout_->addWidget(base_values_.back(), curr_row, 2);
        mapper->setMapping(base_values_.back(), base_values_.size() - 1);
        connect(base_values_.back(), SIGNAL(valueChanged(double)), mapper, SLOT(map()));
    }
    int curr_row = layout_->rowCount();
    layout_->addWidget(new QLabel("Total exalted value"), curr_row, 0);
    total_value_ = new QDoubleSpinBox;
    total_value_->setMaximum(100000);
    total_value_->setEnabled(false);
    UpdateTotalExaltedValue();
    layout_->addWidget(total_value_, curr_row, 1);
    button_close_ = new QPushButton("Close");
    layout_->addWidget(button_close_, curr_row + 1, 0);
    connect(button_close_, SIGNAL(clicked()), this, SLOT(close()));
    setLayout(layout_);
    show();
}

void DialogCurrency::OnBaseValueChanged(int index)
{
    m_.UpdateBaseValue(index, base_values_[index]->value());
    m_.UpdateExaltedValue();
    values_[index]->setValue(m_.currencys()[index].exalt);
    UpdateTotalExaltedValue();
}

void DialogCurrency::Update()
{
    for (unsigned int i=0; i < values_.size(); i++) {
        CurrencyItem curr = m_.currencys()[i];
        std::string text = curr.name + " (" + std::to_string(curr.count) + ")";
        names_[i]->setText(text.c_str());
        values_[i]->setValue(curr.exalt);
    }
    UpdateTotalExaltedValue();
}

void DialogCurrency::UpdateTotalExaltedValue()
{
    total_value_->setValue(m_.TotalExaltedValue());
}
