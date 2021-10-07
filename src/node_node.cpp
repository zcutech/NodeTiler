//
// Created by Charlie Zhong on 2021/9/6.
//

#include <set>

#include "node_node.h"

#include "node_graphics_node.h"
#include "node_graphics_scene.h"
#include "node_graphics_wire.h"
#include "node_graphics_socket.h"
#include "node_content_widget.h"
#include "node_scene.h"
#include "node_socket.h"
#include "node_wire.h"

#include <iostream>
#include <iomanip>


Node::Node(Scene *_scene, const std::string& _title,
           const std::vector<SOCKET_TYPE>& inputs, const std::vector<SOCKET_TYPE>& outputs):
   Serializable(),
   scene(_scene),
   content(Q_NULLPTR),
   grNode(Q_NULLPTR),
   inputSocketPos(SCT_AT_LEFT_BOTTOM),
   outputSocketPos(SCT_AT_RIGHT_TOP),
   socketSpacing(0),                       // socket的间距
   inputs({}),
   outputs({})
{
    std::cout << "Enter Node - 1" << std::endl;

    this->initInnerClasses();
    this->initSettings();
    this->title(_title);

    this->initSockets(inputs, outputs);

    this->scene->addNode(this);
    this->scene->grScene->addItem(this->grNode);
}

void Node::initInnerClasses()
{
    this->content = new QDMNodeContentWidget(this);
    this->grNode = new QDMGraphicsNode(this);
    this->inputSocketPos = SCT_AT_LEFT_BOTTOM;
    this->outputSocketPos = SCT_AT_RIGHT_TOP;
}

void Node::initSettings()
{
    this->socketSpacing = 22;
}

// 创建输入和输出 sockets
void Node::initSockets(const std::vector<SOCKET_TYPE> &_inputs,
                       const std::vector<SOCKET_TYPE> &_outputs, bool resetAll)
{
    if (resetAll) {
        // 清除旧的sockets
        for (auto &socket : this->inputs)
            this->scene->grScene->removeItem(socket->grSocket);
        for (auto &socket : this->outputs)
            this->scene->grScene->removeItem(socket->grSocket);
        this->inputs.clear();
        this->outputs.clear();
    }

    size_t counter = 0;
    for (auto i : _inputs) {
        auto socket = new Socket(this, counter, this->inputSocketPos, i);
        ++counter;
        this->inputs.push_back(socket);
    }
    counter = 0;
    for (auto i : _outputs) {
        auto socket = new Socket(this, counter, this->outputSocketPos, i);
        ++counter;
        this->outputs.push_back(socket);
    }
}

QPointF Node::pos() const
{
    return this->grNode->pos();
}

void Node::setPos(QPointF p)
{
    this->grNode->setPos(p);
}

void Node::setPos(float x, float y)
{
    this->grNode->setPos(QPointF(x, y));
}

inline QString Node::title() const
{
    return this->_title;
}

void Node::title(const std::string& t)
{
    this->_title = QString::fromStdString(t);
    this->grNode->title(this->title());
}

void Node::removeSocket(Socket *s)
{
    auto foundI = std::find(this->inputs.begin(), this->inputs.end(), s);
    if (foundI != this->inputs.end()) {
        this->scene->grScene->removeItem(s->grSocket);
        this->inputs.erase(foundI);
    }
    auto foundO = std::find(this->outputs.begin(), this->outputs.end(), s);
    if (foundO != this->outputs.end()) {
        this->scene->grScene->removeItem(s->grSocket);
        this->outputs.erase(foundO);
    }
}

// 返回指定位置指定序号的socket对象的坐标
// 返回 {x, y} 为socket相对于node的位置偏移
QPointF Node::getSocketPos(int index, SOCKET_POSITION pos) const
{
    qreal x, y;
    if (pos == SCT_AT_LEFT_TOP || pos == SCT_AT_LEFT_BOTTOM) {
        x = 0;
    } else {
        x = this->grNode->width;
    }

    if (pos == SCT_AT_LEFT_BOTTOM || pos == SCT_AT_RIGHT_BOTTOM) {
        // 从底部开始
        y = this->grNode->height - this->grNode->edgeSize - this->grNode->_padding - index * this->socketSpacing;
    } else {
        // 从顶部开始
        y = this->grNode->titleHeight + this->grNode->_padding + this->grNode->edgeSize + index * this->socketSpacing;
    }

    return {x, y};
}

// 刷新节点socket上链接的wire，由mouseMove事件触发
void Node::updateAttachedWires()
{
    for (auto socket : this->inputs) {
        socket->updateAttachedWires();
    }
    for (auto socket : this->outputs) {
        socket->updateAttachedWires();
    }
}

// 选择节点socket上链接的edge，由view的mouseMove事件触发间接调用
void Node::selectAttachedWires(bool revert)
{
    for (auto &socket : this->inputs) {
        for (auto &wire : socket->wires) {
            auto flag = revert ? !this->grNode->isSelected() : true;
            wire->grWire->setSelected(flag);
        }
    }
    for (auto &socket : this->outputs) {
        for (auto &wire : socket->wires) {
            auto flag = revert ? !this->grNode->isSelected() : true;
            wire->grWire->setSelected(flag);
        }
    }
}

