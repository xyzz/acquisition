#ifndef SETTINGSPANE_H
#define SETTINGSPANE_H

#include <QListWidget>
#include <QWidget>
#include <qtablewidget.h>

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
    void updateShops();
    void updateTabExclusions();
public slots:
    void on_darkThemeRadioButton_clicked();
    void on_lightThemeRadioButton_clicked();
    void on_buyoutStyleBox_toggled(bool checked);
    void on_showMenuBarBox_toggled(bool checked);
    void on_showTradeURL_toggled(bool checked);
    void on_refreshTradeBox_toggled(bool checked);
    void on_tradeURLLineEdit_editingFinished();
    void on_refreshItemsIntervalBox_editingFinished();
    void on_refreshItemsBox_toggled(bool checked);
    void on_updateShopBox_toggled(bool checked);
    void on_copyShopButton_clicked();
    void on_bumpShopBox_toggled(bool checked);
private slots:
    void on_addShopButton_clicked();
    void on_shopsWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous);
    void on_removeShopButton_clicked();
    void on_shopsWidget_itemChanged(QTableWidgetItem *item);
    void on_minimizeBox_toggled(bool checked);
    void on_addTabExclusion_clicked();
    void on_removeTabExclusion_clicked();
    void on_tabExclusionListWidget_itemChanged(QListWidgetItem *item);

private:
    Ui::SettingsPane *ui;

    Application* app_;
    MainWindow* parent_;

    bool light_theme_;
    QPalette light_palette_;
    QString light_style_;
    void addShop(int id);
};

#endif // SETTINGSPANE_H
