#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QGridLayout>
#include <QWidget>
#include <QtCharts>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGraphicsView>
#include <QtCharts/QChartGlobal>
#include <QValueAxis>
#include <myserver.h>
#include "view.h"
#include "maphovering.h"


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
    void getMinDateForLPSTATSSig();
    void getLPStatsSig(QString, QString);

public slots:
    void fatalErrorSlot(QString message);
    void printToLogSlot(QString message);
    void drawChartSlot(QMap<QString, int> chartDataToDrawMap);
    void drawMapSlot(QList<QPointF> devicesCoords, QPointF maxEspCoords, QMap<QString, Person>);
    void submitDatesForLPStatsSlot();
    void serverListenSlot();
    void onButtonClicked();
    void setMinuteSlot(int minute);
    void LPStatsWindowCreationSlot(QString, QString);
    void drawLPStatsSlot(QMap<QString, QList<QString>>);

private:
    Ui::MainWindow *ui;
    QWidget *newWindow;
    MyServer server;
    View LPStatsView;
    MapHovering mapHovering;
};

#endif // MAINWINDOW_H
