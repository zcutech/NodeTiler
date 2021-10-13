//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_GRAPHICS_VIEW_H
#define NODETILER_NODE_GRAPHICS_VIEW_H

#include <set>

#include <QtWidgets/QGraphicsView>
#include <QtCore>

#include "node_common.h"

class QDMGraphicsScene;
class QDMGraphicsSocket;
class QDMGraphicsWire;
class QDMCutLine;
class QDMGraphicsSelection;
class Socket;
class Wire;

class QDMGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit QDMGraphicsView(QDMGraphicsScene *_grScene, QWidget *parent = Q_NULLPTR);
    ~QDMGraphicsView() override = default;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void addDragEnterListener(const std::function<void(QDragEnterEvent *event)>& callback);
    void addDropListener(const std::function<void(QDropEvent *event)>& callback);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void middleMousePress(QMouseEvent *event);
    void middleMouseRelease(QMouseEvent *event);
    void leftMousePress(QMouseEvent *event);
    void leftMouseRelease(QMouseEvent *event);
    void rightMousePress(QMouseEvent *event);
    void rightMouseRelease(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool editingFlag;                              // 是否正在TextEdit中
    QPointF last_LMB_ClickScenePos;
    QPointF lastSceneMousePosition;
    void deleteSelected(const QString& cutDesc="");
    std::vector<std::function<void(QDragEnterEvent *event)>> _dragEnterListeners;
    std::vector<std::function<void(QDropEvent *event)>> _dropListeners;
signals:
    void scenePosChanged(QPointF p);
private:
    QDMGraphicsScene *grScene;
    float zoomInFactor;
    bool zoomClamp;
    int zoom;
    int zoomStep;
    std::pair<int, int> zoomRange;
    VIEW_ACTION _elementAction;
    QGraphicsItem *last_LMB_ClickSceneObj;
    QMouseEvent *last_LMB_ClickSceneEvt;
    Socket *lastPressSocket;
    Wire *_socketDraggingWire;
    QMouseEvent *_socketDraggingEvent;             // 鼠标创建wire的事件
    bool _socketDraggingShift;
    bool _dragWireBeenHung;
    VIEW_STATE::Flags viewState;                   // 记录视图操作的状态
    QPointF selectionPos;
    QSet<QGraphicsItem*> selPreSet;
    QDMGraphicsSelection *_viewSelectingGr;
    QDMCutLine *cutLine;

    void paintSelection(QMouseEvent *event);
    QGraphicsItem* itemAtClick(QMouseEvent *event);
    bool _checkEndValid(QMouseEvent *event);
    void wireDragStart(QGraphicsItem *grItem, bool batch=false);
    void wireDragEnd(QDMGraphicsSocket *grSocket, bool batch=false, bool afterTrans=false);
    void wireDragTrans(Socket *socket);
    bool lmbClickReleaseDistAway(QMouseEvent *event);
    void cutIntersectionWires();
};

#endif //NODETILER_NODE_GRAPHICS_VIEW_H
