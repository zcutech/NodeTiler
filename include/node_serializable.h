//
// Created by Charlie Zhong on 2021/9/23.
//

#ifndef NODETILER_NODE_SERIALIZABLE_H
#define NODETILER_NODE_SERIALIZABLE_H

#include <typeinfo>

#include <QtCore>
#include "node_common.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;


json& arrayFindByKV(json& jsonArray, const std::string& k, const std::string& v);
const json::size_type arrayAtByKV(json& jsonArray, const std::string& k, const std::string& v);

class Serializable
{
public:
    friend std::ostream& operator << (std::ostream &os, const Serializable &s);
    Serializable();
    ~Serializable() = default;
    std::string id;
    std::string toString() const;
    // 将图形对象序列化为json
    virtual json serialize() = 0;
    // 将json反序列化生成对应图形对象
    virtual bool deserialize(json data, node_HashMap *hashMap, bool restoreId) = 0;
    // 相对于 `originalSerial`, 计算 `currentSerial` 中的差异, 返回 增改和删除两项
    static void extractSerialDiff(json currentSerial, json originalSerial,
                                  json& changeMap, json& removeMap, json& foundSerial) {};
    // 将更改(及新增)和删除部分的json合并到原序列化对象
    static void mergeWithIncrement(json& origSerial, json changeMap, json removeMap) {};
    // 将多个序列化对象组合为单个
    static json combineChangeMaps(std::initializer_list<json>) {};
    // 返回序列化对象 `current_serial` 与 `original_serial` 之间 更改(及新增)和删除部分的 map
    virtual void serializeIncremental(json currentSerial, json originalSerial,
                                      json& changeMap, json& removeMap) = 0;
    // 根据更改(及新增)和删除部分 `change_map` 还原或重建图形对象
    virtual void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap) = 0;
};

#endif //NODETILER_NODE_SERIALIZABLE_H
