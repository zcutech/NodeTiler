//
// Created by Charlie Zhong on 2021/9/6.
//

#include "node_scene.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "node_node.h"
#include "node_wire.h"
#include "node_socket.h"
#include "node_graphics_scene.h"
#include "node_scene_history.h"
#include "node_scene_clipboard.h"
#include "node_graphics_view.h"


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
    clipboard(new SceneClipboard(this)),
    nodeClsSelector(Q_NULLPTR)
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
        // ??????????????????????????????
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

void Scene::addDragEnterListener(const std::function<void(QDragEnterEvent *event)> &callback) const
{
    this->getView()->addDragEnterListener(callback);
}

void Scene::addDropListener(const std::function<void(QDropEvent *event)> &callback) const
{
    this->getView()->addDropListener(callback);
}

void Scene::addNode(Node *node) {
    this->nodes.push_back(node);
    this->hashMap[node->id] = node;

    for (auto s : node->inputs)
        this->hashMap[s->id] = s;
    for (auto s : node->outputs)
        this->hashMap[s->id] = s;

    this->hasBeenModified(true);
}

void Scene::addWire(Wire *wire) {
    this->wires.push_back(wire);
    this->hashMap[wire->id] = wire;

    this->hasBeenModified(true);
}

void Scene::removeNode(Node *node) {
    auto it = std::find(this->nodes.begin(), this->nodes.end(), node);
    if (it != this->nodes.end()) {
        this->hashMap.erase(node->id);
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

QDMGraphicsView* Scene::getView() const
{
    return dynamic_cast<QDMGraphicsView*>(this->grScene->views()[0]);
}

QGraphicsItem* Scene::getItemAt(QPoint p) const
{
    return this->getView()->itemAt(p);
}

// when set a nodeClsSelector function, we can use different type of node
void Scene::setNodeClsSelector(std::function<NodeClassProxy (json&)> clsSelectingFunc)
{
    this->nodeClsSelector = clsSelectingFunc;
}

// an agent method, maybe return different derived node class, by the specified node serial data
// or just the Node if no selector function set
NodeClassProxy Scene::getNodeClsFromData(json& nodeData) const
{
    if (!this->nodeClsSelector)
        // for code consistency, don't call init here
        return [](Scene* s) { return new Node(s); };
    return this->nodeClsSelector(nodeData);
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

    // ??????????????????????????? - ????????????????????????
    if (restoreId)
        this->id = data["id"];
    this->hashMap[this->id] = this;

    // create nodes
    for (auto &nodeData : data["nodes"]) {
        auto nodeClass = this->getNodeClsFromData(nodeData);
        auto n = (nodeClass(this))->init();
        n->deserialize(nodeData, &(this->hashMap), restoreId);
    }

    // create wires
    for (auto &wire_data : data["wires"]) {
        auto w = new Wire(this, Q_NULLPTR, Q_NULLPTR, WIRE_TYPE_BEZIER);
        w->deserialize(wire_data, &(this->hashMap), restoreId);
    }

    return true;
}


void Scene::extractSerialDiff(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap, json& foundSerial)
{
    changeMap = { {"nodes", {}}, {"wires", {}} };
    removeMap = { {"nodes", {}}, {"wires", {}} };

    // ???????????????scene??????
    if (originalSerial["scene_width"] != currentSerial["scene_width"])
        changeMap["scene_width"] = {originalSerial["scene_width"], currentSerial["scene_width"]};
    if (originalSerial["scene_height"] != currentSerial["scene_height"])
        changeMap["scene_height"] = {originalSerial["scene_height"], currentSerial["scene_height"]};

    // ???????????????????????????node
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
    // ??????????????????node
    json addNodes = json::array();
    for (auto &node : currentSerial["nodes"]) {
        if (!sameNodesId.count(node["id"]))
            addNodes.push_back(node);
    }
    for (auto &node : addNodes)
        changeMap["nodes"].push_back(node);

    // ???????????????????????????wire
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
    // ??????????????????wire
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
            // ????????????socket
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
            // ?????????Node
            if (oldNode.empty())
                allChgNodes.push_back(curNode);
            // ?????????Node
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
                        // ?????????socket
                        if (allChangeIOPuts.empty() || oldSocket.empty())
                            allChangeIOPuts.push_back(curSocket);
                        // ?????????socket
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
            // ?????????wire
            if (oldWire.empty())
                allChgWires.push_back(curWire);
            // ?????????wire
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
            // ?????????Node
            if (oldNode.empty())
                allRemNodes.push_back(curNode);
        }
        auto &allRemWires = totalMap["remove"]["wires"];
        auto &oneRemWires = changeMap["remove"]["wires"];
        for (auto &curWire : oneRemWires) {
            auto &oldWire = arrayFindByKV(allRemWires, "id", curWire["id"]);
            // ?????????Wire
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

    for (auto &nodeData : changeMap["change"]["nodes"]) {
        // redo
        if (!nodeData.contains("modified") && !isUndo) {
            auto nodeClass = this->getNodeClsFromData(nodeData);
            auto newNode = (nodeClass(this))->init();
            newNode->deserialize(nodeData, &(this->hashMap), true);
        } else {
            auto nodeObj = dynamic_cast<Node*>(this->hashMap[nodeData["id"]]);
            // undo
            if (!nodeData.contains("modified"))
                nodeObj->remove();
            else
            {
                std::cout << "nodeObj->deserializeIncremental" << std::endl;
                // // node object has existed, reset its status
                nodeObj->deserializeIncremental(nodeData, isUndo, &(this->hashMap));
            }
        }
    }
    for (auto &wireData : changeMap["change"]["wires"]) {
        // redo
        if (!wireData.contains("modified") && !isUndo) {
            auto newWire = new Wire(this, Q_NULLPTR, Q_NULLPTR, WIRE_TYPE_BEZIER);
            newWire->deserialize(wireData, &(this->hashMap), true);
        } else {
            auto wireObj = dynamic_cast<Wire*>(this->hashMap[wireData["id"]]);
            // undo
            if (!wireData.contains("modified"))
                wireObj->remove();
            else
                // wire object has existed, reset its status
                wireObj->deserializeIncremental(wireData, isUndo, &(this->hashMap));
        }
    }

    for (auto &nodeData : changeMap["remove"]["nodes"]) {
        // ????????????node??????socket
        if (!nodeData.contains("title")) {
            auto nodeObj = dynamic_cast<Node*>(this->hashMap[nodeData["id"]]);
            if (isUndo) {
                for (auto &inputSocket : nodeData["inputs"]) {
                    nodeObj->initSockets({inputSocket["socket_type"]}, {}, false);
                    auto &socketObj = nodeObj->inputs[0];
                    socketObj->deserialize(inputSocket, &(this->hashMap), true);
                }
                for (auto &outputSocket : nodeData["outputs"]) {
                    nodeObj->initSockets({}, {outputSocket["socket_type"]}, false);
                    auto &socketObj = nodeObj->outputs[0];
                    socketObj->deserialize(outputSocket, &(this->hashMap), true);
                }
            } else {
                for (auto &inputSocket : nodeData["inputs"])
                    nodeObj->removeSocket(dynamic_cast<Socket*>(this->hashMap[inputSocket["id"]]));
                for (auto &outputSocket : nodeData["outputs"])
                    nodeObj->removeSocket(dynamic_cast<Socket*>(this->hashMap[outputSocket["id"]]));
            }
        } else {
            if (isUndo) {
                auto nodeClass = this->getNodeClsFromData(nodeData);
                auto newNode = (nodeClass(this))->init();
                newNode->deserialize(nodeData, &(this->hashMap), true);
            } else {
                auto nodeObj = dynamic_cast<Node*>(this->hashMap[nodeData["id"]]);
                nodeObj->remove();
            }
        }
    }
    for (auto &wire : changeMap["remove"]["wires"]) {
        if (isUndo) {
            auto newWire = new Wire(this, Q_NULLPTR, Q_NULLPTR, WIRE_TYPE_BEZIER);
            newWire->deserialize(wire, &(this->hashMap), true);
        } else {
            // ?????????node???????????????????????????
            if (this->hashMap.count(wire["id"])) {
                auto wireObj = dynamic_cast<Wire*>(this->hashMap[wire["id"]]);
                wireObj->remove();
            }
        }
    }
}
