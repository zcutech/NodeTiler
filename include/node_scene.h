//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_SCENE_H
#define NODETILER_NODE_SCENE_H

#include <functional>
#include <vector>

#include <QtCore>
#include <QtGui>

#include "node_common.h"
#include "node_serializable.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class QDMGraphicsScene;
class Wire;
class Node;
class SceneHistory;
class SceneClipboard;
class QDMGraphicsView;

class Scene;
using NodeClassProxy = std::function<Node* (Scene*)>;

class Scene : public QObject, public Serializable
{
    Q_OBJECT
public:
    Scene();
    ~Scene() override = default;
    QDMGraphicsScene *grScene;
    std::vector<Node*> nodes;
    std::vector<Wire*> wires;
    node_HashMap hashMap;
    SceneHistory *history;
    SceneClipboard *clipboard;
    // a callback function to retrieve node class
    std::function<NodeClassProxy (json&)> nodeClsSelector;
    QList<QGraphicsItem*> getSelectedItems() const;
    bool hasBeenModified() const;
    void hasBeenModified(bool value);
    void addHasBeenModifiedListeners(const std::function<void()>& callback);
    void addItemSelectedListener(const std::function<void()>& callback);
    void addItemDeselectedListener(const std::function<void()>& callback);
    void addDragEnterListener(const std::function<void(QDragEnterEvent *event)>& callback) const;
    void addDropListener(const std::function<void(QDropEvent *event)>& callback) const;
    void addNode(Node*);
    void addWire(Wire*);
    void removeNode(Node*);
    void removeWire(Wire*);
    void clear(bool keepModified=false);
    bool saveToFile(const QString& filename, QString *saveMsg);
    bool loadFromFile(const QString& filename, QString *errMsg);
    QDMGraphicsView* getView() const;
    QGraphicsItem* getItemAt(QPoint p) const;
    // set a callback function to retrieve node class, could be called by derived module
    void setNodeClsSelector(std::function<NodeClassProxy (json&)> clsSelectingFunc);
    // an agent method, maybe return different type of node class, or just the Node if no selector
    NodeClassProxy getNodeClsFromData(json& nodeData) const;
    json serialize() override;
    bool deserialize(json data, node_HashMap *_hashMap, bool restoreId) override;
    // ????????? `original_serial`, ?????? `current_serial` ????????????, ?????? ?????????????????????
    static void extractSerialDiff(json currentSerial, json originalSerial,
                                  json& changeMap, json& removeMap, json& foundSerial);
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap);
    // ???????????????????????????????????????
    static json combineChangeMaps(std::initializer_list<json> changeMaps);
    // ????????????????????? `current_serial` ??? `original_serial` ?????? ??????(?????????)?????????????????? map
    void serializeIncremental(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap) override;
    void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *_hashMap) override;

protected slots:
    void onItemSelected(bool mergeLastDeselection = false);
    void onItemDeselected();
    void onSelsChanged();
private:
    int sceneWidth;
    int sceneHeight;
    bool _hasBeenModified;
    QList<QGraphicsItem*> _lastSelectedItems;
    // ???addHasBeenModifiedListeners???????????????????????????lambda????????????????????????????????????
    std::vector<std::function<void()>> _hasBeenModifiedListeners;
    std::vector<std::function<void()>> _itemSelectedListeners;
    std::vector<std::function<void()>> _itemDeselectedListeners;
};


#endif //NODETILER_NODE_SCENE_H
