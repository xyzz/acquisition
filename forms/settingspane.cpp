#include "settingspane.h"
#include "ui_settingspane.h"

#include "application.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "itemsmanager.h"
#include "shop.h"
#include "datamanager.h"

#include "templatedialog.h"
#include "util.h"

SettingsPane::SettingsPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsPane)
{
    ui->setupUi(this);
    light_theme_ = true;

    ui->shopsWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
}

void SettingsPane::initialize(MainWindow* parent) {
    parent_ = parent;
    app_ = parent->application();

    for (QString shop : app_->shop().threadIds()) {
        bool ok = false;
        int id = shop.toInt(&ok);
        if (ok && id) addShop(id);
    }

    std::string exclusions = app_->data_manager().Get("tab_exclusions");
    QList<QRegularExpression> expressions;
    rapidjson::Document exclusionDoc;
    exclusionDoc.Parse(exclusions.c_str());

    if (exclusionDoc.IsArray()) {
        for (auto &excl  : exclusionDoc) {
            QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(excl.GetString()));
            item->setFlags(item->flags () | Qt::ItemIsEditable);
            ui->tabExclusionListWidget->addItem(item);
        }
    }
}

void SettingsPane::showTemplateDialog(const QString &threadId) {
    TemplateDialog dialog(app_, threadId, this);
    dialog.setAttribute(Qt::WA_DeleteOnClose, false);
    if (dialog.exec() == QDialog::Accepted) {
        app_->shop().SetShopTemplate(threadId, dialog.GetTemplate());
    }
}

void SettingsPane::addShop(int id) {
    ui->shopsWidget->blockSignals(true);
    int row = ui->shopsWidget->rowCount();
    ui->shopsWidget->insertRow(row);

    // Setup Thread ID
    QTableWidgetItem* thread = new QTableWidgetItem(QString::number(id));
    ui->shopsWidget->setItem(row, 0, thread);

    QPushButton* button = new QPushButton("Edit Template...");
    ui->shopsWidget->setCellWidget(row, 1, button);
    connect(button, &QPushButton::clicked, [this, row] {
        // Trigger template!
        QString threadId = ui->shopsWidget->item(row, 0)->text();
        showTemplateDialog(threadId);
    });

    shopThreadIds.insert(row, QString::number(id));
    ui->shopsWidget->blockSignals(false);

    if (id == 0) {
        ui->shopsWidget->editItem(thread);
    }
}

void SettingsPane::on_addShopButton_clicked() {
    addShop(0);
}

void SettingsPane::on_shopsWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous) {
    Q_UNUSED(previous)
    if (current == 0) {
        ui->removeShopButton->setEnabled(false);
        ui->copyShopButton->setEnabled(false);
    }
    else {
        ui->removeShopButton->setEnabled(true);
        ui->copyShopButton->setEnabled(true);
    }
}

void SettingsPane::on_removeShopButton_clicked() {
    auto items = ui->shopsWidget->selectedItems();
    for (auto item : items) {
        int row = item->row();
        QString threadId = ui->shopsWidget->item(row, 0)->text();
        ui->shopsWidget->removeRow(row);
        shopThreadIds.remove(row);
        app_->shop().RemoveShop(threadId);
    }
}

void SettingsPane::on_shopsWidget_itemChanged(QTableWidgetItem *item) {
    Q_ASSERT(item->column() == 0);

    int row = item->row();
    QString value = item->text();

    bool ok = false;
    int shop = value.toUInt(&ok);
    if (ok && shop > 0) {
        QString last = shopThreadIds.value(row);
        if (last != value) {
            if (shopThreadIds.values().contains(item->text())) {
                item->setText("0");
                shopThreadIds[row] = "0";
            }
            else {
                QString temp = app_->shop().GetShopTemplate(last);
                app_->shop().RemoveShop(last);
                app_->shop().AddShop(value, temp);
                shopThreadIds[row] = value;
            }
        }
    }
    else {
        item->setText("0");
    }
}

void SettingsPane::updateTabExclusions() {
    rapidjson::Document doc;
    doc.SetArray();
    auto &alloc = doc.GetAllocator();

    for (int i = 0; i < ui->tabExclusionListWidget->count(); i++) {
        QString expr = ui->tabExclusionListWidget->item(i)->text();
        if (!QRegularExpression(expr).isValid()) continue;
        rapidjson::Value val;
        val.SetString(expr.toLatin1().data(), expr.toLatin1().length());
        doc.PushBack(val, alloc);
    }

    app_->data_manager().Set("tab_exclusions", Util::RapidjsonSerialize(doc));
}

