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
    void drawChartSlot(QMap<QString, int> chartDataToDrawMap);
    void drawMapSlot(QList<QPointF> devicesCoords, QPointF maxEspCoords);
    void drawOldCountChartSlot();
    void serverListenSlot();
    void onButtonClicked();
    void setMinuteSlot(int minute);

private:
    Ui::MainWindow *ui;
    MyServer server;
};

#endif // MAINWINDOW_H
