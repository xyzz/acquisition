#include "currencypane.h"
#include "ui_currencypane.h"
#include "external/boolinq.h"

#include "QsLog.h"

#include <QSpinBox>
#include <mainwindow.h>
#include <application.h>
#include <datamanager.h>

using namespace boolinq;

CurrencyPane::CurrencyPane(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CurrencyPane)
{
    ui->setupUi(this);

    currency = QStringList({"Scroll of Wisdom",
                            "Portal Scroll",
                            "Armourer's Scrap",
                            "Blacksmith's Whetstone",
                            "Glassblower's Bauble",
                            "Cartographer's Chisel",
                            "Gemcutter's Prism",
                            "Jeweller's Orb",
                            "Chromatic Orb",
                            "Orb of Fusing",
                            "Orb of Transmutation",
                            "Orb of Chance",
                            "Orb of Alchemy",
                            "Regal Orb",
                            "Orb of Augmentation",
                            "Exalted Orb",
                            "Orb of Alteration",
                            "Chaos Orb",
                            "Blessed Orb",
                            "Divine Orb",
                            "Orb of Scouring",
                            "Mirror of Kalandra",
                            "Orb of Regret",
                            "Vaal Orb",
                            "Eternal Orb"});


    ui->currencyTable->setTabKeyNavigation(true);

    ui->currencyTable->verticalHeader()->show();
    ui->currencyTable->blockSignals(true);
    for (QString currencyItem : currency) {
        int row = ui->currencyTable->rowCount();
        ui->currencyTable->insertRow(row);
        QTableWidgetItem* item = new QTableWidgetItem(currencyItem);
        ui->currencyTable->setVerticalHeaderItem(row, item);
        item = new QTableWidgetItem("0");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->currencyTable->setItem(row, 0, item);
        ui->currencyTable->setItem(row, 1, new QTableWidgetItem("0"));

        // Generate cell widget!
        RatioCell* cell = new RatioCell(ui->currencyTable);
        ui->currencyTable->setCellWidget(row, 1, cell);
        connect(cell, &RatioCell::editingFinished, this, &CurrencyPane::UpdateItemChaosValues);

        item = new QTableWidgetItem("0");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->currencyTable->setItem(row, 2, item);
    }
    ui->currencyTable->blockSignals(false);
    ui->currencyTable->update();
}

void CurrencyPane::initialize(MainWindow* parent) {
    parent_ = parent;
    app_ = parent->application();

    QString data = QString::fromStdString(app_->data_manager().Get("currency_rates"));
    LoadCurrencyRates(data);

    data = QString::fromStdString(app_->data_manager().Get("currency_history"));
    LoadCurrencyHistory(data);
}

void CurrencyPane::UpdateGraph(const QMap<double, double> map) {
    QCustomPlot* customPlot = ui->plotWidget;

    // Only show graph data if we have more than a days worth of currency information (otherwise it looks bad).
    if (map.firstKey() >= QDateTime::currentDateTime().addDays(-1).toTime_t()) {
        customPlot->hide();
        return;
    }
    if (customPlot->isHidden()) {
        customPlot->show();
    }

    //customPlot->setBackground(Qt::transparent);

    double lastDate = map.lastKey();
    double weekAgo = QDateTime::currentDateTime().addDays(-7).toTime_t();

    double highestVal = 0;
    if (!map.isEmpty()) {
        highestVal = from(map.values().toVector().toStdVector()).max();
    }

    if (customPlot->graphCount() > 0) {
        customPlot->removeGraph(0);
    }
    customPlot->addGraph();
    customPlot->graph()->setName("Net Worth");
    QPen pen;
    pen.setColor(QColor(0, 0, 255, 200));
    customPlot->graph()->setLineStyle(QCPGraph::lsLine);
    customPlot->graph()->setPen(pen);
    customPlot->graph()->setBrush(QBrush(QColor(255/4.0,160,50,150)));
    customPlot->graph()->setData(map.keys().toVector(), map.values().toVector());

    // configure bottom axis to show date and time instead of number:
    customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot->xAxis->setDateTimeFormat("ddd");
    // set a more compact font size for bottom and left axis tick labels:
    customPlot->xAxis->setTickLabelFont(QFont(QFont().family(), 8));
    customPlot->yAxis->setTickLabelFont(QFont(QFont().family(), 8));
    // set axis labels:
    customPlot->xAxis->setLabel("Date");
    customPlot->yAxis->setLabel("Total Worth in Chaos");
    // make top and right axes visible but without ticks and labels:
    customPlot->xAxis2->setVisible(true);
    customPlot->yAxis2->setVisible(true);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis2->setTicks(false);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis2->setTickLabels(false);
    // set axis ranges to show all data:
    customPlot->xAxis->setRange(weekAgo, lastDate);
    customPlot->yAxis->setRange(0, highestVal);
    // show legend:
    customPlot->legend->setVisible(true);
}

void CurrencyPane::UpdateItemCounts(const Items &items) {
    for (int i = 0; i < ui->currencyTable->rowCount(); i++) {
        QTableWidgetItem* item = ui->currencyTable->item(i, 0);
        QString header = ui->currencyTable->verticalHeaderItem(i)->text();
        int count = from(items)
                .where([header](std::shared_ptr<Item> item) {return item->typeLine() == header.toStdString();})
                .sum([](std::shared_ptr<Item> item) {
                    return item->count();
                });
        item->setText(QString::number(count));
    }
    UpdateItemChaosValues();

    SaveCurrencyHistoryPoint();
}

