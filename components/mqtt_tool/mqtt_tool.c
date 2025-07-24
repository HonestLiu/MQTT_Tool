/**
 * @file mqtt_tool.c
 * @brief ESP32 MQTT工具库实现文件
 * 
 * 这个文件实现了MQTT工具库的所有功能，包括连接管理、消息发布订阅等。
 * 基于ESP-IDF的mqtt_client组件，提供了更高级的封装和错误处理。
 * 
 * @author HonestLiu
 * @date 2025-07-23
 * @version 1.0
 */

#include "mqtt_tool.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "task_communication.h"

/** @brief 日志标签 */
static const char *TAG = "mqtt_tool";

// 逻辑到UI的消息队列
extern QueueHandle_t logic_to_ui_queue;

/**
 * @brief 线程安全地设置MQTT连接状态
 * 
 * 使用互斥锁保护状态变量，确保多线程环境下的安全性
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] new_state 新的连接状态
 */
static void mqtt_tool_set_state(mqtt_tool_handle_t* handle, mqtt_tool_state_t new_state)
{
    if (handle->state_mutex != NULL) {
        xSemaphoreTake(handle->state_mutex, portMAX_DELAY);
        handle->state = new_state;
        xSemaphoreGive(handle->state_mutex);
    } else {
        handle->state = new_state;
    }
}

/**
 * @brief 线程安全地获取当前MQTT连接状态
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 当前的MQTT连接状态
 */
mqtt_tool_state_t mqtt_tool_get_state(mqtt_tool_handle_t* handle)
{
    mqtt_tool_state_t current_state;
    if (handle->state_mutex != NULL) {
        xSemaphoreTake(handle->state_mutex, portMAX_DELAY);
        current_state = handle->state;
        xSemaphoreGive(handle->state_mutex);
    } else {
        current_state = handle->state;
    }
    return current_state;
}

/**
 * @brief MQTT事件处理回调函数
 * 
 * 处理来自ESP-IDF MQTT客户端的各种事件，包括连接、断开、消息接收等
 * 
 * @param[in] handler_args 用户参数(未使用)
 * @param[in] base 事件基础类型
 * @param[in] event_id 事件ID
 * @param[in] event_data 事件数据指针
 */
