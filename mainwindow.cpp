#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    showWindow(true),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (!server.listen(QHostAddress("192.168.1.172"), 9999)) {

        QMessageBox::critical(this, tr("Server"), tr("Enable to start"));
        showWindow = false;
        return;
    }
    connect(server, SIGNAL(acceptError()), this, SLOT(serverError()));

    server.setLog(ui->log);
    ui->log->insertPlainText("Server started\nip: 192.168.1.172\nport: 9999\n\n");

}

void MainWindow::serverError(){
    qDebug().noquote() << "errore server";
}

MainWindow::~MainWindow()
{
    delete ui;
}
