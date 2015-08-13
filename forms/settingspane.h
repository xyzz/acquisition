#ifndef SETTINGSPANE_H
#define SETTINGSPANE_H

#include <QWidget>

class MainWindow;
class Application;

namespace Ui {
class SettingsPane;
}

class SettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPane(QWidget *parent = 0);
    ~SettingsPane();
    void updateFromStorage();
    void initialize(MainWindow* parent);
public slots:
    void on_darkThemeRadioButton_clicked();
    void on_lightThemeRadioButton_clicked();
    void on_buyoutStyleBox_toggled(bool checked);
    void on_showMenuBarBox_toggled(bool checked);
    void on_updateShopButton_clicked();
    void on_refreshItemsButton_clicked();
    void on_showTradeURL_toggled(bool checked);
    void on_refreshTradeBox_toggled(bool checked);
    void on_tradeURLLineEdit_editingFinished();
    void on_refreshItemsIntervalBox_editingFinished();
    void on_refreshItemsBox_toggled(bool checked);
    void on_shopThreadIdBox_editingFinished();
    void on_editShopTemplateButton_clicked();
    void on_updateShopBox_toggled(bool checked);
    void on_copyClipboardButton_clicked();
    void on_bumpShopBox_toggled(bool checked);
private:
    Ui::SettingsPane *ui;

    Application* app_;
    MainWindow* parent_;

    QPalette light_palette_;
    QString light_style_;
};

#endif // SETTINGSPANE_H
