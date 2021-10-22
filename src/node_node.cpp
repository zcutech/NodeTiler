//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_node.h"

#include <iostream>
#include <set>

#include "node_graphics_node.h"
#include "node_graphics_scene.h"
#include "node_graphics_wire.h"
#include "node_graphics_socket.h"
#include "node_content_widget.h"
#include "node_scene.h"
#include "node_socket.h"
#include "node_wire.h"


Node::Node(Scene *_scene, const std::string& _title,
           std::vector<SOCKET_TYPE> inputs, std::vector<SOCKET_TYPE> outputs):
   Serializable(),
   scene(_scene),
   _title(QString::fromStdString(_title)),
   content(Q_NULLPTR),
   grNode(Q_NULLPTR),
   inputSocketPos(SCT_AT_LEFT_BOTTOM),
   outputSocketPos(SCT_AT_RIGHT_TOP),
   socketSpacing(0),                       // socket的间距
   inputs({}),
   outputs({}),
   inTypeVec(std::move(inputs)),
   outTypeVec(std::move(outputs)),
   _isDirty(false),
   _isInvalid(false)
{
}

Node* Node::init()
{
    this->initSettings();
    this->initInnerClasses();
    this->title(this->_title.toStdString());

    this->initSockets(this->inTypeVec, this->outTypeVec);
    this->scene->addNode(this);
    this->scene->grScene->addItem(this->grNode);

    return this;
}

void Node::initInnerClasses()
{
    this->content = (new QDMNodeContentWidget(this))->init();
    this->grNode = (new QDMGraphicsNode(this))->init();
}

void Node::initSettings()
{
    this->socketSpacing = 22;

    this->inputSocketPos = SCT_AT_LEFT_BOTTOM;
    this->outputSocketPos = SCT_AT_RIGHT_TOP;
}

// 创建输入和输出 sockets
void Node::initSockets(std::vector<SOCKET_TYPE> _inputs,
                       std::vector<SOCKET_TYPE> _outputs, bool resetAll)
{
    if (resetAll) {
        // 清除旧的sockets
        for (auto &socket : this->inputs) {
            this->scene->grScene->removeItem(socket->grSocket);
        }
        for (auto &socket : this->outputs)
            this->scene->grScene->removeItem(socket->grSocket);
        this->inputs.clear();
        this->outputs.clear();
    }
    size_t counter = 0;
    for (auto i : _inputs) {
        auto socket = new Socket(this, counter, this->inputSocketPos, i, _inputs.size());
        ++counter;
        this->inputs.push_back(socket);
    }
    counter = 0;
    for (auto i : _outputs) {
        auto socket = new Socket(this, counter, this->outputSocketPos, i, _outputs.size());
        ++counter;
        this->outputs.push_back(socket);
    }
}

void Node::addSockets(bool isInput, SOCKET_TYPE socketType,
                      const std::string& name, const std::string& desc)
{
    Socket* socket;

    if (isInput) {
        socket = new Socket(this, this->inputs.size(), this->inputSocketPos, socketType,
                            this->inputs.size() + 1, name, desc);
        for (auto && s : this->inputs)
            s->countOnThisNodeSide = this->inputs.size() + 1;
        this->inputs.push_back(socket);
    } else {
        socket = new Socket(this, this->outputs.size(), this->outputSocketPos, socketType,
                            this->outputs.size() + 1, name, desc);
        for (auto && s : this->outputs)
            s->countOnThisNodeSide = this->outputs.size() + 1;
        this->outputs.push_back(socket);
    }

    this->scene->hashMap[socket->id] = socket;
    this->scene->hasBeenModified(true);
}

Socket* Node::findSocketIBySerial(json& socketSerial)
{
    for (auto &s : this->inputs) {
        if (s->index == socketSerial["index"]
            && s->position == socketSerial["position"]
            && s->socketType == socketSerial["socket_type"]) {
            return s;
        }
    }

    return Q_NULLPTR;
}

Socket* Node::findSocketOBySerial(json& socketSerial)
{
    for (auto &s : this->outputs) {
        if (s->index == socketSerial["index"]
            && s->position == socketSerial["position"]
            && s->socketType == socketSerial["socket_type"]) {
            return s;
        }
    }

    return Q_NULLPTR;
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

QString Node::title() const
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
    if (s) {
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
    } else {
        // remove all sockets
        for (auto &socket : this->inputs)
            this->scene->grScene->removeItem(socket->grSocket);
        this->inputs.clear();
        for (auto &socket : this->outputs)
            this->scene->grScene->removeItem(socket->grSocket);
        this->outputs.clear();
    }
}

