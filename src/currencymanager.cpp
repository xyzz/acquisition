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
#include <ctime>
#include "QsLog.h"
#include "application.h"
#include "datamanager.h"
#include "buyoutmanager.h"
#include "currencymanager.h"
#include "itemsmanager.h"
#include "item.h"
CurrencyManager::CurrencyManager(Application &app) : app_(app), data_(app.data_manager())
{
    if (data_.Get("currency_base", "", "data") == "")
        InitCurrency();
    LoadCurrency();
    connect(&app_.items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(UpdateExaltedValue()));
    connect(&app_.items_manager(), SIGNAL(ItemsRefreshed(Items, std::vector<std::string>)),
        this, SLOT(SaveCurrencyValue()));
}

CurrencyManager::~CurrencyManager()
{
    SaveCurrencyBase();
}

void CurrencyManager::Update()
{
    ClearCurrency();
    for (auto &item : app_.items()) {
        ParseSingleItem(item);
    }
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

int CurrencyManager::TotalWisdomValue()
{
    int out = 0;
    for (unsigned int i=0; i < wisdoms_.size(); i++) {
        out += wisdoms_[i] * CurrencyWisdomValue[i];
    }
    return out;
}

void CurrencyManager::ClearCurrency()
{
    for(auto &currency : currencys_) {
        currency.count = 0;
    }
    for( auto &wisdom : wisdoms_) {
        wisdom = 0;
    }
}

void CurrencyManager::InitCurrency()
{
    std::string value = "";
    std::string header_csv = "";
    for (auto curr : CurrencyAsString) {
        value += "0;";
        if (curr != "")
            header_csv += curr + ";";
    }
    value.pop_back();//Remove the last ";"
    header_csv += "Total value; Timestamp";
    data_.Set("currency_base", value, "data");
    data_.Set("currency_last_value",value, "data");
    //Create header for .csv
    QFile file("export_currency.csv");
    if (file.open(QFile::WriteOnly | QFile::Truncate)){
        QTextStream out(&file);
        out  << header_csv.c_str();
    }
    else {
        QLOG_WARN() << "CurrencyManager::ExportCurrency : couldn't open CSV export file ";
    }
}

void CurrencyManager::LoadCurrency()
{
    //Way easier to use QT function instead of re-implementing a split() in C++
    QStringList list = QString(data_.Get("currency_base", "", "data").c_str()).split(";");
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
    for (unsigned int i=0; i < CurrencyWisdomValue.size(); i++)
    {
        wisdoms_.push_back(0);
    }

}

void CurrencyManager::SaveCurrencyBase()
{
    std::string value = "";
    for (auto currency : currencys_) {
        value += std::to_string(currency.base) + ";";
    }
    value.pop_back();//Remove the last ";"
    data_.Set("currency_base", value, "data");
}
void CurrencyManager::SaveCurrencyValue()
{
    std::string value = "";
    //Useless to save if every count is 0.
    bool empty = true;
    for (auto currency : currencys_) {
        if (currency.name !="")
            value += std::to_string(currency.count) + ";";
        if (currency.count !=0)
            empty = false;
    }
    value += std::to_string(TotalExaltedValue());
    std::string old_value = data_.Get("currency_last_value", "", "data");
    if (value != old_value && !empty){
        std::string timestamp = std::to_string(std::time(nullptr));
        data_.Set(timestamp, value, "currency");
        data_.Set("currency_last_value", value, "data");
        ExportCurrency(value + ";" + timestamp);
    }
}

void CurrencyManager::ExportCurrency(std::string value)
{
    QFile file("export_currency.csv");
    if (file.open(QFile::Append | QFile::Text)){
        QTextStream out(&file);
        out << "\n" << value.c_str();
    }
    else {
        QLOG_WARN() << "CurrencyManager::ExportCurrency : couldn't open CSV export file ";
    }
}

void CurrencyManager::ParseSingleItem(std::shared_ptr<Item> item)
{
    for (unsigned int i=0; i < currencys_.size(); i++) {
        if (item->PrettyName() == currencys_[i].name) {
            currencys_[i].count += item->count();
        }
    }
    for (unsigned int i=0; i < wisdoms_.size(); i++) {
        if (item->PrettyName() == CurrencyForWisdom[i]) {
            wisdoms_[i] += item->count();
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
    total_wisdom_value_ = new QDoubleSpinBox;
    total_wisdom_value_->setMaximum(100000);
    total_wisdom_value_->setEnabled(false);
    UpdateTotalWisdomValue();
    layout_->addWidget(new QLabel("Total wisdom scrolls"), curr_row + 1, 0);
    layout_->addWidget(total_wisdom_value_, curr_row + 1, 1);
    button_close_ = new QPushButton("Close");
    layout_->addWidget(button_close_, curr_row + 2, 0);
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
    UpdateTotalWisdomValue();
}

void DialogCurrency::UpdateTotalExaltedValue()
{
    total_value_->setValue(m_.TotalExaltedValue());
}

void DialogCurrency::UpdateTotalWisdomValue()
{
    total_wisdom_value_->setValue(m_.TotalWisdomValue());
}
