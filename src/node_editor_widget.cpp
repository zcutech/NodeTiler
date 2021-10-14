//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_editor_widget.h"

#include <QColor>

#include "node_common.h"
#include "node_node.h"
#include "node_wire.h"
#include "node_scene.h"
#include "node_graphics_view.h"
#include "node_graphics_scene.h"
#include "node_scene_history.h"


NodeEditorWidget::NodeEditorWidget(QApplication *app, QWidget *parent):
    QWidget(parent)
    ,app(app)
    ,layout(new QVBoxLayout())
    ,view(Q_NULLPTR)
    // 创建 GraphicsScene
    ,scene(new Scene())
    ,fileName("")
{
    this->layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(this->layout);

    // 创建 GraphicsView
    this->view = new QDMGraphicsView(this->scene->grScene, this);
    this->layout->addWidget(this->view);

//    this->add_debug_content();
}

void NodeEditorWidget::setTitle()
{;}

bool NodeEditorWidget::isModified() const
{
    return this->scene->hasBeenModified();
}

bool NodeEditorWidget::isFilenameSet() const
{
    return this->fileName != "";
}

QList<QGraphicsItem*> NodeEditorWidget::getSelectedItems() const
{
    return this->scene->getSelectedItems();
}

bool NodeEditorWidget::hasSelectedItems() const
{
    return !this->getSelectedItems().empty();
}

bool NodeEditorWidget::canUndo() const
{
    return this->scene->history->canUndo();
}

bool NodeEditorWidget::canRedo() const
{
    return this->scene->history->canRedo();
}

QString NodeEditorWidget::getUserFriendlyFilename() const
{
    QString name(isFilenameSet() ? QFileInfo(this->fileName).completeBaseName() : "New Graph");
    return name + (this->isModified() ? "*" : "");
}

void NodeEditorWidget::fileNew()
{
    this->scene->clear();
    this->fileName = "";
    this->scene->history->clear();
    this->scene->history->storeInitialHistory();
}

bool NodeEditorWidget::fileLoad(const QString& filename)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto errMsg = new QString();
    auto loadOK = this->scene->loadFromFile(filename, errMsg);
    QApplication::restoreOverrideCursor();
    if (loadOK) {
        this->fileName = filename;
        this->scene->history->clear();
        this->scene->history->storeInitialHistory();
    } else {
        QMessageBox::warning(this, "Error loading " + QFileInfo(filename).completeBaseName(),
                             *errMsg);
    }
    return loadOK;
}

// when `filename` is not empty, it's saveAs action
bool NodeEditorWidget::fileSave(const QString& filename)
{
    if (! this->isModified() && filename == "")
        return true;

    auto saveMsg = new QString();
    if (filename != "") {
        this->fileName = filename;
        *saveMsg = "saveAS";
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    auto saveOK = this->scene->saveToFile(this->fileName, saveMsg);
    QApplication::restoreOverrideCursor();
    if (!saveOK) {
        QMessageBox::warning(this, "Error loading " + QFileInfo(this->fileName).completeBaseName(),
                             *saveMsg);
    }
    return saveOK;
}

void NodeEditorWidget::addNode()
{
    auto node1 = (new Node(this->scene, "Demo Node1",
                          {SCT_TYPE_1, SCT_TYPE_3, SCT_TYPE_4}, {SCT_TYPE_2,}))->init();
    auto node2 = (new Node(this->scene, "Demo Node2",
                          {SCT_TYPE_1, SCT_TYPE_5, SCT_TYPE_6}, {SCT_TYPE_2,}))->init();
//    auto node3 = (new Node(this->scene, "Demo Node3",
//                          {SCT_TYPE_1, SCT_TYPE_1, SCT_TYPE_3}, {SCT_TYPE_2,}))->init();
    node1->setPos(-350, -250);
    node2->setPos(-75, 0);
//    node3->setPos(200, -150);

//    auto wire1 = new Wire(this->scene, node1->outputs[0], node2->inputs[0]);
//    auto wire2 = new Wire(this->scene, node2->outputs[0], node3->inputs[0], WIRE_TYPE_BEZIER);
//    auto wire3 = new Wire(this->scene, node3->outputs[0], node3->inputs[2], WIRE_TYPE_BEZIER);

    this->scene->history->storeInitialHistory();
}

void NodeEditorWidget::add_debug_content()
{
    auto green_brush = new QBrush(Qt::green);
    auto black_pen = new QPen(Qt::black);
    black_pen->setWidth(2);
    auto outline_pen = new QPen(Qt::red);
    outline_pen->setWidth(2);
    outline_pen->setStyle(Qt::DashLine);

    auto rect = this->scene->grScene->addRect(-100, -100, 80, 100, *outline_pen, *green_brush);
    rect->setFlag(QGraphicsItem::ItemIsMovable);
    rect->setCursor(Qt::UpArrowCursor);
    rect->setToolTip("这是一个矩形图元");

    auto text = this->scene->grScene->addText("Debug 调试信息", QFont("Consolas"));
    text->setFlag(QGraphicsItem::ItemIsSelectable);
    text->setFlag(QGraphicsItem::ItemIsMovable);
    text->setDefaultTextColor(QColor::fromRgbF(1.0, 1.0, 1.0));

    auto widget = new QPushButton("Hello World");
    auto proxy1 = this->scene->grScene->addWidget(widget);
    proxy1->setFlag(QGraphicsItem::ItemIsMovable);
    proxy1->setPos(0, 30);

    auto widget2 = new QTextEdit();

    auto proxy2 = this->scene->grScene->addWidget(widget2);
    proxy2->setFlag(QGraphicsItem::ItemIsSelectable);
    proxy2->setPos(0, 60);

    auto line = this->scene->grScene->addLine(-200, -200, 400, -100, *outline_pen);
    line->setFlag(QGraphicsItem::ItemIsSelectable);
    line->setFlag(QGraphicsItem::ItemIsMovable);
}
