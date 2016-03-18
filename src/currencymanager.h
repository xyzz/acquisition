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
#include "buyoutmanager.h"
struct CurrencyRatio {
    Currency curr1;
    Currency curr2;
    double value1;
    double value2;
    CurrencyRatio() :
        curr1(CURRENCY_NONE),
        curr2(CURRENCY_NONE),
        value1(0),
        value2(0)
    {}
    CurrencyRatio(Currency c1, Currency c2, double v1, double v2) :
        curr1(c1),
        curr2(c2),
        value1(v1),
        value2(v2)
    {}
};

struct CurrencyItem {
    int count;
    Currency currency;
    std::string name;
    CurrencyRatio exalt;
    CurrencyRatio chaos;
    CurrencyItem(int co, Currency curr, double chaos_ratio, double exalt_ratio) {
        count = co;
        currency = curr;
        name = curr.AsString();
        chaos = CurrencyRatio(currency, CURRENCY_CHAOS_ORB, chaos_ratio, 1);
        exalt = CurrencyRatio(currency, CURRENCY_EXALTED_ORB, exalt_ratio, 1);
    }

};
struct CurrencyLabels {
    QLabel *name;
    QLabel *count;
    QLabel *chaos_ratio;
    QLabel *chaos_value;
    QLabel *exalt_ratio;
    QLabel *exalt_value;
    QLabel *exalt_total;
    QLabel *chaos_total;
    QLabel *wisdom_total;
    CurrencyLabels() {
        name = new QLabel("Name");
        count = new QLabel("Count");
        chaos_ratio = new QLabel("Amount a chaos Orb can buy");
        chaos_value = new QLabel("Value in Chaos Orb");
        exalt_ratio = new QLabel("Amount an Exalted Orb can buy");
        exalt_value = new QLabel("Value in Exalted Orb");
        exalt_total = new QLabel("Total Exalted Orbs");
        chaos_total = new QLabel("Total Chaos Orbs");
        wisdom_total = new QLabel("Total Scrolls of Wisdom");
    }

};
class CurrencyDialog;
class CurrencyWidget : public QWidget
{
    Q_OBJECT
public slots:
    void Update();
    void UpdateVisual(bool show_chaos, bool show_exalt);
    bool IsNone() const { return currency_->currency.type==CURRENCY_NONE;}
public:
    CurrencyWidget(std::shared_ptr<CurrencyItem> currency);
    //Visual stuff
    QLabel *name;
    QLabel *count;
    QDoubleSpinBox *chaos_ratio;
    QDoubleSpinBox *chaos_value;
    QDoubleSpinBox *exalt_ratio;
    QDoubleSpinBox *exalt_value;

private:
    //Data
    std::shared_ptr<CurrencyItem> currency_;
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
    CurrencyDialog(CurrencyManager &manager, bool show_chaos, bool show_exalt);
    bool ShowChaos() const { return show_chaos_->isChecked();}
    bool ShowExalt() const { return show_exalt_->isChecked();}

public slots:
    void Update();
    void UpdateVisual();
    void UpdateVisibility(bool show_chaos, bool show_exalt);
    void UpdateTotalValue();
private:
    CurrencyManager &currency_manager_;
    std::vector<CurrencyWidget*> currencies_widgets_;
    CurrencyLabels *headers_;
    QVBoxLayout *layout_;
    QLabel *total_exalt_value_;
    QLabel *total_chaos_value_;
    QLabel *total_wisdom_value_;
    QCheckBox *show_chaos_;
    QCheckBox *show_exalt_;
    QFrame *separator_;
    QVBoxLayout* GenerateLayout(bool show_chaos, bool show_exalt);
    void UpdateTotalWisdomValue();
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
    //void UpdateBaseValue(int ind, double value);
    const std::vector<std::shared_ptr<CurrencyItem>> &currencies() const { return currencies_;}
    double TotalExaltedValue();
    double TotalChaosValue();
    int TotalWisdomValue();
    void DisplayCurrency();
    void Update();
    // CSV export
    void ExportCurrency();

private:
    Application &app_;
    DataStore &data_;
    std::vector<std::shared_ptr<CurrencyItem>> currencies_;
    // We only need the "count" of a CurrencyItem so int will be enough
    std::vector<int> wisdoms_;
    std::shared_ptr<CurrencyDialog> dialog_;
    // Used only the first time we launch the app
    void FirstInitCurrency();
    //Migrate from old storage (csv-like serializing) to new one (using json)
    void MigrateCurrency();
    void InitCurrency();
    void SaveCurrencyItems();
    std::string Serialize(const std::vector<std::shared_ptr<CurrencyItem>> &currencies);
    void Deserialize(const std::string &data, std::vector<std::shared_ptr<CurrencyItem>> *currencies);
    void Save();
public slots:
    void SaveCurrencyValue();
};
