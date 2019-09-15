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
      m_coordX(nullptr),
      m_coordY(nullptr),
      chartTooltip(nullptr),
      mapTooltip(nullptr)
{
}

void View::init(QScatterSeries *mac1, QScatterSeries *mac2, QScatterSeries *mac3, QStringList macs) {

    if (scene()->items().contains(this->chart))
        scene()->removeItem(this->chart);

    this->macs = macs;
    this->chart = mac1->chart();

    setRenderHint(QPainter::Antialiasing);

    scene()->addItem(this->chart);

    connect(mac1, &QScatterSeries::clicked, this, &View::keepCallout);
    connect(mac1, &QScatterSeries::hovered, this, &View::drawChartTooltip);

    connect(mac2, &QScatterSeries::clicked, this, &View::keepCallout);
    connect(mac2, &QScatterSeries::hovered, this, &View::drawChartTooltip);

    connect(mac3, &QScatterSeries::clicked, this, &View::keepCallout);
    connect(mac3, &QScatterSeries::hovered, this, &View::drawChartTooltip);
    chartTooltip = new Callout(chart);

    this->setMouseTracking(true);
}

void View::mapInit(QScatterSeries *mapseries, QMap<QString, Person> people) {
    if (scene()->items().contains(this->map))
        scene()->removeItem(this->map);

    this-> people = people;
    this->map = mapseries->chart();

    setRenderHint(QPainter::Antialiasing);

    scene()->addItem(this->map);

    connect(mapseries, &QScatterSeries::hovered, this, &View::drawChartTooltip);
    mapTooltip = new Callout(map);

    this->setMouseTracking(true);
}

void View::resizeEvent(QResizeEvent *event)
{
    if (scene()) {
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
         chart->resize(event->size());
         m_coordX->setPos(chart->size().width()/2 - 50, chart->size().height() - 20);
         m_coordY->setPos(chart->size().width()/2 + 50, chart->size().height() - 20);
         const auto callouts = m_callouts;
         for (Callout *callout : callouts)
             callout->updateGeometry();
    }
    QGraphicsView::resizeEvent(event);
}

void View::mouseMoveEvent(QMouseEvent *event)
{
    QString s = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(event->pos().x())).toString("yyyy/MM/dd HH:mm");
    QString mac = this->macs.at(static_cast<int>(event->pos().y()-1));
    m_coordX->setText(QString("Timestamp: " + s));
    m_coordY->setText(QString("Mac: " + mac));
    QGraphicsView::mouseMoveEvent(event);
}

void View::keepCallout()
{
    m_callouts.append(chartTooltip);
    chartTooltip = new Callout(chart);
}

void View::drawMapTooltip(QPointF point, bool state) {
    if (mapTooltip == nullptr)
        mapTooltip = new Callout(map);

    if (state) {
        for (auto person=people.begin(); person!=people.end(); person++) {
            if (person.value().getAvgPosition().x()==point.x() && person.value().getAvgPosition().y()==point.y()) {
                mapTooltip->setText(QString("Mac : " + person.key()));
                mapTooltip->setAnchor(point);
                mapTooltip->setZValue(11);
                mapTooltip->updateGeometry();
                mapTooltip->show();
                break;
            }
        }
    } else {
        chartTooltip->hide();
    }
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
