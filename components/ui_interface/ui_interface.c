#include "ui_interface.h"
#include <string.h>
#include "esp_log.h"
#include "task_communication.h"

static const char* TAG = "UI_INTERFACE";

static bool cached_mqtt_status = false;    ///< 缓存的MQTT连接状态
static bool cached_wifi_status = false;    ///< 缓存的WiFi连接状态

/**
 * @brief UI订阅MQTT主题
 * @param topic 订阅的MQTT主题
 * @param qos QoS等级
 * @return 成功返回true，失败返回false
 */
bool ui_mqtt_subscribe(const char* topic, int qos) {
  // 参数有效性检查
  if (topic == NULL) {
    ESP_LOGE(TAG, "Invalid topic parameter");
    return false;
  }
  // 检查主题字符串长度，避免缓冲区溢出
  if (strlen(topic) >=
      sizeof(((ui_to_logic_msg_t*)0)->data.subscribe_data.topic)) {
    ESP_LOGE(TAG, "订阅失败 - 主题字符串过长: %s", topic);
    return false;
  }
  // 创建消息结构体并填充数据
  ui_to_logic_msg_t msg = {
      .type = UI_MSG_MQTT_SUBSCRIBE,
  };

  // 复制主题字符串到消息结构体中
  strncpy(msg.data.subscribe_data.topic, topic,
          sizeof(msg.data.subscribe_data.topic) - 1);
  msg.data.subscribe_data.topic[sizeof(msg.data.subscribe_data.topic) - 1] =
      '\0';  // 确保字符串以null结尾
  msg.data.subscribe_data.qos = qos;

  // 发送消息到UI到主逻辑任务的队列
  bool result = send_ui_message(&msg);

  // 记录操作日志
  ESP_LOGI(TAG, "UI订阅MQTT主题: %s, QoS: %d, 结果: %s",
           msg.data.subscribe_data.topic, msg.data.subscribe_data.qos,
           result ? "成功" : "失败");
  return result;
}

/**
 * @brief UI取消订阅MQTT主题
 * @param topic 要取消订阅的MQTT主题
 * @return 成功返回true，失败返回false
 */
bool ui_mqtt_unsubscribe(const char* topic) {
    if (topic == NULL) {
        ESP_LOGE(TAG, "Invalid topic parameter");
        return false;
    }
    if (strlen(topic) >= sizeof(((ui_to_logic_msg_t*)0)->data.subscribe_data.topic)) {
        ESP_LOGE(TAG, "Unsubscribe failed - topic string too long: %s", topic);
        return false;
    }

    // 创建消息结构体并填充数据
    ui_to_logic_msg_t msg = {
        .type = UI_MSG_MQTT_UNSUBSCRIBE,
    };

    // 复制主题字符串到消息结构体中
    strncpy(msg.data.subscribe_data.topic, topic,
            sizeof(msg.data.subscribe_data.topic) - 1);
    msg.data.subscribe_data.topic[sizeof(msg.data.subscribe_data.topic) - 1] =
        '\0';  // 确保字符串以null结尾

    // 发送消息到UI到主逻辑任务的队列
    bool result = send_ui_message(&msg);

    // 记录操作日志
    ESP_LOGI(TAG, "UI取消订阅MQTT主题: %s, 结果: %s",
             msg.data.subscribe_data.topic,
             result ? "成功" : "失败");
    return result;
}

/**
 * @brief UI发布MQTT消息
 * @param topic 发布的MQTT主题
 * @param payload 消息内容
 * @param qos QoS等级
 * @return 成功返回true，失败返回false
 */
