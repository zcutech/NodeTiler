//
// Created by Charlie Zhong on 2021/9/11.
//

#ifndef NODETILER_NODE_GRAPHICS_WIRE_H
#define NODETILER_NODE_GRAPHICS_WIRE_H

#include <QtWidgets>
#include <QtGui>
#include <QtCore>

#include "node_common.h"

class Wire;

class QDMGraphicsWire : public QGraphicsPathItem
{
public:
    explicit QDMGraphicsWire(Wire *wire, QGraphicsItem *parent = Q_NULLPTR);
    ~QDMGraphicsWire() override = default;
    void setSrcPos(QPointF p);
    void setDesPos(QPointF p);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    bool intersectsWith(QPointF p1, QPointF p2);
    virtual QPainterPath* calcPath() = 0;
    enum { Type = GRAPH_TYPE_WIRE};
    int type() const override { return Type; }           // 用于在scene事件中判断对象类型，以及cast

    Wire *wire;
    QColor _color;
    QColor _colorSelected;                             // 选中颜色
    QColor _colorDeleting;                             // 无效预警
    QColor _colorLinkable;                             // 可以连接
    QPen _pen;
    QPen _penSelected;
    QPen _penDragging;                                 // 正在鼠标创建的wire
    QPen _penDeleting;                                 // 正在删除的wire
    QPen _penLinkable;                                 // 可以正常连接绑定
    QPointF posSrc;
    QPointF posDes;
private:
    static qint64 lastReleaseWireTime;      // 直接点击选择另一个wire时，忽略取消选择事件

    void onSelected(bool mergeLastDeselection=false) const;
    void onDeselected() const;
};

class QDMGraphicsWireDirect : public QDMGraphicsWire
{
public:
    explicit QDMGraphicsWireDirect(Wire *wire, QGraphicsItem *parent = Q_NULLPTR):
        QDMGraphicsWire(wire, parent) {}
    ~QDMGraphicsWireDirect() override = default;
    QPainterPath* calcPath() override;
};

class QDMGraphicsWireBezier : public QDMGraphicsWire
{
public:
    explicit QDMGraphicsWireBezier(Wire *wire, QGraphicsItem *parent = Q_NULLPTR):
        QDMGraphicsWire(wire, parent) {}
    ~QDMGraphicsWireBezier() override = default;
    QPainterPath* calcPath() override;
};


#endif //NODETILER_NODE_GRAPHICS_WIRE_H
