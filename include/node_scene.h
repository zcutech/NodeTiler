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
    json serialize() override;
    bool deserialize(json data, node_HashMap *_hashMap, bool restoreId) override;
    // 相对于 `original_serial`, 计算 `current_serial` 中的差异, 返回 增改和删除两项
    static void extractSerialDiff(json currentSerial, json originalSerial,
                                  json& changeMap, json& removeMap, json& foundSerial);
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap);
    // 将多个序列化对象组合为单个
    static json combineChangeMaps(std::initializer_list<json> changeMaps);
    // 返回序列化对象 `current_serial` 与 `original_serial` 之间 更改(及新增)和删除部分的 map
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
    // 与addHasBeenModifiedListeners中相同，对于传入的lambda函数，不可以指针形式存储
    std::vector<std::function<void()>> _hasBeenModifiedListeners;
    std::vector<std::function<void()>> _itemSelectedListeners;
    std::vector<std::function<void()>> _itemDeselectedListeners;
};

#endif //NODETILER_NODE_SCENE_H
