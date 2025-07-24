/**
 * @file mqtt_tool.h
 * @brief ESP32 MQTT工具库头文件
 * 
 * 这个库提供了一个简单易用的MQTT客户端接口，支持连接、发布、订阅等基本功能。
 * 基于ESP-IDF的mqtt_client组件构建，提供了更高级的封装和错误处理。
 * 
 * @author HonestLiu
 * @date 2025-07-23
 * @version 1.0
 */

#ifndef MQTT_TOOL_H
#define MQTT_TOOL_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mqtt_client.h"

/**
 * @defgroup MQTT_TOOL_CONFIG MQTT工具配置
 * @{
 */

/** @brief 默认MQTT代理地址 */
#define MQTT_TOOL_DEFAULT_BROKER_URI "mqtt://mqtt.ernestliu.xyz"

/** @brief 默认MQTT端口 */
#define MQTT_TOOL_DEFAULT_PORT       1883

/** @brief 默认客户端ID */
#define MQTT_TOOL_DEFAULT_CLIENT_ID  "esp32_mqtt_client"


/**
 * @brief MQTT配置结构体
 * 
 * 存储MQTT连接和认证相关的配置信息
 */
typedef struct {
    char broker_uri[128];    /**< MQTT代理服务器URI */
    char client_id[32];      /**< MQTT客户端ID */
    char username[32];       /**< 用户名 */
    char password[32];       /**< 密码 */
    uint16_t port;           /**< 端口号 */
    uint16_t keepalive;      /**< 心跳间隔(秒) */
} mqtt_config_t;

/**
 * @brief MQTT连接状态枚举
 */
typedef enum {
    MQTT_TOOL_STATE_DISCONNECTED = 0,  /**< 已断开连接 */
    MQTT_TOOL_STATE_CONNECTING,        /**< 正在连接 */
    MQTT_TOOL_STATE_CONNECTED          /**< 已连接 */
} mqtt_tool_state_t;

/**
 * @brief MQTT工具主结构体
 * 
 * 包含MQTT客户端句柄、状态信息和同步原语
 */
typedef struct mqtt_tool_handle_t {
    esp_mqtt_client_handle_t client;  /**< ESP-IDF MQTT客户端句柄 */
    mqtt_tool_state_t state;          /**< 当前连接状态 */
    bool initialized;                 /**< 初始化标志 */
    SemaphoreHandle_t state_mutex;    /**< 状态互斥锁 */
    SemaphoreHandle_t connect_sem;    /**< 连接信号量 */
    mqtt_config_t config;             /**< MQTT配置 */
} mqtt_tool_handle_t;


/** @} */

/**
 * @defgroup MQTT_TOOL_ERRORS 错误代码定义
 * @{
 */

/** @brief 操作成功 */
#define MQTT_TOOL_SUCCESS           0

/** @brief 初始化失败 */
#define MQTT_TOOL_ERROR_INIT        1

/** @brief 反初始化失败 */
#define MQTT_TOOL_ERROR_DEINIT      2

/** @brief 连接失败 */
#define MQTT_TOOL_ERROR_CONNECT     3

/** @brief 断开连接失败 */
#define MQTT_TOOL_ERROR_DISCONNECT  4

/** @brief 发布消息失败 */
#define MQTT_TOOL_ERROR_PUBLISH     5

/** @brief 订阅主题失败 */
#define MQTT_TOOL_ERROR_SUBSCRIBE   6

/** @brief 取消订阅失败 */
#define MQTT_TOOL_ERROR_UNSUBSCRIBE 7

/** @brief 工具未初始化 */
#define MQTT_TOOL_ERROR_NOT_INIT    8

/** @brief 无效参数 */
#define MQTT_TOOL_ERROR_INVALID_PARAM 9

/** @} */

/**
 * @defgroup MQTT_TOOL_API MQTT工具API函数
 * @{
 */

/**
 * @brief 初始化MQTT工具
 * 
 * 创建必要的信号量和互斥锁，初始化MQTT客户端，注册事件处理器。
 * 必须在使用其他功能前调用。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 
 *   - MQTT_TOOL_SUCCESS: 初始化成功
 *   - MQTT_TOOL_ERROR_INIT: 初始化失败
 * 
 * @note 重复调用此函数将返回成功但不会重复初始化
 */
uint8_t mqtt_tool_init(mqtt_tool_handle_t* handle);

/**
 * @brief 反初始化MQTT工具
 * 
 * 断开连接，销毁MQTT客户端，释放所有资源。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 
 *   - MQTT_TOOL_SUCCESS: 反初始化成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 */
uint8_t mqtt_tool_deinit(mqtt_tool_handle_t* handle);

/**
 * @brief 连接到MQTT代理服务器
 * 
 * 启动MQTT客户端并连接到配置的代理服务器。
 * 此函数会阻塞最多10秒等待连接建立。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 
 *   - MQTT_TOOL_SUCCESS: 连接成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 *   - MQTT_TOOL_ERROR_CONNECT: 连接失败或超时
 * 
 * @note 如果已经连接，此函数将立即返回成功
 */
