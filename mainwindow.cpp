#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    connect(this, &MainWindow::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::fatalErrorSig, this, &MainWindow::fatalErrorSlot);
    connect(&server, &MyServer::logSig, this, &MainWindow::printToLogSlot);
    connect(&server, &MyServer::drawRuntimeChartSig, this, &MainWindow::drawRuntimeChartSlot);
//    connect(ui->pushButton, &QPushButton::clicked, this, &MainWindow::printOldCountMap);
    ui = new Ui::MainWindow;

    ui->setupUi(this);
}

void MainWindow::serverInit(){
    // server init
    server.init();

    if (!server.listen(QHostAddress("192.168.1.172"), 9999)) {
        emit fatalErrorSig("Errore avvio del server");
        return;
    }

    ui->log->appendPlainText("--- Server started ---\nip: 192.168.1.172\nport: 9999\n");
}

void MainWindow::fatalErrorSlot(QString message){
    qDebug().noquote() << message;
    QMessageBox::critical(this, "", message);
    exit(-1);
}

void MainWindow::printToLogSlot(QString message){
    ui->log->appendPlainText(message);
}

void MainWindow::drawRuntimeChartSlot(QList<QPointF> *runtimeList) {


    //disegno grafico runtime

//    QList<QPointF> *runtimeList = server.DrawOldCountMap(begintime, endtime);
    QWidget *qw = new QWidget;
    QGridLayout *grid = new QGridLayout;
    grid->addWidget(createTimeChartGroup(*runtimeList), 0, 0);
    qw->setLayout(grid);
    ui->centralWidget->layout()->addWidget(qw);
}

//disegna grafico del numero di mac rilevati nel periodo specificato
void MainWindow::drawOldCountChartSlot() {
    QString begintime, endtime;
    //prendi begin time ed end time dalle tendine che ci sono sul grafico
//    QList<QPointF> *list = server.DrawOldCountMap(begintime, endtime);
//    ui->gridLayout->addWidget(createTimeChartGroup(*list), 0, 0);

}

QGroupBox* MainWindow::createTimeChartGroup(QList<QPointF> points)
{
    QGroupBox *groupBox = new QGroupBox(tr("Temporal chart"));

    QLineSeries *series = new QLineSeries();

    for(QList<QPointF>::iterator i=points.begin(); i!=points.end(); i++)
        *series << *i;

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->addSeries(series);
    chart->createDefaultAxes();
    chart->setTitle("Number of people in area");

    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTickCount(13);
    chart->setAxisX(axisX, series);


    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(chartView);
    groupBox->setLayout(layout);

    return groupBox;
}

MainWindow::~MainWindow()
{
    delete ui;
}
