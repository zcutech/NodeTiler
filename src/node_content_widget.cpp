//
// Created by Charlie Zhong on 2021/9/10.
//

#include "node_content_widget.h"

#include "node_node.h"
#include "node_scene.h"
#include "node_graphics_scene.h"
#include "node_graphics_view.h"


QDMNodeContentWidget::QDMNodeContentWidget(Node *node, QWidget *parent):
    QWidget(parent),
    Serializable(),
    node(node),
    layout(Q_NULLPTR),
    wdgLabel(Q_NULLPTR)
{
}

QDMNodeContentWidget* QDMNodeContentWidget::init()
{
    this->layout = new QVBoxLayout();
    this->wdgLabel = new QLabel("Label Text", this);
    this->layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(this->layout);

    this->layout->addWidget(this->wdgLabel);
    this->layout->addWidget(new QDMQTextEdit(QString("Foo bar - %1").arg(this->id.c_str())));

    return this;
}

void QDMNodeContentWidget::setEditingFlag(int value) {
    auto view = dynamic_cast<QDMGraphicsView*>(this->node->scene->grScene->views()[0]);
    view->editingFlag = value;
}

json QDMNodeContentWidget::serialize()
{
    json serialData
    {
    };

    return serialData;
}

bool QDMNodeContentWidget::deserialize(json data, node_HashMap *hashMap=Q_NULLPTR,
                                       bool restoreId=true)
{
    return true;
}

QDMQTextEdit::QDMQTextEdit(QWidget *parent):
    QTextEdit(parent)
{
}

QDMQTextEdit::QDMQTextEdit(const QString &text, QWidget *parent):
    QTextEdit(text, parent)
{
}

void QDMQTextEdit::focusInEvent(QFocusEvent *e)
{
    auto p = dynamic_cast<QDMNodeContentWidget*>(this->parentWidget());
    if (p)
        p->setEditingFlag(1);
    QTextEdit::focusInEvent(e);
}

void QDMQTextEdit::focusOutEvent(QFocusEvent *e)
{
    auto p = dynamic_cast<QDMNodeContentWidget*>(this->parentWidget());
    if (p)
        p->setEditingFlag(0);
    QTextEdit::focusOutEvent(e);
}
