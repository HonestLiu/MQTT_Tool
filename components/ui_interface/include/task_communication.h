#ifndef TASK_COMMUNICATION_H
#define TASK_COMMUNICATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "stdbool.h"

/**
 * @brief UI任务到主逻辑任务的消息队列句柄
 */
extern QueueHandle_t ui_to_logic_queue;

/**
 * @brief 主逻辑任务到UI任务的消息队列句柄
 */
extern QueueHandle_t logic_to_ui_queue;

/**
 * @brief UI任务到主逻辑任务的消息类型枚举
 *
 * 定义了用户界面可以向主逻辑任务发送的所有命令类型。
 * 这些命令通常由用户的界面操作触发（如按钮点击、文本输入等）。
 */
typedef enum {
  UI_MSG_MQTT_SUBSCRIBE,    ///< 订阅MQTT主题请求
  UI_MSG_MQTT_UNSUBSCRIBE,  ///< 取消订阅MQTT主题请求
  UI_MSG_MQTT_PUBLISH,      ///< 发布MQTT消息请求
  UI_MSG_MQTT_CONNECT,      ///< 连接MQTT服务器请求
  UI_MSG_MQTT_DISCONNECT,   ///< 断开MQTT连接请求
  UI_MSG_WIFI_CONFIG,       ///< WiFi网络配置请求
} ui_message_type_t;

/**
 * @brief 主逻辑任务到UI任务的消息类型枚举
 *
 * 定义了主逻辑任务可以向UI任务发送的所有消息类型。
 * 这些消息用于更新界面显示状态、反馈操作结果等。
 */
typedef enum {
  LOGIC_MSG_MQTT_STATUS,    ///< MQTT连接状态变化通知
  LOGIC_MSG_MQTT_RECEIVED,  ///< 收到MQTT消息通知
  LOGIC_MSG_MQTT_RESULT,    ///< MQTT操作结果反馈
  LOGIC_MSG_WIFI_STATUS,    ///< WiFi连接状态变化通知
} logic_message_type_t;

/**
 * @brief UI任务到主逻辑任务的消息结构体
 *
 * @note 包含消息类型和相关数据的联合体，用于封装用户界面的命令请求。
 * - 使用联合体可以节省内存空间，因为同一时间只会使用其中一种数据类型。
 */
typedef struct {
  ui_message_type_t type;  ///< 消息类型
  union {
    struct {
      char topic[64];  ///< MQTT主题（如果适用）
      int qos;         ///< QoS等级（如果适用）
    } subscribe_data;

    struct {
      char topic[64];     ///< MQTT主题（如果适用）
      char payload[256];  ///< MQTT消息内容（如果适用）
      int qos;            ///< QoS等级（如果适用）
    } publish_data;

    struct {
      char ssid[32];      ///< WiFi SSID
      char password[64];  ///< WiFi密码
    } wifi_config_data;

    struct {
      char broker_url[128];  ///< MQTT代理服务器URL
      int port;              ///< 端口号
      char client_id[64];    ///< 客户端ID
      char username[64];     ///< 用户名（如果需要）
      char password[64];     ///< 密码（如果需要）
    } mqtt_connect_data;

  } data;

} ui_to_logic_msg_t;

typedef struct {
  logic_message_type_t type;  ///< 消息类型
  union {
    /** @brief MQTT连接状态数据 */
    struct {
      bool connected;        ///< MQTT连接状态（true=已连接, false=未连接）
      char broker_url[128];  ///< MQTT代理服务器URL
    } mqtt_status;

    /** @brief 接收到的MQTT消息数据 */
    struct {
      char topic[64];     ///< 消息主题
      char payload[256];  ///< 消息内容
      int qos;            ///< 服务质量等级
    } mqtt_received;

    /** @brief MQTT操作结果数据 */
    struct {
      ui_message_type_t request_type;  ///< 对应的原始请求类型
      bool success;                    ///< 操作是否成功
      char error_msg[128];             ///< 错误信息（失败时使用）
    } mqtt_result;

    /** @brief WiFi连接状态数据 */
    struct {
      bool connected;  ///< WiFi连接状态
      char ip[16];     ///< 分配的IP地址字符串（如"192.168.1.100"）
    } wifi_status;
  } data;  ///< 消息数据联合体

} logic_to_ui_msg_t;

// 初始化任务间通信模块
void task_communication_init(void);

// 发送UI消息到主逻辑任务
bool send_ui_message(ui_to_logic_msg_t* msg);

// 发送主逻辑任务消息到UI任务
bool send_logic_message(logic_to_ui_msg_t* msg);

#endif
