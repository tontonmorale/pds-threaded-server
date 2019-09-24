#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    connect(this, &MainWindow::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::logSig, this, &MainWindow::printToLogSlot);
    connect(&server, &MyServer::drawChartSig, this, &MainWindow::drawChartSlot);
    connect(&server, &MyServer::drawMapSig, this, &MainWindow::drawMapSlot);
    connect(&server, &MyServer::dbConnectedSig, this, &MainWindow::serverListenSlot);
    connect(&server, &MyServer::setMinuteSig, this, &MainWindow::setMinuteSlot);
    connect(&server, &MyServer::setClientsSig, this, &MainWindow::setClientsSlot);
    connect(this, &MainWindow::getMinDateForLPSTATSSig, &server, &MyServer::getMinDateForLPSTATSSlot);
    connect(&server, &MyServer::LPStatsWindowCreationSig, this, &MainWindow::LPStatsWindowCreationSlot);
    connect(this, &MainWindow::getLPStatsSig, &server, &MyServer::getLPStatsSlot);
    connect(&server, &MyServer::LPStatsSig, this, &MainWindow::drawLPStatsSlot);
    ui = new Ui::MainWindow;
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
    //    auto i = new QMap<QString, int>();
    //    i->insert("2019/03/22_17:52", 3);
    //    i->insert("2019/03/22_17:52", 1);
    //    setChartDataSlot(i);
}

void MainWindow::setMinuteSlot(int minute){
    ui->minute->setPlainText(QString::number(minute));
}

void MainWindow::setClientsSlot(int clients){
    ui->clients->setPlainText(QString::number(clients));
}

void MainWindow::drawMapSlot(QList<QPointF> devicesCoords, QPointF maxEspCoords, QMap<QString, Person> people, QMap<QString, Esp> *espMap){
    try {

        qDebug().noquote() << "disegno mappa";

        QScatterSeries *mapSeries = new QScatterSeries();
        QScatterSeries *mySeries = new QScatterSeries();

        for(QList<QPointF>::iterator i=devicesCoords.begin(); i!=devicesCoords.end(); i++)
            *mapSeries << *i;
//            mapSeries->append(i->x(), i->y());

        //aggiungo posizioni esp
        for(auto i : *espMap){
            QPointF p = i.getPosition();
            mySeries->append(p.x(), p.y());
        }
        qDebug().noquote() << mapSeries->points();

        ui->noMapData->setVisible(false);

        QChart *chart = new QChart();
        chart->legend()->hide();

        chart->addSeries(mySeries);
        chart->addSeries(mapSeries);
        this->setMouseTracking(true);

        // setta gli assi
//        QValueAxis *axisX = new QValueAxis;
//        axisX->setMin(0.0);
//        axisX->setLabelFormat("%.2f");
//        axisX->setMax(100);
//        axisX->setTitleText("x(metri)");
//        chart->addAxis(axisX, Qt::AlignBottom);
//        mapSeries->attachAxis(axisX);
//        mySeries->attachAxis(axisX);

//        QValueAxis *axisY = new QValueAxis;
//        axisY->setMin(0.0);
//        axisY->setLabelFormat("%.2f");
//        axisY->setMax(100);
//        axisY->setTitleText("y(metri)");
//        chart->addAxis(axisY, Qt::AlignLeft);
//        mapSeries->attachAxis(axisY);
//        mySeries->attachAxis(axisY);



        chart->createDefaultAxes();
        chart->axisX()->setRange((-maxEspCoords.x()*2), maxEspCoords.x()*2);
        chart->axisY()->setRange((-maxEspCoords.y()*2), maxEspCoords.y()*2);

        chart->setTitle("People in the area");
        chart->setAcceptHoverEvents(true);
        mapHovering.mapInit(mapSeries, people);

        QChartView *mapView = ui->mapView;
        mapView->setRenderHint(QPainter::Antialiasing);
        mapView->setChart(chart);
    }catch (...) {
        printToLogSlot("Problemi nella drawMapSlot", "red");
    }
}


void MainWindow::serverInit(){
    // server init
    server.init();
}

void MainWindow::serverListenSlot(){
//    if (!server.listen(QHostAddress("192.168.1.172"), 9999)) {
    if (!server.listen(QHostAddress("192.168.43.97"), 9999)) {
        emit fatalErrorSig("Errore avvio del server");
        return;
    }

//    printToLogSlot("- Server started: ip = 192.168.1.172, port = 9999\n\n Waiting for incoming connections...\n", "green");
    printToLogSlot("- Server started: ip = 192.168.43.97, port = 9999", "green");
    printToLogSlot("Waiting for incoming connections...\n", "orange");
}

