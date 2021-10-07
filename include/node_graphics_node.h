//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_GRAPHICS_NODE_H
#define NODETILER_NODE_GRAPHICS_NODE_H

#include <QtWidgets>
#include <QtGui>
#include <QtCore/QDateTime>

#include "node_common.h"

class QDMNodeContentWidget;
class Node;

class QDMGraphicsNode : public QGraphicsItem
{
public:
    friend Node;
    explicit QDMGraphicsNode(Node* node, QGraphicsItem *parent = Q_NULLPTR);
    ~QDMGraphicsNode() override = default;
    QString title() const;
    void title(const QString& t);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void selectAttachedWires(bool revert=false) const;
    enum { Type = GRAPH_TYPE_NODE };
    inline int type() const override { return Type; }       // 用于在scene事件中判断对象类型，以及cast
    Node *node;
private:
    static qint64 lastDeSelNodeTime;      // 直接点击选择另一个node时，忽略取消选择事件

    void onSelected(bool mergeLastDeselection=false);
    void onDeselected();
    void initTitle();
    void initContent();
    QDMNodeContentWidget *content;
    QGraphicsProxyWidget *grContent;
    QString _title;
    QGraphicsTextItem *title_item;
    bool wasMoved;
    int width;
    int height;
    float edgeSize;              // 圆角半径
    int titleHeight;             // 标题高度
    float _padding;              // 标题边距
    QColor _titleColor;
    QFont _titleFont;
    QPen _penDefault;
    QPen _penSelected;
    QBrush _brushTitle;
    QBrush _brushBackGround;
};


#endif //NODETILER_NODE_GRAPHICS_NODE_H
