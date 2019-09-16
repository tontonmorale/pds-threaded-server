#include "maphovering.h"
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsScene>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtWidgets/QGraphicsTextItem>
#include "callout.h"
#include <QtGui/QMouseEvent>

MapHovering::MapHovering(QWidget *parent)
    : QGraphicsView(new QGraphicsScene, parent),
      chartTooltip(nullptr),
      mapTooltip(nullptr)
{
}

void MapHovering::mapInit(QScatterSeries *mapseries, QMap<QString, Person> people) {

    this-> people = people;
    this->map = mapseries->chart();

    setRenderHint(QPainter::Antialiasing);
    scene()->clear();
    scene()->addItem(this->map);

    connect(mapseries, &QScatterSeries::hovered, this, &MapHovering::drawMapTooltip);
    mapTooltip = new Callout(map);

    this->setMouseTracking(true);
}

void MapHovering::drawMapTooltip(QPointF point, bool state) {
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
                return;
            }
        }
    } else {
        mapTooltip->hide();
    }
}
