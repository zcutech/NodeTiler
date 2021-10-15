//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_graphics_scene.h"

#include "node_scene.h"
#include "node_socket.h"
#include "node_graphics_node.h"
#include "node_content_widget.h"
#include "node_graphics_socket.h"
#include "node_graphics_view.h"


QDMGraphicsScene::QDMGraphicsScene(Scene *_scene, QObject *parent):
    QGraphicsScene(parent),
    scene(_scene),
    isSelecting(false),
    // 属性设置
    gridSize(20),
    gridSquares(5),
    colorBackGround(QColor(0x39, 0x39, 0x39)),
    colorLight(QColor(0x2f, 0x2f, 0x2f)),
    colorDark(QColor(0x29, 0x29, 0x29)),
    penLight(QPen(this->colorLight)),
    penDark(QPen(this->colorDark))
{
    penLight.setWidth(1);
    penDark.setWidth(2);
    this->setBackgroundBrush(this->colorBackGround);
}

void QDMGraphicsScene::setGrScene(int width, int height)
{
    this->setSceneRect(-width/2.0, -height/2.0, width, height);
}

void QDMGraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsScene::drawBackground(painter, rect);

    // 创建网格
    int _left = floor(rect.left());
    int _right = ceil(rect.right());
    int _top = floor(rect.top());
    int _bottom = ceil(rect.bottom());

    int firstLeft = _left - int(_left % this->gridSize);
    int firstTop = _top - int(_top % this->gridSize);

    // 计算要显示的所有线条
    QVector<QLine> linesLight, linesDark;
    for (int x = firstLeft; x < _right; x += this->gridSize) {
        if (x % (this->gridSize * this->gridSquares) != 0)
            linesLight.push_back(QLine(x, _top, x, _bottom));
        else
            linesDark.push_back(QLine(x, _top, x, _bottom));
    }
    for (int y = firstTop; y < _bottom; y += this->gridSize) {
        if (y % (this->gridSize * this->gridSquares) != 0)
            linesLight.push_back(QLine(_left, y, _right, y));
        else
            linesDark.push_back(QLine(_left, y, _right, y));
    }

    // 绘制线条
    painter->setPen(this->penLight);
    painter->drawLines(linesLight);
    painter->setPen(this->penDark);
    painter->drawLines(linesDark);
}

// 不允许drag事件，除非重写dragMoveEvent
void QDMGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
//    QGraphicsScene::dragMoveEvent(event);
}

#include <iostream>

// if the items stack in the clicking position contains the specified type object
QGraphicsItem* QDMGraphicsScene::isClickingOn(QPointF pos, GRAPHICS_TYPE theType)
{
    QList<QGraphicsItem *> items = this->items(pos);
    for (auto i = items.rbegin(); i != items.rend(); ++i) {
        if (this->isTypeOf(*i, theType))
            return *i;
    }
    return Q_NULLPTR;
}

// if the specified object item is the specified type
bool QDMGraphicsScene::isTypeOf(QGraphicsItem *item, GRAPHICS_TYPE theType) const {
    if (item && item->type() == theType)
        return true;
    return false;
}

// if the graphic item is one of our interests object(can be moved and selected)
bool QDMGraphicsScene::itemIsMine(QGraphicsItem *grItem) {
    if (grItem == Q_NULLPTR)
        return false;
    switch (grItem->type()) {
        case GRAPH_TYPE_NODE:
        case GRAPH_TYPE_SOCKET:
        case GRAPH_TYPE_WIRE:
            return true;
        default:
            break;
    }
    return false;
}

Node* QDMGraphicsScene::getNodeByItemPos(QGraphicsItem* item, QPointF pos) const {
    auto nodeItem = this->scene->grScene->isClickingOn(pos, GRAPH_TYPE_NODE);
    if (nodeItem) {
        auto grNode = qgraphicsitem_cast<QDMGraphicsNode*>(nodeItem);
        return grNode->node;
    } else if ((nodeItem = this->scene->grScene->isClickingOn(item->pos(), GRAPH_TYPE_SOCKET))) {
        auto grSocket = qgraphicsitem_cast<QDMGraphicsSocket*>(nodeItem);
        return grSocket->socket->node;
    }
    return nullptr;
}
