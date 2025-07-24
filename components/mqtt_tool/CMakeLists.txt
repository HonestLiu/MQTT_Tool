idf_component_register(SRCS "mqtt_tool.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_event mqtt nvs_flash esp_netif wifi_provisioning)