void CurrencyPane::UpdateItemChaosValues() {
    double total = 0.0;
    for (int i = 0; i < ui->currencyTable->rowCount(); i++) {
        QTableWidgetItem* item = ui->currencyTable->item(i, 0);
        int count = item->data(Qt::DisplayRole).toInt();

        QWidget* widget = ui->currencyTable->cellWidget(i, 1);
        QSpinBox* currencyBox = widget->findChild<QSpinBox*>("CurrencyCount");
        QSpinBox* chaosBox = widget->findChild<QSpinBox*>("ChaosCount");
        double ratio = 0.0;
        if (currencyBox && chaosBox) {
            ratio = ((double) currencyBox->value() / chaosBox->value());
        }
        double result = (ratio > 0.0) ? count / ratio : 0.0;

        item = ui->currencyTable->item(i, 2);
        item->setText(QString::number(result));
        total += result;
    }
    totalValue = total;
    ui->totalWorthLabel->setText(QString::number(total));

    QString rates = SaveCurrencyRates();
    app_->data_manager().Set("currency_rates", rates.toStdString());
}

CurrencyPane::~CurrencyPane() {
    delete ui;
}

bool CurrencyPane::LoadCurrencyRates(const QString &data)
{
    QMap<QString, QJsonObject> dataObjects;
    QJsonDocument doc = QJsonDocument::fromJson(data.toLatin1());
    if (!doc.isNull() && doc.isArray()) {
        QJsonArray arr = doc.array();
        for (QJsonValue v : arr) {
            if (!v.isObject()) continue;
            QJsonObject obj = v.toObject();
            dataObjects.insert(obj.value("currency").toString(), obj);
        }
    }

    for (int i = 0; i < ui->currencyTable->rowCount(); i++) {
        QTableWidgetItem* header = ui->currencyTable->verticalHeaderItem(i);
        if (dataObjects.contains(header->text())) {
            QJsonObject obj = dataObjects.value(header->text());
            int count = obj.value("count").toInt();
            int chaos = obj.value("chaos").toInt();

            QWidget* widget = ui->currencyTable->cellWidget(i, 1);
            QSpinBox* currencyBox = widget->findChild<QSpinBox*>("CurrencyCount");
            QSpinBox* chaosBox = widget->findChild<QSpinBox*>("ChaosCount");

            if (currencyBox && chaosBox) {
                currencyBox->setValue(count);
                chaosBox->setValue(chaos);
            }
        }
    }
    UpdateItemChaosValues();
    return true;
}

QString CurrencyPane::SaveCurrencyRates()
{
    QJsonDocument doc;
    QJsonArray array;
    for (int i = 0; i < ui->currencyTable->rowCount(); i++) {
        QTableWidgetItem* header = ui->currencyTable->verticalHeaderItem(i);

        QWidget* widget = ui->currencyTable->cellWidget(i, 1);
        QSpinBox* currencyBox = widget->findChild<QSpinBox*>("CurrencyCount");
        QSpinBox* chaosBox = widget->findChild<QSpinBox*>("ChaosCount");

        if (currencyBox && chaosBox) {
            QJsonObject obj;
            obj.insert("currency", header->text());
            obj.insert("count", currencyBox->value());
            obj.insert("chaos", chaosBox->value());
            array.append(obj);
        }
    }
    doc.setArray(array);
    return doc.toJson(QJsonDocument::Compact);
}

bool CurrencyPane::LoadCurrencyHistory(const QString &data)
{
    QMap<double, double> map;
    QJsonDocument doc = QJsonDocument::fromJson(data.toLatin1());
    if (!doc.isNull() && doc.isArray()) {
        for (QJsonValue val : doc.array()) {
            QJsonObject obj = val.toObject();
            double value = obj.value("value").toDouble();
            double time = obj.value("time").toDouble();
            if (map.contains(time)) {
                double otherValue = map.value(time);
                if (otherValue < value) {
                    map.remove(time);
                    map.insert(time, value);
                }
            }
            else {
                map.insert(time, value);
            }
        }
    }
    UpdateGraph(map);
    return true;
}

QString CurrencyPane::SaveCurrencyHistoryPoint()
{
    QJsonObject point;
    double time = (double)QDateTime::currentDateTime().toTime_t();
    point.insert("value", totalValue);
    point.insert("time", time);

    QByteArray data = QByteArray::fromStdString(app_->data_manager().Get("currency_history"));
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray array;
    if (!doc.isNull() && doc.isArray()) {
        array = doc.array();
    }
    bool insert = true;
    for (QJsonValue val : array) {
        if (val.isObject() && val.toObject().value("time").toDouble() == time) {
            insert = false;
            break;
        }
    }
    if (insert) array.append(point);
    doc.setArray(array);

    QString newData = doc.toJson(QJsonDocument::Compact);
    app_->data_manager().Set("currency_history", newData.toStdString());
    LoadCurrencyHistory(newData);
    return newData;
}


RatioCell::RatioCell(QTableWidget *parent) : QWidget(parent) {
    setLayout(new QHBoxLayout());
    layout()->setMargin(0);

    first = new QSpinBox(this);
    first->setObjectName("CurrencyCount");
    first->setValue(0);
    first->setMaximum(100000);
    first->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout()->addWidget(first);

    layout()->addWidget(new QLabel(":", this));

    second = new QSpinBox(this);
    second->setObjectName("ChaosCount");
    second->setSuffix("c");
    second->setValue(1);
    second->setMaximum(100000);
    second->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout()->addWidget(second);

    first->setButtonSymbols(QAbstractSpinBox::NoButtons);
    second->setButtonSymbols(QAbstractSpinBox::NoButtons);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(first);
    setTabOrder(first, second);
    connect(first, &QSpinBox::editingFinished, this, &RatioCell::editingFinished);
    connect(second, &QSpinBox::editingFinished, this, &RatioCell::editingFinished);
}
