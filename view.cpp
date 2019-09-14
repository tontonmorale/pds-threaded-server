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
      m_coordX(0),
      m_coordY(0),
      m_tooltip(0)
{
}

void View::init(QScatterSeries *mac1, QScatterSeries *mac2, QScatterSeries *mac3, QStringList macs) {

    this->macs = macs;
    // chart
    this->chart = mac1->chart();

    setRenderHint(QPainter::Antialiasing);
    scene()->clear();
    scene()->addItem(chart);

//    m_coordX = new QGraphicsSimpleTextItem(chart);
//    m_coordX->setPos(chart->size().width()/2 - 50, chart->size().height());
//    m_coordX->setText("Timestamp: ");
//    m_coordY = new QGraphicsSimpleTextItem(chart);
//    m_coordY->setPos(chart->size().width()/2 + 50, chart->size().height());
//    m_coordY->setText("Mac: ");

    connect(mac1, &QScatterSeries::clicked, this, &View::keepCallout);
    connect(mac1, &QLineSeries::hovered, this, &View::tooltip);

    connect(mac2, &QSplineSeries::clicked, this, &View::keepCallout);
    connect(mac2, &QSplineSeries::hovered, this, &View::tooltip);

    connect(mac3, &QSplineSeries::clicked, this, &View::keepCallout);
    connect(mac3, &QSplineSeries::hovered, this, &View::tooltip);
    m_tooltip = new Callout(chart);

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
    m_callouts.append(m_tooltip);
    m_tooltip = new Callout(chart);
}

void View::tooltip(QPointF point, bool state)
{
    if (m_tooltip == 0)
        m_tooltip = new Callout(chart);

    if (state) {
        QString s = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("yyyy/MM/dd HH:mm");
        QString mac = this->macs.at(static_cast<int>(point.y()-1));
        m_tooltip->setText(QString("Timestamp: " + s + " \nMac: " + mac));
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}
