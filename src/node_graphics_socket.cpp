//
// Created by Charlie Zhong on 2021/9/10.
//

#include "node_graphics_socket.h"

#include "node_socket.h"
#include "node_node.h"
#include "node_wire.h"
#include "node_graphics_node.h"
#include "node_graphics_wire.h"


QDMGraphicsSocket::QDMGraphicsSocket(Socket *socket, SOCKET_TYPE socket_type):
    QGraphicsItem(dynamic_cast<QGraphicsItem *>(socket->node->grNode)),
    socket(socket),
    radius(6.0),                                     // 半径大小
    outlineWidth(1.0),                               // 轮廓线宽
    colorOutline(QColor(0x00, 0x00, 0x00, 0xFF)),    // 轮廓颜色
    pen(QPen(colorOutline))                          // 背景颜色
{
    this->_colors = {
            QColor(0xFF, 0x77, 0x00, 0xFF),
            QColor(0x52, 0xE2, 0x20, 0xFF),
            QColor(0x00, 0x56, 0xA6, 0xFF),
            QColor(0xA8, 0x6D, 0xB1, 0xFF),
            QColor(0xB5, 0x47, 0x47, 0xFF),
            QColor(0xDB, 0xE2, 0x20, 0xFF),
    };
    this->colorBackGround = this->_colors[socket_type]; // 背景颜色
    this->pen.setWidthF(this->outlineWidth);
    this->brush = QBrush(colorBackGround);

    this->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

void
QDMGraphicsSocket::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                       QWidget *widget=Q_NULLPTR)
{
    // 绘制圆圈
    painter->setBrush(this->brush);
    painter->setPen(this->pen);
    painter->drawEllipse(-this->radius, -this->radius, 2 * this->radius, 2 * this->radius);
}

QRectF QDMGraphicsSocket::boundingRect() const
{
    return QRectF(
            -this->radius - this->outlineWidth,
            -this->radius - this->outlineWidth,
            2 * (this->radius + this->outlineWidth),
            2 * (this->radius + this->outlineWidth)
    );
}

