#include "lcd.h"

#include "driver/i2c.h"      // 旧版I2C头文件
#include "esp_err.h"
#include "esp_log.h"

// 日志TAG定义
static const char *TAG = "esp32_s3_lcd";

#define I2C_MASTER_NUM      BSP_I2C_NUM
#define I2C_MASTER_SDA_IO   BSP_I2C_SDA
#define I2C_MASTER_SCL_IO   BSP_I2C_SCL
#define I2C_MASTER_FREQ_HZ  BSP_I2C_FREQ_HZ
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define WRITE_BIT  I2C_MASTER_WRITE
#define READ_BIT   I2C_MASTER_READ
#define ACK_CHECK_EN  0x1
#define ACK_CHECK_DIS 0x0

/**
 * @brief I2C硬件初始化（旧版API）
 * @return esp_err_t 返回ESP_OK表示成功，否则失败
 */
esp_err_t bsp_i2c_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0, // v4.x可选
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                                       I2C_MASTER_RX_BUF_DISABLE,
                                       I2C_MASTER_TX_BUF_DISABLE, 0));
    return ESP_OK;
}

/***************    IO扩展芯片 ↓   *************************/

/**
 * @brief 读取PCA9557寄存器的值
 * @param reg_addr  寄存器地址
 * @param data      读取结果指针
 * @param len       读取长度
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t pca9557_register_read(uint8_t reg_addr, uint8_t *data, size_t len) {
    // 先写寄存器地址，再读数据
    return i2c_master_write_read_device(
        I2C_MASTER_NUM,
        PCA9557_SENSOR_ADDR, 
        &reg_addr, 1, 
        data, len, 
        1000 / portTICK_PERIOD_MS
    );
}

/**
 * @brief 向PCA9557寄存器写入一个字节
 * @param reg_addr  寄存器地址
 * @param data      写入数据
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t pca9557_register_write_byte(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, 
        PCA9557_SENSOR_ADDR, 
        write_buf, sizeof(write_buf), 
        1000 / portTICK_PERIOD_MS
    );
}

/**
 * @brief 初始化PCA9557 IO扩展芯片
 */
void pca9557_init(void) {
    esp_err_t ret;
    // 设置控制引脚默认状态 DVP_PWDN=1, PA_EN=0, LCD_CS=1
    ret = pca9557_register_write_byte(
        PCA9557_OUTPUT_PORT, 0x05);  // 0000 0101 (IO0和IO2输出1，其余为0)
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PCA9557 output port: %s", esp_err_to_name(ret));
        return;
    }
    // 设置IO1, IO2, IO3为输出（低三位为0），其余为输入（高五位为1）
    ret = pca9557_register_write_byte(PCA9557_CONFIGURATION_PORT, 0xf8);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PCA9557 configuration port: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief 设置PCA9557芯片某IO引脚输出高低电平
 * @param gpio_bit  对应IO的位
 * @param level     0为低电平，1为高电平
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t pca9557_set_output_state(uint8_t gpio_bit, uint8_t level) {
    uint8_t data;
    esp_err_t res = ESP_FAIL;

    res = pca9557_register_read(PCA9557_OUTPUT_PORT, &data, 1); // 先读取当前输出状态
    if (res != ESP_OK) {
        return res;
    }
    res = pca9557_register_write_byte(
        PCA9557_OUTPUT_PORT, SET_BITS(data, gpio_bit, level)); // 修改后写回

    return res;
}

/**
 * @brief 控制LCD_CS引脚电平
 * @param level 0: 低电平, 1: 高电平
 */
