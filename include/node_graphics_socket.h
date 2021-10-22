//
// Created by Charlie Zhong on 2021/9/10.
//

#ifndef NODETILER_NODE_GRAPHICS_SOCKET_H
#define NODETILER_NODE_GRAPHICS_SOCKET_H

#include <QtWidgets>
#include <QtCore>
#include <QtGui>

#include "node_common.h"

class Socket;
class Wire;

class QDMGraphicsSocket : public QGraphicsItem
{
public:
    explicit QDMGraphicsSocket(Socket *socket, SOCKET_TYPE socket_type=SCT_TYPE_1,
                               std::string text="");
    ~QDMGraphicsSocket() override = default;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QRectF boundingRect() const override;
    Socket *socket;
    enum { Type = GRAPH_TYPE_SOCKET };
    int type() const override { return Type; }           // 用于在scene事件中判断对象类型，以及cast
private:
    std::string text;
    float radius;
    float outlineWidth;
    std::vector<QColor> _colors;
    QColor colorBackGround;
    QColor colorOutline;
    QPen pen;
    QBrush brush;
};

#endif //NODETILER_NODE_GRAPHICS_SOCKET_H
