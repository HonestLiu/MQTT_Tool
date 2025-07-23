#include <stdio.h>
#include "lcd.h"
#include "lvgl-components.h"
#include "pm.h"
#include "ui.h"
#include "lvgl.h"

#include "wifi_setting.h"
#include "mqtt_tool.h"


/**
 * @defgroup HARDWARE_HANDLES 硬件句柄定义
 * @brief 定义LCD相关的硬件句柄
 * @{
 */
static esp_lcd_panel_io_handle_t io_handle = NULL;    ///< LCD IO句柄，用于LCD通信
static esp_lcd_panel_handle_t panel_handle = NULL;    ///< LCD面板句柄，用于LCD控制
/** @} */

/**
 * @brief 应用程序主函数
 * @details 程序入口点，负责初始化硬件、LVGL系统和页面管理系统
 *          根据编译时选择的示例类型，创建相应的页面并启动定时器
 * @note 该函数会根据宏定义选择不同的示例：
 *       - START_PM: 基础页面管理示例
 *       - POPUP_EX: 弹窗示例
 *       - TARGET_SELF_EX: 替换当前页面示例
 */
void app_main(void) {
    // 硬件初始化
    bsp_i2c_init();                                    ///< 初始化I2C接口
    pca9557_init();                                    ///< 初始化PCA9557 IO扩展芯片
    wifi_init();                                       ///< 初始化WiFi连接
    mqtt_tool_init();                                  ///< 初始化MQTT工具

    bsp_lvgl_start(&io_handle, &panel_handle);         ///< 启动LVGL显示系统



    ui_init();
}