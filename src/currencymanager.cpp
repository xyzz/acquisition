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

#include <ctime>
#include <QWidget>
#include <QtGui>
#include "QsLog.h"

#include "application.h"
#include "datastore.h"
#include "buyoutmanager.h"
#include "currencymanager.h"
#include "itemsmanager.h"
#include "item.h"
#include "porting.h"
#include "util.h"

CurrencyManager::CurrencyManager(Application &app):
    app_(app),
    data_(app.data())
{
    if (data_.Get("currency_base", "").empty())
        InitCurrency();
    LoadCurrency();
    dialog_ = std::make_unique<CurrencyDialog>(*this);
}

CurrencyManager::~CurrencyManager() {
    SaveCurrencyBase();
}

void CurrencyManager::Update() {
    ClearCurrency();
    for (auto &item : app_.items_manager().items()) {
        ParseSingleItem(*item);
    }
    UpdateExaltedValue();
    SaveCurrencyValue();
    dialog_->Update();
}

void CurrencyManager::UpdateBaseValue(int ind, double value) {
    currencies_[ind].base = value;
}

const double EPS = 1e-6;

void CurrencyManager::UpdateExaltedValue() {
    for (auto &currency : currencies_) {
        if (fabs(currency.base) > EPS)
            currency.exalt = currency.count / currency.base;
        else
            currency.exalt = 0;
    }
}

double CurrencyManager::TotalExaltedValue() {
    double out = 0;
    for (const auto &currency : currencies_) {
        out += currency.exalt;
    }
    return out;
}

int CurrencyManager::TotalWisdomValue() {
    int out = 0;
    for (unsigned int i = 0; i < wisdoms_.size(); i++) {
        out += wisdoms_[i] * CurrencyWisdomValue[i];
    }
    return out;
}

void CurrencyManager::ClearCurrency() {
    for (auto& currency : currencies_) {
        currency.count = 0;
    }
    for (auto& wisdom : wisdoms_) {
        wisdom = 0;
    }
}

void CurrencyManager::InitCurrency() {
    std::string value = "";
    for (auto curr : CurrencyAsString) {
        value += "0;";
    }
    value.pop_back(); // Remove the last ";"
    data_.Set("currency_base", value);
    data_.Set("currency_last_value", value);
}

void CurrencyManager::LoadCurrency() {
    std::vector<std::string> list = Util::StringSplit(data_.Get("currency_base"), ';');
    for (unsigned int i = 0; i < list.size(); i++) {
        CurrencyItem curr;
        curr.count = 0;
        curr.name = CurrencyAsString[i];
        // We can't use the toDouble function from QT, because it encodes a double with a ","
        // Might be related to localisation issue, but anyway it's safer this way
        curr.base = std::stod(list[i]);
        curr.exalt = 0;
        currencies_.push_back(curr);
    }
    for (unsigned int i = 0; i < CurrencyWisdomValue.size(); i++) {
        wisdoms_.push_back(0);
    }
}

void CurrencyManager::SaveCurrencyBase() {
    std::string value = "";
    for (auto currency : currencies_) {
        value += std::to_string(currency.base) + ";";
    }
    value.pop_back(); // Remove the last ";"
    data_.Set("currency_base", value);
}

void CurrencyManager::SaveCurrencyValue() {
    std::string value = "";
    // Useless to save if every count is 0.
    bool empty = true;
    value = std::to_string(TotalExaltedValue());
    for (auto currency : currencies_) {
        if (currency.name != "")
            value += ";" + std::to_string(currency.count);
        if (currency.count != 0)
            empty = false;
    }
    std::string old_value = data_.Get("currency_last_value", "");
    if (value != old_value && !empty) {
        CurrencyUpdate update = CurrencyUpdate();
        update.timestamp = std::time(nullptr);
        update.value = value;
        data_.InsertCurrencyUpdate(update);
        data_.Set("currency_last_value", value);
    }
}

void CurrencyManager::ExportCurrency() {
    std::string header_csv = "Date; Total value";
    for (auto& name : CurrencyAsString) {
        if (name != "")
            header_csv += ";" + name;
    }
    std::vector<CurrencyUpdate> result = data_.GetAllCurrency();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Export file"),
                                                    QDir::toNativeSeparators(QDir::homePath() + "/" + "acquisition_export_currency.csv"));
    if (fileName.isEmpty())
        return;
    QFile file(QDir::toNativeSeparators(fileName));
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << header_csv.c_str() << "\n";
        for (auto &update : result) {
            char buf[4096];
            std::time_t timestamp = update.timestamp;
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&timestamp));
            out << buf << ";";
            out << update.value.c_str() << "\n";
        }
    } else {
        QLOG_WARN() << "CurrencyManager::ExportCurrency : couldn't open CSV export file ";
    }
}

