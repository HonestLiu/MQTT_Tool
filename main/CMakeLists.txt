idf_component_register(SRCS "main_updated.c" "main.c" "lcd.c" "lvgl-components.c" 
                    INCLUDE_DIRS "."
                    REQUIRES ui_interface wifi_setting mqtt_tool mqtt_ui)