//
// Created by Charlie Zhong on 2021/9/6.
//

#include <iomanip>
#include <algorithm>

#include "node_scene.h"

#include "node_node.h"
#include "node_wire.h"
#include "node_socket.h"
#include "node_graphics_scene.h"
#include "node_scene_history.h"
#include "node_scene_clipboard.h"
#include "node_graphics_view.h"

#include <iostream>
#include <iomanip>

Scene::Scene():
    Serializable(),
    nodes({}),
    wires({}),
    hashMap({}),
    grScene(Q_NULLPTR),
    sceneWidth(64000),
    sceneHeight(64000),
    _hasBeenModified(false),
    _lastSelectedItems({}),
    _hasBeenModifiedListeners({}),
    _itemSelectedListeners({}),
    _itemDeselectedListeners({}),
    history(new SceneHistory(this)),
    clipboard(new SceneClipboard(this))
{
    this->grScene = new QDMGraphicsScene(this);
    this->grScene->setGrScene(this->sceneWidth, this->sceneHeight);

    QObject::connect(this->grScene, &QDMGraphicsScene::itemSelected, this, &Scene::onItemSelected);
    QObject::connect(this->grScene, &QDMGraphicsScene::itemDeselected, this, &Scene::onItemDeselected);
    QObject::connect(this->grScene, &QDMGraphicsScene::selsChanged, this, &Scene::onSelsChanged);
}

void Scene::onItemSelected(bool mergeLastDeselection)
{
    auto curSelectedItems = this->getSelectedItems();
    if (curSelectedItems != this->_lastSelectedItems) {
        this->_lastSelectedItems = curSelectedItems;
        this->history->storeHistory("Selection Changed", VIEW_HIST::SELECT_ITEMS,
                                    false, mergeLastDeselection);
        for (const auto& callback : this->_itemSelectedListeners)
            callback();
    }
}

void Scene::onItemDeselected()
{
    if (!this->_lastSelectedItems.empty()) {
        this->_lastSelectedItems.clear();
        this->history->storeHistory("Deselection Something", VIEW_HIST::DESELECT_ITEMS);
        for (const auto& callback : this->_itemDeselectedListeners)
            callback();
    }
}

void Scene::onSelsChanged()
{
    auto curSelectedItems = this->getSelectedItems();
    if (!curSelectedItems.empty())
        this->onItemSelected();
    else
        this->onItemDeselected();
}

QList<QGraphicsItem*> Scene::getSelectedItems() const
{
    return this->grScene->selectedItems();
}

bool Scene::hasBeenModified() const
{
    return this->_hasBeenModified;
}

void Scene::hasBeenModified(bool value)
{
    if (this->_hasBeenModified != value) {
        this->_hasBeenModified = value;
        // 调用所有注册的监听器
        for (const auto& callback : this->_hasBeenModifiedListeners) {
            callback();
        }
    }

    this->_hasBeenModified = value;
}

void Scene::addHasBeenModifiedListeners(const std::function<void()>& callback)
{
    this->_hasBeenModifiedListeners.push_back(callback);
}

void Scene::addItemSelectedListener(const std::function<void()> &callback)
{
    this->_itemSelectedListeners.push_back(callback);
}

void Scene::addItemDeselectedListener(const std::function<void()> &callback)
{
    this->_itemDeselectedListeners.push_back(callback);
}

void Scene::addDragEnterListener(const std::function<void()> &callback) const
{
    auto view = qobject_cast<QDMGraphicsView*>(this->grScene->views()[0]);
    view->addDragEnterListener(callback);
}

void Scene::addDropListener(const std::function<void()> &callback) const
{
    auto view = qobject_cast<QDMGraphicsView*>(this->grScene->views()[0]);
    view->addDropListener(callback);
}

void Scene::addNode(Node *node) {
    this->nodes.push_back(node);
    this->hashMap[node->id] = node;
    for (auto s : node->inputs)
        this->hashMap[s->id] = s;
    for (auto s : node->outputs)
        this->hashMap[s->id] = s;
}

void Scene::addWire(Wire *wire) {
    this->wires.push_back(wire);
    this->hashMap[wire->id] = wire;
}