void MainWindow::fatalErrorSlot(QString message){
    qDebug().noquote() << message;
    QMessageBox::critical(this, "", message);
    QCoreApplication::exit(-1);
}

void MainWindow::printToLogSlot(QString message, QString color){
    QString s = "<font color=";
    s += color;
    s += ">";
    s += message;
    s += "</font>";
    ui->log->appendHtml(s);
}

void MainWindow::drawChartSlot(QMap<QString, int> chartDataToDrawMap) {
    try{
        //disegno grafico runtime
        qDebug().noquote() << "disegno il grafico runtime";
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
    }catch (...) {
        printToLogSlot("Problemi nella drawChartSlot", "red");
    }
}


void MainWindow::onButtonClicked(){
    emit getMinDateForLPSTATSSig();
}

void MainWindow::LPStatsWindowCreationSlot(QString minDate, QString maxDate) {
    if (minDate.compare("")==0){
        emit fatalErrorSig("Impossibile reperire data minima.");
    }
    else {
        this->newWindow = new QWidget;
        newWindow->resize(1200, 800);

        QGridLayout *layout = new QGridLayout, *mainlayout = new QGridLayout;
        QGroupBox *box = new QGroupBox(tr("LPStats"));
        QStringList minDateAndTime = minDate.split("_");
        QStringList minSplitDate = minDateAndTime[0].split("/");
        QStringList minDplitTime = minDateAndTime[1].split(":");
        QDateTime minDateTime(QDate(minSplitDate[0].toInt(), minSplitDate[1].toInt(), minSplitDate[2].toInt()), QTime(minDplitTime[0].toInt(), minDplitTime[1].toInt()));

        QStringList maxDateAndTime = maxDate.split("_");
        QStringList maxSplitDate = maxDateAndTime[0].split("/");
        QStringList maxSplitTime = maxDateAndTime[1].split(":");
        QDateTime maxDateTime(QDate(maxSplitDate[0].toInt(), maxSplitDate[1].toInt(), maxSplitDate[2].toInt()), QTime(maxSplitTime[0].toInt(), maxSplitTime[1].toInt()));

        QDateTimeEdit *minDateTimePicker = new QDateTimeEdit;
        minDateTimePicker->setObjectName("minDateTimePicker");
        minDateTimePicker->setDisplayFormat("dd.MM.yyyy HH:mm");
        QLabel *minDateTimePickerLabel = new QLabel(tr("&Select minimum date and time:"));
        minDateTimePickerLabel->setBuddy(minDateTimePicker);
        minDateTimePicker->setMinimumDate(minDateTime.date());
        minDateTimePicker->setMinimumTime(minDateTime.time());

        QDateTimeEdit *maxDateTimePicker = new QDateTimeEdit;
        maxDateTimePicker->setObjectName("maxDateTimePicker");
        maxDateTimePicker->setDisplayFormat("dd.MM.yyyy HH:mm");
        QLabel *maxDateTimePickerLabel = new QLabel(tr("&Select maximum date and time:"));
        maxDateTimePickerLabel->setBuddy(maxDateTimePicker);
        maxDateTimePicker->setMaximumDate(maxDateTime.date());
        maxDateTimePicker->setMaximumTime(maxDateTime.time());
        maxDateTimePicker->setDate(maxDateTime.date());
        maxDateTimePicker->setTime(maxDateTime.time());

        layout->addWidget(minDateTimePickerLabel, 0, 0);
        layout->addWidget(minDateTimePicker, 0, 1);
        layout->addWidget(maxDateTimePickerLabel, 1, 0);
        layout->addWidget(maxDateTimePicker, 1, 1);

        QPushButton *submit = new QPushButton;
        submit->setText("Submit");
        connect(submit, &QPushButton::clicked, this, &MainWindow::submitDatesForLPStatsSlot);
        layout->addWidget(submit, 2, 1);
        QChartView *LPStatsChart = new QChartView;
        LPStatsChart->setObjectName("LPStatsChart");
        layout->addWidget(LPStatsChart, 3, 0);
        box->setLayout(layout);
        mainlayout->addWidget(box);
        newWindow->setLayout(mainlayout);

        newWindow->show();
    }
}

