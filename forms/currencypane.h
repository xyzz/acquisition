#ifndef CURRENCYPANE_H
#define CURRENCYPANE_H

#include <QWidget>
#include "item.h"

namespace Ui {
class CurrencyPane;
}

class QTableWidgetItem;

class CurrencyPane : public QWidget
{
    Q_OBJECT

public:
    explicit CurrencyPane(QWidget *parent = 0);
    ~CurrencyPane();

    void UpdateItemCounts(const Items &items);
    void UpdateItemChaosValues();
private slots:
    void on_currencyTable_itemChanged(QTableWidgetItem *item);

private:
    Ui::CurrencyPane *ui;

    QStringList currency; // We'll use our own rather than buyout_manager's.
};

#endif // CURRENCYPANE_H
