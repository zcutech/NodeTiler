//
// Created by Charlie Zhong on 2021/9/6.
//

#ifndef NODETILER_NODE_NODE_H
#define NODETILER_NODE_NODE_H

#include <vector>
#include <string>
#include <QtCore>

#include "node_common.h"
#include "node_serializable.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class QDMGraphicsNode;
class Socket;
class Scene;
class QDMNodeContentWidget;


class Node : public Serializable {
public:
    friend QDMGraphicsNode;
    friend Socket;
    explicit Node(Scene *_scene, const std::string& _title="Undefined Node",
         const std::vector<SOCKET_TYPE>& inputs = {}, const std::vector<SOCKET_TYPE>& outputs ={});
    ~Node() = default;
    Scene *scene;
    QDMGraphicsNode *grNode;
    SOCKET_POSITION inputSocketPos;
    SOCKET_POSITION outputSocketPos;
    std::vector<Socket*> inputs;
    std::vector<Socket*> outputs;
    void initInnerClasses();
    void initSettings();
    // 创建输入和输出 sockets
    void initSockets(const std::vector<SOCKET_TYPE>& _inputs,
                     const std::vector<SOCKET_TYPE>& _outputs, bool resetAll=true);
    QPointF pos() const;
    void setPos(QPointF p);
    void setPos(float x, float y);
    QString title() const;
    void title(const std::string& _title);
    void removeSocket(Socket* s);
    QPointF getSocketPos(int index, SOCKET_POSITION pos) const;
    void updateAttachedWires();
    void selectAttachedWires(bool revert=false);
    void setSelectedSilently(bool isSelected) const;
    void remove();
    json serialize() override;
    bool deserialize(json data, node_HashMap *hashMap, bool restoreId) override;
    void serializeIncremental(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap) override {};
    void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap) override;
    // 在指定序列组中查找和比较与指定的另一个序列的差异，返回更改和删除部分
    static void extractSerialDiff(json anotherSerial, json myArray,
                                  json& changeMap, json& removeMap, json& foundSerial);
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap);

protected:
    const QString filePath = __FILE__;
    const QString& getFilePath() const override { return this->filePath; }
    QDMNodeContentWidget *content;
    QString _title;
    size_t socketSpacing;
};


#endif //NODETILER_NODE_NODE_H
