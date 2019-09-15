#ifndef VIEW_H
#define VIEW_H
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

class View: public QGraphicsView
{
    Q_OBJECT

public:
    View(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

public slots:
    void keepCallout();
    void drawChartTooltip(QPointF point, bool state);
    void drawMapTooltip(QPointF point, bool state);
    void init(QScatterSeries *, QScatterSeries *, QScatterSeries *, QStringList);
    void mapInit(QScatterSeries *, QMap<QString, Person>);

private:
    QGraphicsSimpleTextItem *m_coordX, *m_coordY;
    QChart *chart, *map;
    QMap<QString, Person> people;
    Callout *chartTooltip, *mapTooltip;
    QList<Callout *> m_callouts;
    QStringList macs;
};

#endif
