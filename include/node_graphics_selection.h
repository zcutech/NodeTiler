//
// Created by Charlie Zhong on 2021/9/19.
//

#ifndef NODETILER_NODE_GRAPHICS_SELECTION_H
#define NODETILER_NODE_GRAPHICS_SELECTION_H

#include <QtWidgets>


class QDMGraphicsSelection : public QGraphicsItem
{
public:
    QDMGraphicsSelection(QPointF pos, float width=0.0, float height=0.0);
    ~QDMGraphicsSelection() override = default;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    QPointF pos;
    float width;
    float height;
private:
    QPen _penLessMode;
    QPen _penMoreMode;
    QBrush _brushLessMode;
    QBrush _brushMoreMode;
};

#endif //NODETILER_NODE_GRAPHICS_SELECTION_H
