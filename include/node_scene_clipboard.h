//
// Created by Charlie Zhong on 2021/9/25.
//

#ifndef NODETILER_NODE_SCENE_CLIPBOARD_H
#define NODETILER_NODE_SCENE_CLIPBOARD_H

#include <QtCore>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

class Scene;

class SceneClipboard
{
public:
    SceneClipboard(Scene *scene);
    ~SceneClipboard();
    json serializeSelected(bool deleteThem=false);
    void deserializeFromClipboard(json data);
private:
    Scene *scene;
};

#endif //NODETILER_NODE_SCENE_CLIPBOARD_H
