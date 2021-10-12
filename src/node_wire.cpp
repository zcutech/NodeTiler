//
// Created by Charlie Zhong on 2021/9/11.
//

#include  <QtCore>

#include "node_wire.h"

#include "node_scene.h"
#include "node_socket.h"
#include "node_graphics_wire.h"
#include "node_graphics_scene.h"

#include <iostream>

Wire::Wire(Scene *scene, Socket *startSocket, Socket *endSocket, WIRE_TYPE wire_type):
    Serializable(),
    scene(scene),
    state(0),
    grWire(Q_NULLPTR),
    _startSocket(Q_NULLPTR),
    _endSocket(Q_NULLPTR),
    _wireType(wire_type),
    outputSocket(Q_NULLPTR),
    inputSocket(Q_NULLPTR)
{
    if (startSocket && endSocket && startSocket->node == endSocket->node) {
        throw "A wire can't link on a same node";
    }

    this->state |= WIRE_STATE::WIRE_STATE_OKAY;

    this->scene->addWire(this);
    this->wireType(wire_type);
    this->startSocket(startSocket);
    this->endSocket(endSocket);
}

Socket* Wire::startSocket() const
{
    return this->_startSocket;
}

void Wire::startSocket(Socket *s)
{
    this->_startSocket = s;
    if (this->startSocket()) {
        this->startSocket()->setAttachedWire(this);
        this->state |= WIRE_STATE::WIRE_STATE_HEAD;
        this->updatePositions();
    }
}

Socket* Wire::endSocket() const
{
    return this->_endSocket;
}

void Wire::endSocket(Socket *s)
{
    this->_endSocket = s;
    if (this->endSocket()) {
        this->endSocket()->setAttachedWire(this);
        this->state |= WIRE_STATE::WIRE_STATE_TAIL;
        this->updatePositions();
    }
}

WIRE_TYPE Wire::wireType() const
{
    return this->_wireType;
}

void Wire::wireType(WIRE_TYPE t)
{
    if (this->grWire)
        this->scene->grScene->removeItem(this->grWire);

    this->_wireType = t;
    switch (this->wireType()) {
        case WIRE_TYPE_DIRECT:
            this->grWire = new QDMGraphicsWireDirect(this); break;
        case WIRE_TYPE_BEZIER:
            this->grWire = new QDMGraphicsWireBezier(this); break;
        default:
            this->grWire = new QDMGraphicsWireBezier(this); break;
    }
    this->scene->grScene->addItem(this->grWire);
    this->state |= WIRE_STATE::WIRE_STATE_SHOW;
    this->updatePositions();
}

// 更新wire端点位置
void Wire::updatePositions()
{
    if (this->startSocket()) {
        if (this->startSocket()->isOutput())
            this->outputSocket = this->startSocket();
        else
            this->inputSocket = this->startSocket();
    }
    if (this->endSocket()) {
        if (this->endSocket()->isOutput())
            this->outputSocket = this->endSocket();
        else
            this->inputSocket = this->endSocket();
    }
    // 正常创建时，有输入和输出端口
    if (this->outputSocket && this->inputSocket) {
        this->grWire->setSrcPos(this->outputSocket->getSocketPos());
        this->grWire->setDesPos(this->inputSocket->getSocketPos());
    }
    // 刚点击输出端口创建时
    else if (this->outputSocket && !this->inputSocket) {
        this->grWire->setSrcPos(this->outputSocket->getSocketPos());
        this->grWire->setDesPos(this->outputSocket->getSocketPos());
    }
    // 刚点击输入端口创建时
    else if (!this->outputSocket && this->inputSocket) {
        this->grWire->setSrcPos(this->inputSocket->getSocketPos());
        this->grWire->setDesPos(this->inputSocket->getSocketPos());
    }

    this->grWire->update();
}

void Wire::setSelectedSilently(bool isSelected) const
{
    this->grWire->setData(VIEW_A_ITEM_SET_SELECTED, true);
    this->grWire->setSelected(isSelected);
    this->grWire->setData(VIEW_A_ITEM_SET_SELECTED, false);
}

