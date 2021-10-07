//
// Created by Charlie Zhong on 2021/9/19.
//

#ifndef NODETILER_NODE_GRAPHICS_CUTLINE_H
#define NODETILER_NODE_GRAPHICS_CUTLINE_H

# include <QtWidgets>
# include <QtGui>
# include <QtCore/QCollatorSortKey>

class QDMCutLine : public QGraphicsItem
{
public:
    QDMCutLine(QGraphicsItem *parent=Q_NULLPTR);
    ~QDMCutLine() override = default;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    QVector<QPointF> linePoints;
private:
    QPen _pen;
};

#endif //NODETILER_NODE_GRAPHICS_CUTLINE_H
