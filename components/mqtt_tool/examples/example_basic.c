/**
 * @file example_basic.c
 * @brief MQTT工具库基本使用示例
 * 
 * 演示如何使用MQTT工具库进行基本的连接、发布和订阅操作
 */

#include "mqtt_tool.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "mqtt_example";

/**
 * @brief 基本MQTT使用示例
 */
void basic_mqtt_example(void)
{
    ESP_LOGI(TAG, "Starting basic MQTT example");
    
    // 1. 初始化MQTT工具
    uint8_t result = mqtt_tool_init();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT tool init failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "MQTT tool initialized successfully");

    // 2. 连接到MQTT代理
    result = mqtt_tool_connect();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT connect failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "Connected to MQTT broker");

    // 3. 订阅主题
    result = mqtt_tool_subscribe("esp32/test", 1);
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT subscribe failed: %d", result);
    } else {
        ESP_LOGI(TAG, "Subscribed to topic: esp32/test");
    }

    // 4. 发布消息
    for (int i = 0; i < 5; i++) {
        char message[64];
        snprintf(message, sizeof(message), "Hello MQTT! Message #%d", i + 1);
        
        result = mqtt_tool_publish("esp32/test", message, 1);
        if (result != MQTT_TOOL_SUCCESS) {
            ESP_LOGE(TAG, "MQTT publish failed: %d", result);
        } else {
            ESP_LOGI(TAG, "Published: %s", message);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // 等待2秒
    }

    ESP_LOGI(TAG, "Basic MQTT example completed");
}

/**
 * @brief 高级配置示例
 */
void advanced_mqtt_example(void)
{
    ESP_LOGI(TAG, "Starting advanced MQTT example");
    
    // 1. 配置自定义代理服务器
    uint8_t result = mqtt_tool_set_broker_uri("mqtt://broker.hivemq.com:1883");
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "Failed to set broker URI: %d", result);
        return;
    }
    
    // 2. 设置客户端ID
    result = mqtt_tool_set_client_id("esp32_advanced_example");
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "Failed to set client ID: %d", result);
        return;
    }
    
    // 3. 设置认证信息（如果需要）
    // mqtt_tool_set_credentials("username", "password");
    
    // 4. 初始化和连接
    result = mqtt_tool_init();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT tool init failed: %d", result);
        return;
    }
    
    result = mqtt_tool_connect();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT connect failed: %d", result);
        return;
    }
    
    ESP_LOGI(TAG, "Advanced MQTT example setup completed");
}

/**
 * @brief 连接状态监控示例
 */
void connection_monitor_task(void *pvParameters)
{
    mqtt_tool_state_t last_state = MQTT_TOOL_STATE_DISCONNECTED;
    
    while (1) {
        mqtt_tool_state_t current_state = mqtt_tool_get_state();
        
        if (current_state != last_state) {
            switch (current_state) {
                case MQTT_TOOL_STATE_CONNECTED:
                    ESP_LOGI(TAG, "✅ MQTT连接已建立");
                    break;
                    
                case MQTT_TOOL_STATE_DISCONNECTED:
                    ESP_LOGW(TAG, "❌ MQTT连接已断开");
                    break;
                    
                case MQTT_TOOL_STATE_CONNECTING:
                    ESP_LOGI(TAG, "🔄 正在连接MQTT...");
                    break;
            }
            last_state = current_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 应用程序入口点
 */
void app_main(void)
{
    ESP_LOGI(TAG, "MQTT Tool Example Starting...");
    
    // 等待WiFi连接建立
    // wifi_init_and_connect(); // 您需要实现WiFi连接代码
    
    // 创建连接监控任务
    xTaskCreate(connection_monitor_task, "mqtt_monitor", 2048, NULL, 5, NULL);
    
    // 运行基本示例
    basic_mqtt_example();
    
    // 可选：运行高级示例
    // advanced_mqtt_example();
}