void Wire::detachFromSockets(Socket* socket)
{
    if (socket) {
        if (socket == this->startSocket()) {
            this->startSocket()->delAttachedWire(this);
            this->startSocket(Q_NULLPTR);
            this->state &= ~WIRE_STATE::WIRE_STATE_HEAD;
        } else if (socket == this->endSocket()) {
            this->endSocket()->delAttachedWire(this);
            this->endSocket(Q_NULLPTR);
            this->state &= ~WIRE_STATE::WIRE_STATE_TAIL;
        }
        return;
    }

    if (this->startSocket())
        this->startSocket()->delAttachedWire(this);
    this->startSocket(Q_NULLPTR);
    this->state &= ~WIRE_STATE::WIRE_STATE_HEAD;
    if (this->endSocket())
        this->endSocket()->delAttachedWire(this);
    this->endSocket(Q_NULLPTR);
    this->state &= ~WIRE_STATE::WIRE_STATE_TAIL;
}

void Wire::remove()
{
    this->scene->removeWire(this);
    this->detachFromSockets();
    this->scene->grScene->removeItem(this->grWire);
    this->grWire = Q_NULLPTR;
}

json Wire::serialize()
{
    return {
        {"id", this->id},
        {"wire_type", this->wireType()},
        {"start", this->startSocket()->id},
        {"end", this->endSocket()->id},
        {"selected", this->grWire->isSelected()},
    };
}

bool Wire::deserialize(json data, node_HashMap *hashMap, bool restoreId=true)
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

    this->wireType(WIRE_TYPE(data["wire_type"]));
    this->startSocket(dynamic_cast<Socket*>((*hashMap)[data["start"]]));
    this->endSocket(dynamic_cast<Socket*>((*hashMap)[data["end"]]));

    if (data.contains("selected"))
        this->setSelectedSilently(data["selected"]);

    return true;
}

void Wire::deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap)
{
    int dataSelector = isUndo ? 0 : 1;

    if (changeMap.contains("wire_type"))
        this->wireType(WIRE_TYPE(changeMap["wire_type"][dataSelector]));
    if (changeMap.contains("start")) {
        auto startSocket = (*hashMap)[changeMap["start"][dataSelector]];
        // detach from its original socket before set new socket
        this->detachFromSockets(this->startSocket());
        this->startSocket(dynamic_cast<Socket*>(startSocket));
    }
    if (changeMap.contains("end")) {
        auto endSocket = (*hashMap)[changeMap["end"][dataSelector]];
        // detach from its original socket before set new socket
        this->detachFromSockets(this->endSocket());
        this->endSocket(dynamic_cast<Socket*>(endSocket));
    }
    if (changeMap.contains("selected")) {
        auto selected = changeMap["selected"][dataSelector];
        this->setSelectedSilently(selected);
    }
}

void Wire::extractSerialDiff(json anotherSerial, json myArray,
                             json &changeMap, json &removeMap, json &foundSerial)
{
    foundSerial = arrayFindByKV(myArray, "id", anotherSerial["id"]);
    if (!foundSerial.empty()) {
        changeMap = {};
        for (auto key : {"wire_type", "start", "end", "selected"}) {
            if (foundSerial[key] != anotherSerial[key]) {
                changeMap[key] = {anotherSerial[key], foundSerial[key]};
                changeMap["modified"] = true;
            }
        }

        // 已更改的wire
        if (!changeMap.empty())
            changeMap["id"] = foundSerial["id"];
    } else
        // 已删除的wire
        removeMap = anotherSerial;
}

void Wire::mergeWithIncrement(json &origSerial, json changeMap, json removeMap)
{
    QSet<QString> changeKeys({});
    for (auto &ele : changeMap.items())
        changeKeys.insert(QString(ele.key().c_str()));
    for (const auto& _k : changeKeys - QSet<QString>({"id", "modified"})) {
        auto key = _k.toStdString();
        origSerial[key] = changeMap[key][1];
    }
}
