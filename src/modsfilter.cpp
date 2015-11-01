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

#include "modsfilter.h"

#include <QComboBox>
#include <QLineEdit>
#include <QObject>
#include <QPushButton>
#include "QsLog.h"

#include "modlist.h"
#include "porting.h"

SelectedMod::SelectedMod(const std::string &name, double min, double max, bool min_filled, bool max_filled) :
    data_(name, min, max, min_filled, max_filled),
    mod_select_(std::make_unique<QComboBox>()),
    min_text_(std::make_unique<QLineEdit>()),
    max_text_(std::make_unique<QLineEdit>()),
    delete_button_(std::make_unique<QPushButton>("x"))
{
    mod_select_->setEditable(true);
    mod_select_->addItems(mod_string_list);
    mod_completer_ = new QCompleter(mod_string_list);
    mod_completer_->setCompletionMode(QCompleter::PopupCompletion);
    mod_completer_->setFilterMode(Qt::MatchContains);
    mod_completer_->setCaseSensitivity(Qt::CaseInsensitive);
    mod_select_->setCompleter(mod_completer_);

    mod_select_->setCurrentIndex(mod_select_->findText(name.c_str()));
    if (min_filled)
        min_text_->setText(QString::number(min));
    if (max_filled)
        max_text_->setText(QString::number(max));
}

void SelectedMod::Update() {
    data_.mod = mod_select_->currentText().toStdString();
    data_.min = min_text_->text().toDouble();
    data_.max = max_text_->text().toDouble();
    data_.min_filled = !min_text_->text().isEmpty();
    data_.max_filled = !max_text_->text().isEmpty();
}

enum LayoutColumn {
    kMinField,
    kMaxField,
    kDeleteButton,
    kColumnCount
};

void SelectedMod::AddToLayout(QGridLayout *layout, int index) {
    int combobox_pos = index * 2;
    int minmax_pos = index * 2 + 1;
    layout->addWidget(mod_select_.get(), combobox_pos, 0, 1, LayoutColumn::kColumnCount);
    layout->addWidget(min_text_.get(), minmax_pos, LayoutColumn::kMinField);
    layout->addWidget(max_text_.get(), minmax_pos, LayoutColumn::kMaxField);
    layout->addWidget(delete_button_.get(), minmax_pos, LayoutColumn::kDeleteButton);
}

void SelectedMod::CreateSignalMappings(QSignalMapper *signal_mapper, int index) {
    QObject::connect(mod_select_.get(), SIGNAL(currentIndexChanged(int)), signal_mapper, SLOT(map()));
    QObject::connect(min_text_.get(), SIGNAL(textEdited(const QString&)), signal_mapper, SLOT(map()));
    QObject::connect(max_text_.get(), SIGNAL(textEdited(const QString&)), signal_mapper, SLOT(map()));
    QObject::connect(delete_button_.get(), SIGNAL(clicked()), signal_mapper, SLOT(map()));

    signal_mapper->setMapping(mod_select_.get(), index + 1);
    signal_mapper->setMapping(min_text_.get(), index + 1);
    signal_mapper->setMapping(max_text_.get(), index + 1);
    // hack to distinguish update and delete, delete event has negative (id + 1)
    signal_mapper->setMapping(delete_button_.get(), -index - 1);
}

void SelectedMod::RemoveSignalMappings(QSignalMapper *signal_mapper) {
    signal_mapper->removeMappings(mod_select_.get());
    signal_mapper->removeMappings(min_text_.get());
    signal_mapper->removeMappings(max_text_.get());
    signal_mapper->removeMappings(delete_button_.get());
}

ModsFilter::ModsFilter(QLayout *parent):
    signal_handler_(*this)
{
    Initialize(parent);
    QObject::connect(&signal_handler_, SIGNAL(SearchFormChanged()), 
        parent->parentWidget()->window(), SLOT(OnSearchFormChange()));
}

void ModsFilter::FromForm(FilterData *data) {
    auto &mod_data = data->mod_data;
    mod_data.clear();
    for (auto &mod : mods_)
        mod_data.push_back(mod.data());
}

void ModsFilter::ToForm(FilterData *data) {
    Clear();
    for (auto &mod : data->mod_data)
        mods_.push_back(SelectedMod(mod.mod, mod.min, mod.max, mod.min_filled, mod.max_filled));
    Refill();
}

void ModsFilter::ResetForm() {
    Clear();
    Refill();
}

bool ModsFilter::Matches(const std::shared_ptr<Item> &item, FilterData *data) {
    for (auto &mod : data->mod_data) {
        if (mod.mod.empty())
            continue;
        const ModTable &mod_table = item->mod_table();
        if (!mod_table.count(mod.mod))
            return false;
        double value = mod_table.at(mod.mod);
        if (mod.min_filled && value < mod.min)
            return false;
        if (mod.max_filled && value > mod.max)
            return false;
    }
    return true;
}

void ModsFilter::Initialize(QLayout *parent) {
    layout_ = std::make_unique<QGridLayout>();
    add_button_ = std::make_unique<QPushButton>("Add mod");
    QObject::connect(add_button_.get(), SIGNAL(clicked()), &signal_handler_, SLOT(OnAddButtonClicked()));
    Refill();

    auto widget = new QWidget;
    widget->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(layout_.get());
    parent->addWidget(widget);

    QObject::connect(&signal_mapper_, SIGNAL(mapped(int)), &signal_handler_, SLOT(OnModChanged(int)));
}

void ModsFilter::AddMod() {
    SelectedMod mod("", 0, 0, false, false);
    mods_.push_back(std::move(mod));
    Refill();
}

void ModsFilter::UpdateMod(int id) {
    mods_[id].Update();
}

void ModsFilter::DeleteMod(int id) {
    mods_.erase(mods_.begin() + id);

    Refill();
}

void ModsFilter::ClearSignalMapper() {
    for (auto &mod : mods_) {
        mod.RemoveSignalMappings(&signal_mapper_);
    }
}

void ModsFilter::ClearLayout() {
    QLayoutItem *item;
    while ((item = layout_->takeAt(0))) {}
}

void ModsFilter::Clear() {
    ClearSignalMapper();
    ClearLayout();
    mods_.clear();
}

void ModsFilter::Refill() {
    ClearSignalMapper();
    ClearLayout();

    int i = 0;
    for (auto &mod : mods_) {
        mod.AddToLayout(layout_.get(), i);
        mod.CreateSignalMappings(&signal_mapper_, i);

        ++i;
    }

    layout_->addWidget(add_button_.get(), 2 * mods_.size(), 0, 1, LayoutColumn::kColumnCount);
}

void ModsFilterSignalHandler::OnAddButtonClicked() {
    parent_.AddMod();
}

void ModsFilterSignalHandler::OnModChanged(int id) {
    if (id < 0)
        parent_.DeleteMod(-id - 1);
    else
        parent_.UpdateMod(id - 1);
    emit SearchFormChanged();
}