bool ui_mqtt_publish(const char* topic, const char* payload, int qos) {
    // 参数有效性检查
    if (topic == NULL || payload == NULL) {
        ESP_LOGE(TAG, "发布失败 - 主题或消息内容为NULL (topic=%p, payload=%p)", 
                 topic, payload);
        return false;
    }

    // 检查主题字符串长度
    if (strlen(topic) >= sizeof(((ui_to_logic_msg_t*)0)->data.publish_data.topic)) {
        ESP_LOGE(TAG, "发布失败 - 主题字符串过长: %s", topic);
        return false;
    }

    // 检查消息内容长度
    if (strlen(payload) >= sizeof(((ui_to_logic_msg_t*)0)->data.publish_data.payload)) {
        ESP_LOGE(TAG, "发布失败 - 消息内容过长，长度: %zu", strlen(payload));
        return false;
    }

    // 构造发送给主逻辑任务的消息
    ui_to_logic_msg_t msg = {
        .type = UI_MSG_MQTT_PUBLISH, // 设置消息类型为发布
    };

    // 安全地复制主题字符串
    strncpy(msg.data.publish_data.topic, topic,
            sizeof(msg.data.publish_data.topic) - 1);
    strncpy(msg.data.publish_data.payload, payload,
            sizeof(msg.data.publish_data.payload) - 1);
    msg.data.publish_data.topic[sizeof(msg.data.publish_data.topic) - 1] = '\0'; // 确保字符串以null结尾
    
    // 设置服务质量等级
    msg.data.publish_data.qos = qos;

    // 发送消息到主逻辑任务
    bool result = send_ui_message(&msg);

    // 记录操作日志
    ESP_LOGI(TAG, "UI发布MQTT消息: 主题=%s, QoS=%d, 消息内容=%s, 结果=%s",
             msg.data.publish_data.topic, msg.data.publish_data.qos,
             msg.data.publish_data.payload, result ? "成功" : "失败");
    return result;
}

/**
 * @brief UI连接MQTT服务器
 * @return 成功返回true，失败返回false
 */
bool ui_mqtt_connect(char *broker_url,  ///< MQTT代理服务器URL
      int port,              ///< 端口号
      char *client_id,      ///< 客户端ID
      char *username,       ///< 用户名（如果需要）
      char *password) {     ///< 密码（如果需要）

    if (!broker_url || !client_id) {
        ESP_LOGE(TAG, "连接MQTT服务器失败 - URL或客户端ID为NULL (broker_url=%p, client_id=%p)", 
                 broker_url, client_id);
        return false;
    }
    
    ui_to_logic_msg_t msg;
    msg.type = UI_MSG_MQTT_CONNECT; // 设置消息类型为连接请求

    // 安全地复制字符串数据
    strncpy(msg.data.mqtt_connect_data.broker_url, broker_url, sizeof(msg.data.mqtt_connect_data.broker_url) - 1);
    strncpy(msg.data.mqtt_connect_data.client_id, client_id, sizeof(msg.data.mqtt_connect_data.client_id) - 1);
    
    // 可选参数处理
    if (username) {
        strncpy(msg.data.mqtt_connect_data.username, username, sizeof(msg.data.mqtt_connect_data.username) - 1);
    }
    if (password) {
        strncpy(msg.data.mqtt_connect_data.password, password, sizeof(msg.data.mqtt_connect_data.password) - 1);
    }
    
    msg.data.mqtt_connect_data.port = port;

    // 发送连接请求到主逻辑任务
    bool result = send_ui_message(&msg);

    ESP_LOGI(TAG, "UI连接MQTT服务器 %s:%d, 客户端ID: %s, 结果: %s", 
             broker_url, port, client_id, result ? "成功" : "失败");

    return result;
}

/**
 * @brief UI断开MQTT连接
 * @return 成功返回true，失败返回false
 */
bool ui_mqtt_disconnect(void) {
    // 创建断开连接请求消息
    ui_to_logic_msg_t msg = {
        .type = UI_MSG_MQTT_DISCONNECT, // 设置消息类型为断开连接请求
    };

    // 发送断开连接请求到主逻辑任务
    bool result = send_ui_message(&msg);

    // 记录操作日志
    ESP_LOGI(TAG, "UI断开MQTT连接, 结果: %s", result ? "成功" : "失败");
    return result;
}

/**
 * @brief UI配置WiFi网络
 * @param ssid WiFi SSID
 * @param password WiFi密码
 * @return 成功返回true，失败返回false
 */
