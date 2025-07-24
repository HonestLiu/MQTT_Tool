#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "lvgl.h"

#include "task_communication.h"
#include "ui_interface.h"

#include "mqtt_tool.h"
#include "ui.h"

// 日志标签
static const char* TAG = "MAIN_UPDATED";

/**
 * @brief GUI任务函数
 * 该任务负责处理用户界面相关的操作和事件。
 * 它会监听UI消息队列，并根据收到的消息更新界面状态。
 * @param pvParameters 任务参数（未使用）
 */
void gui_task(void* pvParameters) {
  logic_to_ui_msg_t rec_msg;  // 用来接收来自主逻辑任务的消息
  ESP_LOGI(TAG, "GUI Task started");
  while (1) {
    // 等待接收消息
    if (xQueueReceive(logic_to_ui_queue, &rec_msg, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      // 处理接收到的消息
      switch (rec_msg.type) {
        case LOGIC_MSG_MQTT_STATUS:  // MQTT连接状态消息
          break;

        case LOGIC_MSG_MQTT_RECEIVED:  // 接收到的MQTT消息
          break;
        case LOGIC_MSG_MQTT_RESULT:  // MQTT操作结果消息
          break;
        case LOGIC_MSG_WIFI_STATUS:  // WiFi连接状态消息
          break;
        default:
          ESP_LOGW(TAG, "Unknown message type");
          break;
      }
    }
  }

  lv_timer_handler();            // LVGL定时器处理
  vTaskDelay(pdMS_TO_TICKS(5));  // 延时5毫秒，
}

static mqtt_tool_handle_t mqtt_tool = {0};

/**
 * @brief 主逻辑任务函数
 * 该任务负责处理主逻辑相关的操作和事件。
 * 它会定期发送消息到UI任务队列，以更新用户界面状态。
 * @param pvParmeters 任务参数（未使用）
 */
void main_logic_task(void* pvParmeters) {
  ui_to_logic_msg_t received_msg;  // 用来接收来自UI任务的消息
  uint8_t ret;
  ESP_LOGI(TAG, "Main Logic Task started");

  while (1) {
    if (xQueueReceive(ui_to_logic_queue, &received_msg, pdMS_TO_TICKS(10)) ==
        pdTRUE) {
      // 处理接收到的UI消息
      switch (received_msg.type) {
        case UI_MSG_MQTT_CONNECT:  // MQTT连接请求
          ESP_LOGI(TAG, "Received MQTT connect request");
          
          // 确保 MQTT 工具未初始化，如果已初始化则先清理
          if (mqtt_tool.initialized) {
            mqtt_tool_deinit(&mqtt_tool);
          }
          
          // 重置结构体
          memset(&mqtt_tool, 0, sizeof(mqtt_tool_handle_t));
          
          // 构建完整的 broker URI（确保有 mqtt:// 前缀）
          char full_broker_uri[256];  // 增加缓冲区大小以避免截断
          const char* broker_url = received_msg.data.mqtt_connect_data.broker_url;
          if (strncmp(broker_url, "mqtt://", 7) != 0 && strncmp(broker_url, "mqtts://", 8) != 0) {
            // 检查长度以避免截断
            if (strlen(broker_url) + 7 < sizeof(full_broker_uri)) {
              snprintf(full_broker_uri, sizeof(full_broker_uri), "mqtt://%s", broker_url);
            } else {
              ESP_LOGE(TAG, "Broker URL too long");
              break;
            }
          } else {
            strncpy(full_broker_uri, broker_url, sizeof(full_broker_uri) - 1);
            full_broker_uri[sizeof(full_broker_uri) - 1] = '\0';
          }
          
          // 设置 MQTT 配置
          mqtt_tool_set_broker_uri(&mqtt_tool, full_broker_uri);
          mqtt_tool_set_client_id(&mqtt_tool, received_msg.data.mqtt_connect_data.client_id);
          mqtt_tool_set_keepalive(&mqtt_tool, 60);
          
          // 设置用户名和密码（如果提供）
          if (strlen(received_msg.data.mqtt_connect_data.username) > 0) {
            mqtt_tool_set_credentials(&mqtt_tool, 
                                    received_msg.data.mqtt_connect_data.username,
                                    received_msg.data.mqtt_connect_data.password);
          }
          
          // 初始化 MQTT 工具（只初始化一次）
          ret = mqtt_tool_init(&mqtt_tool);
          if (ret != MQTT_TOOL_SUCCESS) {
            ESP_LOGE(TAG, "MQTT tool initialization failed");
            break;
          }
          
          // 连接到 MQTT 代理服务器
          ret = mqtt_tool_connect(&mqtt_tool);

          break;
        case UI_MSG_MQTT_SUBSCRIBE:  // MQTT订阅请求
          mqtt_tool_subscribe(
              &mqtt_tool, 
              received_msg.data.subscribe_data.topic, 
              received_msg.data.subscribe_data.qos);
          // 处理订阅逻辑
          ESP_LOGI(TAG, "Subscribed to topic: %s", received_msg.data.subscribe_data.topic);
          break;

        case UI_MSG_MQTT_PUBLISH:  // MQTT发布请求
          ESP_LOGI(TAG, "Received MQTT publish request");
          // 处理发布逻辑
          break;

        case UI_MSG_WIFI_CONFIG:  // WiFi配置请求
          ESP_LOGI(TAG, "Received WiFi config request");
          // 处理WiFi配置逻辑
          break;

        default:
          ESP_LOGW(TAG, "Unknown message type from UI");
          break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));  // 延时10毫秒
  }
}



