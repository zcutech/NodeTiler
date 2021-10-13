//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_GRAPHICS_SCENE_H
#define NODETILER_NODE_GRAPHICS_SCENE_H

#include <QtCore>
#include <QtWidgets>
#include <QtGui>

#include "node_common.h"

class Scene;
class Node;

class QDMGraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit QDMGraphicsScene(Scene *_scene, QObject *parent = Q_NULLPTR);
    ~QDMGraphicsScene() override = default;
    Scene *scene;
    bool isSelecting;               // 划选区时由view设置为true
    void setGrScene(int width, int height);
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    QGraphicsItem* isClickingOn(QPointF pos, GRAPHICS_TYPE theType);
    bool isTypeOf(QGraphicsItem *item, GRAPHICS_TYPE theType);
    bool itemIsMine(QGraphicsItem *grItem);
    Node* getNodeByGraphicItem(QGraphicsItem* item) const;
signals:
    void itemSelected(bool mergeLast);
    void itemDeselected();
    void selsChanged();             // 划选区时由view触发
private:
    int gridSize;
    int gridSquares;
    QColor colorBackGround;
    QColor colorLight;
    QColor colorDark;

    QPen penLight;
    QPen penDark;
};

#endif //NODETILER_NODE_GRAPHICS_SCENE_H
