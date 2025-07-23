idf_component_register(SRCS "wifi_setting.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event esp_wifi esp_netif nvs_flash)
