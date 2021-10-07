//
// Created by Charlie Zhong on 2021/9/10.
//

#ifndef NODETILER_NODE_SOCKET_H
#define NODETILER_NODE_SOCKET_H

#include <QtCore>

#include "node_common.h"
#include "node_serializable.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class Node;
class Wire;
class QDMGraphicsSocket;

class Socket : public Serializable
{
public:
    explicit Socket(Node *node, size_t index=0, SOCKET_POSITION pos=SCT_AT_LEFT_TOP,
           SOCKET_TYPE socket_type=SCT_TYPE_1);
    ~Socket() = default;
    Node *node;
    std::vector<Wire*> wires;
    QDMGraphicsSocket *grSocket;
    QPointF getSocketPos() const;
    void setAttachedWire(Wire *w=Q_NULLPTR);
    void delAttachedWire(Wire *w);
    void updateAttachedWires();
    bool hasWire(Wire *w=Q_NULLPTR);
    void clearAllWires();
    bool isOutput() const;
    SOCKET_POSITION position;
    void remove();
    json serialize() override;
    bool deserialize(json data, node_HashMap *hashMap, bool restoreId) override;
    void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap) override;
    void serializeIncremental(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap) override {};
    // 在指定序列组中查找和比较与指定的另一个序列的差异，返回更改和删除部分
    static void extractSerialDiff(json anotherSerial, json myArray,
                                  json& changeMap, json& removeMap, json& foundSerial);
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap);

protected:
    const QString filePath = __FILE__;
    const QString& getFilePath() const override { return this->filePath; }
private:
    size_t index;
    SOCKET_TYPE socketType;
};

#endif //NODETILER_NODE_SOCKET_H
