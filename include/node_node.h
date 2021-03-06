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
    // must call init method firstly after construction
    explicit Node(Scene *_scene, const std::string& _title="Undefined Node",
         std::vector<SOCKET_TYPE> inputs={}, std::vector<SOCKET_TYPE> outputs={});
    ~Node() = default;
    Scene *scene;
    QDMGraphicsNode *grNode;
    SOCKET_POSITION inputSocketPos;
    SOCKET_POSITION outputSocketPos;
    std::vector<Socket*> inputs;
    std::vector<Socket*> outputs;
    virtual Node* init();
    virtual void initInnerClasses();
    virtual void initSettings();
    // create input and output sockets
    void initSockets(std::vector<SOCKET_TYPE> _inputs,
                     std::vector<SOCKET_TYPE> _outputs, bool resetAll=true);
    void addSockets(bool isInput, SOCKET_TYPE socketType,
                    const std::string& name, const std::string &desc);
    Socket* findSocketIBySerial(json &socketSerial);
    Socket* findSocketOBySerial(json &socketSerial);
    QPointF pos() const;
    void setPos(QPointF p);
    void setPos(float x, float y);
    QString title() const;
    void title(const std::string& _title);
    void removeSocket(Socket* s=Q_NULLPTR);
    QPointF getSocketPos(size_t index, SOCKET_POSITION pos, size_t numOutOf = 1) const;
    void updateAttachedWires();
    void selectAttachedWires(bool revert=false);
    void setSelectedSilently(bool isSelected) const;
    void remove();
    bool isDirty() const;
    void markDirty(bool newVal=true);
    void onMarkedDirty();
    void markChildDirty(bool newVal=true);
    void markDescendantDirty(bool newVal=true);
    bool isInvalid() const;
    void markInvalid(bool newVal=true);
    void onMarkedInvalid();
    void markChildInvalid(bool newVal=true);
    void markDescendantInvalid(bool newVal=true);
    int eval();
    void evalChildren();
    std::vector<Node*> getChildrenNodes();
    json serialize() override;
    bool deserialize(json data, node_HashMap *hashMap, bool restoreId) override;
    void serializeIncremental(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap) override {};
    void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap) override;
    // ??????????????????????????????????????????????????????????????????????????????????????????????????????
    static void extractSerialDiff(json anotherSerial, json myArray,
                                  json& changeMap, json& removeMap, json& foundSerial);
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap);

protected:
    QDMNodeContentWidget *content;
    QString _title;
    size_t socketSpacing;
    std::vector<SOCKET_TYPE> inTypeVec;
    std::vector<SOCKET_TYPE> outTypeVec;

private:
    bool _isDirty;
    bool _isInvalid;
};


#endif //NODETILER_NODE_NODE_H