void CurrencyManager::ParseSingleItem(const Item &item) {
    for (unsigned int i = 0; i < currencies_.size(); i++)
        if (item.PrettyName() == currencies_[i].name)
            currencies_[i].count += item.count();

    for (unsigned int i = 0; i < wisdoms_.size(); i++)
        if (item.PrettyName() == CurrencyForWisdom[i])
            wisdoms_[i] += item.count();
}

void CurrencyManager::DisplayCurrency() {
    dialog_->show();
}

CurrencyDialog::CurrencyDialog(CurrencyManager& manager) : currency_manager_(manager) {
    mapper = new QSignalMapper;
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(OnBaseValueChanged(int)));
    layout_ = new QGridLayout;
    layout_->addWidget(new QLabel("Name"), 0, 0);
    layout_->addWidget(new QLabel("Value in Exalted Orbs"), 0, 1);
    layout_->addWidget(new QLabel("Amount an Exalted Orb can buy"), 0, 2);
    for (auto curr : currency_manager_.currencies()) {
        std::string text = curr.name + " (" + std::to_string(curr.count) + ")";
        QLabel* tmpName = new QLabel(text.c_str());
        QDoubleSpinBox* tmpValue = new QDoubleSpinBox;
        tmpValue->setMaximum(100000);
        tmpValue->setValue(curr.exalt);
        tmpValue->setEnabled(false);
        QDoubleSpinBox* tmpBaseValue = new QDoubleSpinBox;
        tmpBaseValue->setMaximum(100000);
        tmpBaseValue->setValue(curr.base);
        names_.push_back(tmpName);
        values_.push_back(tmpValue);
        base_values_.push_back(tmpBaseValue);
        int curr_row = layout_->rowCount() + 1;
        // To keep every vector the same size, we DO create spinboxes for the empty currency, just don't display them
        if (curr.name == "")
            continue;
        layout_->addWidget(names_.back(), curr_row, 0);
        layout_->addWidget(values_.back(), curr_row, 1);
        layout_->addWidget(base_values_.back(), curr_row, 2);
        mapper->setMapping(base_values_.back(), base_values_.size() - 1);
        connect(base_values_.back(), SIGNAL(valueChanged(double)), mapper, SLOT(map()));
    }
    int curr_row = layout_->rowCount();
    layout_->addWidget(new QLabel("Total Exalted Orbs"), curr_row, 0);
    total_value_ = new QDoubleSpinBox;
    total_value_->setMaximum(100000);
    total_value_->setEnabled(false);
    UpdateTotalExaltedValue();
    layout_->addWidget(total_value_, curr_row, 1);
    total_wisdom_value_ = new QDoubleSpinBox;
    total_wisdom_value_->setMaximum(100000);
    total_wisdom_value_->setEnabled(false);
    UpdateTotalWisdomValue();
    layout_->addWidget(new QLabel("Total Scrolls of Wisdom"), curr_row + 1, 0);
    layout_->addWidget(total_wisdom_value_, curr_row + 1, 1);
#if defined(Q_OS_LINUX)
    setWindowFlags(Qt::WindowCloseButtonHint);
#endif
    setLayout(layout_);
}

void CurrencyDialog::OnBaseValueChanged(int index) {
    currency_manager_.UpdateBaseValue(index, base_values_[index]->value());
    currency_manager_.UpdateExaltedValue();
    values_[index]->setValue(currency_manager_.currencies()[index].exalt);
    UpdateTotalExaltedValue();
}

void CurrencyDialog::Update() {
    for (unsigned int i = 0; i < values_.size(); i++) {
        CurrencyItem curr = currency_manager_.currencies()[i];
        std::string text = curr.name + " (" + std::to_string(curr.count) + ")";
        names_[i]->setText(text.c_str());
        values_[i]->setValue(curr.exalt);
    }
    UpdateTotalExaltedValue();
    UpdateTotalWisdomValue();
}

void CurrencyDialog::UpdateTotalExaltedValue() {
    total_value_->setValue(currency_manager_.TotalExaltedValue());
}

void CurrencyDialog::UpdateTotalWisdomValue() {
    total_wisdom_value_->setValue(currency_manager_.TotalWisdomValue());
}