void Scene::removeNode(Node *node) {
    auto it = std::find(this->nodes.begin(), this->nodes.end(), node);
    if (it != this->nodes.end()) {
        this->hashMap.erase(node->id);
//        this->nodes.erase(std::remove(this->nodes.begin(), this->nodes.end(), node),
//                          this->nodes.end());
        for (auto s : node->inputs)
            this->hashMap.erase(s->id);
        for (auto s : node->outputs)
            this->hashMap.erase(s->id);
        this->nodes.erase(it);
    }
}

void Scene::removeWire(Wire *wire) {
    auto it = std::find(this->wires.begin(), this->wires.end(), wire);
    if (it != this->wires.end()) {
        this->hashMap.erase(wire->id);
        this->wires.erase(it);
    }
}

void Scene::clear(bool keepModified)
{
    while (!this->nodes.empty())
        this->nodes[0]->remove();
    if (!keepModified)
        this->_hasBeenModified = false;
}

bool Scene::saveToFile(const QString& filename, QString *saveMsg)
{
    auto isSaveAs = (*saveMsg != "");
    *saveMsg = "";

    QFile fp(filename);
    if (!fp.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *saveMsg = "Save failed for \"" + filename + "\"\n" + "Open this file failed.";
        return false;
    }

    json doc;
    try {
        doc = this->serialize();
    } catch (json::parse_error::exception &e) {
        *saveMsg = "Save failed for \"" + filename + "\"\n" + "Invalid document content." + "\n" +
                   e.what();
        fp.close();
        return false;
    }
    *saveMsg = "";

    std::ostringstream oss;
    oss << std::setw(4) << doc;
    fp.write(oss.str().c_str());
    fp.close();
    this->_hasBeenModified = false;

    if (isSaveAs)
        this->history->clear();

    return true;
}

bool Scene::loadFromFile(const QString& filename, QString *errMsg)
{
    QFile fp(filename);
    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errMsg = "Load failed from " + filename + "\n" + "Open this file failed.";
        return false;
    }
    QTextStream rawData(&fp);
    rawData.setCodec("UTF-8");
    json doc;
    std::istringstream ifs(rawData.readAll().toStdString());

    try {
        ifs >> doc;
    } catch (json::parse_error::exception &e) {
        *errMsg = "Load failed from " + filename + "\n" + "This file is not a valid JSON file." +
                "\n" + e.what();
        fp.close();
        return false;
    }

    this->history->clear();
    this->deserialize(doc, Q_NULLPTR, true);
    this->_hasBeenModified = false;
    fp.close();
    *errMsg = "";
    return true;
}

json Scene::serialize()
{
    json _nodes = json::array(), _wires = json::array();
    for (auto &node : this->nodes)
        _nodes.push_back(node->serialize());
    for (auto &wire : this->wires)
        _wires.push_back(wire->serialize());

    return {
       {"id", this->id},
       {"scene_width", this->sceneWidth},
       {"scene_height", this->sceneHeight},
       {"nodes", _nodes},
       {"wires", _wires},
    };
}

bool Scene::deserialize(json data, node_HashMap *_hashMap=Q_NULLPTR, bool restoreId=true)
{
    this->clear(true);
    this->hashMap.clear();

    for (auto &ele : this->hashMap) {
        if (ele.first == this->id) {
            this->hashMap.erase(ele.first);
            break;
        }
    }

    // 来自文件或历史记录 - 而不是来自粘贴板
    if (restoreId)
        this->id = data["id"];
    this->hashMap[this->id] = this;

    // 创建nodes
    for (auto &node_data : data["nodes"]) {
        auto n = new Node(this);
        n->deserialize(node_data, &(this->hashMap), restoreId);
    }

    // 创建wires
    for (auto &wire_data : data["wires"]) {
        auto w = new Wire(this);
        w->deserialize(wire_data, &(this->hashMap), restoreId);
    }

    return true;
}


