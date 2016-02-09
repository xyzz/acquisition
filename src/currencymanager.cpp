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
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"
#include "rapidjson_util.h"


CurrencyManager::CurrencyManager(Application &app):
    app_(app),
    data_(app.data())
{
    if (data_.Get("currency_items", "").empty()) {

        FirstInitCurrency();
        if (!data_.Get("currency_base", "").empty()) {
            MigrateCurrency();
            QLOG_INFO() << "Found old currency values, migrated them to the new system";
        }
    }
    else
        InitCurrency();

    dialog_ = std::make_shared<CurrencyDialog>(*this);
}

CurrencyManager::~CurrencyManager() {
    SaveCurrencyItems();
}

void CurrencyManager::Update() {
    ClearCurrency();
    for (auto &item : app_.items_manager().items()) {
        ParseSingleItem(*item);
    }
    SaveCurrencyValue();
    dialog_->Update();
}

const double EPS = 1e-6;
double CurrencyManager::TotalExaltedValue() {
    double out = 0;
    for (const auto &currency : currencies_) {
        if (fabs(currency->exalt.value1) > EPS)
            out += currency->count / currency->exalt.value1;
    }
    return out;
}

double CurrencyManager::TotalChaosValue() {
    double out = 0;
    for (const auto &currency : currencies_) {
        if (fabs(currency->chaos.value1) > EPS)
            out += currency->count / currency->chaos.value1;
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
        currency->count = 0;
    }
    for (auto& wisdom : wisdoms_) {
        wisdom = 0;
    }
}
void CurrencyManager::InitCurrency() {
    Deserialize(data_.Get("currency_items"), &currencies_);
    for (unsigned int i = 0; i < CurrencyWisdomValue.size(); i++) {
        wisdoms_.push_back(0);
    }
}

void CurrencyManager::FirstInitCurrency() {
    std::string value = "";
    //Dummy items + dummy currency_last_value
    //TODO : can i get the size of the Currency enum ??
    for(unsigned int i = 0; i < CurrencyAsString.size();i++) {
        currencies_.push_back(std::make_shared<CurrencyItem>(0, static_cast<Currency>(i), 1, 1));

        value += "0;";
    }
    value.pop_back(); // Remove the last ";"
    data_.Set("currency_items", Serialize(currencies_));
    data_.Set("currency_last_value", value);
}

void CurrencyManager::MigrateCurrency() {
    std::vector<std::string> list = Util::StringSplit(data_.Get("currency_base"), ';');
        for (unsigned int i = 0; i < list.size(); i++) {
            // We can't use the toDouble function from QT, because it encodes a double with a ","
            // Might be related to localisation issue, but anyway it's safer this way
            currencies_[i]->exalt.value1 = std::stod(list[i]);
        }
    //Set to empty so we won't trigger it next time !
    data_.Set("currency_base", "");
}

std::string CurrencyManager::Serialize(const std::vector<std::shared_ptr<CurrencyItem>> &currencies) {
    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();
    for (auto &curr : currencies) {
        rapidjson::Value item(rapidjson::kObjectType);
        item.AddMember("count", curr->count, alloc);
        item.AddMember("chaos_ratio", curr->chaos.value1, alloc);
        item.AddMember("exalt_ratio", curr->exalt.value1, alloc);
        Util::RapidjsonAddConstString(&item, "currency", CurrencyAsTag[curr->currency], alloc);
        rapidjson::Value name(curr->name.c_str(), alloc);
        doc.AddMember(name, item, alloc);
    }
    return Util::RapidjsonSerialize(doc);
}

void CurrencyManager::Deserialize(const std::string &data, std::vector<std::shared_ptr<CurrencyItem> > *currencies) {
    //Maybe clear something would be good
    if (data.empty())
        return;
    rapidjson::Document doc;
    if (doc.Parse(data.c_str()).HasParseError()) {
        QLOG_ERROR() << "Error while parsing currency ratios.";
        QLOG_ERROR() << rapidjson::GetParseError_En(doc.GetParseError());
        return;
    }
    if (!doc.IsObject())
        return;
    for (auto itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
        auto &object = itr->value;
        Currency curr = static_cast<Currency>(Util::TagAsCurrency(object["currency"].GetString()));
        std::shared_ptr<CurrencyItem> item = std::make_shared<CurrencyItem>(object["count"].GetDouble(), curr,
                object["chaos_ratio"].GetDouble(), object["exalt_ratio"].GetDouble());
        currencies->push_back(item);
    }
}

void CurrencyManager::SaveCurrencyItems() {
    data_.Set("currency_items", Serialize(currencies_));
}

