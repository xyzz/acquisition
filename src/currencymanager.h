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

#pragma once

#include <QtGui>
#include <QtWidgets>

#include "application.h"

struct CurrencyItem {
    int count;
    std::string name;
    double exalt;
    double base;
};

// For now we just serialize/deserialize 'value' inside CurrencyManager
// Later we might need more logic if GGG adds more currency types and we want to be backwards compatible
struct CurrencyUpdate {
    long long timestamp;
    std::string value;
};

const std::vector<std::string> CurrencyForWisdom({
    "Scroll of Wisdom",
    "Portal Scroll",
    "Armourer's Scrap",
    "Blacksmith's Whetstone",
    "Orb of Transmutation"
});

const std::vector<int> CurrencyWisdomValue({
    1,
    1,
    2,
    4,
    4
});

class CurrencyManager;

class CurrencyDialog : public QDialog
{
    Q_OBJECT
public:
    CurrencyDialog(CurrencyManager &manager);
public slots:
    void Update();
private:
    CurrencyManager &currency_manager_;
    QGridLayout *layout_;
    std::vector<QLabel *> names_;
    std::vector<QDoubleSpinBox *> values_;
    std::vector<QDoubleSpinBox *> base_values_;
    QDoubleSpinBox *total_value_;
    QDoubleSpinBox *total_wisdom_value_;
    QSignalMapper *mapper;
    void UpdateTotalExaltedValue();
    void UpdateTotalWisdomValue();
private slots:
    void OnBaseValueChanged(int index);
};

class CurrencyManager : public QWidget
{
    Q_OBJECT
public:
    explicit CurrencyManager(Application &app);
    ~CurrencyManager();
    void ClearCurrency();
    // Called in itemmanagerworker::ParseItem
    void ParseSingleItem(const Item &item);
    void UpdateBaseValue(int ind, double value);
    const std::vector<CurrencyItem> &currencies() const { return currencies_;}
    double TotalExaltedValue();
    int TotalWisdomValue();
    void DisplayCurrency();
    void Update();
    // CSV export
    void ExportCurrency();

private:
    Application &app_;
    DataStore &data_;
    std::vector<CurrencyItem> currencies_;
    // We only need the "count" of a CurrencyItem so int will be enough
    std::vector<int> wisdoms_;
    std::unique_ptr<CurrencyDialog> dialog_;
    // database interaction
    void InitCurrency();
    void LoadCurrency();
    void SaveCurrencyBase();

public slots:
    void UpdateExaltedValue();
    void SaveCurrencyValue();
};
