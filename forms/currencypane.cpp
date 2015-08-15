#include "currencypane.h"
#include "ui_currencypane.h"
#include "external/boolinq.h"

#include <QSpinBox>

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
        QWidget* widget = new QWidget(ui->currencyTable);
        widget->setLayout(new QHBoxLayout());
        widget->layout()->setMargin(0);
        QSpinBox* box = new QSpinBox();
        box->setObjectName("CurrencyCount");
        box->setValue(0);
        box->setMaximum(100000);
        box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        widget->layout()->addWidget(box);

        widget->layout()->addWidget(new QLabel(":"));

        box = new QSpinBox();
        box->setObjectName("ChaosCount");
        box->setSuffix("c");
        box->setValue(1);
        box->setMaximum(100000);
        box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        widget->layout()->addWidget(box);

        ui->currencyTable->setCellWidget (row, 1, widget);

        item = new QTableWidgetItem("0");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->currencyTable->setItem(row, 2, item);
    }
    ui->currencyTable->blockSignals(false);
    ui->currencyTable->update();

    QCustomPlot* customPlot = ui->plotWidget;
    // set locale to english, so we get english month names:
    customPlot->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
    // seconds of current time, we'll use it as starting point in time for data:
    double now = QDateTime::currentDateTime().toTime_t();
    srand(8); // set the random seed, so we always get the same random data

    customPlot->addGraph();
    customPlot->graph()->setName("Net Worth");
    QPen pen;
    pen.setColor(QColor(0, 0, 255, 200));
    customPlot->graph()->setLineStyle(QCPGraph::lsLine);
    customPlot->graph()->setPen(pen);
    customPlot->graph()->setBrush(QBrush(QColor(255/4.0,160,50,150)));
    // generate random walk data:
    QVector<double> time(250), value(250);
    for (int i=0; i<250; ++i)
    {
    time[i] = now + 24*3600*i;
    if (i == 0)
      value[i] = (i/50.0+1)*(rand()/(double)RAND_MAX-0.5);
    else
      value[i] = fabs(value[i-1])*(1+0.02/4.0*(4)) + (i/50.0+1)*(rand()/(double)RAND_MAX-0.5);
    }
    customPlot->graph()->setData(time, value);

    // configure bottom axis to show date and time instead of number:
    customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    customPlot->xAxis->setDateTimeFormat("MMMM\nyyyy");
    // set a more compact font size for bottom and left axis tick labels:
    customPlot->xAxis->setTickLabelFont(QFont(QFont().family(), 8));
    customPlot->yAxis->setTickLabelFont(QFont(QFont().family(), 8));
    // set a fixed tick-step to one tick per month:
    customPlot->xAxis->setAutoTickStep(false);
    customPlot->xAxis->setTickStep(2628000); // one month in seconds
    customPlot->xAxis->setSubTickCount(3);
    // set axis labels:
    customPlot->xAxis->setLabel("Date");
    customPlot->yAxis->setLabel("Value");
    // make top and right axes visible but without ticks and labels:
    customPlot->xAxis2->setVisible(true);
    customPlot->yAxis2->setVisible(true);
    customPlot->xAxis2->setTicks(false);
    customPlot->yAxis2->setTicks(false);
    customPlot->xAxis2->setTickLabels(false);
    customPlot->yAxis2->setTickLabels(false);
    // set axis ranges to show all data:
    customPlot->xAxis->setRange(now, now+24*3600*249);
    customPlot->yAxis->setRange(0, 60);
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

    ui->totalWorthLabel->setText(QString::number(total));
}

CurrencyPane::~CurrencyPane()
{
    delete ui;
}

void CurrencyPane::on_currencyTable_itemChanged(QTableWidgetItem *item) {
    if (item->row() == 1)
        UpdateItemChaosValues();
}
