//
// Created by Charlie Zhong on 2021/9/23.
//

#include "node_editor_window.h"

#include "node_editor_widget.h"
#include "node_scene.h"
#include "node_graphics_scene.h"
#include "node_graphics_view.h"
#include "node_scene_history.h"
#include "node_scene_clipboard.h"

#include <iostream>


NodeEditorWindow::NodeEditorWindow(QApplication *app):
        app(app),
        nameAuthor("ZCutech"),
        nameProduct("NodeEditor"),
        nodeEditor(Q_NULLPTR),
        fileMenu(Q_NULLPTR),
        editMenu(Q_NULLPTR),
        actNew(Q_NULLPTR),
        actOpen(Q_NULLPTR),
        actSave(Q_NULLPTR),
        actSaveAs(Q_NULLPTR),
        actExit(Q_NULLPTR),
        actUndo(Q_NULLPTR),
        actRedo(Q_NULLPTR),
        actCut(Q_NULLPTR),
        actCopy(Q_NULLPTR),
        actPaste(Q_NULLPTR),
        actDelete(Q_NULLPTR),
        statusMousePos(Q_NULLPTR)
{

}

void NodeEditorWindow::demoAddNode() {
    this->nodeEditor->addNode();
}

void NodeEditorWindow::initUI()
{
    // 创建菜单和按键绑定
    this->createActions();
    this->createMenus();

    this->nodeEditor = new NodeEditorWidget(this->app, this);
    this->nodeEditor->scene->addHasBeenModifiedListeners([this] { this->setTitle(); });
    this->setCentralWidget(this->nodeEditor);

    // 状态栏
    this->createStatusBar();

    // 设置窗口属性
    this->setGeometry(200, 200, 800, 600);
    this->setTitle();
    this->show();
}

QAction* NodeEditorWindow::createAct(const QString& name, const QString& shortcut,
                                     const QString& tooltip, void (NodeEditorWindow::*callback)() )
{
    auto act = new QAction(name, this);
    act->setShortcut(shortcut);
    act->setToolTip(tooltip);
    connect(act, &QAction::triggered, this, callback);
    return act;
}

void NodeEditorWindow::createStatusBar()
{
    this->statusBar()->showMessage("");
    this->statusMousePos = new QLabel("");
    this->statusBar()->addPermanentWidget(this->statusMousePos);
    connect(this->nodeEditor->view, &QDMGraphicsView::scenePosChanged, this,
            &NodeEditorWindow::onScenePosChanged);
}

void NodeEditorWindow::createActions()
{
    this->actNew = this->createAct("&New", "Ctrl+N", "Create New graph",
                                   &NodeEditorWindow::onFileNew);
    this->actOpen = this->createAct("&Open", "Ctrl+O", "Open file",
                                    reinterpret_cast<void (NodeEditorWindow::*)()>
                                    (&NodeEditorWindow::onFileOpen));
    this->actSave = this->createAct("&Save", "Ctrl+S", "Save file",
                                    reinterpret_cast<void (NodeEditorWindow::*)()>
                                    (&NodeEditorWindow::onFileSave));
    this->actSaveAs = this->createAct("Save &As", "Ctrl+Shift+S", "Save file as...",
                                      reinterpret_cast<void (NodeEditorWindow::*)()>
                                      (&NodeEditorWindow::onFileSaveAs));
    this->actExit = this->createAct("E&xit", "Ctrl+Q", "Exit application",
                                    &NodeEditorWindow::onClose);
    this->actUndo = this->createAct("&Undo", "Ctrl+Z", "Undo last operation",
                                    &NodeEditorWindow::onEditUndo);
    this->actRedo = this->createAct("&Redo", "Ctrl+Shift+Z", "Redo last operation",
                                    &NodeEditorWindow::onEditRedo);
    this->actCut = this->createAct("Cu&t", "Ctrl+X", "Cut to clipboard",
                                   &NodeEditorWindow::onEditCut);
    this->actCopy = this->createAct("&Copy", "Ctrl+C", "Copy to clipboard",
                                    &NodeEditorWindow::onEditCopy);
    this->actPaste = this->createAct("&Paste", "Ctrl+V", "Paste from clipboard",
                                     &NodeEditorWindow::onEditPaste);
    this->actDelete = this->createAct("&Delete", "Del", "Delete selected items",
                                      &NodeEditorWindow::onEditDelete);
}

void NodeEditorWindow::createMenus()
{
    // 初始化菜单
    auto menubar = this->menuBar();
    this->fileMenu = menubar->addMenu("&File");
    this->fileMenu->addAction(this->actNew);
    this->fileMenu->addSeparator();
    this->fileMenu->addAction(this->actOpen);
    this->fileMenu->addAction(this->actSave);
    this->fileMenu->addAction(this->actSaveAs);
    fileMenu->addSeparator();
    this->fileMenu->addAction(this->actExit);

    this->editMenu = this->menuBar()->addMenu("&Edit");
    this->editMenu->addAction(this->actUndo);
    this->editMenu->addAction(this->actRedo);
    editMenu->addSeparator();
    this->editMenu->addAction(this->actCut);
    this->editMenu->addAction(this->actCopy);
    this->editMenu->addAction(this->actPaste);
    editMenu->addSeparator();
    this->editMenu->addAction(this->actDelete);
}

