#include <string.h>
#include "wifi_setting.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

static const char *TAG = "wifi_setting";                 ///< 日志输出TAG
static int s_retry_num = 0;                          ///< 当前重连次数

static EventGroupHandle_t s_wifi_event_group;        ///< WiFi事件组句柄

/**
 * @brief WiFi事件回调处理函数
 * @param arg 用户参数
 * @param event_base 事件基类型(WIFI_EVENT/IP_EVENT)
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    // 处理WiFi启动事件
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // 启动后立即尝试连接WiFi
    // 处理WiFi断开连接事件
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();        // 尝试重连
            s_retry_num++;             // 重连次数+1
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); // 超过最大重试，设置失败位
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    // 处理获取IP事件
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0; // 成功获取IP后，重试次数清零
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); // 设置连接成功位
    }
}

/**
 * @brief 初始化WiFi为STA模式并连接
 */
static void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();                       // 创建事件组
    ESP_ERROR_CHECK(esp_netif_init());                              // 初始化TCP/IP栈
    ESP_ERROR_CHECK(esp_event_loop_create_default());               // 创建默认事件循环
    esp_netif_create_default_wifi_sta();                            // 创建默认STA网络接口

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();            // 获取WiFi初始化默认参数
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                           // 初始化WiFi驱动

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    // 注册WiFi事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    // 注册IP事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // 配置WiFi连接参数
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));              // 设置为STA模式
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));// 设置STA参数
    ESP_ERROR_CHECK(esp_wifi_start());                              // 启动WiFi

    // 等待连接结果
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    // 连接成功/失败日志输出
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

uint8_t wifi_init(void) {
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // 如果NVS初始化失败，擦除NVS
        ret = nvs_flash_init();             // 重新初始化NVS
    }
    ESP_ERROR_CHECK(ret);

    // 初始化WiFi连接
    wifi_init_sta();

    return 0; // 返回0表示初始化成功
}