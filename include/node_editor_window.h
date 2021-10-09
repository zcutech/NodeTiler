//
// Created by Charlie Zhong on 2021/9/23.
//

#ifndef NODETILER_NODE_EDITOR_WINDOW_H
#define NODETILER_NODE_EDITOR_WINDOW_H

#include <QtWidgets>

class NodeEditorWidget;

class NodeEditorWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit NodeEditorWindow(QApplication *app);
    ~NodeEditorWindow() override = default;
    virtual void initUI();
    void demoAddNode();
protected:
    QApplication *app;
    NodeEditorWidget *nodeEditor;

    QMenu* fileMenu;
    QMenu* editMenu;

    QAction* actNew;
    QAction* actOpen;
    QAction* actSave;
    QAction* actSaveAs;
    QAction* actExit;
    QAction* actUndo;
    QAction* actRedo;
    QAction* actCut;
    QAction* actCopy;
    QAction* actPaste;
    QAction* actDelete;

    void createStatusBar();
    virtual void createActions();
    virtual void createMenus();
    virtual void setTitle();
    void closeEvent(QCloseEvent *event) override;
    bool isModified() const;
    virtual NodeEditorWidget* getCurrentNodeEditorWidget() const;
    bool maybeSave();
    virtual void onFileNew();
    virtual bool onFileOpen();
    bool onFileSave();
    bool onFileSaveAs();
    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditPaste();
    void onEditDelete();
    void onClose() { QMainWindow::close(); }
    void readSettings();
    void writeSettings();
    QAction* createAct(const QString& name, const QString& shortcut,
                       const QString& tooltip, void (NodeEditorWindow::*callback)());
protected slots:
    void onScenePosChanged(QPointF p);
private:
    QString nameAuthor;
    QString nameProduct;
    QLabel *statusMousePos;
};

#endif //NODETILER_NODE_EDITOR_WINDOW_H
