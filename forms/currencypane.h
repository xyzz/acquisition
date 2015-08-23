#ifndef CURRENCYPANE_H
#define CURRENCYPANE_H

#include <QWidget>
#include <QMap>
#include <QAbstractItemDelegate>
#include "item.h"

namespace Ui {
class CurrencyPane;
}

class QTableWidget;
class QTableWidgetItem;
class MainWindow;
class Application;
class QDoubleSpinBox;

class RatioCell : public QWidget {
    Q_OBJECT
private:
    QDoubleSpinBox* first;
    QDoubleSpinBox* second;
public:
    explicit RatioCell(QTableWidget *parent = 0);
signals:
    void editingFinished();
};

class CurrencyPane : public QWidget
{
    Q_OBJECT

public:
    explicit CurrencyPane(QWidget *parent = 0);
    ~CurrencyPane();

    bool LoadCurrencyRates(const QString &data);
    QString SaveCurrencyRates();

    bool LoadCurrencyHistory(const QString &data);
    QString SaveCurrencyHistoryPoint();
    void initialize(MainWindow *parent);
public slots:
    void UpdateGraph(const QMap<double, double> map);
    void UpdateItemCounts(const Items &items);
    void UpdateItemChaosValues();
private:
    Ui::CurrencyPane *ui;

    double totalValue;

    MainWindow* parent_;
    Application* app_;

    QStringList currency; // We'll use our own rather than buyout_manager's.
};

#endif // CURRENCYPANE_H
