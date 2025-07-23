#include "lvgl-components.h"

static esp_lcd_touch_handle_t tp = NULL;            // 触摸屏句柄
static lv_disp_t *disp = NULL;                      // LVGL显示句柄
static lv_indev_t *disp_indev = NULL;               // LVGL输入设备句柄



static const char *TAG = "esp32_s3_lvgl";  // 日志TAG定义

/**
 * @brief LCD显示初始化(带LVGL)
 * @return lv_disp_t* 返回LVGL显示句柄
 */
static lv_disp_t *bsp_display_lcd_init(esp_lcd_panel_io_handle_t *io_handle,esp_lcd_panel_handle_t *panel_handle) {
  /* LCD Init */
  bsp_display_new(panel_handle,io_handle);                              // 初始化LCD相关驱动
  lcd_set_color(0x0000,panel_handle);                          // 设置整屏为黑色
  esp_lcd_panel_disp_on_off(*panel_handle, true);  // 打开LCD显示

  /* Add LCD screen */
  ESP_LOGD(TAG, "Add LCD screen");
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = *io_handle,
      .panel_handle = *panel_handle,
      .buffer_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,  // LVGL缓存大小
      .double_buffer = false,                                  // 不使用双缓冲
      .hres = BSP_LCD_H_RES,
      .vres = BSP_LCD_V_RES,
      .monochrome = false,
      /* 此处设置需要与液晶屏配置一样 */
      .rotation =
          {
              .swap_xy = true,    // 横纵翻转
              .mirror_x = true,   // 水平镜像
              .mirror_y = false,  // 垂直不镜像
          },
      /* 此处设置二者只能存一 */
      .flags = {
          .buff_dma = false,    // 使用DMA缓冲区
          .buff_spiram = true,  // 使用SPIRAM缓冲区
      }};

  return lvgl_port_add_disp(&disp_cfg);
}

/**
 * @brief 创建触摸屏句柄
 * @param ret_touch 返回创建的触摸屏句柄
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_touch_new(esp_lcd_touch_handle_t *ret_touch) {
  /* Initialize touch */
  const esp_lcd_touch_config_t tp_cfg = {
      // 前面开启了SWAP_XY,所以这里要颠倒
      .x_max = BSP_LCD_V_RES,
      .y_max = BSP_LCD_H_RES,
      .rst_gpio_num = GPIO_NUM_NC,  // Shared with LCD reset
      .int_gpio_num = GPIO_NUM_NC,  // Shared with LCD interrupt
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              // 此处设置同样需要与液晶屏配置一样
              .swap_xy = 1,
              .mirror_x = 1,
              .mirror_y = 0,
          },
  };

  /* 触摸
   * 库:https://components.espressif.com/components/espressif/esp_lcd_touch_ft5x06/versions/1.0.7
   */
  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t tp_io_config =
      ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();

  ESP_RETURN_ON_ERROR(
      esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_I2C_NUM,
                               &tp_io_config, &tp_io_handle),
      TAG, "");
  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg,
                                               ret_touch));  // 创建触摸屏句柄

  return ESP_OK;
}

/**
 * @brief 初始化LVGL显示
 * @return lv_disp_t* 返回LVGL显示句柄
 */
static lv_indev_t *bsp_display_indev_init(lv_disp_t *disp) {
  ESP_ERROR_CHECK(bsp_touch_new(&tp));
  assert(tp);

  /* Add touch input (for selected screen) */
  const lvgl_port_touch_cfg_t touch_cfg = {
      .disp = disp,
      .handle = tp,
  };

  return lvgl_port_add_touch(&touch_cfg);
}

/**
 * @brief 启动LVGL显示
 * @note 初始化LVGL、液晶屏和触摸屏，并打开液晶屏背光
 */
void bsp_lvgl_start(esp_lcd_panel_io_handle_t *io_handle, esp_lcd_panel_handle_t *panel_handle) {
  /* 初始化LVGL */
  lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_port_init(&lvgl_cfg);

  /* 初始化液晶屏 并添加LVGL接口 */
  disp = bsp_display_lcd_init(io_handle, panel_handle);

  /* 初始化触摸屏 并添加LVGL接口 */
  disp_indev = bsp_display_indev_init(disp);

  /* 打开液晶屏背光 */
  bsp_display_backlight_on();
}

