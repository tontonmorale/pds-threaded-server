#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    connect(this, &MainWindow::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::logSig, this, &MainWindow::printToLogSlot);
    connect(&server, &MyServer::drawChartSig, this, &MainWindow::drawChartSlot);
    connect(&server, &MyServer::drawMapSig, this, &MainWindow::drawMapSlot);
    connect(&server, &MyServer::dbConnectedSig, this, &MainWindow::serverListenSlot);
//    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::printOldCountMap);
    ui = new Ui::MainWindow;

    ui->setupUi(this);
//    auto i = new QMap<QString, int>();
//    i->insert("2019/03/22_17:52", 3);
//    i->insert("2019/03/22_17:52", 1);
//    setChartDataSlot(i);
}

void MainWindow::drawMapSlot(QList<QPointF> devicesCoords, QPointF maxEspCoords){
    QScatterSeries *mapSeries = new QScatterSeries();

    for(QList<QPointF>::iterator i=devicesCoords.begin(); i!=devicesCoords.end(); i++)
        *mapSeries << *i;

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(mapSeries);

    // setta gli assi
    QValueAxis *axisX = new QValueAxis;
    axisX->setMin(0.0);
    axisX->setMax(maxEspCoords.x());
    axisX->setTitleText("x");
    chart->addAxis(axisX, Qt::AlignBottom);
    mapSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setMin(0.0);
    axisY->setMax(maxEspCoords.y());
    axisY->setTitleText("y");
    chart->addAxis(axisY, Qt::AlignLeft);
    mapSeries->attachAxis(axisY);

//    chart->axisX()->setRange(0.0, maxEspCoords);
//    chart->axisY()->setRange(0.0, maxEspCoords);
    chart->setTitle("People in the area");

    QChartView *mapView = ui->mapView;
    mapView->setRenderHint(QPainter::Antialiasing);
    mapView->setChart(chart);
}

//void MainWindow::drawPeopleCountChart(){
//    QChart *chart = new QChart();
//    this->chartSeries = new QLineSeries();

////    QDateTime dateTime(QDate(19, 6, 13), QTime(13, 10));
////    chartSeries->append(dateTime.toMSecsSinceEpoch(), 5);
////    dateTime = QDateTime(QDate(19, 6, 13), QTime(13, 15));
////    chartSeries->append(dateTime.toMSecsSinceEpoch(), 15);

//    // setta chart
//    chart->addSeries(chartSeries);
//    chart->legend()->hide();
//    chart->setTitle("People count");

//    // setta gli assi
//    QDateTimeAxis *axisX = new QDateTimeAxis;
//    axisX->setTickCount(chartSeries->count());
//    axisX->setFormat("yy/MM/dd <br> hh:mm");
//    axisX->setTitleText("Date");
//    chart->addAxis(axisX, Qt::AlignBottom);
//    chartSeries->attachAxis(axisX);

//    QValueAxis *axisY = new QValueAxis;
//    axisY->setLabelFormat("%i");
//    axisY->setTitleText("People count");
//    chart->addAxis(axisY, Qt::AlignLeft);
//    chartSeries->attachAxis(axisY);

//    // crea chart view
//    QChartView *chartView = ui->countChartView;
//    chartView->setChart(chart);
//    chartView->setRenderHint(QPainter::Antialiasing);
//}

void MainWindow::serverInit(){
    // server init
    server.init();
}

void MainWindow::serverListenSlot(){
    if (!server.listen(QHostAddress("192.168.1.172"), 9999)) {
        emit fatalErrorSig("Errore avvio del server");
        return;
    }

    printToLogSlot("[ server ] Server started: ip = 192.168.1.172, port = 9999\n\n Waiting for incoming connections...\n");
}

void MainWindow::fatalErrorSlot(QString message){
    qDebug().noquote() << message;
    QMessageBox::critical(this, "", message);
    exit(-1);
}

void MainWindow::printToLogSlot(QString message){
    ui->log->appendPlainText(message);
}

void MainWindow::drawChartSlot(QMap<QString, int> chartDataToDrawMap) {

    //disegno grafico runtime


    if(chartDataToDrawMap.size()!=0){
        QChart* oldChart = ui->countChartView->chart();
        if(oldChart!=nullptr)
            oldChart->deleteLater();

        ui->noChartData->hide();

        QLineSeries *chartSeries = new QLineSeries();

    //    key: 2019/03/22_17:52, value: 14
        for (auto i = chartDataToDrawMap.begin(); i != chartDataToDrawMap.end(); i++) {
            QStringList dateAndTime = i.key().split("_");
            QStringList splitDate = dateAndTime[0].split("/");
            QStringList splitTime = dateAndTime[1].split(":");
            QDateTime dateTime(QDate(splitDate[0].toInt(), splitDate[1].toInt(), splitDate[2].toInt()), QTime(splitTime[0].toInt(), splitTime[1].toInt()));
            chartSeries->append(dateTime.toMSecsSinceEpoch(), i.value());
        }

        QChart *chart = new QChart();

        // setta chart

        chart->addSeries(chartSeries);
        chart->legend()->hide();
        chart->setTitle("People count");

        // setta gli assi
        QDateTimeAxis *axisX = new QDateTimeAxis;
        axisX->setTickCount(chartSeries->count());
        axisX->setFormat("yy/MM/dd <br> hh:mm");
        axisX->setTitleText("Date");
        chart->addAxis(axisX, Qt::AlignBottom);
        chartSeries->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis;
        axisY->setTitleText("People count");
//        axisY->setTickCount(chartSeries->count());
        chart->addAxis(axisY, Qt::AlignLeft);
        chartSeries->attachAxis(axisY);

        // crea chart view
        QChartView *chartView = ui->countChartView;
        chartView->setChart(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
    }

    else{
        ui->countChartView->chart()->hide();
        ui->noChartData->show();
    }
}

//disegna grafico del numero di mac rilevati nel periodo specificato
void MainWindow::drawOldCountChartSlot() {
    QString begintime, endtime;
    //prendi begin time ed end time dalle tendine che ci sono sul grafico
//    QList<QPointF> *list = server.DrawOldCountMap(begintime, endtime);
//    ui->gridLayout->addWidget(createTimeChartGroup(*list), 0, 0);

}

//QGroupBox* MainWindow::createTimeChartGroup(QList<QPointF> points)
//{
//    QGroupBox *groupBox = new QGroupBox(tr("Temporal chart"));

//    QLineSeries *series = new QLineSeries();

//    for(QList<QPointF>::iterator i=points.begin(); i!=points.end(); i++)
//        *series << *i;

//    QChart *chart = new QChart();
//    chart->legend()->hide();
//    chart->addSeries(series);
//    chart->createDefaultAxes();
//    chart->setTitle("Number of people in area");

//    QValueAxis *axisX = new QValueAxis;
//    axisX->setRange(0, 60);
//    axisX->setTickCount(13);
//    chart->setAxisX(axisX, series);


//    QChartView *chartView = new QChartView(chart);
//    chartView->setRenderHint(QPainter::Antialiasing);
//    QHBoxLayout *layout = new QHBoxLayout;
//    layout->addWidget(chartView);
//    groupBox->setLayout(layout);

//    return groupBox;
//}

MainWindow::~MainWindow()
{
    delete ui;
}
