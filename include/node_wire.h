//
// Created by Charlie Zhong on 2021/9/11.
//

#ifndef NODETILER_NODE_WIRE_H
#define NODETILER_NODE_WIRE_H

#include <QtCore>

#include "node_common.h"
#include "node_serializable.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class Socket;
class QDMGraphicsWire;
class Scene;

class Wire : public Serializable
{
public:
    explicit Wire(Scene *scene, Socket *startSocket=Q_NULLPTR, Socket *endSocket=Q_NULLPTR,
         WIRE_TYPE wire_type=WIRE_TYPE_BEZIER);
    ~Wire() = default;
    Scene *scene;
    Socket *outputSocket;
    Socket *inputSocket;
    WIRE_STATE::Flags state;
    QDMGraphicsWire *grWire;
    WIRE_TYPE wireType() const;
    void wireType(WIRE_TYPE t);
    Socket* startSocket() const;
    void startSocket(Socket *s);
    Socket* endSocket() const;
    void endSocket(Socket *s);
    void updatePositions();
    void setSelectedSilently(bool isSelected) const;
    void detachFromSockets(Socket* socket=Q_NULLPTR);
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
private:
    Socket *_startSocket;
    Socket *_endSocket;
    WIRE_TYPE _wireType;
};

#endif //NODETILER_NODE_WIRE_H
