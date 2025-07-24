#ifndef MQTT_MESSAGE_DISPLAY_H
#define MQTT_MESSAGE_DISPLAY_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

// 初始化消息显示管理器（传入现有的UI组件）
void mqtt_display_init(lv_obj_t *textarea_obj, lv_obj_t *msg_count_label, lv_obj_t *state_label);

// 添加MQTT消息
void mqtt_display_add_message(const char *topic, const char *message, int qos, bool retained);

// 添加系统消息
void mqtt_display_add_system_msg(const char *message, const char *level);

// 清空消息
void mqtt_display_clear(void);

// 更新状态
void mqtt_display_update_state(const char *state);

// 获取消息数量
uint32_t mqtt_display_get_msg_count(void);

// 滚动控制
void mqtt_display_scroll_to_bottom(void);
void mqtt_display_scroll_to_top(void);

#endif