static void mqtt_tool_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    mqtt_tool_handle_t* handle = (mqtt_tool_handle_t*) handler_args;
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_tool_set_state(handle, MQTT_TOOL_STATE_CONNECTED);
        if (handle->connect_sem != NULL) {
            xSemaphoreGive(handle->connect_sem);
        }
        break;
        
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_tool_set_state(handle, MQTT_TOOL_STATE_DISCONNECTED);
        break;
        
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
        
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if (event->topic && event->topic_len > 0) {
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        }
        if (event->data && event->data_len > 0) {
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
            // 将接收到的消息发送到UI处理
            if (logic_to_ui_queue != NULL) {
                logic_to_ui_msg_t msg = {
                    .type = LOGIC_MSG_MQTT_RECEIVED,
                    .data = {},
                };
                
                // 安全地复制topic，使用实际长度
                int topic_copy_len = (event->topic_len < sizeof(msg.data.mqtt_received.topic) - 1) ? 
                                    event->topic_len : sizeof(msg.data.mqtt_received.topic) - 1;
                memcpy(msg.data.mqtt_received.topic, event->topic, topic_copy_len);
                msg.data.mqtt_received.topic[topic_copy_len] = '\0';
                
                // 安全地复制payload，使用实际长度
                int payload_copy_len = (event->data_len < sizeof(msg.data.mqtt_received.payload) - 1) ? 
                                      event->data_len : sizeof(msg.data.mqtt_received.payload) - 1;
                memcpy(msg.data.mqtt_received.payload, event->data, payload_copy_len);
                msg.data.mqtt_received.payload[payload_copy_len] = '\0';
                
                // 发送到UI
                ESP_LOGI(TAG, "Sending MQTT message to UI: topic=%s, payload=%s", msg.data.mqtt_received.topic, msg.data.mqtt_received.payload);
                xQueueSend(logic_to_ui_queue, &msg, portMAX_DELAY);
            }
        }
        break;
        
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        mqtt_tool_set_state(handle, MQTT_TOOL_STATE_DISCONNECTED);
        if (event->error_handle) {
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP transport error: %d", event->error_handle->esp_transport_sock_errno);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGE(TAG, "Connection refused, return code: %d", event->error_handle->connect_return_code);
            }
        }
        break;
        
    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
        break;
        
    default:
        ESP_LOGD(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

uint8_t mqtt_tool_init(mqtt_tool_handle_t* handle)
{
    if (handle == NULL) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (handle->initialized) {
        ESP_LOGW(TAG, "MQTT tool already initialized");
        return MQTT_TOOL_SUCCESS;
    }

    // 创建互斥锁和信号量
    handle->state_mutex = xSemaphoreCreateMutex();
    handle->connect_sem = xSemaphoreCreateBinary();
    
    if (handle->state_mutex == NULL || handle->connect_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphores");
        if (handle->state_mutex) vSemaphoreDelete(handle->state_mutex);
        if (handle->connect_sem) vSemaphoreDelete(handle->connect_sem);
        return MQTT_TOOL_ERROR_INIT;
    }

    // MQTT客户端配置
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = handle->config.broker_uri,
        },
        .network = {
            .disable_auto_reconnect = false,
            .timeout_ms = 5000,
        },
        .session = {
            .keepalive = handle->config.keepalive,
            .disable_clean_session = false,
        }
    };

    // 设置客户端ID
    if (strlen(handle->config.client_id) > 0) {
        mqtt_cfg.credentials.client_id = handle->config.client_id;
    }

    // 设置用户名和密码（如果提供）
    if (strlen(handle->config.username) > 0) {
        mqtt_cfg.credentials.username = handle->config.username;
        if (strlen(handle->config.password) > 0) {
            mqtt_cfg.credentials.authentication.password = handle->config.password;
        }
    }

    // 初始化MQTT客户端
    handle->client = esp_mqtt_client_init(&mqtt_cfg);
    if (handle->client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        vSemaphoreDelete(handle->state_mutex);
        vSemaphoreDelete(handle->connect_sem);
        handle->state_mutex = NULL;
        handle->connect_sem = NULL;
        return MQTT_TOOL_ERROR_INIT;
    }

    // 注册事件处理器
    esp_err_t err = esp_mqtt_client_register_event(handle->client, ESP_EVENT_ANY_ID, mqtt_tool_event_handler, handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(handle->client);
        vSemaphoreDelete(handle->state_mutex);
        vSemaphoreDelete(handle->connect_sem);
        handle->client = NULL;
        handle->state_mutex = NULL;
        handle->connect_sem = NULL;
        return MQTT_TOOL_ERROR_INIT;
    }

    handle->initialized = true;
    handle->state = MQTT_TOOL_STATE_DISCONNECTED;
    
    ESP_LOGI(TAG, "MQTT tool initialized successfully with broker: %s", handle->config.broker_uri);
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_deinit(mqtt_tool_handle_t* handle)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGW(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    // 如果已连接，先断开连接
    if (mqtt_tool_get_state(handle) == MQTT_TOOL_STATE_CONNECTED) {
        mqtt_tool_disconnect(handle);
    }

    // 停止并销毁客户端
    esp_mqtt_client_stop(handle->client);
    esp_mqtt_client_destroy(handle->client);

    // 删除信号量
    if (handle->state_mutex != NULL) {
        vSemaphoreDelete(handle->state_mutex);
        handle->state_mutex = NULL;
    }
    if (handle->connect_sem != NULL) {
        vSemaphoreDelete(handle->connect_sem);
        handle->connect_sem = NULL;
    }

    handle->initialized = false;
    handle->client = NULL;
    handle->state = MQTT_TOOL_STATE_DISCONNECTED;

    ESP_LOGI(TAG, "MQTT tool deinitialized");
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_connect(mqtt_tool_handle_t* handle)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGE(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    if (mqtt_tool_get_state(handle) == MQTT_TOOL_STATE_CONNECTED) {
        ESP_LOGW(TAG, "Already connected");
        return MQTT_TOOL_SUCCESS;
    }

    mqtt_tool_set_state(handle, MQTT_TOOL_STATE_CONNECTING);
    
    esp_err_t err = esp_mqtt_client_start(handle->client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        mqtt_tool_set_state(handle, MQTT_TOOL_STATE_DISCONNECTED);
        return MQTT_TOOL_ERROR_CONNECT;
    }

    // 等待连接成功（最多等待10秒）
    if (xSemaphoreTake(handle->connect_sem, pdMS_TO_TICKS(10000)) == pdTRUE) {
        ESP_LOGI(TAG, "MQTT connected successfully");
        return MQTT_TOOL_SUCCESS;
    } else {
        ESP_LOGE(TAG, "MQTT connection timeout");
        mqtt_tool_set_state(handle, MQTT_TOOL_STATE_DISCONNECTED);
        return MQTT_TOOL_ERROR_CONNECT;
    }
}

uint8_t mqtt_tool_disconnect(mqtt_tool_handle_t* handle)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGE(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    if (mqtt_tool_get_state(handle) == MQTT_TOOL_STATE_DISCONNECTED) {
        ESP_LOGW(TAG, "Already disconnected");
        return MQTT_TOOL_SUCCESS;
    }

    esp_err_t err = esp_mqtt_client_disconnect(handle->client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disconnect MQTT client");
        return MQTT_TOOL_ERROR_DISCONNECT;
    }

    // 停止客户端
    esp_mqtt_client_stop(handle->client);
    mqtt_tool_set_state(handle, MQTT_TOOL_STATE_DISCONNECTED);

    ESP_LOGI(TAG, "MQTT disconnected");
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_publish(mqtt_tool_handle_t* handle, const char* topic, const char* message, int qos)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGE(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    if (topic == NULL || message == NULL) {
        ESP_LOGE(TAG, "Invalid parameters: topic=%p, message=%p", topic, message);
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (strlen(topic) == 0) {
        ESP_LOGE(TAG, "Topic cannot be empty");
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (qos < 0 || qos > 2) {
        ESP_LOGE(TAG, "Invalid QoS level: %d (must be 0, 1, or 2)", qos);
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (mqtt_tool_get_state(handle) != MQTT_TOOL_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Not connected to MQTT broker");
        return MQTT_TOOL_ERROR_PUBLISH;
    }

    int msg_id = esp_mqtt_client_publish(handle->client, topic, message, strlen(message), qos, 0);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish message to topic: %s", topic);
        return MQTT_TOOL_ERROR_PUBLISH;
    }

    ESP_LOGI(TAG, "Published message to topic: %s, msg_id: %d, qos: %d", topic, msg_id, qos);
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_subscribe(mqtt_tool_handle_t* handle, const char* topic, int qos)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGE(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    if (topic == NULL) {
        ESP_LOGE(TAG, "Invalid topic parameter");
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (strlen(topic) == 0) {
        ESP_LOGE(TAG, "Topic cannot be empty");
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (qos < 0 || qos > 2) {
        ESP_LOGE(TAG, "Invalid QoS level: %d (must be 0, 1, or 2)", qos);
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (mqtt_tool_get_state(handle) != MQTT_TOOL_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Not connected to MQTT broker");
        return MQTT_TOOL_ERROR_SUBSCRIBE;
    }

    int msg_id = esp_mqtt_client_subscribe(handle->client, topic, qos);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe to topic: %s", topic);
        return MQTT_TOOL_ERROR_SUBSCRIBE;
    }

    ESP_LOGI(TAG, "Subscribed to topic: %s, qos: %d, msg_id: %d", topic, qos, msg_id);
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_unsubscribe(mqtt_tool_handle_t* handle, const char* topic)
{
    if (handle == NULL || !handle->initialized) {
        ESP_LOGE(TAG, "MQTT tool not initialized");
        return MQTT_TOOL_ERROR_NOT_INIT;
    }

    if (topic == NULL) {
        ESP_LOGE(TAG, "Invalid topic parameter");
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (strlen(topic) == 0) {
        ESP_LOGE(TAG, "Topic cannot be empty");
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }

    if (mqtt_tool_get_state(handle) != MQTT_TOOL_STATE_CONNECTED) {
        ESP_LOGE(TAG, "Not connected to MQTT broker");
        return MQTT_TOOL_ERROR_UNSUBSCRIBE;
    }

    int msg_id = esp_mqtt_client_unsubscribe(handle->client, topic);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to unsubscribe from topic: %s", topic);
        return MQTT_TOOL_ERROR_UNSUBSCRIBE;
    }

    ESP_LOGI(TAG, "Unsubscribed from topic: %s, msg_id: %d", topic, msg_id);
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_set_broker_uri(mqtt_tool_handle_t* handle, const char* uri)
{
    if (handle == NULL || uri == NULL) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    if (strlen(uri) >= sizeof(handle->config.broker_uri)) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    strncpy(handle->config.broker_uri, uri, sizeof(handle->config.broker_uri) - 1);
    handle->config.broker_uri[sizeof(handle->config.broker_uri) - 1] = '\0';
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_set_credentials(mqtt_tool_handle_t* handle, const char* username, const char* password)
{
    if (handle == NULL || username == NULL || password == NULL) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    if (strlen(username) >= sizeof(handle->config.username) || strlen(password) >= sizeof(handle->config.password)) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    strncpy(handle->config.username, username, sizeof(handle->config.username) - 1);
    handle->config.username[sizeof(handle->config.username) - 1] = '\0';
    strncpy(handle->config.password, password, sizeof(handle->config.password) - 1);
    handle->config.password[sizeof(handle->config.password) - 1] = '\0';
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_set_client_id(mqtt_tool_handle_t* handle, const char* client_id)
{
    if (handle == NULL || client_id == NULL) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    if (strlen(client_id) >= sizeof(handle->config.client_id)) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    strncpy(handle->config.client_id, client_id, sizeof(handle->config.client_id) - 1);
    handle->config.client_id[sizeof(handle->config.client_id) - 1] = '\0';
    return MQTT_TOOL_SUCCESS;
}

uint8_t mqtt_tool_set_keepalive(mqtt_tool_handle_t* handle, uint32_t keepalive_s)
{
    if (handle == NULL) {
        return MQTT_TOOL_ERROR_INVALID_PARAM;
    }
    handle->config.keepalive = keepalive_s;
    return MQTT_TOOL_SUCCESS;
}