void Scene::extractSerialDiff(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap, json& foundSerial)
{
    changeMap = { {"nodes", {}}, {"wires", {}} };
    removeMap = { {"nodes", {}}, {"wires", {}} };

    // 确定修改的scene属性
    if (originalSerial["scene_width"] != currentSerial["scene_width"])
        changeMap["scene_width"] = {originalSerial["scene_width"], currentSerial["scene_width"]};
    if (originalSerial["scene_height"] != currentSerial["scene_height"])
        changeMap["scene_height"] = {originalSerial["scene_height"], currentSerial["scene_height"]};

    // 记录已修改或删除的node
    std::set<std::string> sameNodesId;
    for (auto &node_1 : originalSerial["nodes"]) {
        json changeNode, removeNode, foundNode;
        Node::extractSerialDiff(node_1, currentSerial["nodes"], changeNode, removeNode, foundNode);
        if (!changeNode.empty())
            changeMap["nodes"].push_back(changeNode);
        if (!removeNode.empty())
            removeMap["nodes"].push_back(removeNode);
        if (!foundNode.empty())
            sameNodesId.insert(foundNode["id"].get<std::string>());
    }
    // 记录新增加的node
    json addNodes = json::array();
    for (auto &node : currentSerial["nodes"]) {
        if (!sameNodesId.count(node["id"]))
            addNodes.push_back(node);
    }
    for (auto &node : addNodes)
        changeMap["nodes"].push_back(node);

    // 记录已修改或删除的wire
    std::set<std::string> sameWiresId;
    for (auto &wire_1 : originalSerial["wires"]) {
        json changeWire, removeWire, foundWire;
        Wire::extractSerialDiff(wire_1, currentSerial["wires"], changeWire, removeWire, foundWire);
        if (!changeWire.empty())
            changeMap["wires"].push_back(changeWire);
        if (!removeWire.empty())
            removeMap["wires"].push_back(removeWire);
        if (!foundWire.empty())
            sameWiresId.insert(foundWire["id"].get<std::string>());
    }
    // 记录新增加的wire
    json addWires = json::array();
    for (auto &wire : currentSerial["wires"]) {
        if (!sameWiresId.count(wire["id"]))
            addWires.push_back(wire);
    }
    for (auto &wire : addWires)
        changeMap["wires"].push_back(wire);
}

void Scene::mergeWithIncrement(json& origSerial, json changeMap, json removeMap)
{
    for (auto &nodeChange : changeMap["nodes"]) {
        if (nodeChange.contains("modified") && nodeChange["modified"].get<bool>()) {
            json& origNode = arrayFindByKV(origSerial["nodes"], "id", nodeChange["id"]);
            Node::mergeWithIncrement(origNode, nodeChange, {});
        } else {
            origSerial["nodes"].push_back(nodeChange);
        }
    }
    for (auto &wireChange : changeMap["wires"]) {
        if (wireChange.contains("modified") && wireChange["modified"].get<bool>()) {
            json& origWire = arrayFindByKV(origSerial["wires"], "id", wireChange["id"]);
            Wire::mergeWithIncrement(origWire, wireChange, {});
        } else {
            origSerial["wires"].push_back(wireChange);
        }
    }

    for (auto &nodeRemove : removeMap["nodes"]) {
        json& origNode = arrayFindByKV(origSerial["nodes"], "id", nodeRemove["id"]);
        if (nodeRemove.contains("title")) {
            auto origNodePos = arrayAtByKV(origSerial["nodes"], "id", nodeRemove["id"]);
            origSerial["nodes"].erase(origNodePos);
        } else
            // 删除的是socket
            Socket::mergeWithIncrement(origNode, {}, nodeRemove);
    }
    for (auto &wireRemove : removeMap["wires"]) {
        auto origWirePos = arrayAtByKV(origSerial["wires"], "id", wireRemove["id"]);
        origSerial["wires"].erase(origWirePos);
    }
}