void SettingsPane::updateFromStorage() {
    // Items
    ui->refreshItemsBox->setChecked(app_->items_manager().auto_update());
    ui->refreshItemsIntervalBox->setValue(app_->items_manager().auto_update_interval());

    // Shop
    // Ignore shop threads as they are handled seperately...
    ui->updateShopBox->setChecked(app_->shop().IsAutoUpdateEnabled());
    ui->bumpShopBox->setChecked(app_->shop().IsBumpEnabled());

    // Trade
    ui->refreshTradeBox->setChecked(parent_->auto_online_.enabled());
    ui->tradeURLLineEdit->setText(QString::fromStdString(parent_->auto_online_.GetUrl()));
    ui->showTradeURL->setChecked(false);
    ui->tradeURLLineEdit->setEchoMode(QLineEdit::Password);

    // Visual
    ui->darkThemeRadioButton->setChecked(app_->data_manager().GetBool("DarkTheme"));
    if (ui->darkThemeRadioButton->isChecked()) {
        on_darkThemeRadioButton_clicked();
    }
    bool flag = app_->data_manager().GetBool("ProcBuyoutStyle");
    ui->buyoutStyleBox->setChecked(flag);
    // Force update
    on_buyoutStyleBox_toggled(flag);

    // Visual
    flag = app_->data_manager().GetBool("MinimizeToTray");
    ui->minimizeBox->setChecked(flag);
}

void SettingsPane::on_buyoutStyleBox_toggled(bool checked) {
    parent_->ui->buyoutCurrencyComboBox->setHidden(checked);
    app_->data_manager().SetBool("ProcBuyoutStyle", checked);
}

void SettingsPane::on_showTradeURL_toggled(bool checked) {
    if (checked)
        ui->tradeURLLineEdit->setEchoMode(QLineEdit::Normal);
    else
        ui->tradeURLLineEdit->setEchoMode(QLineEdit::Password);
}

void SettingsPane::on_refreshTradeBox_toggled(bool checked)
{
    parent_->auto_online_.SetEnabled(checked);
    QString url = ui->tradeURLLineEdit->text();
    if (!url.isEmpty())
        parent_->auto_online_.SetUrl(url.toStdString());
    parent_->UpdateOnlineGui();
}

void SettingsPane::on_tradeURLLineEdit_editingFinished() {
    parent_->auto_online_.SetUrl(ui->tradeURLLineEdit->text().toStdString());
    parent_->UpdateOnlineGui();
}

void SettingsPane::on_refreshItemsIntervalBox_editingFinished() {
    app_->items_manager().SetAutoUpdateInterval(ui->refreshItemsIntervalBox->value());
}

void SettingsPane::on_refreshItemsBox_toggled(bool checked) {
    app_->items_manager().SetAutoUpdate(checked);
}

void SettingsPane::on_updateShopBox_toggled(bool checked) {
    app_->shop().SetAutoUpdate(checked);
}

void SettingsPane::on_copyShopButton_clicked() {
    int row = ui->shopsWidget->currentRow();
    if (row < 0) return;
    QTableWidgetItem* item = ui->shopsWidget->item(row, 0);
    if (!item) return;
    app_->shop().CopyToClipboard(item->text());
}

void SettingsPane::on_bumpShopBox_toggled(bool checked) {
    app_->shop().SetDoBump(checked);
}

void SettingsPane::on_darkThemeRadioButton_clicked() {
    if (!light_theme_) return;
    light_palette_ = qApp->palette();
    light_style_ = qApp->styleSheet();

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53,53,53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(15,15,15));
    palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53,53,53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);
    palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
    palette.setColor(QPalette::HighlightedText, Qt::black);

    qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");


    parent_->setPalette(palette);
    QList<QWidget*> widgets = parent_->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(palette);

    qApp->setPalette(palette);
    app_->data_manager().SetBool("DarkTheme", true);
    light_theme_ = false;
}

void SettingsPane::on_lightThemeRadioButton_clicked() {
    if (light_theme_) return;
    qApp->setStyleSheet(light_style_);

    parent_->setPalette(light_palette_);
    QList<QWidget*> widgets = parent_->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(light_palette_);

    qApp->setPalette(light_palette_);
    app_->data_manager().SetBool("DarkTheme", false);
    light_theme_ = true;
}

SettingsPane::~SettingsPane() {
    delete ui;
}

void SettingsPane::on_minimizeBox_toggled(bool checked)
{
    app_->data_manager().SetBool("MinimizeToTray", checked);
}

void SettingsPane::on_addTabExclusion_clicked()
{
    QListWidgetItem* item = new QListWidgetItem("");
    item->setFlags(item->flags () | Qt::ItemIsEditable);
    ui->tabExclusionListWidget->addItem(item);
    ui->tabExclusionListWidget->editItem(item);
    updateTabExclusions();
}


void SettingsPane::on_removeTabExclusion_clicked()
{
    auto items = ui->tabExclusionListWidget->selectedItems();
    while (!items.isEmpty()) {
        QListWidgetItem* item = items.takeFirst();
        int row = ui->tabExclusionListWidget->row(item);
        ui->tabExclusionListWidget->takeItem(row);
        delete item;
    }
    updateTabExclusions();
}

void SettingsPane::on_tabExclusionListWidget_itemChanged(QListWidgetItem *item) {
    Q_UNUSED(item);
    updateTabExclusions();
}