void lcd_cs(uint8_t level) {
    esp_err_t ret = pca9557_set_output_state(LCD_CS_GPIO, level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LCD_CS level: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief 控制PA_EN引脚电平
 * @param level 0: 低电平, 1: 高电平
 */
void pa_en(uint8_t level) {
    esp_err_t ret = pca9557_set_output_state(PA_EN_GPIO, level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PA_EN level: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief 控制DVP_PWDN引脚电平
 * @param level 0: 低电平, 1: 高电平
 */
void dvp_pwdn(uint8_t level) {
    esp_err_t ret = pca9557_set_output_state(DVP_PWDN_GPIO, level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set DVP_PWDN level: %s", esp_err_to_name(ret));
    }
}

/****************    LCD显示屏 ↓   *************************/

/**
 * @brief LCD背光PWM初始化
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_display_brightness_init(void) {
    // 配置PWM通道
    const ledc_channel_config_t LCD_backlight_channel = {
        .gpio_num = BSP_LCD_BACKLIGHT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = 1,
        .duty = 0,
        .hpoint = 0,
        .flags.output_invert = true
    };
    // 配置PWM定时器
    const ledc_timer_config_t LCD_backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = 1,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ESP_ERROR_CHECK(ledc_timer_config(&LCD_backlight_timer));      // 初始化PWM定时器
    ESP_ERROR_CHECK(ledc_channel_config(&LCD_backlight_channel));  // 初始化PWM通道

    return ESP_OK;
}

/**
 * @brief 设置LCD背光亮度
 * @note 亮度范围为0~100%，0表示关闭背光，100表示最大亮度
 * @param brightness_percent 亮度（0~100%）
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_display_brightness_set(int brightness_percent) {
    if (brightness_percent > 100) {
        brightness_percent = 100;
    } else if (brightness_percent < 0) {
        brightness_percent = 0;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
    // 10位分辨率，100% = 1023
    uint32_t duty_cycle = (1023 * brightness_percent) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH));

    return ESP_OK;
}

/**
 * @brief 关闭背光
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_display_backlight_off(void) {
    return bsp_display_brightness_set(0);
}

/**
 * @brief 打开背光（最亮）
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_display_backlight_on(void) {
    return bsp_display_brightness_set(100);
}

/**
 * @brief 液晶屏初始化，包括SPI、控制IO、驱动芯片等
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_display_new(esp_lcd_panel_handle_t *panel_handle,esp_lcd_panel_io_handle_t *io_handle) {
    //esp_lcd_panel_io_handle_t io_handle = NULL; 
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_ERROR(bsp_display_brightness_init(), TAG, "Brightness init failed");
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = BSP_LCD_SPI_CLK,
        .mosi_io_num = BSP_LCD_SPI_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = BSP_LCD_H_RES * BSP_LCD_V_RES * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(BSP_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");
    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BSP_LCD_DC,
        .cs_gpio_num = BSP_LCD_SPI_CS,
        .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 2,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BSP_LCD_SPI_NUM, &io_config, io_handle), err, TAG, "New panel IO failed");
    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = BSP_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(*io_handle, &panel_config, panel_handle), err, TAG, "New panel failed");
    
    esp_lcd_panel_reset(*panel_handle);
    lcd_cs(0);
    esp_lcd_panel_init(*panel_handle);
    esp_lcd_panel_invert_color(*panel_handle, true);
    esp_lcd_panel_swap_xy(*panel_handle, true);
    esp_lcd_panel_mirror(*panel_handle, true, false);

    return ret;

err:
    if (*panel_handle) {
        esp_lcd_panel_del(*panel_handle);
    }
    if (io_handle) {
        esp_lcd_panel_io_del(*io_handle);
    }
    spi_bus_free(BSP_LCD_SPI_NUM);
    return ret;
}

/**
 * @brief 显示图片
 * @param x_start 起始X坐标
 * @param y_start 起始Y坐标
 * @param x_end   结束X坐标
 * @param y_end   结束Y坐标
 * @param gImage  图像数据指针
 */
void lcd_draw_picture(int x_start, int y_start, int x_end, int y_end, const unsigned char *gImage, esp_lcd_panel_handle_t *panel_handle)
{
    size_t pixels_byte_size = (x_end - x_start) * (y_end - y_start) * 2;
    uint16_t *pixels = (uint16_t *)heap_caps_malloc(pixels_byte_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (NULL == pixels)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return;
    }
    memcpy(pixels, gImage, pixels_byte_size);
    esp_lcd_panel_draw_bitmap(*panel_handle, x_start, y_start, x_end, y_end, (uint16_t *)pixels);
    heap_caps_free(pixels);
}

/**
 * @brief 设置整屏颜色
 * @param color 颜色值
 */
void lcd_set_color(uint16_t color, esp_lcd_panel_handle_t *panel_handle)
{
    uint16_t *buffer = (uint16_t *)heap_caps_malloc(BSP_LCD_H_RES * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    
    if (NULL == buffer)
    {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
    }
    else
    {
        for (size_t i = 0; i < BSP_LCD_H_RES; i++)
        {
            buffer[i] = color;
        }
        for (int y = 0; y < 240; y++)
        {
            esp_lcd_panel_draw_bitmap(*panel_handle, 0, y, 320, y + 1, buffer);
        }
        heap_caps_free(buffer);
    }
}

/**
 * @brief LCD显示初始化
 * @return esp_err_t 返回ESP_OK表示成功
 */
esp_err_t bsp_lcd_init(esp_lcd_panel_handle_t *panel_handle,esp_lcd_panel_io_handle_t *io_handle) {
    esp_err_t ret = ESP_OK;

    ret = bsp_display_new(panel_handle,io_handle);
    lcd_set_color(0x0000, panel_handle);
    ret = esp_lcd_panel_disp_on_off(*panel_handle, true);
    ret = bsp_display_backlight_on();

    return  ret;
}