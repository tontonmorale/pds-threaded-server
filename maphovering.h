#ifndef MAPHOVERING_H
#define MAPHOVERING_H


#include <QtWidgets/QGraphicsView>
#include <QtCharts/QChartGlobal>
#include <QChartView>
#include <QWidget>
#include <QtCharts>
#include <QtCharts/QChartGlobal>
#include "person.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QMouseEvent;
class QResizeEvent;
QT_END_NAMESPACE

QT_CHARTS_BEGIN_NAMESPACE
class QChart;
QT_CHARTS_END_NAMESPACE

class Callout;

QT_CHARTS_USE_NAMESPACE

class MapHovering: public QGraphicsView
{
    Q_OBJECT

public:
    MapHovering(QWidget *parent = 0);


public slots:
    void drawMapTooltip(QPointF point, bool state);
    void mapInit(QScatterSeries *, QMap<QString, Person>);

private:
    QChart *chart, *map;
    QMap<QString, Person> people;
    Callout *chartTooltip, *mapTooltip;
    QList<Callout *> m_callouts;
    QStringList macs;
};


#endif // MAPHOVERING_H
