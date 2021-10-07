//
// Created by Charlie Zhong on 2021/9/10.
//

#ifndef NODETILER_NODE_CONTENT_WIDGET_H
#define NODETILER_NODE_CONTENT_WIDGET_H

# include "node_serializable.h"

#include <QtWidgets>
#include <QtCore>

#include "nlohmann/json.hpp"
using json = nlohmann::json;


class Node;

class QDMNodeContentWidget : public QWidget, public Serializable
{
    Q_OBJECT
public:
    explicit QDMNodeContentWidget(Node* node, QWidget *parent=Q_NULLPTR);
    ~QDMNodeContentWidget() override = default;
    void setEditingFlag(int value);
    json serialize() override;
    bool deserialize(json data, node_HashMap *hashMap, bool restoreId) override;
    void serializeIncremental(json currentSerial, json originalSerial,
                              json& changeMap, json& removeMap) override {};
    void deserializeIncremental(json changeMap, bool isUndo, node_HashMap *hashMap) override {};
protected:
    const QString filePath = __FILE__;
    const QString& getFilePath() const override { return this->filePath; }
private:
    Node *node;
    QVBoxLayout *layout;
    QLabel *wdgLabel;
};

class QDMQTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit QDMQTextEdit(QWidget *parent = Q_NULLPTR);
    explicit QDMQTextEdit(const QString &text, QWidget *parent = Q_NULLPTR);
    ~QDMQTextEdit() override = default;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
};

#endif //NODETILER_NODE_CONTENT_WIDGET_H
