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
//    QGroupBox* createTimeChartGroup(QList<QPointF> points);
    void drawPeopleCountChart();
    ~MainWindow();

signals:
    void fatalErrorSig(QString message);

public slots:
    void fatalErrorSlot(QString message);
    void printToLogSlot(QString message);
    void setChartDataSlot(QMap<QString, int> *runtimeMap);
    void drawOldCountChartSlot();

private:
    Ui::MainWindow *ui;
    MyServer server;
    QLineSeries* chartSeries;
};

#endif // MAINWINDOW_H
