//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_graphics_node.h"

#include "node_node.h"
#include "node_scene.h"
#include "node_content_widget.h"
#include "node_scene_history.h"
#include "node_graphics_scene.h"

#include <iostream>
#include <utility>


qint64 QDMGraphicsNode::lastDeSelNodeTime = 0;

QDMGraphicsNode::QDMGraphicsNode(Node *node, QGraphicsItem *parent):
    QGraphicsItem(parent),
    node(node),
    content(node->content),
    grContent(Q_NULLPTR),
    _title(""),
    title_item(Q_NULLPTR),
    wasMoved(false),
    width(180),                         // 节点宽度
    height(240),                        // 节点高度
    edgeSize(10),                       // 圆角半径
    titleHeight(22),                    // 标题高度
    _padding(5.0),                      // 标题边距
    _titleColor(Qt::white),
    _titleFont(QFont("Consolas", 10)),
    _penDefault(QPen(QColor(0x00, 0x00, 0x00, 0x7F))),
    _penSelected(QPen(QColor(0xFF, 0xA6, 0x37, 0xFF))),
    _brushTitle(QBrush(QColor(0x31, 0x31, 0x31, 0xFF))),
    _brushBackGround(QBrush(QColor(0x12, 0x12, 0x12, 0xE3)))
{
    this->setFlag(QGraphicsItem::ItemIsSelectable);
    this->setFlag(QGraphicsItem::ItemIsMovable);
    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges);   // 可以通过 itemChange 监听移动和尺寸的变化

    // 初始化 title
    this->initTitle();
    this->title(this->node->title());
    // 初始化 content
    this->initContent();

}

QString QDMGraphicsNode::title() const
{
    return this->_title;
}

void QDMGraphicsNode::title(const QString& t)
{
    this->_title = t;
    this->title_item->setPlainText(this->_title);
}

void QDMGraphicsNode::onSelected(bool mergeLastDeselection)
{
    emit this->node->scene->grScene->itemSelected(mergeLastDeselection);
}

void QDMGraphicsNode::onDeselected()
{
    emit this->node->scene->grScene->itemDeselected();
}

QVariant QDMGraphicsNode::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange)
        if (this->isSelected())
            this->node->updateAttachedWires();

    // 划选区操作时的信号由view触发，调用setSelected情况下不触发(通过设置data)，只计入鼠标点选的
    if (change == QGraphicsItem::ItemSelectedHasChanged &&
        !this->node->scene->grScene->isSelecting &&
        !this->data(VIEW_A_ITEM_SET_SELECTED).toBool())
    {
        if (this->isSelected()) {
            auto nodeJumpSelected = QDateTime::currentMSecsSinceEpoch() -
                    QDMGraphicsNode::lastDeSelNodeTime < 50;
            this->onSelected(nodeJumpSelected);
        } else {
            QDMGraphicsNode::lastDeSelNodeTime = QDateTime::currentMSecsSinceEpoch();
            this->onDeselected();
        }
    }

    return QGraphicsItem::itemChange(change, value);
}

void QDMGraphicsNode::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mousePressEvent(event);
}

void QDMGraphicsNode::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    this->wasMoved = true;
}

void QDMGraphicsNode::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);

    if (this->wasMoved) {
        this->wasMoved = false;
        this->node->scene->history->storeHistory("Node Moved", VIEW_HIST::MOVE_ITEMS, true);
    }
}

QRectF QDMGraphicsNode::boundingRect() const
{
    return QRectF(
            0,
            0,
            this->width,
            this->height
    ).normalized();
}

void QDMGraphicsNode::initTitle()
{
    this->title_item = new QGraphicsTextItem(this);
    this->title_item->setFlag(QGraphicsItem::ItemIsSelectable, false);
    this->title_item->setDefaultTextColor(this->_titleColor);
    this->title_item->setFont(this->_titleFont);
    this->title_item->setPos(this->_padding, 0);
    this->title_item->setTextWidth(this->width - 2 * this->_padding);
}

void QDMGraphicsNode::initContent()
{
    this->grContent = new QGraphicsProxyWidget(this);
    this->content->setGeometry(this->edgeSize, this->titleHeight + this->edgeSize,
                               this->width - 2*this->edgeSize, this->height - 2*this->edgeSize - this->titleHeight);
    this->grContent->setWidget(this->content);
    this->grContent->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

void QDMGraphicsNode::selectAttachedWires(bool revert) const
{
    this->node->selectAttachedWires(revert);
}

void QDMGraphicsNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget=Q_NULLPTR)
{
    // 标题
    auto path_title = new QPainterPath();
    path_title->setFillRule(Qt::WindingFill);
    path_title->addRoundedRect(0, 0, this->width, this->titleHeight, this->edgeSize, this->edgeSize);
    path_title->addRect(0, this->titleHeight - this->edgeSize, this->edgeSize, this->edgeSize);
    path_title->addRect(this->width - this->edgeSize, this->titleHeight - this->edgeSize, this->edgeSize, this->edgeSize);
    painter->setPen(Qt::NoPen);
    painter->setBrush(this->_brushTitle);
    painter->drawPath(path_title->simplified());

    // 内容
    auto path_content = new QPainterPath();
    path_content->setFillRule(Qt::WindingFill);
    path_content->addRoundedRect(0, this->titleHeight, this->width, this->height - this->titleHeight,
                                 this->edgeSize, this->edgeSize);
    path_content->addRect(0, this->titleHeight, this->edgeSize, this->edgeSize);
    path_content->addRect(this->width - this->edgeSize, this->titleHeight, this->edgeSize, this->edgeSize);
    painter->setPen(Qt::NoPen);
    painter->setBrush(this->_brushBackGround);
    painter->drawPath(path_content->simplified());

    // 轮廓
    auto path_outline = new QPainterPath();
    path_outline->addRoundedRect(0, 0, this->width, this->height, this->edgeSize, this->edgeSize);
    painter->setPen(!(this->isSelected()) ? this->_penDefault : this->_penSelected);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path_outline->simplified());
}
