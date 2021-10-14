//
// Created by Charlie Zhong on 2021/9/23.
//

#include "node_scene_history.h"

#include <iostream>
#include <iomanip>
#include <utility>

#include "node_graphics_scene.h"
#include "node_graphics_node.h"
#include "node_graphics_wire.h"
#include "node_scene.h"
#include "node_node.h"
#include "node_wire.h"


SceneHistory::SceneHistory(Scene *scene):
    scene(scene),
    historyStack({}),
    historyCurStep(-1),
    historyLimit(32),
    headSnapshot({}),
    tailSnapshot({}),
    _historyModifiedListeners({})
{
}

void SceneHistory::clear()
{
    this->historyStack.clear();
    this->historyCurStep = -1;
}

void SceneHistory::storeInitialHistory()
{
    this->storeHistory("Initial State", VIEW_HIST::INIT_VIEW);
}

bool SceneHistory::canUndo() const
{
    return this->historyCurStep > 0;
}

bool SceneHistory::canRedo() const
{
    return this->historyCurStep + 1 < this->historyStack.size();
}

void SceneHistory::undo()
{
    if (this->canUndo()) {
        this->historyCurStep -= 1;
        this->restoreHistory(true, this->historyCurStep + 1);

        auto isModified = false;
        if (this->historyCurStep > 0) {
            for (auto i = 1; i <= this->historyCurStep; ++i) {
                auto h = this->historyStack[i];
                // not only select and deselect action, consider it's modified
                if ((h["type"].get<int>() & ~VIEW_HIST::SEL_DESEL_ITEMS) != 0) {
                    isModified = true;
                    break;
                }
            }
        }
        this->scene->hasBeenModified(isModified);
    }
}

void SceneHistory::redo()
{
    if (this->canRedo()) {
        this->historyCurStep += 1;
        this->restoreHistory(false);

        auto isModified = false;
        if (this->historyCurStep > 0) {
            for (auto i = 1; i <= this->historyCurStep; ++i) {
                auto h = this->historyStack[i];
                if ((h["type"].get<int>() & ~VIEW_HIST::SEL_DESEL_ITEMS) != 0) {
                    isModified = true;
                    break;
                }
            }
        }
        this->scene->hasBeenModified(isModified);
    }
}

void SceneHistory::addHistoryModifiedListener(const std::function<void()> &callback)
{
    this->_historyModifiedListeners.push_back(callback);
}

void SceneHistory::restoreHistory(bool isUndo, int toStep)
{
    std::cout << (isUndo ? " < UNDO " : " > REDO ") << std::endl;
    auto step = (toStep == -1) ? this->historyCurStep : toStep;
    this->restoreHistoryStamp(isUndo, this->historyStack[step]);
    for (const auto& callback : this->_historyModifiedListeners)
        callback();
}

void SceneHistory::storeHistory(const QString& desc, VIEW_HIST::Flags opType,
                                bool setModified, bool mergeLast)
{
    std::cout << QString("-- store_history - [%1] ... \t\tcur_step: (%2)").arg(
            desc).arg(this->historyCurStep).toStdString() << std::endl;

    if (setModified)
        this->scene->hasBeenModified(true);
    // check if the stack pointer is not at the end of stack
    if (this->historyCurStep + 1 < this->historyStack.size()) {
        this->historyStack = this->historyStack.mid(0, this->historyCurStep+1);
        this->tailSnapshot = this->compositeHistoryStamp();
    }
    // check if current history stack length beyond limit
    if (this->historyCurStep + 1 >= this->historyLimit) {
        this->historyStack = this->historyStack.mid(1);
        this->historyCurStep -= 1;
    }

    auto hs = this->createHistoryStamp(desc, opType);

    // do not change the first history item
    if (mergeLast && this->historyCurStep > 0) {
        this->historyStack[this->historyCurStep] = Scene::combineChangeMaps(
                {this->historyStack[this->historyCurStep], hs});
    } else {
        this->historyStack.append(hs);
        this->historyCurStep += 1;
    }

    std::cout << "====================================================== ↓ store_history" << std::endl;
    std::cout << std::setw(4) << this->historyStack[this->historyCurStep] << std::endl;
    std::cout << "====================================================== ↑ store_history" << std::endl;

    for (const auto& callback : this->_historyModifiedListeners)
        callback();
}

json SceneHistory::createHistoryStamp(const QString& desc, VIEW_HIST::Flags opType)
{
    json historyStamp = json::object();

    if (this->historyCurStep == -1) {
        this->headSnapshot = this->tailSnapshot = this->scene->serialize();
        historyStamp = {
            {"desc", desc.toStdString()},
            {"type", int(opType)},
            {"snapshot", this->tailSnapshot},
        };
    } else {
        auto currentSnapshot = this->scene->serialize();
        json changeMap, removeMap;
        this->scene->serializeIncremental(currentSnapshot, this->tailSnapshot, changeMap, removeMap);
        historyStamp = {
            {"desc", desc.toStdString()},
            {"type", int(opType)},
            {"change", changeMap},
            {"remove", removeMap},
        };
        this->tailSnapshot = currentSnapshot;
    }

    return historyStamp;
}

json SceneHistory::compositeHistoryStamp()
{
    json comp = this->headSnapshot;
    for (auto i = 1; i < this->historyStack.size(); ++i) {
        auto stamp = this->historyStack[i];
        Scene::mergeWithIncrement(comp, stamp["change"], stamp["remove"]);
    }
    return comp;
}

void SceneHistory::restoreHistoryStamp(bool isUndo, json historyStamp)
{
    std::cout << "++++++++++++++++++++++++++++ ↓" << std::endl;
    for (const auto& p : this->scene->hashMap)
        std::cout << p.first << "\t" << *(p.second) << std::endl;
    std::cout << "++++++++++++++++++++++++++++ ↑" << std::endl;

    std::cout << "====================================================== ↓ restore_history_stamp" << std::endl;
    std::cout << std::setw(4) << this->historyStack[this->historyCurStep] << std::endl;
    std::cout << "====================================================== ↑ restore_history_stamp" << std::endl;

    this->scene->deserializeIncremental(std::move(historyStamp), isUndo, Q_NULLPTR);

    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ↓" << std::endl;
    for (const auto& p : this->scene->hashMap)
        std::cout << p.first << "\t" << *(p.second) << std::endl;
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ↑" << std::endl;
}

