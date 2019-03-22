#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <myserver.h>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    void serverInit();
    ~MainWindow();

signals:
    void error(QString message);

public slots:
    void fatalError(QString message);
    void printToLog(QString message);

private:
    Ui::MainWindow *ui;
    MyServer server;
};

#endif // MAINWINDOW_H
