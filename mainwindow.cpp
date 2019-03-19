#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    connect(this, &MainWindow::error, this, &MainWindow::fatalError);
    connect(&server, &MyServer::error, this, &MainWindow::fatalError);
    ui = new Ui::MainWindow;
    ui->setupUi(this);
}

void MainWindow::serverInit(){
    // server init
    server.init();
    server.setLog(ui->log);

    if (!server.listen(QHostAddress("192.168.1.172"), 9999)) {
        emit error("Errore avvio del server");
        return;
    }

    ui->log->insertPlainText("Server started\nip: 192.168.1.172\nport: 9999\n\n");
}

void MainWindow::fatalError(QString message){
    qDebug().noquote() << message;
    QMessageBox::critical(this, "", message);
    exit(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
}
