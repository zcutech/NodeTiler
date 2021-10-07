//
// Created by Charlie Zhong on 2021/9/11.
//

#include <cmath>

#include "node_graphics_wire.h"
#include "node_wire.h"
#include "node_scene.h"
#include "node_socket.h"
#include "node_graphics_scene.h"

#include <iostream>

qint64 QDMGraphicsWire::lastReleaseWireTime = 0;

QDMGraphicsWire::QDMGraphicsWire(Wire *wire, QGraphicsItem *parent):
    QGraphicsPathItem(parent),
    wire(wire),
    _color(QColor(0x00, 0x10, 0x00)),
    _colorSelected(QColor(0x00, 0xFF, 0x00)),
    _colorDeleting(QColor(0xCC, 0x00, 0x00)),
    _colorLinkable(QColor(0xC0, 0xFF, 0x3E)),
    _pen(QPen(_color)),
    _penSelected(_colorSelected),
    _penDragging(_color),
    _penDeleting(_colorDeleting),
    _penLinkable(_colorLinkable),
    posSrc(QPointF(0, 0)),
    posDes(QPointF(200, 100))
{
    this->_pen.setWidthF(1.7);
    this->_penSelected.setWidthF(1.7);
    this->_penDragging.setWidthF(1.6);
    this->_penDragging.setStyle(Qt::DashLine);                      // 以虚线呈现
    this->_penDeleting.setWidthF(1.4);
    this->_penDeleting.setStyle(Qt::DashLine);                      // 以虚线呈现
    this->_penLinkable.setWidthF(1.4);
    this->_penLinkable.setStyle(Qt::DashDotLine);                   // 以虚线呈现

    this->setFlags(QGraphicsItem::ItemIsSelectable);
    this->setZValue(-1);                                            // 将wire线置于node之下
}

void QDMGraphicsWire::onSelected(bool mergeLastDeselection) const
{
    emit this->wire->scene->grScene->itemSelected(mergeLastDeselection);
}

void QDMGraphicsWire::onDeselected() const
{
    emit this->wire->scene->grScene->itemDeselected();
}

QVariant QDMGraphicsWire::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // 划选区操作时的信号由view触发，调用setSelected情况下不触发
    if (change == QGraphicsItem::ItemSelectedHasChanged &&
        !this->wire->scene->grScene->isSelecting &&
        !this->data(VIEW_A_ITEM_SET_SELECTED).toBool())
    {
        if (this->isSelected()) {
            auto nodeJumpSelected = QDateTime::currentMSecsSinceEpoch() -
                    QDMGraphicsWire::lastReleaseWireTime < 50;
            this->onSelected(nodeJumpSelected);
        } else {
            QDMGraphicsWire::lastReleaseWireTime = QDateTime::currentMSecsSinceEpoch();
            this->onDeselected();
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

// src 一定是在输出端口
void QDMGraphicsWire::setSrcPos(QPointF p)
{
    this->posSrc = p;
}

// des 一定是在输入端口
void QDMGraphicsWire::setDesPos(QPointF p)
{
    this->posDes = p;
}

// 修复dragging中的edge在放大的视图时不可见
QPainterPath QDMGraphicsWire::shape() const
{
    return *(const_cast<QDMGraphicsWire*>(this)->calcPath());
}

void
QDMGraphicsWire::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    this->setPath(*this->calcPath());

    auto pen = this->isSelected() ? this->_penSelected : this->_pen;
    if (this->wire->state & WIRE_STATE::WIRE_STATE_HANG)
        pen = this->_penDragging;
    else {
        if (!(this->wire->state & WIRE_STATE::WIRE_STATE_HEAD) ||
        !(this->wire->state & WIRE_STATE::WIRE_STATE_TAIL)) {
            if (this->wire->state & WIRE_STATE::WIRE_STATE_GOON)
                pen = this->_penLinkable;
            else
                pen = this->_penDeleting;
        }
    }
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(this->path());
}

bool QDMGraphicsWire::intersectsWith(QPointF p1, QPointF p2)
{
    auto cutPath = new QPainterPath(p1);
    cutPath->lineTo(p2);
    auto path = this->calcPath();
    return cutPath->intersects(*path);
}

// 更新计算待绘制的线条路径，A to B
QPainterPath* QDMGraphicsWireDirect::calcPath()
{
    auto d_path = new QPainterPath(this->posSrc);
    d_path->lineTo(this->posDes);
    return d_path;
}

// 更新计算待绘制的线条路径，A to B
QPainterPath* QDMGraphicsWireBezier::calcPath()
{
    auto s = this->posSrc;
    auto d = this->posDes;
    auto dist = (d.x() - s.x()) * 0.5;

    // 计算贝塞尔曲线的控制点坐标，使双向创建时均能正常显示
    auto cpx_s = +dist; auto cpx_d = -dist;
    auto cpy_s = 0.0; auto cpy_d = 0.0;

    if (s.x() > d.x()) {
        cpx_d *= -1; cpx_s *= -1;
        cpy_d = ( ((s.y() - d.y()) != 0) ? (s.y() - d.y()) / fabs(s.y() - d.y()) : 0.00001 )
                * WIRE_CP_ROUNDNESS;
        cpy_s = ( ((d.y() - s.y()) != 0) ? (d.y() - s.y()) / fabs(d.y() - s.y()) : 0.00001 )
                * WIRE_CP_ROUNDNESS;
    }

    auto b_path = new QPainterPath(this->posSrc);
    b_path->cubicTo(QPointF(s.x() + cpx_s, s.y() + cpy_s),
                    QPointF(d.x() + cpx_d, d.y() + cpy_d),
                    this->posDes);
    return b_path;
}

