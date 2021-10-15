//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_graphics_node.h"

#include <iostream>

#include "node_node.h"
#include "node_scene.h"
#include "node_content_widget.h"
#include "node_scene_history.h"
#include "node_graphics_scene.h"


qint64 QDMGraphicsNode::lastDeSelNodeTime = 0;

QDMGraphicsNode::QDMGraphicsNode(Node *node, QGraphicsItem *parent):
    QGraphicsItem(parent),
    node(node),
    content(node->content),
    grContent(Q_NULLPTR),
    _title(""),
    title_item(Q_NULLPTR),
    wasMoved(false)
{
}

QDMGraphicsNode* QDMGraphicsNode::init()
{
    this->initSize();
    this->initAssets();
    this->initUI();

    return this;
}

void QDMGraphicsNode::initUI()
{
    this->setFlag(QGraphicsItem::ItemIsSelectable);
    this->setFlag(QGraphicsItem::ItemIsMovable);
    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges);  // 可以通过 itemChange 监听移动和尺寸的变化
    // 初始化 title
    this->initTitle();
    this->title(this->node->title());
    // 初始化 content
    this->initContent();
}

void QDMGraphicsNode::initSize()
{
    this->width = 180;
    this->height = 240;
    this->edgeRoundness = 10;
    this->edgePadding = 10.0;
    this->titleHeight = 22;
    this->titleHoriPad = 5.0;
    this->titleVertPad = 5.0;
}

void QDMGraphicsNode::initAssets()
{
    this->_titleColor = Qt::white;
    this->_titleFont = QFont("Consolas", 10);
    this->_penDefault = QPen(QColor(0x00, 0x00, 0x00, 0x7F));
    this->_penSelected = QPen(QColor(0xFF, 0xA6, 0x37, 0xFF));
    this->_brushTitle = QBrush(QColor(0x31, 0x31, 0x31, 0xFF));
    this->_brushBackGround = QBrush(QColor(0x12, 0x12, 0x12, 0xE3));
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

void QDMGraphicsNode::updateSizeFromContent()
{
    this->width = this->content->width();
    this->height = this->content->height() + this->titleHeight;
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

    if (event->button() == Qt::LeftButton && this->wasMoved) {
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
    this->title_item->setPos(this->titleHoriPad, 0);
    this->title_item->setTextWidth(this->width - 2 * this->titleHoriPad);
}

void QDMGraphicsNode::initContent()
{
    this->grContent = new QGraphicsProxyWidget(this);
    this->grContent->setWidget(this->content);
    this->content->setGeometry(this->edgePadding, this->titleHeight + this->edgePadding,
                               this->width - 2 * this->edgePadding,
                               this->height - 2 * this->edgePadding - this->titleHeight);
    this->grContent->setFlag(QGraphicsItem::ItemIsSelectable, false);
}

void QDMGraphicsNode::selectAttachedWires(bool revert) const
{
    this->node->selectAttachedWires(revert);
}

void QDMGraphicsNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                           QWidget *widget=Q_NULLPTR)
{
    // paint title
    auto path_title = new QPainterPath();
    path_title->setFillRule(Qt::WindingFill);
    path_title->addRoundedRect(0, 0, this->width, this->titleHeight,
                               this->edgeRoundness, this->edgeRoundness);
    path_title->addRect(0, this->titleHeight - this->edgeRoundness,
                        this->edgeRoundness, this->edgeRoundness);
    path_title->addRect(this->width - this->edgeRoundness, this->titleHeight - this->edgeRoundness,
                        this->edgeRoundness, this->edgeRoundness);
    painter->setPen(Qt::NoPen);
    painter->setBrush(this->_brushTitle);
    painter->drawPath(path_title->simplified());

    // paint content
    auto path_content = new QPainterPath();
    path_content->setFillRule(Qt::WindingFill);
    path_content->addRoundedRect(0, this->titleHeight, this->width, this->height - this->titleHeight,
                                 this->edgeRoundness, this->edgeRoundness);
    path_content->addRect(0, this->titleHeight, this->edgeRoundness, this->edgeRoundness);
    path_content->addRect(this->width - this->edgeRoundness, this->titleHeight,
                          this->edgeRoundness, this->edgeRoundness);
    painter->setPen(Qt::NoPen);
    painter->setBrush(this->_brushBackGround);
    painter->drawPath(path_content->simplified());

    // paint outline
    auto path_outline = new QPainterPath();
    path_outline->addRoundedRect(0, 0, this->width, this->height,
                                 this->edgeRoundness, this->edgeRoundness);
    painter->setPen(!(this->isSelected()) ? this->_penDefault : this->_penSelected);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path_outline->simplified());
}