json Scene::combineChangeMaps(std::initializer_list<json> changeMaps)
{
    json totalMap = {
        {"change", {
                {"nodes", {}},
                {"wires", {}},
            }
        },
        {"remove", {
                {"nodes", {}},
                {"wires", {}},
            }
        }
    };

    QString changeDesc = "<COMBINED>";
    int changeType = 0;

    for (auto &changeMap : changeMaps) {
        changeDesc += " - " + QString(changeMap["desc"].get<std::string>().c_str());
        changeType |= changeMap["type"].get<int>();

        auto &allChange = totalMap["change"];
        auto &oneChange = changeMap["change"];

        QSet<QString> changeKeys({});
        for (auto &ele : oneChange.items())
            changeKeys.insert(QString(ele.key().c_str()));
        for (const auto& _k : changeKeys - QSet<QString>({"id", "wires", "nodes"})) {
            auto key = _k.toStdString();
            if (!allChange.contains(key))
                allChange[key] = oneChange[key];
            else
                allChange[key] = {allChange[key][0], oneChange[key][1]};
        }

        auto &allChgNodes = allChange["nodes"];
        auto &oneChgNodes = oneChange["nodes"];
        for (auto &curNode : oneChgNodes) {
            auto &oldNode = arrayFindByKV(allChgNodes, "id", curNode["id"]);
            // 添加新Node
            if (oldNode.empty())
                allChgNodes.push_back(curNode);
            // 整合旧Node
            else {
                if (curNode.contains("modified"))
                    oldNode["modified"] = curNode["modified"];
                QSet<QString> nodeKeys({});
                for (auto &ele : curNode.items())
                    nodeKeys.insert(QString(ele.key().c_str()));
                for (const auto& _k : nodeKeys - QSet<QString>({
                    "id", "modified", "inputs", "outputs", "content"})) {
                    auto key = _k.toStdString();
                    oldNode[key] = {oldNode[key][0], curNode[key][1]};
                }

                for (auto ioPuts : {"inputs", "outputs"}) {
                    if (!curNode.contains(ioPuts))
                        continue;
                    auto &allChangeIOPuts = oldNode[ioPuts];
                    auto &oneChangeIOPuts = curNode[ioPuts];
                    for (auto &curSocket : oneChangeIOPuts) {
                        auto &oldSocket = arrayFindByKV(allChangeIOPuts, "id", curSocket["id"]);
                        // 添加新socket
                        if (allChangeIOPuts.empty() || oldSocket.empty())
                            allChangeIOPuts.push_back(curSocket);
                        // 整合旧socket
                        else {
                            if (curSocket.contains("modified"))
                                oldSocket["modified"] = curSocket["modified"];
                            QSet<QString> socketKeys({});
                            for (auto &ele : curSocket.items())
                                socketKeys.insert(QString(ele.key().c_str()));
                            for (const auto& _k : socketKeys - QSet<QString>({"id", "modified"})) {
                                auto key = _k.toStdString();
                                oldSocket[key] = {oldSocket[key][0], curSocket[key][1]};
                            }
                        }
                    }
                }
            }
        }

        auto &allChgWires = allChange["wires"];
        auto &oneChgWires = oneChange["wires"];
        for (auto &curWire : oneChgWires) {
            auto &oldWire = arrayFindByKV(allChgWires, "id", curWire["id"]);
            // 添加新wire
            if (oldWire.empty())
                allChgWires.push_back(curWire);
            // 整合旧wire
            else {
                if (curWire.contains("modified"))
                    oldWire["modified"] = curWire["modified"];
                QSet<QString> wireKeys({});
                for (auto &ele : curWire.items())
                    wireKeys.insert(QString(ele.key().c_str()));
                for (const auto& _k : wireKeys - QSet<QString>({"id", "modified"})) {
                    auto key = _k.toStdString();
                    oldWire[key] = {oldWire[key][0], curWire[key][1]};
                }
            }
        }

        auto &allRemNodes = totalMap["remove"]["nodes"];
        auto &oneRemNodes = changeMap["remove"]["nodes"];
        for (auto &curNode : oneRemNodes) {
            auto &oldNode = arrayFindByKV(allRemNodes, "id", curNode["id"]);
            // 添加新Node
            if (oldNode.empty())
                allRemNodes.push_back(curNode);
        }
        auto &allRemWires = totalMap["remove"]["wires"];
        auto &oneRemWires = changeMap["remove"]["wires"];
        for (auto &curWire : oneRemWires) {
            auto &oldWire = arrayFindByKV(allRemWires, "id", curWire["id"]);
            // 添加新Wire
            if (oldWire.empty())
                allRemWires.push_back(curWire);
        }
    }

    totalMap["desc"] = changeDesc.toStdString();
    totalMap["type"] = changeType;

    return totalMap;
}

