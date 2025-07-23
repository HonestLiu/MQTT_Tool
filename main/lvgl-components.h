#ifndef LVGL_COMPONENTS_H
#define LVGL_COMPONENTS_H

#include "lcd.h"
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lvgl_port.h"

#define BSP_LCD_DRAW_BUF_HEIGHT    (20)  // LVGL绘图缓冲区高度

void bsp_lvgl_start(esp_lcd_panel_io_handle_t *io_handle,
                    esp_lcd_panel_handle_t *panel_handle);


#endif // !LVGL_COMPONENTS_H