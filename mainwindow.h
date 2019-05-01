#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QGridLayout>
#include <QWidget>
#include <QtCharts>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QValueAxis>
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
    QGroupBox* createTimeChartGroup(QList<QPointF> points);
    ~MainWindow();

signals:
    void error(QString message);

public slots:
    void fatalError(QString message);
    void printToLog(QString message);
    void printOldCountMap();

private:
    Ui::MainWindow *ui;
    MyServer server;
};

#endif // MAINWINDOW_H
