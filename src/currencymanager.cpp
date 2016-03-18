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

    dialog_ = std::make_shared<CurrencyDialog>(*this, data_.GetBool("currency_show_chaos"), data_.GetBool("currency_show_exalt"));
}

CurrencyManager::~CurrencyManager() {
    Save();
}

void CurrencyManager::Save() {
    SaveCurrencyItems();
    SaveCurrencyValue();
    data_.SetBool("currency_show_chaos", dialog_->ShowChaos());
    data_.SetBool("currency_show_exalt", dialog_->ShowExalt());

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
    for(auto type: Currency::Types()) {
        currencies_.push_back(std::make_shared<CurrencyItem>(0, Currency(type), 1, 1));
    }
    Deserialize(data_.Get("currency_items"), &currencies_);
    for (unsigned int i = 0; i < CurrencyWisdomValue.size(); i++) {
        wisdoms_.push_back(0);
    }
}

void CurrencyManager::FirstInitCurrency() {
    std::string value = "";
    //Dummy items + dummy currency_last_value
    //TODO : can i get the size of the Currency enum ??
    for(auto type: Currency::Types()) {
        currencies_.push_back(std::make_shared<CurrencyItem>(0, Currency(type), 1, 1));

        value += "0;";
    }
    value.pop_back(); // Remove the last ";"
    data_.Set("currency_items", Serialize(currencies_));
    data_.Set("currency_last_value", value);
    data_.SetBool("currency_show_chaos", true);
    data_.SetBool("currency_show_exalt", true);
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
        Util::RapidjsonAddConstString(&item, "currency", curr->currency.AsTag(), alloc);
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
        Currency curr = Currency::FromTag(object["currency"].GetString());
        for (auto &item : *currencies) {
            if(item->currency == curr) {
                item = std::make_shared<CurrencyItem>(object["count"].GetDouble(), curr,
                        object["chaos_ratio"].GetDouble(), object["exalt_ratio"].GetDouble());
            }
        }
        //currencies->push_back(item);
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
    for (auto& item : currencies_) {
        auto label = item->currency.AsString();
        if (label != "")
            header_csv += ";" + label;
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
    name = new QLabel("");
    count = new QLabel("");
    chaos_ratio = new QDoubleSpinBox;
    chaos_ratio->setMaximum(100000);
    chaos_ratio->setValue(currency->chaos.value1);
    chaos_value = new QDoubleSpinBox;
    chaos_value->setMaximum(100000);
    chaos_value->setEnabled(false);
    exalt_ratio = new QDoubleSpinBox;
    exalt_ratio->setMaximum(100000);
    exalt_ratio->setValue(currency->exalt.value1);
    exalt_value = new QDoubleSpinBox;
    exalt_value->setMaximum(100000);
    exalt_value->setEnabled(false);
    Update();
    connect(chaos_ratio, SIGNAL(valueChanged(double)), this, SLOT(Update()));
    connect(exalt_ratio, SIGNAL(valueChanged(double)), this, SLOT(Update()));
}

void CurrencyWidget::UpdateVisual(bool show_chaos, bool show_exalt) {

    chaos_value->setVisible(show_chaos);
    chaos_ratio->setVisible(show_chaos);
    exalt_value->setVisible(show_exalt);
    exalt_ratio->setVisible(show_exalt);
}

void CurrencyWidget::Update() {
    currency_->chaos.value1 = chaos_ratio->value();
    currency_->exalt.value1 = exalt_ratio->value();
    name->setText(currency_->name.c_str());
    count->setText(QString::number(currency_->count));
    if (fabs(currency_->chaos.value1) > EPS)
        chaos_value->setValue(currency_->count / currency_->chaos.value1);
    if (fabs(currency_->exalt.value1) > EPS)
        exalt_value->setValue(currency_->count / currency_->exalt.value1);
}

CurrencyDialog::CurrencyDialog(CurrencyManager& manager, bool show_chaos, bool show_exalt) : currency_manager_(manager) {
    headers_ = new CurrencyLabels;
    for (auto &curr : currency_manager_.currencies()) {
        CurrencyWidget* tmp = new CurrencyWidget(curr);
        // To keep every vector the same size, we DO create spinboxes for the empty currency, just don't display them
        if (curr->currency == CURRENCY_NONE)
            continue;
        connect(tmp->exalt_ratio, SIGNAL(valueChanged(double)), this, SLOT(UpdateTotalValue()));
        connect(tmp->chaos_ratio, SIGNAL(valueChanged(double)), this, SLOT(UpdateTotalValue()));

        currencies_widgets_.push_back(tmp);

    }
    separator_ = new QFrame;
    separator_->setFrameShape(QFrame::Shape::HLine);
    total_exalt_value_ = new QLabel("");
    show_exalt_ = new QCheckBox("show exalt ratio");
    show_exalt_->setChecked(show_exalt);
    connect(show_exalt_, SIGNAL(stateChanged(int)), this, SLOT(UpdateVisual()));

    total_chaos_value_ = new QLabel("");
    show_chaos_ = new QCheckBox("show chaos ratio");
    show_chaos_->setChecked(show_chaos);
    connect(show_chaos_, SIGNAL(stateChanged(int)), this, SLOT(UpdateVisual()));
    total_wisdom_value_ = new QLabel("");
    layout_ = new QVBoxLayout;
    Update();
    UpdateVisual();
#if defined(Q_OS_LINUX)
    setWindowFlags(Qt::WindowCloseButtonHint);
#endif
}

void CurrencyDialog::Update() {
    for (auto widget : currencies_widgets_) {
        widget->Update();
    }
    UpdateTotalValue();
    UpdateTotalWisdomValue();

}

void CurrencyDialog::UpdateVisual() {
    //Destroy old layout (because you can't replace a layout, that would be too fun :/)
    delete layout_;
    layout_ = GenerateLayout(ShowChaos(), ShowExalt());
    setLayout(layout_);
    UpdateVisibility(ShowChaos(), ShowExalt());
    adjustSize();
}

void CurrencyDialog::UpdateVisibility(bool show_chaos, bool show_exalt) {
    headers_->chaos_value->setVisible(show_chaos);
    headers_->chaos_ratio->setVisible(show_chaos);
    headers_->exalt_value->setVisible(show_exalt);
    headers_->exalt_ratio->setVisible(show_exalt);

    for(auto &item : currencies_widgets_) {
        item->UpdateVisual(show_chaos, show_exalt);
    }
    headers_->chaos_total->setVisible(show_chaos);
    total_chaos_value_->setVisible(show_chaos);
    headers_->exalt_total->setVisible(show_exalt);
    total_exalt_value_->setVisible(show_exalt);
}

QVBoxLayout* CurrencyDialog::GenerateLayout(bool show_chaos, bool show_exalt) {
    //Header
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(headers_->name, 0, 0);
    grid->addWidget(headers_->count, 0, 1);
    int col = 2;
    //Qt is shit, if we don't hide widget that aren't in the grid, it still display them like shit
    //Also, if we don't show widget in the grid, if they weren't in it before, it doesn't display them


    if (show_chaos) {
        grid->addWidget(headers_->chaos_value, 0, col);
        grid->addWidget(headers_->chaos_ratio, 0, col + 1);
        col += 2;
    }


    if (show_exalt) {
        grid->addWidget(headers_->exalt_value, 0, col);
        grid->addWidget(headers_->exalt_ratio, 0, col +1);
    }
    //Main part
    for (auto &item : currencies_widgets_) {
        int curr_row = grid->rowCount() + 1;
        // To keep every vector the same size, we DO create spinboxes for the empty currency, just don't display them
        if (item->IsNone())
            continue;
        grid->addWidget(item->name, curr_row, 0);
        grid->addWidget(item->count, curr_row, 1, 1, -1);
        int col = 2;

        if (show_chaos) {
            grid->addWidget(item->chaos_value, curr_row, col);
            grid->addWidget(item->chaos_ratio, curr_row, col + 1);
            col += 2;
        }
        if (show_exalt) {
            grid->addWidget(item->exalt_value, curr_row, col);
            grid->addWidget(item->exalt_ratio, curr_row, col + 1);
        }


    }

    //Bottom header
    QGridLayout *bottom = new QGridLayout;
    //Used to display in the first column the checkbox if we don't display chaos/exalt
    col = 0;

    if (show_chaos) {
        bottom->addWidget(headers_->chaos_total,  0, 0);
        bottom->addWidget(total_chaos_value_, 0, 1);
        col = 2;
    }
    bottom->addWidget(show_chaos_, 0, col);
    col = 0;
    if (show_exalt) {
        bottom->addWidget(headers_->exalt_total, 1, 0);
        bottom->addWidget(total_exalt_value_, 1, 1);
        col = 2;
    }
    bottom->addWidget(show_exalt_, 1 , col);
    bottom->addWidget(headers_->wisdom_total, 2, 0);
    bottom->addWidget(total_wisdom_value_, 2, 1);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(grid);
    layout->addWidget(separator_);
    layout->addLayout(bottom);
    return layout;

}



void CurrencyDialog::UpdateTotalValue() {
    total_exalt_value_->setText(QString::number(currency_manager_.TotalExaltedValue()));
    total_chaos_value_->setText(QString::number(currency_manager_.TotalChaosValue()));
}

void CurrencyDialog::UpdateTotalWisdomValue() {
    total_wisdom_value_->setText(QString::number(currency_manager_.TotalWisdomValue()));
}