void CurrencyManager::SaveCurrencyValue() {
    std::string value = "";
    // Useless to save if every count is 0.
    bool empty = true;
    value = std::to_string(TotalExaltedValue());
    for (auto &currency : currencies_) {
        if (currency->name != "")
            value += ";" + std::to_string(currency->count);
        if (currency->count != 0)
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
        if (item.PrettyName() == currencies_[i]->name)
            currencies_[i]->count += item.count();

    for (unsigned int i = 0; i < wisdoms_.size(); i++)
        if (item.PrettyName() == CurrencyForWisdom[i])
            wisdoms_[i] += item.count();
}

void CurrencyManager::DisplayCurrency() {

    dialog_->show();
}


CurrencyWidget::CurrencyWidget(std::shared_ptr<CurrencyItem> currency)  :
    currency_(currency)
{
    name_ = new QLabel("");
    chaos_ratio_ = new QDoubleSpinBox;
    chaos_ratio_->setMaximum(100000);
    chaos_ratio_->setValue(currency->chaos.value1);
    chaos_value_ = new QDoubleSpinBox;
    chaos_value_->setMaximum(100000);
    chaos_value_->setEnabled(false);
    exalt_ratio_ = new QDoubleSpinBox;
    exalt_ratio_->setMaximum(100000);
    exalt_ratio_->setValue(currency->exalt.value1);
    exalt_value_ = new QDoubleSpinBox;
    exalt_value_->setMaximum(100000);
    exalt_value_->setEnabled(false);
    Update();
    connect(chaos_ratio_, SIGNAL(valueChanged(double)), this, SLOT(Update()));
    connect(exalt_ratio_, SIGNAL(valueChanged(double)), this, SLOT(Update()));
}



void CurrencyWidget::Update() {
    currency_->chaos.value1 = chaos_ratio_->value();
    currency_->exalt.value1 = exalt_ratio_->value();
    std::string text = currency_->name + "(" + std::to_string(currency_->count) + ")";
    name_->setText(text.c_str());
    chaos_value_->setValue(currency_->count / currency_->chaos.value1);
    exalt_value_->setValue(currency_->count / currency_->exalt.value1);
}

CurrencyDialog::CurrencyDialog(CurrencyManager& manager) : currency_manager_(manager) {
    mapper = new QSignalMapper;
    layout_ = new QGridLayout;
    layout_->addWidget(new QLabel("Name"), 0, 0);
    layout_->addWidget(new QLabel("Value in Exalted Orbs"), 0, 1);
    layout_->addWidget(new QLabel("Amount an Exalted Orb can buy"), 0, 2);
    layout_->addWidget(new QLabel("Value in Chaos Orbs"), 0, 3);
    layout_->addWidget(new QLabel("Amount a Chaos Orb can buy"), 0, 4);
    for (auto &curr : currency_manager_.currencies()) {
        CurrencyWidget* tmp = new CurrencyWidget(curr);
        int curr_row = layout_->rowCount() + 1;
        // To keep every vector the same size, we DO create spinboxes for the empty currency, just don't display them
        if (curr->currency == CURRENCY_NONE)
            continue;
        layout_->addWidget(tmp->name_, curr_row, 0);
        layout_->addWidget(tmp->exalt_value_, curr_row, 1);
        layout_->addWidget(tmp->exalt_ratio_, curr_row, 2);
        layout_->addWidget(tmp->chaos_value_, curr_row, 3);
        layout_->addWidget(tmp->chaos_ratio_, curr_row, 4);
        connect(tmp->exalt_ratio_, SIGNAL(valueChanged(double)), this, SLOT(UpdateTotalValue()));
        connect(tmp->chaos_ratio_, SIGNAL(valueChanged(double)), this, SLOT(UpdateTotalValue()));

        currencies_widgets_.push_back(tmp);

    }
    int curr_row = layout_->rowCount();
    layout_->addWidget(new QLabel("Total Exalted Orbs"), curr_row, 0);
    total_exalt_value_ = new QDoubleSpinBox;
    total_exalt_value_->setMaximum(100000);
    total_exalt_value_->setEnabled(false);
    layout_->addWidget(total_exalt_value_, curr_row, 1);
    layout_->addWidget(new QLabel("Total Chaos Orbs"), curr_row + 1, 0);
    total_chaos_value_ = new QDoubleSpinBox;
    total_chaos_value_->setMaximum(100000);
    total_chaos_value_->setEnabled(false);
    layout_->addWidget(total_chaos_value_, curr_row + 1, 1);

    total_wisdom_value_ = new QDoubleSpinBox;
    total_wisdom_value_->setMaximum(100000);
    total_wisdom_value_->setEnabled(false);
    UpdateTotalValue();
    UpdateTotalWisdomValue();
    layout_->addWidget(new QLabel("Total Scrolls of Wisdom"), curr_row + 2, 0);
    layout_->addWidget(total_wisdom_value_, curr_row + 2, 1);
#if defined(Q_OS_LINUX)
    setWindowFlags(Qt::WindowCloseButtonHint);
#endif
    setLayout(layout_);
}

void CurrencyDialog::Update() {
    for (auto widget : currencies_widgets_) {
        widget->Update();
    }
    UpdateTotalValue();
    UpdateTotalWisdomValue();
}

void CurrencyDialog::UpdateTotalValue() {
    total_exalt_value_->setValue(currency_manager_.TotalExaltedValue());
    total_chaos_value_->setValue(currency_manager_.TotalChaosValue());
}

void CurrencyDialog::UpdateTotalWisdomValue() {
    total_wisdom_value_->setValue(currency_manager_.TotalWisdomValue());
}
