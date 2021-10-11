//
// Created by Charlie Zhong on 2021/9/23.
//

#ifndef NODETILER_NODE_SCENE_HISTORY_H
#define NODETILER_NODE_SCENE_HISTORY_H

#include <QtCore>
#include "node_common.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class Scene;

class SceneHistory
{
public:
    explicit SceneHistory(Scene *scene);
    ~SceneHistory() = default;
    void clear();
    void storeInitialHistory();
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    void addHistoryModifiedListener(const std::function<void()>& callback);
    void restoreHistory(bool isUndo, int toStep=-1);
    void storeHistory(const QString& desc, VIEW_HIST::Flags opType,
                      bool setModified=false, bool mergeLast=false);
    json createHistoryStamp(const QString& desc, VIEW_HIST::Flags opType);
    // 从head_snapshot至当前stamp合成为一个完整序列化对象
    json compositeHistoryStamp();
    void restoreHistoryStamp(bool isUndo, json historyStamp);
private:
    Scene *scene;
    QVector<json> historyStack;
    int historyCurStep;
    size_t historyLimit;
    json headSnapshot;       // 最初创建的完整序列化对象
    json tailSnapshot;       // 栈最右端的完整序列化对象
    std::vector<std::function<void()>> _historyModifiedListeners;
};

#endif //NODETILER_NODE_SCENE_HISTORY_H