void Node::setSelectedSilently(bool isSelected) const
{
    this->grNode->setData(VIEW_A_ITEM_SET_SELECTED, true);
    this->grNode->setSelected(isSelected);
    this->grNode->setData(VIEW_A_ITEM_SET_SELECTED, false);
}

void Node::remove()
{
    // 从scene删除node
    this->scene->removeNode(this);
    // 从所有socket删除所有edge
    for (auto &socket : this->inputs) {
        if (socket->hasWire())
            socket->clearAllWires();
        this->removeSocket(socket);
    }
    for (auto &socket : this->outputs) {
        if (socket->hasWire())
            socket->clearAllWires();
        this->removeSocket(socket);
    }
    // 删除gr_node
    this->scene->grScene->removeItem(this->grNode);
    this->grNode = Q_NULLPTR;
}

json Node::serialize()
{
    json inputs_S = json::array(), outputs_S = json::array();
    for (auto &socket : this->inputs)
        inputs_S.push_back(socket->serialize());
    for (auto &socket : this->outputs)
        outputs_S.push_back(socket->serialize());

    return {
       {"id", this->id},
       {"title", this->title().toStdString()},
       {"pos_x", this->grNode->scenePos().x()},
       {"pos_y", this->grNode->scenePos().y()},
       {"inputs", inputs_S},
       {"outputs", outputs_S},
       {"selected", this->grNode->isSelected()},
       {"content", this->content->serialize()},
    };
}

bool Node::deserialize(json data, node_HashMap *hashMap, bool restoreId=true)
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

    this->title(data["title"]);
    this->setPos(data["pos_x"], data["pos_y"]);

    this->inputs.clear();
    for (auto &s_Data : data["inputs"]) {
        auto newSocket = new Socket(this, s_Data["index"], SOCKET_POSITION(s_Data["position"]),
                                    SOCKET_TYPE(s_Data["socket_type"]));
        newSocket->deserialize(s_Data, hashMap, restoreId);
        this->inputs.push_back(newSocket);
    }
    this->outputs.clear();
    for (auto &s_Data : data["outputs"]) {
        auto newSocket = new Socket(this, s_Data["index"], SOCKET_POSITION(s_Data["position"]),
                                    SOCKET_TYPE(s_Data["socket_type"]));
        newSocket->deserialize(s_Data, hashMap, restoreId);
        this->outputs.push_back(newSocket);
    }

    if (data.contains("selected"))
        this->setSelectedSilently(data["selected"]);

    return true;
}

void Node::deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap)
{
    int dataSelector = isUndo ? 0 : 1;

    if (changeMap.contains("title"))
        this->title(changeMap["title"][dataSelector]);
    auto pos_x = this->pos().x();
    if (changeMap.contains("pos_x")) {
        pos_x = changeMap["pos_x"][dataSelector];
    }
    auto pos_y = this->pos().y();
    if (changeMap.contains("pos_y")) {
        pos_y = changeMap["pos_y"][dataSelector];
    }
    if (pos_x != this->pos().x() || pos_y != this->pos().y())
        this->setPos(pos_x, pos_y);
    if (changeMap.contains("inputs")) {
        for (auto &inputSocket : changeMap["inputs"]) {
            auto socket = dynamic_cast<Socket*>((*hashMap)[inputSocket["id"]]);
            if (inputSocket.contains("modified") && inputSocket["modified"])
                socket->deserializeIncremental(inputSocket, isUndo, hashMap);
            else {
                this->scene->grScene->removeItem(socket->grSocket);
                this->inputs.erase(std::remove(this->inputs.begin(), this->inputs.end(), socket),
                                   this->inputs.end());
            }
        }
    }
    if (changeMap.contains("outputs")) {
        for (auto &outputSocket : changeMap["outputs"]) {
            auto socket = dynamic_cast<Socket*>((*hashMap)[outputSocket["id"]]);
            if (outputSocket.contains("modified") && outputSocket["modified"])
                socket->deserializeIncremental(outputSocket, isUndo, hashMap);
            else {
                this->scene->grScene->removeItem(socket->grSocket);
                this->outputs.erase(std::remove(this->outputs.begin(), this->outputs.end(), socket),
                                   this->outputs.end());
            }
        }
    }

    if (changeMap.contains("selected")) {
        auto selected = changeMap["selected"][dataSelector];
        this->setSelectedSilently(selected);
    }

    this->updateAttachedWires();
}

