//
// Created by Charlie Zhong on 2021/9/6.
//


#include <cmath>

#include <QtWidgets>

#include "node_graphics_view.h"

#include "node_graphics_wire.h"
#include "node_graphics_socket.h"
#include "node_graphics_scene.h"
#include "node_graphics_cutline.h"
#include "node_graphics_selection.h"
#include "node_graphics_node.h"
#include "node_wire.h"
#include "node_socket.h"
#include "node_node.h"
#include "node_scene.h"
#include "node_scene_history.h"


QDMGraphicsView::QDMGraphicsView(QDMGraphicsScene *_grScene, QWidget *parent):
        QGraphicsView(parent),
        grScene(_grScene),
        zoomInFactor(1.25),
        zoomClamp(true),
        zoom(10),
        zoomStep(1),
        zoomRange({0, 15}),
        _elementAction(VIEW_A_NOOP),
        lastSceneMousePosition(QPointF(0, 0)),
        last_LMB_ClickScenePos(QPointF(0, 0)),
        last_LMB_ClickSceneObj(Q_NULLPTR),
        last_LMB_ClickSceneEvt(Q_NULLPTR),
        lastPressSocket(Q_NULLPTR),
        _socketDraggingWire(Q_NULLPTR),
        _socketDraggingEvent(Q_NULLPTR),
        _socketDraggingShift(false),
        _dragWireBeenHung(false),
        viewState(0),
        selectionPos(this->pos()),
        selPreSet({}),
        _viewSelectingGr(Q_NULLPTR),
        cutLine(Q_NULLPTR),
        editingFlag(false),
        _dragEnterListeners({}),
        _dropListeners({})
{
    this->setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing |
                         QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    this->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 使 view 以鼠标光标位置为中心进行缩放
    this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    // 设置选区功能
    // this->setDragMode(QGraphicsView::RubberBandDrag);
    // this->setRubberBandSelectionMode(Qt::ContainsItemShape);

    this->cutLine = new QDMCutLine();
    this->grScene->addItem(this->cutLine);

    this->setScene(this->grScene);

    // 开启拖拽放置
    this->setAcceptDrops(true);
}

void QDMGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    for (const auto& callback : this->_dragEnterListeners)
        callback(event);
}

void QDMGraphicsView::dropEvent(QDropEvent *event)
{
    for (const auto& callback : this->_dropListeners)
        callback(event);
}

void QDMGraphicsView::addDragEnterListener(const std::function<void(QDragEnterEvent *event)> &callback)
{
    this->_dragEnterListeners.push_back(callback);
}

void QDMGraphicsView::addDropListener(const std::function<void(QDropEvent *event)> &callback)
{
    this->_dropListeners.push_back(callback);
}

// 绘制选区
void QDMGraphicsView::paintSelection(QMouseEvent *event)
{
    // 矩形选区尺寸参数
    auto curPos = event->pos();
    // 选择器对象尺寸参数
    auto curPos_s = this->mapToScene(curPos);        // 需要跟随scene缩放
    auto selWidth = curPos_s.x() - this->last_LMB_ClickScenePos.x();
    auto selHeight = curPos_s.y() - this->last_LMB_ClickScenePos.y();

    // 首次创建选择器
    if (! this->_viewSelectingGr) {
        this->_viewSelectingGr = new QDMGraphicsSelection(this->last_LMB_ClickScenePos,
                                                          selWidth, selHeight);
        this->grScene->addItem(this->_viewSelectingGr);
    }
    // 选取动作开始时，时被隐藏的选择器对象重新显示在新的位置
    if (!this->_viewSelectingGr->isVisible()) {
        this->_viewSelectingGr->setPos(this->last_LMB_ClickScenePos);
        this->_viewSelectingGr->setVisible(true);
    }
    // 刷新选择器尺寸
    this->_viewSelectingGr->width = selWidth;
    this->_viewSelectingGr->height = selHeight;
    this->_viewSelectingGr->setZValue(32);
    this->_viewSelectingGr->update();

    // 选区矩形图元
    auto recWidth = curPos.x() - this->selectionPos.x();
    auto recHeight = curPos.y() - this->selectionPos.y();
    auto selRec = new QRectF(
            fmin(this->selectionPos.x(), curPos.x()), fmin(this->selectionPos.y(), curPos.y()),
            std::abs(recWidth), std::abs(recHeight));
    auto rangeItems = this->items(selRec->toRect(),
                                  recWidth < 0 ? Qt::IntersectsItemShape : Qt::ContainsItemShape);
    if (!rangeItems.empty())
        rangeItems.erase(std::remove_if(rangeItems.begin(), rangeItems.end(),
            [](const QGraphicsItem* i) { return !(i->flags() & QGraphicsItem::ItemIsSelectable); })
        );

    // 选区中的对象集合
    auto selCurSet = rangeItems.toSet();

    // 实时刷新选区外的和新选择的
    QSet<QGraphicsItem*> off_set;
    QSet<QGraphicsItem*> add_set;
    off_set = this->selPreSet - selCurSet;
    add_set = selCurSet - this->selPreSet;
    for (auto &i : off_set)
        i->setSelected(false);
    for (auto &i : add_set)
        i->setSelected(true);
    for (auto &i : selCurSet)
        // 按住Ctrl时拖拽可减选
        i->setSelected(!(event->modifiers() & Qt::ControlModifier));
    this->selPreSet = selCurSet;
}

void QDMGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        this->middleMousePress(event);
    else if (event->button() == Qt::LeftButton)
        this->leftMousePress(event);
    else if (event->button() == Qt::RightButton)
        this->rightMousePress(event);
    else
        QGraphicsView::mousePressEvent(event);
}

void QDMGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton)
        this->middleMouseRelease(event);
    else if (event->button() == Qt::LeftButton)
        this->leftMouseRelease(event);
    else if (event->button() == Qt::RightButton)
        this->rightMouseRelease(event);
    else
        QGraphicsView::mouseReleaseEvent(event);
}

void QDMGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        this->viewState |= VIEW_STATE::VIEW_S_K_CTRL_PRESSED;
    else if (event->key() == Qt::Key_Shift) {
        this->viewState |= VIEW_STATE::VIEW_S_K_SHIFT_PRESSED;
        if (this->_elementAction == VIEW_A_WIRE_BATCHING ||
            this->_elementAction == VIEW_A_WIRE_COPYING)
            this->_socketDraggingShift = true;
    }
    else if (event->key() == Qt::Key_Alt)
        this->viewState |= VIEW_STATE::VIEW_S_K_ALT_PRESSED;
    else
        QGraphicsView::keyPressEvent(event);
}

void QDMGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
        this->viewState &= ~VIEW_STATE::VIEW_S_K_CTRL_PRESSED;
    else if (event->key() == Qt::Key_Shift) {
        this->viewState &= ~VIEW_STATE::VIEW_S_K_SHIFT_PRESSED;
        if (this->_elementAction != VIEW_A_WIRE_BATCHING &&
            this->_elementAction != VIEW_A_WIRE_COPYING)
            this->_socketDraggingShift = false;
    }
    else if (event->key() == Qt::Key_Alt)
        this->viewState &= ~VIEW_STATE::VIEW_S_K_ALT_PRESSED;
    else
        QGraphicsView::keyReleaseEvent(event);
}

