//
// Created by Charlie Zhong on 2021/9/10.
//

#ifndef NODETILER_NODE_COMMON_H
#define NODETILER_NODE_COMMON_H

#include <QtWidgets/QGraphicsItem>

#include <map>

// 从socket创建新wire时距离阈值
extern int SCT_DRAG_START_THRESHOLD;

// 贝塞尔曲线，控制弧度
extern double WIRE_CP_ROUNDNESS;

// 定义一种新map，增加get方法
class easyMap : public std::map<int, std::string> {
public:
    easyMap(std::initializer_list<std::pair<int, std::string> > pl);
    std::string& get(int &&k);
    std::string& get(const int &k);
};


// hash map for deserialize - id : pointer
class Serializable;
typedef std::map<std::string, Serializable*> node_HashMap;

// 图元类型
enum GRAPHICS_TYPE {
    GRAPH_TYPE_NODE = QGraphicsItem::UserType + 1,
    GRAPH_TYPE_SOCKET,
    GRAPH_TYPE_WIRE,
};

extern easyMap grTypeName;

// 端口方位
enum SOCKET_POSITION {
    SCT_AT_LEFT_TOP = 1,
    SCT_AT_LEFT_CENTER = 2,
    SCT_AT_LEFT_BOTTOM = 3,
    SCT_AT_RIGHT_TOP = 4,
    SCT_AT_RIGHT_CENTER = 5,
    SCT_AT_RIGHT_BOTTOM = 6,
};
// 端口类型
enum SOCKET_TYPE {
    SCT_TYPE_1,
    SCT_TYPE_2,
    SCT_TYPE_3,
    SCT_TYPE_4,
    SCT_TYPE_5,
    SCT_TYPE_6,
};
// 端口动作
enum SOCKET_ACTION {
    SCT_ACT_NOOP = 0,                                           // 端口无操作
    SCT_ACT_WIRE_DRAGGING = 1,                                  // 端口创建出连线
    SCT_ACT_WIRE_BATCHING = 2,                                  // 输出端口批量创建连线
    SCT_ACT_WIRE_SHIFTING = 3,                                  // 输入端口连线转移
    SCT_ACT_WIRE_COPYING = 4,                                   // 输入端口连线复制
};

// 连线类型
enum WIRE_TYPE {
    WIRE_TYPE_DIRECT,
    WIRE_TYPE_BEZIER,
};
// 连线状态
namespace WIRE_STATE {
    enum {
        WIRE_STATE_OKAY = 0x01,                                 // 生命状态 - 正常实例/未初始化
        WIRE_STATE_HEAD = 0x02,                                 // 起始端口 - 已绑定/未绑定
        WIRE_STATE_TAIL = 0x04,                                 // 结束端口 - 已绑定/未绑定
        WIRE_STATE_HANG = 0x08,                                 // 端口状态 - 在端口上/末端悬空(端点悬空时视觉提示)
        WIRE_STATE_GOON = 0x10,                                 // 连接预测 - 可以连接/无法连接
        WIRE_STATE_HIGH = 0x20,                                 // 选中状态 - 已被选择/未被选择
        WIRE_STATE_SHOW = 0x40,                                 // 显示状态 - 已经显示/隐藏显示
    };
    typedef long Flags;
}


// 视图状态
namespace VIEW_STATE {
    enum {
        VIEW_S_CURSOR_MOVING = 0x01,                            // 鼠标移动状态 - 移动中/未移动
        VIEW_S_LMB_PRESSED = 0x02,                              // 鼠标左键状态 - 已按住/未按键
        VIEW_S_MMB_PRESSED = 0x04,                              // 鼠标中键状态 - 已按住/未按键
        VIEW_S_RMB_PRESSED = 0x08,                              // 鼠标右键状态 - 已按住/未按键
        VIEW_S_K_CTRL_PRESSED = 0x10,                           // 键盘Ctrl状态 - 已按住/未按键
        VIEW_S_K_SHIFT_PRESSED = 0x20,                          // 键盘Shift状态 - 已按住/未按键
        VIEW_S_K_ALT_PRESSED = 0x40,                            // 键盘Alt状态 - 已按住/未按键
        VIEW_S_SCENE_MOVING = 0x80,                             // 视图移动状态 - 移动中/未移动
        VIEW_S_S_ZOOM_IN = 0x100,                               // 视图放大状态 - 放大中/未放大
        VIEW_S_S_ZOOM_OUT = 0x200,                              // 视图缩小状态 - 缩小中/未缩小
        //            便于查询状态，不用于置位
        VIEW_S_MOUSE_PRESSED = VIEW_S_LMB_PRESSED | VIEW_S_MMB_PRESSED | VIEW_S_RMB_PRESSED,
        VIEW_S_KEY_PRESSED = VIEW_S_K_CTRL_PRESSED | VIEW_S_K_SHIFT_PRESSED | VIEW_S_K_ALT_PRESSED,
        VIEW_S_LMB_DRAGGING = VIEW_S_LMB_PRESSED | VIEW_S_CURSOR_MOVING,
        VIEW_S_MMB_DRAGGING = VIEW_S_MMB_PRESSED | VIEW_S_CURSOR_MOVING,
        VIEW_S_RMB_DRAGGING = VIEW_S_RMB_PRESSED | VIEW_S_CURSOR_MOVING,
        VIEW_S_WIRE_CUTTING = VIEW_S_K_SHIFT_PRESSED | VIEW_S_RMB_PRESSED | VIEW_S_CURSOR_MOVING,
    };
    typedef long Flags;
}
// 视图动作
enum VIEW_ACTION {
    VIEW_A_NOOP = 0,                                            // 无操作
    VIEW_A_WIRE_DRAGGING = SCT_ACT_WIRE_DRAGGING,               // 端口创建出连线
    VIEW_A_WIRE_BATCHING = SCT_ACT_WIRE_BATCHING,               // 输出端口批量创建连线
    VIEW_A_WIRE_SHIFTING = SCT_ACT_WIRE_SHIFTING,               // 输入端口连线转移
    VIEW_A_WIRE_COPYING = SCT_ACT_WIRE_COPYING,                 // 输入端口连线复制
    VIEW_A_WIRE_CUTTING,                                        // 画线删除连线
    VIEW_A_ITEM_SET_SELECTED = 6,                               // 调用 setSelected()
};

// 历史记录
namespace VIEW_HIST {
    enum {
        INIT_VIEW = 0x01,                                       // 初始化场景视图
        SELECT_ITEMS = 0x02,                                    // 选择条目
        DESELECT_ITEMS = 0x04,                                  // 取消选择
        MOVE_ITEMS = 0x08,                                      // 移动条目
        CREATE_ITEMS = 0x10,                                    // 创建条目
        DELETE_ITEMS = 0x20,                                    // 删除条目
        CUT_ITEMS = 0x40,                                       // 剪切条目
        PASTE_ITEMS = 0x80,                                     // 粘贴条目
        // 便于查询，不用于置位
        SEL_DESEL_ITEMS = SELECT_ITEMS | DESELECT_ITEMS,
    };
    typedef long Flags;
}

#endif //NODETILER_NODE_COMMON_H
