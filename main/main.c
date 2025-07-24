#include <stdio.h>
#include "lcd.h"
#include "lvgl-components.h"
#include "ui.h"
#include "lvgl.h"
#include "esp_log.h"

#include "wifi_setting.h"
#include "mqtt_tool.h"
#include "main_update.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "task_communication.h"
#include "ui_interface.h"
#include "mqtt_message_display.h"

static const char *TAG = "main";


/**
 * @defgroup HARDWARE_HANDLES 硬件句柄定义
 * @brief 定义LCD相关的硬件句柄
 * @{
 */
static esp_lcd_panel_io_handle_t io_handle = NULL;    ///< LCD IO句柄，用于LCD通信
static esp_lcd_panel_handle_t panel_handle = NULL;    ///< LCD面板句柄，用于LCD控制
/** @} */

static TaskHandle_t gui_task_handle = NULL;           ///< GUI任务句柄
static TaskHandle_t main_logic_task_handle = NULL;    ///< 主逻辑任务句柄


void hardware_init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting hardware initialization...");

    // 硬件初始化
    bsp_i2c_init();                                    ///< 初始化I2C接口
    pca9557_init();                                    ///< 初始化PCA9557 IO扩展芯片
    wifi_init();                                       ///< 初始化WiFi连接
    bsp_lvgl_start(&io_handle, &panel_handle);         ///< 启动LVGL显示系统

    // 初始化UI
    ui_init();                                         ///< 初始化UI界面
    mqtt_display_init(ui_reviceMsg,ui_MsgNum,ui_MqttState);                               ///< 初始化MQTT消息显示管理器
    mqtt_display_add_system_msg("System initialized", "info");  ///< 添加系统初始化消息到显示管理器

    ESP_LOGI(TAG, "Hardware initialization completed successfully.");

    ui_to_logic_queue = xQueueCreate(10, sizeof(ui_to_logic_msg_t));  ///< 创建UI到主逻辑任务的消息队列
    if (ui_to_logic_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create UI to logic queue");
        vQueueDelete(ui_to_logic_queue);
        vTaskDelete(NULL);                            ///< 删除当前任务
    }
    logic_to_ui_queue = xQueueCreate(10, sizeof(logic_to_ui_msg_t));  ///< 创建主逻辑任务到UI的消息队列
    if (logic_to_ui_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create logic to UI queue");
        vQueueDelete(logic_to_ui_queue);              ///< 删除主逻辑任务到UI的消息队列
        vTaskDelete(NULL);                            ///< 删除当前任务
    }

        // 创建GUI任务（高优先级，保证界面响应性）
    xTaskCreate(
        gui_task,           // 任务函数
        "GUI_Task",         // 任务名称
        8192,               // 栈大小（GUI可能需要更大的栈）
        NULL,               // 参数
        6,                  // 高优先级
        &gui_task_handle    // 任务句柄
    );
    
    // 创建主逻辑任务（中等优先级）
    xTaskCreate(
        main_logic_task,         // 任务函数
        "Main_Logic_Task",       // 任务名称
        4096,                    // 栈大小
        NULL,                    // 参数
        4,                       // 中等优先级
        &main_logic_task_handle  // 任务句柄
    );
    
    ESP_LOGI(TAG, "所有任务创建完成");
    

    // 任务完成后删除自身
    vTaskDelete(NULL);                                ///< 删除当前任务
}


void app_main(void) {
    ESP_LOGI(TAG, "Starting main application...");

    // 创建硬件初始化任务
    xTaskCreate(
        hardware_init_task,   ///< 任务函数
        "hardware_init",      ///< 任务名称
        4096,                 ///< 堆栈大小
        NULL,                 ///< 任务参数
        5,                    ///< 任务优先级
        NULL                  ///< 任务句柄
    );
}