// get coordinate of Socket object from specified position and number
// the return Point{x, y} is the offset coordinate of socket to its belonging node
QPointF Node::getSocketPos(size_t index, SOCKET_POSITION pos, size_t numOutOf) const
{
    qreal x, y;
    if (pos == SCT_AT_LEFT_TOP || pos == SCT_AT_LEFT_CENTER || pos == SCT_AT_LEFT_BOTTOM) {
        x = 0;
    } else {
        x = this->grNode->width;
    }

    if (pos == SCT_AT_LEFT_BOTTOM || pos == SCT_AT_RIGHT_BOTTOM) {
        // start from bottom
        y = this->grNode->height - this->grNode->edgeRoundness -
                this->grNode->titleVertPad - index * this->socketSpacing;
    } else if (pos == SCT_AT_LEFT_CENTER || pos == SCT_AT_RIGHT_CENTER) {
        auto numSockets = numOutOf;
        auto nodeHeight = this->grNode->height;
        auto topOffset = this->grNode->titleHeight + 2 * this->grNode->titleVertPad +
                this->grNode->edgePadding;
        auto availableHeight = nodeHeight - topOffset;

        y = topOffset + availableHeight / 2.0 + (index - 0.5) * this->socketSpacing;
        if (numSockets > 1)
            y -= this->socketSpacing * (numSockets - 1) / 2.0;
    } else if (pos == SCT_AT_LEFT_TOP || pos == SCT_AT_RIGHT_TOP) {
        // start from top
        y = this->grNode->titleHeight + this->grNode->titleHoriPad +
                this->grNode->edgeRoundness + index * this->socketSpacing;
    } else {
        y = 0;
    }

    return {x, y};
}

// update wires linked by the node's sockets, invoke by mouseMove event
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
    // remove this node element from scene's vector
    this->scene->removeNode(this);
    // remove all wires from all sockets
    for (auto &socket : this->inputs) {
        if (socket->hasWire()) {
            socket->clearAllWires();
        }
    }
    for (auto &socket : this->outputs) {
        if (socket->hasWire())
            socket->clearAllWires();
    }
    // remove all sockets of this node
    this->removeSocket();
    // remove this graphical node item from scene
    this->scene->grScene->removeItem(this->grNode);
    this->grNode = Q_NULLPTR;
}

/* ---------------------- node evaluation functions ------------------------ */
bool Node::isDirty() const
{
    return this->_isDirty;
}

void Node::markDirty(bool newVal)
{
    this->_isDirty = newVal;
    if (this->_isDirty)
        this->onMarkedDirty();
}

void Node::onMarkedDirty()
{

}

void Node::markChildDirty(bool newVal)
{
    for (const auto &otherNode : this->getChildrenNodes()) {
        otherNode->markDirty(newVal);
    }
}

void Node::markDescendantDirty(bool newVal)
{
    for (const auto &otherNode : this->getChildrenNodes()) {
        otherNode->markDirty(newVal);
        otherNode->markChildDirty(newVal);
    }
}

bool Node::isInvalid() const
{
    return this->_isInvalid;
}

void Node::markInvalid(bool newVal)
{
    this->_isInvalid = newVal;
    if (this->_isInvalid)
        this->onMarkedInvalid();
}

void Node::onMarkedInvalid()
{

}

void Node::markChildInvalid(bool newVal)
{
    for (const auto &otherNode : this->getChildrenNodes()) {
        otherNode->markInvalid(newVal);
    }
}

void Node::markDescendantInvalid(bool newVal)
{
    for (const auto &otherNode : this->getChildrenNodes()) {
        otherNode->markInvalid(newVal);
        otherNode->markChildInvalid(newVal);
    }
}

int Node::eval()
{
    this->markDirty(false);
    this->markInvalid(false);

    return 0;
}

void Node::evalChildren()
{
    for (const auto &node : this->getChildrenNodes())
        node->eval();
}

/* ---------------------- traversing nodes functions ----------------------- */
std::vector<Node*> Node::getChildrenNodes()
{
    std::vector<Node*> otherNodes{};

    if (this->outputs.empty())
        return otherNodes;

    for (const auto &outputSocket : this->outputs) {
        for (const auto &wire : outputSocket->wires) {
            auto otherNode = wire->inputSocket->node;
            otherNodes.push_back(otherNode);
        }
    }

    return otherNodes;
}

/* ------------- serialization and deserialization functions --------------- */
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

    auto numInputs = data["inputs"].size();
    auto numOutputs = data["outputs"].size();

    for (auto &s_Data : data["inputs"]) {
        auto existingSocket = this->findSocketIBySerial(s_Data);
        if (existingSocket) {
            existingSocket->deserialize(s_Data, hashMap, restoreId);
        } else {
            auto newSocket = new Socket(this, s_Data["index"], SOCKET_POSITION(s_Data["position"]),
                                        SOCKET_TYPE(s_Data["socket_type"]), numInputs);
            newSocket->deserialize(s_Data, hashMap, restoreId);
            this->inputs.push_back(newSocket);
        }
    }

    for (auto &s_Data : data["outputs"]) {
        auto existingSocket = this->findSocketOBySerial(s_Data);
        if (existingSocket) {
            existingSocket->deserialize(s_Data, hashMap, restoreId);
        } else {
            auto newSocket = new Socket(this, s_Data["index"], SOCKET_POSITION(s_Data["position"]),
                                        SOCKET_TYPE(s_Data["socket_type"]), numOutputs);
            newSocket->deserialize(s_Data, hashMap, restoreId);
            this->outputs.push_back(newSocket);
        }
    }

    if (data.contains("selected"))
        this->setSelectedSilently(data["selected"]);

    // deserialize the content of this node
    auto deserializeOk = this->content->deserialize(data["content"], hashMap, restoreId);

    return deserializeOk;
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

/* ------------------ serial compare and merge functions ------------------- */

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
