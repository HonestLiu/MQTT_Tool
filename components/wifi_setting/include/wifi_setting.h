#ifndef WIFI_SETTING_H
#define WIFI_SETTING_H

#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#define WIFI_SSID      "My-WiFi"         ///< WiFi SSID
#define WIFI_PASS      "roll991-arm5"             ///< WiFi 密码
#define MAXIMUM_RETRY  5                 ///< 最大重连次数

//--------------------------事件组和Tag定义--------------------------------
#define WIFI_CONNECTED_BIT BIT0                      ///< WiFi已连接事件位
#define WIFI_FAIL_BIT      BIT1                      ///< WiFi连接失败事件位



uint8_t wifi_init(void);                        ///< WiFi初始化函数




#endif