uint8_t mqtt_tool_connect(mqtt_tool_handle_t* handle);

/**
 * @brief 断开与MQTT代理服务器的连接
 * 
 * 主动断开与MQTT代理服务器的连接。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 
 *   - MQTT_TOOL_SUCCESS: 断开成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 *   - MQTT_TOOL_ERROR_DISCONNECT: 断开失败
 */
uint8_t mqtt_tool_disconnect(mqtt_tool_handle_t* handle);

/**
 * @brief 发布消息到指定主题
 * 
 * 向MQTT代理服务器发布消息到指定主题。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] topic 目标主题，不能为空
 * @param[in] message 要发布的消息内容，不能为空
 * @param[in] qos 服务质量等级 (0, 1, 或 2)
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 发布成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数无效
 *   - MQTT_TOOL_ERROR_PUBLISH: 发布失败
 * 
 * @note 只有在连接状态下才能发布消息
 */
uint8_t mqtt_tool_publish(mqtt_tool_handle_t* handle, const char* topic, const char* message, int qos);

/**
 * @brief 订阅指定主题
 * 
 * 订阅指定主题，接收该主题下的消息。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] topic 要订阅的主题，不能为空
 * @param[in] qos 服务质量等级 (0, 1, 或 2)
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 订阅成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数无效
 *   - MQTT_TOOL_ERROR_SUBSCRIBE: 订阅失败
 * 
 * @note 只有在连接状态下才能订阅主题
 */
uint8_t mqtt_tool_subscribe(mqtt_tool_handle_t* handle, const char* topic, int qos);

/**
 * @brief 取消订阅指定主题
 * 
 * 取消对指定主题的订阅。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] topic 要取消订阅的主题，不能为空
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 取消订阅成功
 *   - MQTT_TOOL_ERROR_NOT_INIT: 工具未初始化
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数无效
 *   - MQTT_TOOL_ERROR_UNSUBSCRIBE: 取消订阅失败
 */
uint8_t mqtt_tool_unsubscribe(mqtt_tool_handle_t* handle, const char* topic);

/** @} */

/**
 * @defgroup MQTT_TOOL_CONFIG_API 配置API函数
 * @{
 */

/**
 * @brief 设置MQTT代理服务器地址
 * 
 * 设置要连接的MQTT代理服务器的URI。
 * 
 * @param[in] uri MQTT代理服务器URI (例如: "mqtt://broker.example.com:1883")
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 设置成功
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: URI参数无效或过长
 *   - MQTT_TOOL_ERROR_INIT: 工具已初始化，无法修改配置
 * 
 * @note 必须在mqtt_tool_init()之前调用
 */
uint8_t mqtt_tool_set_broker_uri(mqtt_tool_handle_t* handle, const char* uri);

/**
 * @brief 设置MQTT认证凭据
 * 
 * 设置连接MQTT代理服务器时使用的用户名和密码。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] username 用户名，可以为NULL表示不使用认证
 * @param[in] password 密码，可以为NULL
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 设置成功
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数过长
 *   - MQTT_TOOL_ERROR_INIT: 工具已初始化，无法修改配置
 * 
 * @note 必须在mqtt_tool_init()之前调用
 */
uint8_t mqtt_tool_set_credentials(mqtt_tool_handle_t* handle, const char* username, const char* password);

/**
 * @brief 设置MQTT客户端ID
 * 
 * 设置连接MQTT代理服务器时使用的客户端标识符。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] client_id 客户端ID字符串
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 设置成功
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数无效或过长
 *   - MQTT_TOOL_ERROR_INIT: 工具已初始化，无法修改配置
 * 
 * @note 必须在mqtt_tool_init()之前调用
 */
uint8_t mqtt_tool_set_client_id(mqtt_tool_handle_t* handle, const char* client_id);

/**
 * @brief 设置MQTT keep-alive间隔
 * 
 * 设置MQTT协议的心跳间隔时间（秒）。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @param[in] keepalive_s 心跳间隔，单位为秒
 * 
 * @return 
 *   - MQTT_TOOL_SUCCESS: 设置成功
 *   - MQTT_TOOL_ERROR_INVALID_PARAM: 参数无效
 * 
 * @note 必须在mqtt_tool_init()之前调用
 */
uint8_t mqtt_tool_set_keepalive(mqtt_tool_handle_t* handle, uint32_t keepalive_s);

/** @} */

/**
 * @defgroup MQTT_TOOL_STATUS 状态查询函数
 * @{
 */

/**
 * @brief 获取当前MQTT连接状态
 * 
 * 线程安全地获取当前的MQTT连接状态。
 * 
 * @param[in] handle 指向mqtt_tool_handle_t实例的指针
 * @return 当前连接状态 (mqtt_tool_state_t)
 *   - MQTT_TOOL_STATE_DISCONNECTED: 已断开连接
 *   - MQTT_TOOL_STATE_CONNECTING: 正在连接
 *   - MQTT_TOOL_STATE_CONNECTED: 已连接
 */
mqtt_tool_state_t mqtt_tool_get_state(mqtt_tool_handle_t* handle);

/** @} */

#endif // MQTT_TOOL_H