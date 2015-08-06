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
#include <QWidget>
#include <QtGui>

#include <QtWidgets>
#include "item.h"
#include "itemsmanagerworker.h"
#include "buyoutmanager.h"
struct CurrencyItem {
    int count;
    std::string name;
    double exalt;
    double base;
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

class DialogCurrency : public QDialog
{
    Q_OBJECT
public:
    DialogCurrency(CurrencyManager &manager);
public slots:
    void Update();
private:
    CurrencyManager &m_;
    QGridLayout *layout_;
    QPushButton *button_close_;
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
    CurrencyManager(Application &app);
    ~CurrencyManager();
    void ClearCurrency();
    //Called in itemmanagerworker::ParseItem
    void ParseSingleItem(std::shared_ptr<Item> item);
    void UpdateBaseValue(int ind, double value);
    std::vector<CurrencyItem> currencys() const { return currencys_;}
    double TotalExaltedValue();
    int TotalWisdomValue();
    void DisplayCurrency();

private:
    Application &app_;
    DataManager &data_;
    std::vector<CurrencyItem> currencys_;
    //We only need the "count" of a CurrencyItem so int will be enough
    std::vector<int> wisdoms_;
    DialogCurrency *dialog_;
    //database interaction
    void InitCurrency();
    void LoadCurrency();
    void SaveCurrency();

public slots:
    void UpdateExaltedValue();
};
