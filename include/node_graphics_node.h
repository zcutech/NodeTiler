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
    // 必须在构造后先调用init方法
    explicit QDMGraphicsNode(Node* node, QGraphicsItem *parent = Q_NULLPTR);
    ~QDMGraphicsNode() override = default;
    virtual QDMGraphicsNode* init();
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
protected:
    static qint64 lastDeSelNodeTime;      // 直接点击选择另一个node时，忽略取消选择事件
    void onSelected(bool mergeLastDeselection=false);
    void onDeselected();
    virtual void initSize();
    virtual void initAssets();
    virtual void initUI();
    void initTitle();
    void initContent();
    QDMNodeContentWidget *content;
    QGraphicsProxyWidget *grContent;
    QString _title;
    QGraphicsTextItem *title_item;
    bool wasMoved;
    int width;
    int height;
    float edgeRoundness;            // circle radius on the edges of a node
    float edgePadding;
    int titleHeight;
    float titleHoriPad;             // padding between title and its box edge - horizontal
    float titleVertPad;             // padding between title and its bxo edge - vertical
    QColor _titleColor;
    QFont _titleFont;
    QPen _penDefault;
    QPen _penSelected;
    QBrush _brushTitle;
    QBrush _brushBackGround;
};


#endif //NODETILER_NODE_GRAPHICS_NODE_H