// 使鼠标中键按住可以拖拽视图
void QDMGraphicsView::middleMousePress(QMouseEvent *event)
{
    auto item = this->itemAtClick(event);

    // 正在拖拽创建edge时不响应中键拖拽
    if (item != Q_NULLPTR || this->_elementAction != VIEW_A_NOOP)
        return;

    auto release_event = new QMouseEvent(QEvent::MouseButtonRelease,
                                         event->localPos(), event->screenPos(),
                                         Qt::LeftButton, Qt::NoButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(release_event);
    this->setDragMode(QGraphicsView::ScrollHandDrag);
    auto fake_event = new QMouseEvent(event->type(), event->localPos(), event->screenPos(),
                                      Qt::LeftButton, event->buttons() | Qt::LeftButton, event->modifiers());
    QGraphicsView::mousePressEvent(fake_event);
}

void QDMGraphicsView::middleMouseRelease(QMouseEvent *event)
{
    auto fake_event = new QMouseEvent(event->type(), event->localPos(), event->screenPos(),
                                      Qt::LeftButton, event->button() | Qt::LeftButton, event->modifiers());
    QGraphicsView::mouseReleaseEvent(fake_event);
    this->setDragMode(QGraphicsView::NoDrag);
}

void QDMGraphicsView::leftMousePress(QMouseEvent *event)
{
    // 记录上一次鼠标点击位置
    this->last_LMB_ClickScenePos = this->mapToScene(event->pos());
    this->viewState |= VIEW_STATE::VIEW_S_LMB_PRESSED;
    auto item = this->itemAtClick(event);
    this->last_LMB_ClickSceneObj = item;
    this->last_LMB_ClickSceneEvt = event;
    this->selectionPos = event->pos();
    auto withShift = bool(event->modifiers() & Qt::ShiftModifier);
    auto withAlt = bool(event->modifiers() & Qt::AltModifier);
    if (withShift)
        this->_socketDraggingShift = true;

    /* ===========================   Alt键+左键可拖拽   ========================== */
    // 正在拖拽创建edge时不响应中键拖拽
    if (withAlt && !item && this->_elementAction == VIEW_A_NOOP) {
        this->setDragMode(QGraphicsView::ScrollHandDrag);
        QGraphicsView::mousePressEvent(event);
        return;
    }

    /* =========================   画选区或点节点或者连线   ======================== */
    if (!item || (item->type() != GRAPH_TYPE_SOCKET
        && this->grScene->isClickingOn(event->pos(), GRAPH_TYPE_NODE))
        || item->type() == GRAPH_TYPE_WIRE) {
        // 点选和画选时Shift等效Ctrl
        if (withShift) {
            event->ignore();
            auto fakeEvent = new QMouseEvent(QEvent::MouseButtonPress, event->localPos(), event->screenPos(),
                                             Qt::LeftButton, event->buttons() | Qt::LeftButton,
                                             event->modifiers() | Qt::ControlModifier);
            QGraphicsView::mousePressEvent(fakeEvent);
            // 按住Shift选择node时，同时选中其wires
            if (auto _node = this->grScene->isClickingOn(event->pos(), GRAPH_TYPE_NODE)) {
                auto grNode = qgraphicsitem_cast<QDMGraphicsNode*>(_node);
                grNode->selectAttachedWires(true);
            }
            return;
        }
    }

    /* =========================   点击在节点端口，移动创建连线   ======================== */
    if (item && item->type() == GRAPH_TYPE_SOCKET) {
        auto grSocket = qgraphicsitem_cast<QDMGraphicsSocket*>(item);
        if (!this->_socketDraggingShift) {
            // 场景1. 无shift键，点击在无连线输出端口
            // 场景2. 无shift键，点击在有连线输出端口
            // 场景3. 无shift键，点击在无连线输入端口
            if (grSocket->socket->isOutput() || !grSocket->socket->hasWire()) {
                // 首次在一个端口上左击未释放，更改action，开始拖拽创建
                if (!this->lastPressSocket) {
                    this->lastPressSocket = grSocket->socket;
                    this->_elementAction = VIEW_A_WIRE_DRAGGING;
                    this->wireDragStart(grSocket);
                }
                    // 再次在另一个端口上点击，更改action，完成拖拽创建
                else if (this->lastPressSocket != grSocket->socket) {
                    this->_elementAction = VIEW_A_NOOP;
                    this->wireDragEnd(grSocket);
                    this->lastPressSocket = Q_NULLPTR;
                    this->_dragWireBeenHung = false;
                }
            }
                // 场景4. 无shift键，点击在有连线输入端口
            else {
                // 首次在一个端口上左击未释放，更改action，开始拖拽移动
                if (!this->lastPressSocket) {
                    this->lastPressSocket = grSocket->socket;
                    this->_elementAction = VIEW_A_WIRE_SHIFTING;
                    this->wireDragTrans(grSocket->socket);
                }
            }
        }
        else {
            // 场景1. 有shift键，点击在无连线输出端口
            // 场景2. 有shift键，点击在有连线输出端口
            if (grSocket->socket->isOutput()) {
                // 首次在一个socket上左击未释放，更改action，等到原地释放后拖拽创建第一个
                if (!this->lastPressSocket && this->_elementAction != VIEW_A_WIRE_BATCHING &&
                    this->_elementAction != VIEW_A_WIRE_COPYING) {
                    this->lastPressSocket = grSocket->socket;
                    this->_elementAction = VIEW_A_WIRE_BATCHING;
                }
                    // 再次在同一个socket上左击未释放，无效
                else if (this->lastPressSocket == grSocket->socket)
                    this->lastPressSocket = Q_NULLPTR;
            }
                // 场景3. 有shift键，点击在无连线输入端口
            else if (!grSocket->socket->hasWire()) {
                // 再次在另一个socket上左击未释放，更改action，完成批量创建当前的一个
                if (this->_elementAction == VIEW_A_WIRE_BATCHING) {
                    this->lastPressSocket = grSocket->socket;
                    this->wireDragEnd(grSocket, true);
                }
                    // 再次在另一个socket上左击未释放，更改action，完成批量复制当前的一个
                else if (this->_elementAction == VIEW_A_WIRE_COPYING) {
                    this->lastPressSocket = grSocket->socket;
                    this->wireDragEnd(grSocket, true);
                }
            }
                // 场景4. 有shift键，点击在有连线输入端口
            else {
                if (this->_elementAction == VIEW_A_WIRE_BATCHING) {
                    // 再次在同一个socket上左击，无效
                    if (this->lastPressSocket == grSocket->socket)
                        this->lastPressSocket = Q_NULLPTR;
                }
                else if (this->_elementAction == VIEW_A_NOOP) {
                    this->lastPressSocket = grSocket->socket;
                    this->_elementAction = VIEW_A_WIRE_COPYING;
                }
                    // 再次在同一个socket上左击未释放，无效
                else if (this->lastPressSocket == grSocket->socket)
                    this->lastPressSocket = Q_NULLPTR;
            }
        }
        return;
    }

    /* =========================   点击在空白位置，清理残留状态   ======================== */
    this->_dragWireBeenHung = false;
    this->lastPressSocket = Q_NULLPTR;
    this->_elementAction = VIEW_A_NOOP;
    this->viewport()->unsetCursor();
    if (this->_socketDraggingWire) {
        this->_socketDraggingWire->remove();
        this->_socketDraggingWire = Q_NULLPTR;
    }

    // 在node区域中的socket等item须单独处理事件，避免不想要的可拖拽位置
    QGraphicsView::mousePressEvent(event);
}

void QDMGraphicsView::leftMouseRelease(QMouseEvent *event)
{
    this->viewState &= ~VIEW_STATE::VIEW_S_LMB_PRESSED;
    // 获取鼠标释放处图元对象
    auto item = this->itemAtClick(event);
    auto withShift = bool(event->modifiers() & Qt::ShiftModifier);
    auto withAlt = bool(event->modifiers() & Qt::AltModifier);

    /* =======================   释放鼠标左键，清除选区对象   ===================== */
    if (this->_viewSelectingGr && this->_viewSelectingGr->isVisible()
        && this->_elementAction == VIEW_A_NOOP) {
        // 恢复自定义选择器状态
        this->_viewSelectingGr->hide();
        this->grScene->isSelecting = false;
        emit this->grScene->selsChanged();
        this->selPreSet.clear();
        return;
    }

    /* ==========================   Alt键+左键可拖拽   ========================== */
    if (withAlt && this->_elementAction == VIEW_A_NOOP) {
        this->setDragMode(QGraphicsView::NoDrag);
        QGraphicsView::mousePressEvent(event);
        return;
    }

    /* ===========================   点击节点或者连线   ========================== */
    if (this->grScene->isClickingOn(event->pos(), GRAPH_TYPE_NODE)
        || (item && item->type() == GRAPH_TYPE_WIRE)
        || !item) {
        // 点选和画选时Shift等效Ctrl
        if (withShift && this->_elementAction == VIEW_A_NOOP) {
            event->ignore();
            auto fake_event = new QMouseEvent(event->type(), event->localPos(), event->screenPos(),
                                              Qt::LeftButton, Qt::NoButton,
                                              event->modifiers() | Qt::ControlModifier);
            QGraphicsView::mouseReleaseEvent(fake_event);
            return;
        }
    }

    /* ======================   释放在节点端口，完成创建连线   ===================== */
    if (item && item->type() == GRAPH_TYPE_SOCKET) {
        auto grSocket = qgraphicsitem_cast<QDMGraphicsSocket*>(item);
        if (this->_elementAction == VIEW_A_WIRE_DRAGGING) {
            // 首次在另一个端口上释放，更改action，完成拖拽创建
            if (grSocket->socket != this->lastPressSocket) {
                this->_elementAction = VIEW_A_NOOP;
                this->lastPressSocket = Q_NULLPTR;
                this->wireDragEnd(grSocket);
            }
        }
        else if (this->_elementAction == VIEW_A_WIRE_SHIFTING) {
            // 只要是曾移动到本端口外返回，更改action，完成拖拽移动
            if (this->_dragWireBeenHung) {
                this->_elementAction = VIEW_A_NOOP;
                this->lastPressSocket = Q_NULLPTR;
                this->_dragWireBeenHung = false;
                this->wireDragEnd(grSocket, false, true);
            }
        }
        else if (this->_elementAction == VIEW_A_WIRE_BATCHING) {
            // 在一个端口上立即释放，更改action，开始批量创建新的一个
            if (grSocket->socket == this->lastPressSocket)
                this->wireDragStart(grSocket, bool(this->_socketDraggingWire));
        }
        else if (this->_elementAction == VIEW_A_WIRE_COPYING) {
            // 在一个端口上立即释放，更改action，开始复制创建新的一个
            if (grSocket->socket == this->lastPressSocket)
                this->wireDragStart(grSocket, true);
        }
        return;
    }

    /* ======================   释放在其他地方，删除临时连线   ===================== */
    if (this->_socketDraggingWire || this->_elementAction != VIEW_A_NOOP) {
        auto grSocket = qgraphicsitem_cast<QDMGraphicsSocket*>(item);
        this->lastPressSocket = Q_NULLPTR;
        this->wireDragEnd(grSocket);
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void QDMGraphicsView::rightMousePress(QMouseEvent *event)
{
    this->viewState |= VIEW_STATE::VIEW_S_RMB_PRESSED;
    auto item = this->itemAtClick(event);
    if (!item && this->viewState & VIEW_STATE::VIEW_S_K_SHIFT_PRESSED &&
        (this->viewState & VIEW_STATE::VIEW_S_MOUSE_PRESSED) == VIEW_STATE::VIEW_S_RMB_PRESSED) {
        this->viewport()->setCursor(QCursor(Qt::CrossCursor));
        return;
    }

    // 删除连线
    if (this->_socketDraggingWire && this->_elementAction != VIEW_A_NOOP) {
        this->lastPressSocket = Q_NULLPTR;
        this->wireDragEnd(Q_NULLPTR);
    }

    QGraphicsView::mousePressEvent(event);
}

void QDMGraphicsView::rightMouseRelease(QMouseEvent *event)
{
    this->viewState &= ~VIEW_STATE::VIEW_S_RMB_PRESSED;
    // 结束画删除线，删除相交连线
    if (this->_elementAction == VIEW_A_WIRE_CUTTING) {
        this->cutIntersectionWires();
        this->cutLine->linePoints = {};
        this->cutLine->update();
        this->viewport()->unsetCursor();
        this->_elementAction = VIEW_A_NOOP;
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void QDMGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    auto item = this->itemAtClick(event);
    auto grabItem = this->scene()->mouseGrabberItem();
    this->viewState |= VIEW_STATE::VIEW_S_CURSOR_MOVING;
    auto withShift = bool(event->modifiers() & Qt::ShiftModifier);
    auto withAlt = bool(event->modifiers() & Qt::AltModifier);

    /* ==========================   正在拖拽视图场景   ========================== */
    if (this->dragMode() == QGraphicsView::ScrollHandDrag) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    /* ==========================   点击节点对象拖拽   ========================== */
    if (grabItem and grabItem->type() == GRAPH_TYPE_NODE)
    {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    // std::cout << "mouse Move - after 点击节点对象拖拽" << std::endl;

    /* ==========================   点击背景拖拽选区   ========================== */
    if (!this->last_LMB_ClickSceneObj &&
        (this->viewState & VIEW_STATE::VIEW_S_LMB_DRAGGING) == VIEW_STATE::VIEW_S_LMB_DRAGGING)
    {
        if (this->dragMode() != QGraphicsView::ScrollHandDrag) {
            this->paintSelection(event);                    // 绘制选区
            this->grScene->isSelecting = true;
            return;
        }
        else if (!withAlt)
            this->setDragMode(QGraphicsView::NoDrag);       // 视图拖拽时松开了alt键
    }

    /* ==========================   点击端口拖拽连线   ========================== */
    if (this->_elementAction != VIEW_A_WIRE_BATCHING &&
        this->_elementAction != VIEW_A_WIRE_COPYING)
        this->_socketDraggingShift = false;
    if (this->_elementAction == VIEW_A_WIRE_DRAGGING ||
        this->_elementAction == VIEW_A_WIRE_BATCHING ||
        this->_elementAction == VIEW_A_WIRE_SHIFTING ||
        this->_elementAction == VIEW_A_WIRE_COPYING)
    {
        if (!this->_socketDraggingWire)
            return;
        this->_socketDraggingEvent = event;
        auto pos = this->mapToScene(event->pos());
        if (!item || item->type() != GRAPH_TYPE_SOCKET) {
            this->_socketDraggingWire->state |= WIRE_STATE::WIRE_STATE_HANG;
            this->_dragWireBeenHung = true;
        }
        if (this->_socketDraggingWire->outputSocket)
            this->_socketDraggingWire->grWire->setDesPos(pos);
        else if (this->_socketDraggingWire->inputSocket)
            this->_socketDraggingWire->grWire->setSrcPos(pos);
        this->_socketDraggingWire->grWire->update();
        this->_checkEndValid(event);

        return;
    }

    /* ==========================   批量和复制功能预览   ========================== */
    // wire批量和复制功能 - 改变鼠标来预览
    if (item && item->type() == GRAPH_TYPE_SOCKET && withShift) {
        auto grSocket = qgraphicsitem_cast<QDMGraphicsSocket*>(item);
        if (this->_elementAction == VIEW_A_NOOP &&
            !(this->viewState & VIEW_STATE::VIEW_S_LMB_PRESSED)) {
            if (grSocket->socket->isOutput() || grSocket->socket->hasWire())
                this->viewport()->setCursor(QCursor(Qt::DragCopyCursor));
        }
    }

    /* ==========================   Shift+右键画删除线   ========================= */
    if (this->_elementAction == VIEW_A_WIRE_CUTTING ||
        (this->viewState & VIEW_STATE::VIEW_S_WIRE_CUTTING) == VIEW_STATE::VIEW_S_WIRE_CUTTING)
    {
        this->_elementAction = VIEW_A_WIRE_CUTTING;
        auto pos = this->mapToScene(event->pos());
        this->cutLine->linePoints.append(pos);
        this->cutLine->update();
        return;
    }

    if (!item && this->_elementAction == VIEW_A_NOOP &&
        this->dragMode() != QGraphicsView::ScrollHandDrag)
        this->viewport()->unsetCursor();

    /* ==========================   鼠标移动触发信号   =========================== */
    this->lastSceneMousePosition = this->mapToScene(event->pos());
    emit scenePosChanged(this->lastSceneMousePosition);

    QGraphicsView::mouseMoveEvent(event);
}

void QDMGraphicsView::wheelEvent(QWheelEvent *event)
{
    // 竖向Wheel + ALT = 横向Wheel， 反应在不同的angleDelta
    auto angleDelta = (event->modifiers() & Qt::AltModifier) ?
                      event->angleDelta().x() : event->angleDelta().y();

    // 计算缩放因子
    float zoomOutFactor = 1.0f / this->zoomInFactor;
    float zoomFactor;

    // 计算缩放
    if (angleDelta > 0) {
        zoomFactor = this->zoomInFactor;
        this->zoom += this->zoomStep;
    } else {
        zoomFactor = zoomOutFactor;
        this->zoom -= this->zoomStep;
    }

    bool clamped = false;
    if (this->zoom < this->zoomRange.first) {
        this->zoom = this->zoomRange.first;
        clamped = true;
    }
    if (this->zoom > this->zoomRange.second) {
        this->zoom = this->zoomRange.second;
        clamped = true;
    }

    if (!clamped || !this->zoomClamp) {
        // 同时解决拖拽时缩放的锚点问题
        auto viewPos = event->pos();
        auto scenePos = this->mapToScene(viewPos);
        this->centerOn(scenePos);
        // 设置 scene 比例
        this->scale(zoomFactor, zoomFactor);
        auto delta = this->mapToScene(viewPos) - this->mapToScene(this->viewport()->rect().center());
        this->centerOn(scenePos - delta);
    }
}

void QDMGraphicsView::deleteSelected(const QString& cutDesc)
{
    for (auto item : this->grScene->selectedItems()) {
        if (item->type() == GRAPH_TYPE_WIRE) {
            auto grWire = qgraphicsitem_cast<QDMGraphicsWire*>(item);
            grWire->wire->remove();
        } else if (item->type() == GRAPH_TYPE_NODE) {
            auto grNode = qgraphicsitem_cast<QDMGraphicsNode*>(item);
            grNode->node->remove();
        }
    }
    if (!cutDesc.isEmpty())
        this->grScene->scene->history->storeHistory(cutDesc, VIEW_HIST::CUT_ITEMS, true);
    else
        this->grScene->scene->history->storeHistory("Delete selected", VIEW_HIST::DELETE_ITEMS, true);
}

// return the object item on the position of mouse clicking
QGraphicsItem* QDMGraphicsView::itemAtClick(QMouseEvent *event)
{
    auto pos = event->pos();
    auto obj = this->itemAt(pos);
    // 所在位置不是QGraphicsItem类型对象时，返回空指针
    return obj;
}

// 检查和确保wire终点有效
bool QDMGraphicsView::_checkEndValid(QMouseEvent *event) {
    if (event == Q_NULLPTR)
        return true;

    Socket *boundSocket;
    if (this->_socketDraggingWire->state & WIRE_STATE::WIRE_STATE_HEAD)
        boundSocket = this->_socketDraggingWire->startSocket();      // 已绑定的一端
    else
        boundSocket = this->_socketDraggingWire->endSocket();
    auto grItem = this->itemAtClick(event);
    if (grItem && grItem->type() == GRAPH_TYPE_SOCKET) {
        this->_socketDraggingWire->state &= ~WIRE_STATE::WIRE_STATE_HANG;
        auto item = qgraphicsitem_cast<QDMGraphicsSocket*>(grItem);
        auto socket = item->socket;
        // 输入端口只允许一个wire
        if (!socket->isOutput() && socket->hasWire()) {
            this->_socketDraggingWire->state &= ~WIRE_STATE::WIRE_STATE_GOON;
            return false;
        }
        // 两个端口不能同时为输入或输出
        else if ((boundSocket->isOutput() ^ socket->isOutput()) == 0) {
            this->_socketDraggingWire->state &= ~WIRE_STATE::WIRE_STATE_GOON;
            return false;
        }
        // 两个端口不能在同一个node上
        else if (boundSocket->node == socket->node) {
            this->_socketDraggingWire->state &= ~WIRE_STATE::WIRE_STATE_GOON;
            return false;
        }
        // 可以正常连接
        else {
            this->_socketDraggingWire->state |= WIRE_STATE::WIRE_STATE_GOON;
            return true;
        }
    }

    return true;
}

void QDMGraphicsView::wireDragStart(QGraphicsItem *grItem, bool batch) {
    auto item = qgraphicsitem_cast<QDMGraphicsSocket*>(grItem);
    auto socket = item->socket;

    if (!this->_socketDraggingWire && !batch)
        this->_socketDraggingWire = new Wire(
                this->grScene->scene, socket, Q_NULLPTR, WIRE_TYPE_BEZIER);
    else {
        Wire *curWire;
        Socket *startSocket;
        Socket *endSocket;
        if (this->_socketDraggingWire)
            curWire = this->_socketDraggingWire;
        else
            curWire = socket->wires[0];
        if (socket == curWire->endSocket()) {
            startSocket = curWire->startSocket();
            endSocket = Q_NULLPTR;
        }
        else {
            startSocket = Q_NULLPTR;
            endSocket = curWire->endSocket();
        }
        auto wireType = curWire->wireType();
        this->_socketDraggingWire = new Wire(
                this->grScene->scene, startSocket, endSocket, wireType);
    }

    this->_socketDraggingWire->updatePositions();
}

void QDMGraphicsView::wireDragEnd(QDMGraphicsSocket *grSocket, bool batch, bool afterTrans)
{
    if (!batch)
        this->_elementAction = VIEW_A_NOOP;

    if (grSocket && this->_socketDraggingWire) {
        auto socket = grSocket->socket;
        if (this->_checkEndValid(this->_socketDraggingEvent)) {
            if (this->_socketDraggingWire->state & WIRE_STATE::WIRE_STATE_HEAD) {
                this->_socketDraggingWire->endSocket(socket);
                this->_socketDraggingWire->state |= WIRE_STATE::WIRE_STATE_TAIL;
            } else {
                this->_socketDraggingWire->startSocket(socket);
                this->_socketDraggingWire->state |= WIRE_STATE::WIRE_STATE_HEAD;
            }

            this->_socketDraggingWire->updatePositions();
            if (!batch)
                this->_socketDraggingWire = Q_NULLPTR;
            auto desc = afterTrans? "Drag wire end to a new socket" : "Create new wire by dragging";
            this->grScene->scene->history->storeHistory(desc, VIEW_HIST::CREATE_ITEMS, true);
            return;
        }
    }

    this->viewport()->unsetCursor();
    this->_elementAction = VIEW_A_NOOP;
    this->lastPressSocket = Q_NULLPTR;
    if (this->_socketDraggingWire) {
        this->_socketDraggingWire->remove();
        this->_socketDraggingWire = Q_NULLPTR;
    }
}

// 拖拽输入端口的wire可移动到其他socket
void QDMGraphicsView::wireDragTrans(Socket *socket)
{
    auto curWire = socket->wires[0];
    curWire->detachFromSockets(socket);
    this->_socketDraggingWire = curWire;
}

// 检查鼠标左键点击释放间距是否大于阈值
bool QDMGraphicsView::lmbClickReleaseDistAway(QMouseEvent *event) {
    auto new_lmb_ReleaseScenePos = this->mapToScene(event->pos());
    auto dragVec = new_lmb_ReleaseScenePos - this->last_LMB_ClickScenePos;
    auto dragSqr = dragVec.x() * dragVec.x() + dragVec.y() * dragVec.y();
    return dragSqr > SCT_DRAG_START_THRESHOLD * SCT_DRAG_START_THRESHOLD;
}

void QDMGraphicsView::cutIntersectionWires()
{
    auto hit = false;
    for (size_t ix = 0; ix < this->cutLine->linePoints.size() - 1; ++ix) {
        auto p1 = this->cutLine->linePoints[ix];
        auto p2 = this->cutLine->linePoints[ix + 1];
        for (auto &wire : this->grScene->scene->wires) {
            if (wire->grWire->intersectsWith(p1, p2)) {
                hit = true;
                wire->remove();
            }
        }
    }
    if (hit)
        this->grScene->scene->history->storeHistory("Delete cut wires", VIEW_HIST::DELETE_ITEMS, true);
}