void NodeEditorWindow::setTitle()
{
    QString title = "NodeTiler - ";
    title += this->getCurrentNodeEditorWidget()->getUserFriendlyFilename();

    this->setWindowTitle(title);
}

void NodeEditorWindow::closeEvent(QCloseEvent *event)
{
    if (this->maybeSave())
        event->accept();
    else
        event->ignore();
}

bool NodeEditorWindow::isModified() const
{
    return this->getCurrentNodeEditorWidget()->isModified();
}

NodeEditorWidget* NodeEditorWindow::getCurrentNodeEditorWidget() const
{
    auto editorWidget = qobject_cast<NodeEditorWidget*>(this->centralWidget());
    return editorWidget;
}

bool NodeEditorWindow::maybeSave()
{
    if (!this->isModified())
        return true;

    auto res = QMessageBox::warning(this, "About to loose your work?",
                                    "The document has been modified.\n"
                                    "Do you want to save your changes?",
                                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (res == QMessageBox::Save)
        return this->onFileSave();
    else if (res == QMessageBox::Cancel)
        return false;

    return true;
}

void NodeEditorWindow::onScenePosChanged(QPointF p)
{
    this->statusMousePos->setText(QString("Scene Pos: [%1, %2]").arg(
            QString::number(p.x(), 'f', 2), QString::number(p.y(), 'f', 2)));
}

void NodeEditorWindow::onFileNew()
{
    if (this->maybeSave()) {
        this->getCurrentNodeEditorWidget()->fileNew();
        this->setTitle();
    }
}

bool NodeEditorWindow::onFileOpen()
{
    if (this->maybeSave()) {
        auto getFileName = QFileDialog::getOpenFileName(this, "Open graph from file", ".");
        if (getFileName == "")
            return false;
        QFileInfo fi(getFileName);
        if (fi.isFile()) {
            auto ok = this->getCurrentNodeEditorWidget()->fileLoad(getFileName);
            if (!ok) {
                return false;
            }
        }
        this->setTitle();
    }
    return true;
}

bool NodeEditorWindow::onFileSave()
{
    auto currentNodeEditor = this->getCurrentNodeEditorWidget();
    if (!currentNodeEditor)
        return false;
    if (! currentNodeEditor->isFilenameSet())
        return this->onFileSaveAs();
    if (currentNodeEditor->fileSave()) {
        this->statusBar()->showMessage("Successfully saved " + currentNodeEditor->fileName, 5000);
        this->setTitle();
        currentNodeEditor->setTitle();
        return true;
    }
    return false;
}

bool NodeEditorWindow::onFileSaveAs()
{
    auto currentNodeEditor = this->getCurrentNodeEditorWidget();
    if (!currentNodeEditor)
        return false;
    auto getFileName = QFileDialog::getSaveFileName(this, "Save graph to file");
    if (getFileName == "")
        return false;
    // when success to save as a new file, the current opened file is this new file
    if (currentNodeEditor->fileSave(getFileName)) {
        this->statusBar()->showMessage("Successfully saved as " + currentNodeEditor->fileName, 5000);
        this->setTitle();
        currentNodeEditor->setTitle();
        return true;
    }
    return false;
}

void NodeEditorWindow::onEditUndo()
{
    if (this->getCurrentNodeEditorWidget())
        this->getCurrentNodeEditorWidget()->scene->history->undo();
}

void NodeEditorWindow::onEditRedo()
{
    if (this->getCurrentNodeEditorWidget())
        this->getCurrentNodeEditorWidget()->scene->history->redo();
}

void NodeEditorWindow::onEditCut()
{
    if (this->getCurrentNodeEditorWidget()) {
        auto data = getCurrentNodeEditorWidget()->scene->clipboard->serializeSelected(true);
        auto strData = data.dump();
        QApplication::clipboard()->setText(QString(strData.c_str()));
    }
}

void NodeEditorWindow::onEditCopy()
{
    if (this->getCurrentNodeEditorWidget()) {
        auto data = getCurrentNodeEditorWidget()->scene->clipboard->serializeSelected(false);
        auto strData = data.dump();
        QApplication::clipboard()->setText(QString(strData.c_str()));
    }
}

void NodeEditorWindow::onEditPaste()
{
    if (this->getCurrentNodeEditorWidget()) {
        auto rawData = QApplication::clipboard()->text();

        json doc;
        try {
            doc = json::parse(rawData.toStdString());
        } catch (json::parse_error::exception &e) {
            std::cout << "Pasting with invalid json data!\n" << e.what() << std::endl;
            return;
        }

        // 检查json数据有效
        if (!doc.contains("nodes")) {
            std::cout << "Json does not contain any nodes" << std::endl;
            return;
        }

        this->getCurrentNodeEditorWidget()->scene->clipboard->deserializeFromClipboard(doc);
    }
}

void NodeEditorWindow::onEditDelete()
{
    this->getCurrentNodeEditorWidget()->scene->getView()->deleteSelected();
}

void NodeEditorWindow::readSettings()
{
    QSettings settings(this->nameAuthor, this->nameProduct);
    auto pos = settings.value("pos", QPoint(200, 200)).toPoint();
    auto size = settings.value("size", QSize(400, 400)).toSize();
    this->move(pos);
    this->resize(size);
}

void NodeEditorWindow::writeSettings()
{
    QSettings settings(this->nameAuthor, this->nameProduct);
    settings.setValue("pos", this->pos());
    settings.setValue("size", this->size());
}
