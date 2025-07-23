#include "mqtt_tool.h"
#include "esp_log.h"

static const char* TAG = "mqtt_tool";  ///< 日志输出TAG

/**
 * @brief MQTT事件处理函数
 * @param handler_args 用户参数
 * @param base 事件基类型
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
static void mqtt_event_handler(void* handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void* event_data) {
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;

  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
      esp_mqtt_client_subscribe(client, "/led/control", 1);  // 连接后订阅主题
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "MQTT_EVENT_DATA");
      ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
      ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
      // 判断是否为LED控制主题
      if (strncmp(event->topic, "/led/control", event->topic_len) == 0) {
        // 开灯指令
        if (strncmp(event->data, "on", event->data_len) == 0) {
          // gpio_set_level(LED_GPIO_PIN, 1);
          ESP_LOGI(TAG, "LED ON");
          // 关灯指令
        } else if (strncmp(event->data, "off", event->data_len) == 0) {
          // gpio_set_level(LED_GPIO_PIN, 0);
          ESP_LOGI(TAG, "LED OFF");
        }
      }
      break;
    default:
      break;
  }
}

uint8_t mqtt_tool_init(void) {
  // 配置MQTT服务器地址
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = "mqtt://mqtt.ernestliu.xyz",
  };

  esp_mqtt_client_handle_t client =
      esp_mqtt_client_init(&mqtt_cfg);  // 初始化MQTT客户端
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 NULL);  // 注册MQTT事件回调
  esp_mqtt_client_start(client);
  return 0;  // Return 0 on success
}

uint8_t mqtt_tool_deinit(void) {
  return 0;  // Return 0 on success
}

uint8_t mqtt_tool_connect(void) {
  return 0;
}

uint8_t mqtt_tool_disconnect(void) {
  return 0;
}

uint8_t mqtt_tool_publish(const char* topic, const char* message, int qos) {
  return 0;
}

uint8_t mqtt_tool_subscribe(const char* topic, int qos) {
  return 0;
}

uint8_t mqtt_tool_unsubscribe(const char* topic) {
  return 0;
}