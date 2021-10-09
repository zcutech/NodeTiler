#include <QApplication>

#include "node_editor_window.h"
#include "node_editor_widget.h"
#include "utils.h"

#include <iostream>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    loadStyleSheet(&a, QFileInfo(__FILE__).absolutePath() + "/qss/nodestyle.qss");

    auto *wnd = new NodeEditorWindow(&a);
    wnd->initUI();
    wnd->demoAddNode();

    return QApplication::exec();
}
