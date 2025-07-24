#include "mqtt_message_display.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "esp_log.h"

static const char *TAG = "MQTT_DISPLAY";

// 内部变量
static lv_obj_t *g_textarea = NULL;
static lv_obj_t *g_msg_count_label = NULL;
static lv_obj_t *g_state_label = NULL;
static char message_buffer[8192];
static uint32_t message_count = 0;
static bool auto_scroll_enabled = true;

// 获取当前时间字符串
static void get_time_string(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%H:%M:%S", tm_info);
}

// 初始化显示管理器
void mqtt_display_init(lv_obj_t *textarea_obj, lv_obj_t *msg_count_label, lv_obj_t *state_label) {
    if (!textarea_obj) {
        ESP_LOGE(TAG, "Textarea对象不能为空");
        return;
    }
    
    g_textarea = textarea_obj;
    g_msg_count_label = msg_count_label;
    g_state_label = state_label;
    
    // 清空缓冲区
    memset(message_buffer, 0, sizeof(message_buffer));
    message_count = 0;
    
    // 设置textarea样式
    lv_obj_add_state(g_textarea, LV_STATE_DISABLED);  // 设为只读
    lv_textarea_set_cursor_click_pos(g_textarea, false);
    
    // 设置外观
    lv_obj_set_style_text_font(g_textarea, &lv_font_montserrat_12, 0);
    lv_obj_set_style_bg_color(g_textarea, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(g_textarea, lv_color_hex(0x00FF00), 0);
    
    // 初始化显示
    lv_textarea_set_text(g_textarea, "");
    lv_textarea_set_placeholder_text(g_textarea, "等待MQTT消息...");
    
    // 更新计数和状态
    if (g_msg_count_label) {
        lv_label_set_text(g_msg_count_label, "0");
    }
    
    if (g_state_label) {
        lv_label_set_text(g_state_label, "Disconnected");
        lv_obj_set_style_text_color(g_state_label, lv_color_hex(0xFF0000), 0);
    }
    
    ESP_LOGI(TAG, "MQTT消息显示管理器初始化成功");
}

// 清理旧消息（保留最新的消息）
static void trim_old_messages(void) {
    char *lines[100];
    int line_count = 0;
    char *line_start = message_buffer;
    char *line_end;
    
    // 分割行
    while ((line_end = strchr(line_start, '\n')) != NULL && line_count < 100) {
        lines[line_count++] = line_start;
        line_start = line_end + 1;
    }
    
    // 如果超过40行，保留最新的40行
    if (line_count > 40) {
        int keep_lines = 40;
        int start_line = line_count - keep_lines;
        
        char new_buffer[8192] = {0};
        
        for (int i = start_line; i < line_count; i++) {
            char *next_line = (i < line_count - 1) ? lines[i + 1] - 1 : line_start;
            size_t line_len = next_line - lines[i];
            
            if (line_len > 0 && line_len < 400) {
                strncat(new_buffer, lines[i], line_len);
                strcat(new_buffer, "\n");
            }
        }
        
        strcpy(message_buffer, new_buffer);
        ESP_LOGI(TAG, "清理旧消息，保留%d行", keep_lines);
    }
}

// 添加MQTT消息
void mqtt_display_add_message(const char *topic, const char *message, int qos, bool retained) {
    if (!g_textarea || !topic || !message) {
        ESP_LOGE(TAG, "参数无效");
        return;
    }
    
    char time_str[16];
    char formatted_msg[400];
    
    get_time_string(time_str, sizeof(time_str));
    message_count++;
    
    // 格式化消息
    snprintf(formatted_msg, sizeof(formatted_msg),
             "[%lu] %s [Q%d%s] %s: %s\n",
             (unsigned long)message_count,
             time_str,
             qos,
             retained ? "R" : "",
             topic,
             message);
    
    // 检查缓冲区空间
    size_t current_len = strlen(message_buffer);
    size_t new_msg_len = strlen(formatted_msg);
    
    if (current_len + new_msg_len >= sizeof(message_buffer) - 100) {
        trim_old_messages();
    }
    
    // 添加消息
    strcat(message_buffer, formatted_msg);
    
    // 更新显示
    lv_textarea_set_text(g_textarea, message_buffer);
    
    // 更新消息计数
    if (g_msg_count_label) {
        char count_str[16];
        snprintf(count_str, sizeof(count_str), "%lu", (unsigned long)message_count);
        lv_label_set_text(g_msg_count_label, count_str);
    }
    
    // 自动滚动
    if (auto_scroll_enabled) {
        mqtt_display_scroll_to_bottom();
    }
    
    ESP_LOGI(TAG, "添加消息: %s", topic);
}

// 添加系统消息
void mqtt_display_add_system_msg(const char *message, const char *level) {
    if (!g_textarea || !message || !level) {
        ESP_LOGE(TAG, "参数无效");
        return;
    }
    
    char time_str[16];
    char formatted_msg[256];
    
    get_time_string(time_str, sizeof(time_str));
    
    snprintf(formatted_msg, sizeof(formatted_msg),
             "[SYS] %s [%s] %s\n",
             time_str,
             level,
             message);
    
    // 检查缓冲区空间
    size_t current_len = strlen(message_buffer);
    size_t new_msg_len = strlen(formatted_msg);
    
    if (current_len + new_msg_len >= sizeof(message_buffer) - 100) {
        trim_old_messages();
    }
    
    // 添加系统消息
    strcat(message_buffer, formatted_msg);
    
    // 更新显示
    lv_textarea_set_text(g_textarea, message_buffer);
    
    // 自动滚动
    if (auto_scroll_enabled) {
        mqtt_display_scroll_to_bottom();
    }
    
    ESP_LOGI(TAG, "添加系统消息: [%s] %s", level, message);
}

// 清空消息
void mqtt_display_clear(void) {
    if (!g_textarea) {
        ESP_LOGE(TAG, "Textarea未初始化");
        return;
    }
    
    memset(message_buffer, 0, sizeof(message_buffer));
    message_count = 0;
    
    lv_textarea_set_text(g_textarea, "");
    
    if (g_msg_count_label) {
        lv_label_set_text(g_msg_count_label, "0");
    }
    
    ESP_LOGI(TAG, "clear all messages");
}

// 更新状态
void mqtt_display_update_state(const char *state) {
    if (!state || !g_state_label) {
        return;
    }
    
    lv_label_set_text(g_state_label, state);
    
    // 根据状态设置颜色
    if (strcmp(state, "Connected") == 0) {
        lv_obj_set_style_text_color(g_state_label, lv_color_hex(0x00FF00), 0);
    } else if (strcmp(state, "Connecting") == 0) {
        lv_obj_set_style_text_color(g_state_label, lv_color_hex(0xFFFF00), 0);
    } else {
        lv_obj_set_style_text_color(g_state_label, lv_color_hex(0xFF0000), 0);
    }
}

// 获取消息数量
uint32_t mqtt_display_get_msg_count(void) {
    return message_count;
}

// 滚动到底部
void mqtt_display_scroll_to_bottom(void) {
    if (g_textarea) {
        lv_obj_scroll_to_y(g_textarea, LV_COORD_MAX, LV_ANIM_ON);
    }
}

// 滚动到顶部
void mqtt_display_scroll_to_top(void) {
    if (g_textarea) {
        lv_obj_scroll_to_y(g_textarea, 0, LV_ANIM_ON);
    }
}