// 在指定序列组中查找和比较与指定的另一个序列的差异，返回更改和删除部分
void Node::extractSerialDiff(json anotherSerial, json myArray,
                             json &changeMap, json &removeMap, json &foundSerial)
{
    foundSerial = arrayFindByKV(myArray, "id", anotherSerial["id"]);
    if (!foundSerial.empty()) {
        changeMap = {};
        for (auto key : {"title", "pos_x", "pos_y", "content", "selected"}) {
            if (foundSerial[key] != anotherSerial[key]) {
                changeMap[key] = {anotherSerial[key], foundSerial[key]};
                changeMap["modified"] = true;
            }
        }

        // 记录已修改或删除的socket
        json remSocketsNode = { {"id", foundSerial["id"]}, {"inputs", {}}, {"outputs", {}} };

        std::set<std::string> sameSocketsId_i({});
        for (auto &socketI_1 : anotherSerial["inputs"]) {
            json change_s, remove_s, found_socket;
            Socket::extractSerialDiff(socketI_1, foundSerial["inputs"],
                                      change_s, remove_s, found_socket);
            if (!change_s.empty()) {
                if (!changeMap.contains("inputs"))
                    changeMap["inputs"] = {};
                changeMap["inputs"].push_back(change_s);
                if (change_s.contains("modified") && change_s["modified"])
                    changeMap["modified"] = true;
            }
            if (!remove_s.empty())
                remSocketsNode["inputs"].push_back(remove_s);
            if (!found_socket.empty())
                sameSocketsId_i.insert(found_socket["id"].get<std::string>());
        }

        std::set<std::string> sameSocketsId_o({});
        for (auto &socketO_1 : anotherSerial["outputs"]) {
            json change_s, remove_s, found_socket;
            Socket::extractSerialDiff(socketO_1, foundSerial["outputs"],
                                      change_s, remove_s, found_socket);
            if (!change_s.empty()) {
                if (!changeMap.contains("outputs"))
                    changeMap["outputs"] = {};
                changeMap["outputs"].push_back(change_s);
                if (change_s.contains("modified") && change_s["modified"])
                    changeMap["modified"] = true;
            }
            if (!remove_s.empty())
                remSocketsNode["outputs"].push_back(remove_s);
            if (!found_socket.empty())
                sameSocketsId_o.insert(found_socket["id"].get<std::string>());
        }

        if (!remSocketsNode["inputs"].empty() || !remSocketsNode["outputs"].empty())
            removeMap = remSocketsNode;

        // 记录新增加的socket
        json addSockets_i = json::array();
        json addSockets_o = json::array();
        for (auto &socket : anotherSerial["inputs"]) {
            if (!sameSocketsId_i.count(socket["id"]))
                addSockets_i.push_back(socket);
        }
        for (auto &socket : anotherSerial["outputs"]) {
            if (!sameSocketsId_o.count(socket["id"]))
                addSockets_o.push_back(socket);
        }
        if (!addSockets_i.empty()) {
            if (!changeMap.contains("inputs"))
                changeMap["inputs"] = {};
            for (auto &s : addSockets_i)
                changeMap["inputs"].push_back(s);
        }
        if (!addSockets_o.empty()) {
            if (!changeMap.contains("outputs"))
                changeMap["outputs"] = {};
            for (auto &s : addSockets_o)
                changeMap["outputs"].push_back(s);
        }

        // 已更改的node
        if (!changeMap.empty())
            changeMap["id"] = foundSerial["id"];
    } else
        // 已删除的node
        removeMap = anotherSerial;
}

void Node::mergeWithIncrement(json &origSerial, json changeMap, json removeMap)
{
    // 修改 nodes
    QSet<QString> changeKeys({});
    for (auto &ele : changeMap.items())
        changeKeys.insert(QString(ele.key().c_str()));
    for (const auto& key : changeKeys - QSet<QString>({
        "id", "inputs", "outputs", "content", "modified"}))
        origSerial[key.toStdString()] = changeMap[key.toStdString()][1];
    if (changeMap.contains("inputs")) {
        for (auto &inputSocket: changeMap["inputs"]) {
            if (inputSocket.contains("modified")) {
                json& origSocket = arrayFindByKV(origSerial["inputs"], "id", inputSocket["id"]);
                Socket::mergeWithIncrement(origSocket, inputSocket, {});
            } else
                origSerial["inputs"].push_back(inputSocket);
        }
    }
    if (changeMap.contains("outputs")) {
        for (auto &outputSocket: changeMap["outputs"]) {
            if (outputSocket.contains("modified")) {
                json& origSocket = arrayFindByKV(origSerial["outputs"], "id", outputSocket["id"]);
                Socket::mergeWithIncrement(origSocket, outputSocket, {});
            } else
                origSerial["outputs"].push_back(outputSocket);
        }
    }

    // 删除 sockets
    if (removeMap.contains("inputs")) {
        for (auto &socketRemove : removeMap["inputs"]) {
            auto origSocketPos = arrayAtByKV(origSerial["inputs"], "id", socketRemove["id"]);
            origSerial["inputs"].erase(origSocketPos);
        }
    }
    if (removeMap.contains("outputs")) {
        for (auto &socketRemove : removeMap["outputs"]) {
            auto origSocketPos = arrayAtByKV(origSerial["outputs"], "id", socketRemove["id"]);
            origSerial["outputs"].erase(origSocketPos);
        }
    }
}
