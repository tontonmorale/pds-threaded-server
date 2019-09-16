#include "view.h"
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtWidgets/QGraphicsTextItem>
#include "callout.h"
#include <QtGui/QMouseEvent>

View::View(QWidget *parent)
    : QGraphicsView(new QGraphicsScene, parent),
      chartTooltip(nullptr)
{
}

void View::init(QScatterSeries *mac1, QScatterSeries *mac2, QScatterSeries *mac3, QStringList macs) {

//    if (scene()->items().contains(this->chart))
//        scene()->removeItem(this->chart);

    this->macs = macs;
    this->chart = mac1->chart();

    setRenderHint(QPainter::Antialiasing);
    scene()->clear();
    scene()->addItem(this->chart);

    connect(mac1, &QScatterSeries::hovered, this, &View::drawChartTooltip);
    connect(mac2, &QScatterSeries::hovered, this, &View::drawChartTooltip);
    connect(mac3, &QScatterSeries::hovered, this, &View::drawChartTooltip);
    chartTooltip = new Callout(chart);

    this->setMouseTracking(true);
}


void View::drawChartTooltip(QPointF point, bool state)
{
    if (chartTooltip == nullptr)
        chartTooltip = new Callout(chart);

    if (state) {
        QString s = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("yyyy/MM/dd HH:mm");
        QString mac = this->macs.at(static_cast<int>(point.y()-1));
        chartTooltip->setText(QString("Timestamp: " + s + " \nMac: " + mac));
        chartTooltip->setAnchor(point);
        chartTooltip->setZValue(11);
        chartTooltip->updateGeometry();
        chartTooltip->show();
    } else {
        chartTooltip->hide();
    }
}