bool ui_wifi_config(const char* ssid, const char* password) {
        // 参数有效性检查
    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "WiFi配置失败 - SSID或密码为NULL (ssid=%p, password=%p)", 
                 ssid, password);
        return false;
    }

    // 检查SSID字符串长度
    if (strlen(ssid) >= sizeof(((ui_to_logic_msg_t*)0)->data.wifi_config_data.ssid)) {
        ESP_LOGE(TAG, "WiFi配置失败 - SSID过长: %s", ssid);
        return false;
    }

    // 检查密码字符串长度
    if (strlen(password) >= sizeof(((ui_to_logic_msg_t*)0)->data.wifi_config_data.password)) {
        ESP_LOGE(TAG, "WiFi配置失败 - 密码过长，长度: %zu", strlen(password));
        return false;
    }

    // 构造发送给主逻辑任务的消息
    ui_to_logic_msg_t msg = {
        .type = UI_MSG_WIFI_CONFIG  // 设置消息类型为WiFi配置请求
    };

    // 安全地复制SSID
    strncpy(msg.data.wifi_config_data.ssid, ssid, 
            sizeof(msg.data.wifi_config_data.ssid) - 1);
    msg.data.wifi_config_data.ssid[sizeof(msg.data.wifi_config_data.ssid) - 1] = '\0';

    // 安全地复制密码
    strncpy(msg.data.wifi_config_data.password, password, 
            sizeof(msg.data.wifi_config_data.password) - 1);
    msg.data.wifi_config_data.password[sizeof(msg.data.wifi_config_data.password) - 1] = '\0';

    // 发送消息到主逻辑任务
    bool result = send_ui_message(&msg);
    
    // 记录操作日志（密码用*号遮蔽保护隐私）
    ESP_LOGI(TAG, "WiFi配置请求 - SSID: \"%s\", 密码: %s, 结果: %s",
             ssid, "******", result ? "已发送" : "发送失败");

    return result;
}

/**
 * @brief UI获取MQTT连接状态
 * @return true表示已连接，false表示未连接
 */
bool ui_get_mqtt_status(void) {
    ESP_LOGD(TAG,"MQTT连接状态: %s", cached_mqtt_status ? "已连接" : "未连接");
    return cached_mqtt_status;
}

/**
 * @brief UI获取WiFi连接状态
 * @return true表示已连接，false表示未连接
 */
bool ui_get_wifi_status(void) {
    ESP_LOGD(TAG,"WiFi连接状态: %s", cached_wifi_status ? "已连接" : "未连接");
    return cached_wifi_status;
}



// ============================================================================
// 内部状态更新函数（由GUI任务调用）
// ============================================================================

/**
 * @brief 更新缓存的MQTT连接状态
 * 
 * 该函数由GUI任务在收到LOGIC_MSG_MQTT_STATUS消息时调用，
 * 用于更新本模块缓存的MQTT连接状态。
 * 
 * @param connected 新的MQTT连接状态
 * 
 * @note 该函数仅供内部使用，不在头文件中声明
 */
void ui_update_mqtt_status(bool connected)
{
    if (cached_mqtt_status != connected) {
        ESP_LOGI(TAG, "MQTT状态更新: %s -> %s", 
                 cached_mqtt_status ? "已连接" : "未连接",
                 connected ? "已连接" : "未连接");
        cached_mqtt_status = connected;
    }
}

/**
 * @brief 更新缓存的WiFi连接状态
 * 
 * 该函数由GUI任务在收到LOGIC_MSG_WIFI_STATUS消息时调用，
 * 用于更新本模块缓存的WiFi连接状态。
 * 
 * @param connected 新的WiFi连接状态
 * 
 * @note 该函数仅供内部使用，不在头文件中声明
 */
void ui_update_wifi_status(bool connected)
{
    if (cached_wifi_status != connected) {
        ESP_LOGI(TAG, "WiFi状态更新: %s -> %s", 
                 cached_wifi_status ? "已连接" : "未连接",
                 connected ? "已连接" : "未连接");
        cached_wifi_status = connected;
    }
}