void Scene::serializeIncremental(json currentSerial, json originalSerial,
                                 json& changeMap, json& removeMap)
{
    if (originalSerial.empty()) {
        changeMap = this->serialize();
        removeMap = {};
        return;
    }

    json curSerial = !currentSerial.empty() ? currentSerial : this->serialize();
    json _;
    this->extractSerialDiff(curSerial, originalSerial, changeMap, removeMap, _);
}

void Scene::deserializeIncremental(json changeMap, bool isUndo, node_HashMap *_hashMap)
{
    if (changeMap.contains("scene_width") || changeMap.contains("scene_height")) {
        if (changeMap.contains("scene_width"))
            this->sceneWidth = changeMap["scene_width"];
        if (changeMap.contains("scene_height"))
            this->sceneHeight = changeMap["scene_height"];
    }

    for (auto &node : changeMap["change"]["nodes"]) {
        // redo
        if (!node.contains("modified") && !isUndo) {
            auto newNode = new Node(this);
            newNode->deserialize(node, &(this->hashMap), true);
        } else {
            auto nodeObj = dynamic_cast<Node*>(this->hashMap[node["id"]]);
            // undo
            if (!node.contains("modified"))
                nodeObj->remove();
            else
            {
                std::cout << "nodeObj->deserializeIncremental" << std::endl;
                // 已有node对象，重置状态
                nodeObj->deserializeIncremental(node, isUndo, &(this->hashMap));
            }
        }
    }
    for (auto &wire : changeMap["change"]["wires"]) {
        // redo
        if (!wire.contains("modified") && !isUndo) {
            auto newWire = new Wire(this);
            newWire->deserialize(wire, &(this->hashMap), true);
        } else {
            auto wireObj = dynamic_cast<Wire*>(this->hashMap[wire["id"]]);
            // undo
            if (!wire.contains("modified"))
                wireObj->remove();
            else
                // 已有wire对象，重置状态
                wireObj->deserializeIncremental(wire, isUndo, &(this->hashMap));
        }
    }

    for (auto &node : changeMap["remove"]["nodes"]) {
        // 删除的是node上的socket
        if (!node.contains("title")) {
            auto nodeObj = dynamic_cast<Node*>(this->hashMap[node["id"]]);
            if (isUndo) {
                for (auto &inputSocket : node["inputs"]) {
                    nodeObj->initSockets({inputSocket["socket_type"]}, {}, false);
                    auto &socketObj = nodeObj->inputs[0];
                    socketObj->deserialize(inputSocket, &(this->hashMap), true);
                }
                for (auto &outputSocket : node["outputs"]) {
                    nodeObj->initSockets({}, {outputSocket["socket_type"]}, false);
                    auto &socketObj = nodeObj->outputs[0];
                    socketObj->deserialize(outputSocket, &(this->hashMap), true);
                }
            } else {
                for (auto &inputSocket : node["inputs"])
                    nodeObj->removeSocket(dynamic_cast<Socket*>(this->hashMap[inputSocket["id"]]));
                for (auto &outputSocket : node["outputs"])
                    nodeObj->removeSocket(dynamic_cast<Socket*>(this->hashMap[outputSocket["id"]]));
            }
        } else {
            if (isUndo) {
                auto newNode = new Node(this);
                newNode->deserialize(node, &(this->hashMap), true);
            } else {
                auto nodeObj = dynamic_cast<Node*>(this->hashMap[node["id"]]);
                nodeObj->remove();
            }
        }
    }
    for (auto &wire : changeMap["remove"]["wires"]) {
        if (isUndo) {
            auto newWire = new Wire(this);
            newWire->deserialize(wire, &(this->hashMap), true);
        } else {
            // 可能随node一起被删除，需检查
            if (this->hashMap.count(wire["id"])) {
                auto wireObj = dynamic_cast<Wire*>(this->hashMap[wire["id"]]);
                wireObj->remove();
            }
        }
    }
}
