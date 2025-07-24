#include "task_communication.h"
#include "esp_log.h"

static const char* TAG = "TASK_COMM";

/**
 * @brief UI任务到主逻辑任务的消息队列句柄
 */
QueueHandle_t ui_to_logic_queue = NULL;

/**
 * @brief 主逻辑任务到UI任务的消息队列句柄
 */
QueueHandle_t logic_to_ui_queue = NULL;

/**
 * @brief 初始化任务通信模块
 * @note 创建UI到主逻辑任务和主逻辑到UI的消息队列
 */
void task_communication_init(void) {
  ESP_LOGI(TAG, "Start Init Task Communication");

  // 创建UI到主逻辑任务的信息队列
  ui_to_logic_queue = xQueueCreate(10, sizeof(ui_to_logic_msg_t));
  if (ui_to_logic_queue == NULL) {
    ESP_LOGE(TAG, "创建ui_to_logic_queue失败-内存不足或参数错误");
  }
  ESP_LOGI(TAG, "ui_to_logic_queue创建成功");

  // 创建主逻辑到UI任务的消息队列
  logic_to_ui_queue = xQueueCreate(10, sizeof(logic_to_ui_msg_t));
  if (logic_to_ui_queue == NULL) {
    ESP_LOGE(TAG, "创建logic_to_ui_queue失败 - 内存不足或参数错误");
    return;
  }
  ESP_LOGI(TAG, "logic_to_ui_queue创建成功");

  ESP_LOGI(TAG, "任务间通信模块初始化完成");
}

/**
 * @brief 发送UI到主逻辑任务的消息
 * @param msg 要发送的消息指针
 * @return 成功返回true，失败返回false
 */
bool send_ui_message(ui_to_logic_msg_t* msg) {
  // 参数有效性检查
  if (ui_to_logic_queue == NULL || msg == NULL) {
    ESP_LOGE(TAG, "消息队列或消息指针无效");
    return false;
  }
  // 尝试发送消息到队列
  BaseType_t result = xQueueSend(ui_to_logic_queue, msg, pdMS_TO_TICKS(100));
  if (result == pdPASS) {
    ESP_LOGI(TAG, "UI消息发送成功");
    return true;
  } else {
    ESP_LOGE(TAG, "UI消息发送失败");
    return false;
  }
}

/**
 * @brief 发送逻辑到UI的消息
 * @param msg 要发送的消息指针
 * @return 成功返回true，失败返回false
 */
bool send_logic_message(logic_to_ui_msg_t* msg) {
  // 参数有效性检查
  if (logic_to_ui_queue == NULL || msg == NULL) {
    ESP_LOGE(TAG, "消息队列或消息指针无效");
    return false;
  }
  // 尝试发送消息到队列
  BaseType_t result = xQueueSend(logic_to_ui_queue, msg, pdMS_TO_TICKS(100));
  if (result == pdPASS) {
    ESP_LOGI(TAG, "逻辑消息发送成功");
    return true;
  } else {
    ESP_LOGE(TAG, "逻辑消息发送失败");
    return false;
  }
}