//
// Created by Charlie Zhong on 2021/9/10.
//

#include "node_socket.h"

#include "node_graphics_socket.h"
#include "node_graphics_scene.h"
#include "node_scene.h"
#include "node_node.h"
#include "node_wire.h"

#include <iostream>

Socket::Socket(Node *node, size_t index, SOCKET_POSITION pos, SOCKET_TYPE type):
    Serializable(),
    node(node),
    index(index),
    position(pos),
    socketType(type),
    grSocket(Q_NULLPTR),
    wires({})
{
    this->grSocket = new QDMGraphicsSocket(this, this->socketType);
    this->grSocket->setPos(this->node->getSocketPos(this->index, this->position));
}

// 获取socket的绝对坐标位置
QPointF Socket::getSocketPos() const
{
    auto posToNode = this->node->getSocketPos(this->index, this->position);
    return posToNode + this->node->pos();
}

void Socket::setAttachedWire(Wire *w)
{
    this->wires.push_back(w);
}

void Socket::delAttachedWire(Wire *w)
{
    auto it = std::find(this->wires.begin(), this->wires.end(), w);
    if (it != this->wires.end()) {
        this->wires.erase(it);
    }
}

void Socket::updateAttachedWires()
{
    for (auto &_wire : this->wires)
        _wire->updatePositions();
}

bool Socket::hasWire(Wire *wire)
{
    if (wire) {
        std::vector<Wire*>::iterator it;
        for (it = this->wires.begin(); it != this->wires.end(); ++it) {
            if (*it == wire)
                return true;
        }
    } else {
        return !this->wires.empty();
    }
    return false;
}

void Socket::clearAllWires()
{
    // 被循环变量会被wire.remove()修改，需迭代其副本
    std::vector<Wire*> wiresCopy(this->wires);
    for (auto &wire : wiresCopy)
        wire->remove();
    this->wires.clear();  // 可省
}

bool Socket::isOutput() const
{
    return (this->position == SCT_AT_RIGHT_TOP || this->position == SCT_AT_RIGHT_BOTTOM);
}

void Socket::remove()
{
    if (!this->hasWire()) {
        this->node->scene->grScene->removeItem(this->grSocket);
        this->grSocket = Q_NULLPTR;
    }
}

json Socket::serialize()
{
    return {
       {"id", this->id},
       {"index", int(this->index)},
       {"position", this->position},
       {"socket_type", this->socketType},
    };
}

bool Socket::deserialize(json data, node_HashMap *hashMap, bool restoreId=true)
{
    for (auto &ele : *hashMap) {
        if (ele.first == this->id) {
            hashMap->erase(ele.first);
            break;
        }
    }
    // 来自文件或历史记录 - 而不是来自粘贴板
    if (restoreId)
        this->id = data["id"];
    (*hashMap)[this->id] = this;

    return true;
}

void Socket::deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap)
{
    int dataSelector = isUndo ? 0 : 1;

    if (changeMap.contains("index"))
        this->index = changeMap["index"][dataSelector];
    if (changeMap.contains("position"))
        this->position = SOCKET_POSITION(changeMap["position"][dataSelector]);
    if (changeMap.contains("socket_type"))
        this->socketType = SOCKET_TYPE(changeMap["socket_type"][dataSelector]);

    this->grSocket->update();
}

void Socket::extractSerialDiff(json anotherSerial, json myArray,
                               json &changeMap, json &removeMap, json &foundSerial)
{
    foundSerial = arrayFindByKV(myArray, "id", anotherSerial["id"]);
    if (!foundSerial.empty()) {
        changeMap = {};
        for (auto key : {"index", "position", "socket_type"}) {
            if (foundSerial[key] != anotherSerial[key]) {
                changeMap[key] = {anotherSerial[key], foundSerial[key]};
                changeMap["modified"] = true;
            }
        }
        // 已更改的socket
        if (!changeMap.empty())
            changeMap["id"] = foundSerial["id"];
    } else
        // 已删除的socket
        removeMap = anotherSerial;
}

void Socket::mergeWithIncrement(json& origSerial, json changeMap, json removeMap)
{
    QSet<QString> changeKeys({});
    for (auto &ele : changeMap.items())
        changeKeys.insert(QString(ele.key().c_str()));
    for (const auto& key : changeKeys - QSet<QString>({"id"}))
        origSerial[key.toStdString()] = changeMap[key.toStdString()][1];
}
