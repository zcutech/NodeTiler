//
// Created by Charlie Zhong on 2021/9/25.
//

#include <QtGui>

#include "node_scene_clipboard.h"
#include "node_scene_history.h"
#include "node_graphics_scene.h"
#include "node_graphics_node.h"
#include "node_graphics_wire.h"
#include "node_graphics_view.h"
#include "node_node.h"
#include "node_wire.h"
#include "node_socket.h"
#include "node_scene.h"

#include <iostream>
#include <iomanip>


SceneClipboard::SceneClipboard(Scene *scene):
    scene(scene)
{
}

json SceneClipboard::serializeSelected(bool deleteThem)
{
    json selNodes = json::array();
    QVector<Wire*> selWires;
    node_HashMap selSockets;

    // 排序 wires, nodes
    for (auto item : this->scene->grScene->selectedItems()) {
        if (item->type() == GRAPH_TYPE_NODE) {
            auto nodeItem = qgraphicsitem_cast<QDMGraphicsNode*>(item);
            selNodes.push_back(nodeItem->node->serialize());
            for (auto socket : nodeItem->node->inputs)
                selSockets[socket->id] = socket;
            for (auto socket : nodeItem->node->outputs)
                selSockets[socket->id] = socket;
        }
        else if (item->type() == GRAPH_TYPE_WIRE) {
            auto wireItem = qgraphicsitem_cast<QDMGraphicsWire*>(item);
            selWires.append(wireItem->wire);
        }
    }

    // 移除所有未连接到列表中node的wires
    QVector<Wire*> wiresToRemove;
    for (auto wire : selWires) {
        if (!selSockets.count(wire->startSocket()->id) || !selSockets.count(wire->endSocket()->id)) {
            wiresToRemove.append(wire);
        }
    }
    for (auto it = wiresToRemove.begin(); it != wiresToRemove.end(); ++it)
        selWires.erase(std::find(selWires.begin(), selWires.end(), *it));

    // 生成最终的wires序列化数据列表
    json wiresFinal;
    for (auto wire : selWires)
        wiresFinal.push_back(wire->serialize());

    json data({
         {"nodes", selNodes},
         {"wires", wiresFinal},
    });

    // 执行剪切时，删除所选
    if (deleteThem) {
        auto view = qobject_cast<QDMGraphicsView*>(this->scene->grScene->views()[0]);
        view->deleteSelected("Cut out elements from scene");
    }

    return data;
}

void SceneClipboard::deserializeFromClipboard(json data)
{
    std::cout << "SceneClipboard::deserializeFromClipboard" << std::endl;
    std::cout << std::setw(4) << data << std::endl;

    // 计算鼠标指针 - scene位置
    auto view = qobject_cast<QDMGraphicsView*>(this->scene->grScene->views()[0]);
    auto mouseScenePos = view->lastSceneMousePosition;

    // 计算所选对象的 bounding box 和 center
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    for (auto &nodeData : data["nodes"]) {
        auto x = nodeData["pos_x"].get<double>();
        auto y = nodeData["pos_y"].get<double>();
        minX = x < minX ? x : minX;
        maxX = x > maxX ? x : maxX;
        minY = y < minY ? y : minY;
        maxY = y > maxY ? y : maxY;
    }
    auto bboxCenterX = (minX + maxX) / 2.0;
    auto bboxCenterY = (minY + maxY) / 2.0;

    // auto center = view->mapToScene(view->rect().center())

    // 计算新创建 node 的位置偏移
    auto offsetX = mouseScenePos.x() - bboxCenterX;
    auto offsetY = mouseScenePos.y() - bboxCenterY;

    // 创建各个 node
    for (auto &nodeData : data["nodes"]) {
        auto newNode = (new Node(this->scene))->init();
        newNode->deserialize(nodeData, &(this->scene->hashMap), false);
        // 调整新建node的位置
        auto pos = newNode->pos();
        // ToDo: a little bit bad of the pasted new Node position
        newNode->setPos(pos.x() + offsetX, pos.y() + offsetY);
    }

    // 创建各个 wire
    for (auto &wireData : data["wires"]) {
        auto newWire = new Wire(this->scene);
        newWire->deserialize(wireData, &(this->scene->hashMap), false);
    }

    // 记录操作历史
    this->scene->history->storeHistory("Pasted elements in scene", VIEW_HIST::PASTE_ITEMS, true);
}
