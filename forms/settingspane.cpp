#include "settingspane.h"
#include "ui_settingspane.h"

#include "application.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "itemsmanager.h"
#include "shop.h"
#include "datamanager.h"

SettingsPane::SettingsPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsPane)
{
    ui->setupUi(this);
}

void SettingsPane::initialize(MainWindow* parent) {
    parent_ = parent;
    app_ = parent->application();
}

void SettingsPane::updateFromStorage() {
    // Items
    ui->refreshItemsBox->setChecked(app_->items_manager().auto_update());
    ui->refreshItemsIntervalBox->setValue(app_->items_manager().auto_update_interval());

    // Shop
    std::vector<std::string> threads = app_->shop().threads();

    if (!threads.empty())
        ui->shopThreadIdBox->setValue(QString::fromStdString(threads.front()).toUInt());
    ui->updateShopBox->setChecked(app_->shop().auto_update());

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

    flag = app_->data_manager().GetBool("ShowOldMenu");
    ui->showMenuBarBox->setChecked(flag);
    // Force update
    on_showMenuBarBox_toggled(flag);
}

void SettingsPane::on_buyoutStyleBox_toggled(bool checked) {
    parent_->ui->buyoutCurrencyComboBox->setHidden(checked);
    app_->data_manager().SetBool("ProcBuyoutStyle", checked);
}

void SettingsPane::on_showMenuBarBox_toggled(bool checked) {
    parent_->ui->menuBar->setVisible(checked);
    app_->data_manager().SetBool("ShowOldMenu", checked);
}

void SettingsPane::on_updateShopButton_clicked() {
    app_->shop().SubmitShopToForum();
}

void SettingsPane::on_refreshItemsButton_clicked() {
    app_->items_manager().Update();

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

void SettingsPane::on_shopThreadIdBox_editingFinished() {
    QString thread = QString::number(ui->shopThreadIdBox->value());
    app_->shop().SetThread(QList<std::string>({thread.toStdString()}).toVector().toStdVector());
    parent_->UpdateShopMenu();
    parent_->UpdateSettingsBox();
}

void SettingsPane::on_editShopTemplateButton_clicked() {
    parent_->on_actionShop_template_triggered();
}

void SettingsPane::on_updateShopBox_toggled(bool checked) {
    app_->shop().SetAutoUpdate(checked);
}

void SettingsPane::on_copyClipboardButton_clicked() {
    parent_->on_actionCopy_shop_data_to_clipboard_triggered();
}

void SettingsPane::on_bumpShopBox_toggled(bool checked) {
    app_->data_manager().SetBool("shop_bump", checked);
}

void SettingsPane::on_darkThemeRadioButton_clicked() {
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
}

void SettingsPane::on_lightThemeRadioButton_clicked() {
    qApp->setStyleSheet(light_style_);

    parent_->setPalette(light_palette_);
    QList<QWidget*> widgets = parent_->findChildren<QWidget*>();
    foreach (QWidget* w, widgets)
        w->setPalette(light_palette_);

    qApp->setPalette(light_palette_);
    app_->data_manager().SetBool("DarkTheme", false);
}

SettingsPane::~SettingsPane()
{
    delete ui;
}
