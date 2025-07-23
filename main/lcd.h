#ifndef LCD_H
#define LCD_H

#include <string.h>

#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"
#include "stdio.h"

/***************************  I2C ↓ *******************************************/
#define BSP_I2C_SDA (GPIO_NUM_1)  // SDA引脚
#define BSP_I2C_SCL (GPIO_NUM_2)  // SCL引脚

#define BSP_I2C_NUM (0)         // I2C外设
#define BSP_I2C_FREQ_HZ 100000  // 100kHz

esp_err_t bsp_i2c_init(void);  // 初始化I2C接口

/***************    IO扩展芯片 ↓   *************************/
#define PCA9557_INPUT_PORT 0x00
#define PCA9557_OUTPUT_PORT 0x01
#define PCA9557_POLARITY_INVERSION_PORT 0x02
#define PCA9557_CONFIGURATION_PORT 0x03

#define LCD_CS_GPIO BIT(0)    // PCA9557_GPIO_NUM_1
#define PA_EN_GPIO BIT(1)     // PCA9557_GPIO_NUM_2
#define DVP_PWDN_GPIO BIT(2)  // PCA9557_GPIO_NUM_3

#define PCA9557_SENSOR_ADDR 0x19 /*!< Slave address of the PCA9557 sensor */

/**
 * @brief 对某一组位进行按位设置或清零
 * @param _m 当前值
 * @param _s 需要设置或清零的位掩码
 * @param _v 设置的值，0或1
 * @return 返回修改后的值
 */
#define SET_BITS(_m, _s, _v) ((_v) ? (_m) | ((_s)) : (_m) & ~((_s)))

void pca9557_init(void);
void lcd_cs(uint8_t level);
void pa_en(uint8_t level);
void dvp_pwdn(uint8_t level);

/****************    LCD显示屏 ↓   *************************/
#define BSP_LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)
#define BSP_LCD_SPI_NUM (SPI3_HOST)
#define LCD_CMD_BITS (8)
#define LCD_PARAM_BITS (8)
#define BSP_LCD_BITS_PER_PIXEL (16)
#define LCD_LEDC_CH LEDC_CHANNEL_0

#define BSP_LCD_H_RES (320)
#define BSP_LCD_V_RES (240)

#define BSP_LCD_SPI_MOSI (GPIO_NUM_40)
#define BSP_LCD_SPI_CLK (GPIO_NUM_41)
#define BSP_LCD_SPI_CS (GPIO_NUM_NC)
#define BSP_LCD_DC (GPIO_NUM_39)
#define BSP_LCD_RST (GPIO_NUM_NC)
#define BSP_LCD_BACKLIGHT (GPIO_NUM_42)

esp_err_t bsp_display_new(esp_lcd_panel_handle_t *panel_handle,esp_lcd_panel_io_handle_t *io_handle);

esp_err_t bsp_display_brightness_init(void);
esp_err_t bsp_display_brightness_set(int brightness_percent);
esp_err_t bsp_display_backlight_off(void);
esp_err_t bsp_display_backlight_on(void);
esp_err_t bsp_lcd_init(esp_lcd_panel_handle_t *panel_handle,esp_lcd_panel_io_handle_t *io_handle);
void lcd_set_color(uint16_t color, esp_lcd_panel_handle_t *panel_handle);
void lcd_draw_picture(int x_start, int y_start, int x_end, int y_end,
                      const unsigned char *gImage,
                      esp_lcd_panel_handle_t *panel_handle);

#endif  // !LCD_H
