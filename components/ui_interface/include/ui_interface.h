#ifndef UI_INTERFACE_H
#define UI_INTERFACE_H

#include <stdbool.h>

// UI订阅MQTT主题
bool ui_mqtt_subscribe(const char* topic, int qos);

// UI取消订阅MQTT主题
bool ui_mqtt_unsubscribe(const char* topic);

// UI发布MQTT消息
bool ui_mqtt_publish(const char* topic, const char* payload, int qos);

// UI连接MQTT服务器
bool ui_mqtt_connect(char *broker_url,  ///< MQTT代理服务器URL
      int port,              ///< 端口号
      char *client_id,      ///< 客户端ID
      char *username,       ///< 用户名（如果需要）
      char *password) ;     ///< 密码（如果需要）

// UI断开MQTT连接
bool ui_mqtt_disconnect(void);

// UI配置WiFi网络
bool ui_wifi_config(const char* ssid, const char* password);

// UI获取MQTT连接状态
bool ui_get_mqtt_status(void);

// UI获取WiFi连接状态
bool ui_get_wifi_status(void);
#endif
