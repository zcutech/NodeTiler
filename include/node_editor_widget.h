//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_EDITOR_WIDGET_H
#define NODETILER_NODE_EDITOR_WIDGET_H

#include <QtWidgets>

class QDMGraphicsView;
class Scene;

class NodeEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NodeEditorWidget(QApplication *app, QWidget *parent= Q_NULLPTR);
    ~NodeEditorWidget() override = default;
    QDMGraphicsView *view;
    Scene *scene;
    QString fileName;
    void setTitle();
    bool isModified() const;
    bool isFilenameSet() const;
    QString getUserFriendlyFilename() const;
    void fileNew();
    bool fileLoad(const QString& filename);
    bool fileSave(const QString& filename = "");
    void addNode();
    void add_debug_content();
protected:
    QApplication *app;
    QList<QGraphicsItem*> getSelectedItems() const;
    bool hasSelectedItems() const;
    bool canUndo() const;
    bool canRedo() const;
private:
    QVBoxLayout *layout;
};

#endif //NODETILER_NODE_EDITOR_WIDGET_H