//disegna grafico del numero di mac rilevati nel periodo specificato
void MainWindow::submitDatesForLPStatsSlot() {
    try{
        QString begintime, endtime;
        QDateTimeEdit *minDateTimePicker = newWindow->findChild<QDateTimeEdit *>("minDateTimePicker");
        QDateTimeEdit *maxDateTimePicker = newWindow->findChild<QDateTimeEdit *>("maxDateTimePicker");

        if (minDateTimePicker->dateTime().toMSecsSinceEpoch() >= maxDateTimePicker->dateTime().toMSecsSinceEpoch()) {
            QMessageBox::critical(this, "", "La data di inizio intervallo deve essere minore della data di fine intervallo.");
        }
        else if (maxDateTimePicker->dateTime().toMSecsSinceEpoch() <= minDateTimePicker->dateTime().toMSecsSinceEpoch()) {
            QMessageBox::critical(this, "", "La data di fine intervallo deve essere maggiore della data di inizio intervallo.");
        }
        else {
            begintime = minDateTimePicker->dateTime().toString("yyyy/MM/dd_HH:mm");
            endtime = maxDateTimePicker->dateTime().toString("yyyy/MM/dd_HH:mm");
            emit getLPStatsSig(begintime, endtime);
        }
    }catch (...) {
        printToLogSlot("Problemi nella submitDateForLPsStatsSlot", "red");
    }
    //prendi begin time ed end time dalle tendine che ci sono sul grafico
    //    QList<QPointF> *list = server.DrawOldCountMap(begintime, endtime);
    //    ui->gridLayout->addWidget(createTimeChartGroup(*list), 0, 0);

}

void MainWindow::drawLPStatsSlot(QMap<QString, QList<QString>> map) {
    QScatterSeries *mac1 = new QScatterSeries();
    QScatterSeries *mac2 = new QScatterSeries();
    QScatterSeries *mac3 = new QScatterSeries();

    QList<QString> timestampList;
    QStringList macs;
    QList<qint64> timestamps;
    int j = 1;
    for (auto i = map.begin(); i != map.end(); i++) {
        macs.append(i.key());
        timestampList = i.value();
        for (auto timestamp : timestampList) {
            QStringList dateAndTime = timestamp.split("_");
            QStringList splitDate = dateAndTime[0].split("/");
            QStringList splitTime = dateAndTime[1].split(":");
            QDateTime dateTime(QDate(splitDate[0].toInt(), splitDate[1].toInt(), splitDate[2].toInt()), QTime(splitTime[0].toInt(), splitTime[1].toInt()));
            if (j == 1)
                mac1->append(dateTime.toMSecsSinceEpoch(), j);
            else if (j == 2)
                mac2->append(dateTime.toMSecsSinceEpoch(), j);
            else if (j == 3)
                mac3->append(dateTime.toMSecsSinceEpoch(), j);
            if (!timestamps.contains(dateTime.toMSecsSinceEpoch())){
                int i = 0;
                if (timestamps.size() == 0)
                    timestamps.append(dateTime.toMSecsSinceEpoch());
                for (auto t = timestamps.begin(); t!=timestamps.end(); t++) {
                    if (*t<dateTime.toMSecsSinceEpoch()) {
                        timestamps.insert(i, dateTime.toMSecsSinceEpoch());
                        break;
                    }
                    i++;
                }

            }
        }
        j++;
    }
    qint64 delta;
    if (timestamps.size()==1) {
        delta = timestamps.front()+600000;
    }
    else
        delta = (timestamps.front() - timestamps.back())/8;

    mac1->setMarkerSize(10);
    mac2->setMarkerSize(10);
    mac3->setMarkerSize(10);

    QChart *chart = new QChart();
    chart->addSeries(mac1);
    chart->addSeries(mac2);
    chart->addSeries(mac3);
    this->setMouseTracking(true);

    // setta gli assi
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setFormat("yy/MM/dd <br> hh:mm");
    axisX->setMin(QDateTime::fromMSecsSinceEpoch(timestamps.back()-delta));
    axisX->setMax(QDateTime::fromMSecsSinceEpoch(delta + timestamps.front()));
    axisX->setTitleText("timestamp");
    chart->addAxis(axisX, Qt::AlignBottom);
    mac1->attachAxis(axisX);
    mac2->attachAxis(axisX);
    mac3->attachAxis(axisX);


    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%d");
    axisY->setRange(0, 4);
//    axisY->setTickCount(1);
    axisY->setTitleText("mac");
    chart->addAxis(axisY, Qt::AlignLeft);
    mac1->attachAxis(axisY);
    mac2->attachAxis(axisY);
    mac3->attachAxis(axisY);

    chart->setTitle("Long period statistics");
    chart->setAcceptHoverEvents(true);
    LPStatsView.init(mac1, mac2, mac3, macs);

    QChartView *mapView = newWindow->findChild<QChartView *>("LPStatsChart");
    mapView->setRenderHint(QPainter::Antialiasing);
    mapView->setChart(chart);


}


MainWindow::~MainWindow()
{
    delete ui;
}
