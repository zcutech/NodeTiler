//
// Created by Charlie Zhong on 2021/9/23.
//

#include "node_serializable.h"

#include <sstream>


json& arrayFindByKV(json& jsonArray, const std::string& k, const std::string& v)
{
    for (auto &ele : jsonArray) {
        if (ele.contains(k) && ele[k] == v)
            return ele;
    }
    return *(new json());
}

const json::size_type arrayAtByKV(json& jsonArray, const std::string& k, const std::string& v)
{
    json::size_type i = 0;
    for (auto &ele : jsonArray) {
        if (ele.contains(k) && ele[k] == v)
            return i;
        ++i;
    }
    return -1;
}


Serializable::Serializable()
{
    this->id = QUuid::createUuid().toString().remove("{").remove("}").remove("-").toStdString();
}

std::ostream &operator<<(std::ostream &os, const Serializable &s) {
    QString typeInfo = QString(typeid(s).name()).remove( QRegExp("^[0-9]*") );
    QString objectId = QString("0x%1").arg((quintptr)(&s), QT_POINTER_SIZE * 2, 16, QChar('0'));
    auto desc = QString("<%1 object at %3>").arg(typeInfo, objectId);
    return (os << desc.toStdString());
}



std::string Serializable::toString() const {
    std::stringstream ss;
    ss << (*this);
    return ss.str();
}

