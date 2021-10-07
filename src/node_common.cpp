//
// Created by Charlie Zhong on 2021/9/15.
//

#include "node_common.h"

#include <map>

// 从socket创建新wire时距离阈值
int SCT_DRAG_START_THRESHOLD = 10;

// 贝塞尔曲线，控制弧度
double WIRE_CP_ROUNDNESS = 100.0;


std::string& easyMap::get(int &&k)
{
    auto e = this->find(k);
    if (e != this->end())
        return e->second;
    else
        return *(new std::string());        // 不存在时返回空字符串
}

std::string& easyMap::get(const int &k)
{
    auto e = this->find(k);
    if (e != this->end())
        return e->second;
    else
        return *(new std::string());        // 不存在时返回空字符串
}

easyMap::easyMap(std::initializer_list<std::pair<int, std::string> > pl)
{
    for (auto &p : pl)
        this->insert(p);
}

// GRAPHICS_TYPE 与 字符串名称的映射
easyMap grTypeName = {
        {GRAPH_TYPE_NODE,           "QDMGraphicsNode"},
        {GRAPH_TYPE_SOCKET,         "QDMGraphicsSocket"},
        {GRAPH_TYPE_WIRE,           "QDMGraphicsWire"},
};

typedef std::map<int, std::string> grTypeMap;


