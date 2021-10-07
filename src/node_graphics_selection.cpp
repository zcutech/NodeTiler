//
// Created by Charlie Zhong on 2021/9/19.
//

#include "node_graphics_selection.h"

#include <iostream>

QDMGraphicsSelection::QDMGraphicsSelection(QPointF pos, float width, float height):
    pos(pos),
    width(width),
    height(height),
    _penLessMode(QPen(QColor(0xFF, 0xFF, 0xFF, 0x9F))),
    _penMoreMode(QPen(QColor(0xFF, 0xFF, 0xFF, 0x9F))),
    _brushLessMode(QBrush(QColor(0x00, 0xFF, 0x00, 0x2F))),
    _brushMoreMode(QBrush(QColor(0xFF, 0xFF, 0x00, 0x2F)))
{
    this->_penLessMode.setWidthF(1.0);
    this->_penLessMode.setStyle(Qt::DashLine);
    this->_penMoreMode.setWidthF(1.0);
    this->_penMoreMode.setStyle(Qt::DashLine);

    this->setPos(this->pos);
    this->setFlag(QGraphicsItem::ItemIsSelectable, false);
    this->setFlag(QGraphicsItem::ItemIsMovable, false);
}

QRectF QDMGraphicsSelection::boundingRect() const
{
    return QRectF(
            0,
            0,
            this->width,
            this->height
    ).normalized();
}

void
QDMGraphicsSelection::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                            QWidget *widget)
{
    auto pathContent = new QPainterPath();
    pathContent->setFillRule(Qt::WindingFill);
    pathContent->addRect(0, 0, this->width, this->height);
    painter->setPen(this->_penLessMode);
    if (this->width < 0)
        painter->setBrush(this->_brushLessMode);
    else
        painter->setBrush(this->_brushMoreMode);
    painter->drawPath(pathContent->simplified());
}
