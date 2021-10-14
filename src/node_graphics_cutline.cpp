//
// Created by Charlie Zhong on 2021/9/19.
//

#include "node_graphics_cutline.h"


QDMCutLine::QDMCutLine(QGraphicsItem *parent):
    QGraphicsItem(parent),
    linePoints({}),
    _pen(QPen(Qt::white))
{
    this->_pen.setWidthF(1.6);
    this->_pen.setDashPattern({2, 2});

    this->setZValue(2);
}

QRectF QDMCutLine::boundingRect() const
{
    return {0, 0, 1, 1};
}

void QDMCutLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(this->_pen);

    QPolygonF poly(this->linePoints);
    painter->drawPolyline(poly);
}

