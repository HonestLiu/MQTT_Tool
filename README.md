# ESP32-S3 LVGL 页面管理系统

![version v0.1.0](https://img.shields.io/badge/version-v0.1.1-brightgreen)
![license MIT](https://img.shields.io/badge/license-MIT-green)

## 项目简介

这是一个基于ESP32-S3微控制器的LCD显示项目，使用LVGL图形库创建用户界面，并实现了完整的页面管理系统。项目支持ST7789驱动的TFT LCD显示屏和FT5x06触摸控制器，提供了三种不同的页面管理示例。

## 功能特性

### 🚀 页面管理系统
- **基础页面管理**: 支持页面间的前进和后退操作
- **弹窗系统**: 从下往上弹出的模态页面
- **页面替换**: 直接替换当前页面而不产生堆栈

### 🎨 UI特性
- 流畅的页面切换动画
- 支持滑动、弹窗等多种动画效果
- 完整的页面生命周期管理
- 自动化演示定时器

### 🔧 硬件特性
- **微控制器**: ESP32-S3
- **显示屏**: 320x240 TFT LCD (ST7789驱动)
- **触摸控制器**: FT5x06 电容触摸屏
- **IO扩展**: PCA9557 I2C IO扩展芯片
- **接口**: SPI (显示), I2C (触摸和IO扩展)

## 项目结构

```
├── main/
│   ├── main.c              # 主程序入口，包含三种示例
│   ├── lcd.c/lcd.h         # LCD驱动层
│   ├── lvgl-components.c   # LVGL组件配置
│   ├── page1.c/page1.h     # 基础示例页面1
│   ├── page2.c/page2.h     # 基础示例页面2
│   ├── page3.c/page3.h     # 弹窗示例主页面
│   ├── popup.c/popup.h     # 弹窗页面
│   ├── page4.c/page4.h     # 替换示例页面4
│   ├── page5.c/page5.h     # 替换示例页面5
│   └── CMakeLists.txt      # 构建配置
├── components/
│   └── lvgl_pm/            # 页面管理组件
│       ├── include/
│       │   ├── pm.h        # 页面管理器头文件
│       │   ├── anima.h     # 动画效果头文件
│       │   └── pm_utils.h  # 工具函数头文件
│       ├── pm.c            # 页面管理器实现
│       ├── anima.c         # 动画效果实现
│       └── pm_utils.c      # 工具函数实现
└── managed_components/     # ESP-IDF托管组件
    ├── espressif__esp_lcd_touch/
    ├── espressif__esp_lcd_touch_ft5x06/
    ├── espressif__esp_lvgl_port/
    └── lvgl__lvgl/
```

## 示例说明

### 1. 基础页面管理示例 (START_PM)
```c
#define START_PM 1       // 启用基础页面管理示例
#define POPUP_EX 0       // 禁用弹窗示例
#define TARGET_SELF_EX 0 // 禁用替换示例
```

**特性：**
- 使用滑动动画在页面1和页面2之间切换
- 支持页面历史记录和返回功能
- 每1秒自动切换演示

**页面生命周期：**
- `onLoad`: 页面加载时调用
- `unLoad`: 页面卸载时调用

### 2. 弹窗示例 (POPUP_EX)
```c
#define START_PM 0       // 禁用基础页面管理示例
#define POPUP_EX 1       // 启用弹窗示例
#define TARGET_SELF_EX 0 // 禁用替换示例
```

**特性：**
- 主页面+弹窗页面模式
- 弹窗使用从下往上的动画效果
- 支持弹窗的显示和隐藏

### 3. 页面替换示例 (TARGET_SELF_EX)
```c
#define START_PM 0       // 禁用基础页面管理示例
#define POPUP_EX 0       // 禁用弹窗示例
#define TARGET_SELF_EX 1 // 启用替换示例
```

**特性：**
- 页面4和页面5之间直接替换
- 不产生页面堆栈，节省内存
- 每5秒自动切换演示
- 适用于状态切换场景

## 硬件连接

### LCD显示屏 (SPI接口)
| ESP32-S3引脚 | LCD信号 | 描述 |
|-------------|---------|------|
| GPIO40      | MOSI    | SPI数据输出 |
| GPIO41      | SCLK    | SPI时钟 |
| GPIO39      | DC      | 数据/命令选择 |
| GPIO42      | BL      | 背光控制(PWM) |
| CS信号      | 通过PCA9557控制 | 片选信号 |

### 触摸屏 (I2C接口)
| ESP32-S3引脚 | 触摸屏信号 | 描述 |
|-------------|-----------|------|
| GPIO1       | SDA       | I2C数据线 |
| GPIO2       | SCL       | I2C时钟线 |

### PCA9557 IO扩展芯片
| PCA9557引脚 | 功能      | 描述 |
|------------|----------|------|
| IO0        | LCD_CS   | LCD片选控制 |
| IO1        | PA_EN    | 功放使能 |
| IO2        | DVP_PWDN | 摄像头电源控制 |

## 页面管理API

### 核心函数

```c
/**
 * @brief 页面管理初始化
 * @return 0 成功
 */
uint8_t lv_pm_init();

/**
 * @brief 创建页面并添加到路由表
 * @param id 页面ID
 * @return 页面结构体指针或NULL
 */
lv_pm_page_t *lv_pm_create_page(uint8_t id);

/**
 * @brief 打开页面
 * @param id 页面ID
 * @param options 打开选项（动画类型、目标方式等）
 * @return 0 成功，其他为错误码
 */
uint8_t lv_pm_open_page(uint8_t id, lv_pm_open_options_t *options);

/**
 * @brief 返回上一个页面
 * @return 0 成功，其他为错误码
 */
uint8_t lv_pm_back();
```

### 页面选项配置

```c
// 基础滑动动画
lv_pm_open_options_t options = {
    .animation = LV_PM_ANIMA_SLIDE,
};

// 弹窗动画
lv_pm_open_options_t popup_options = {
    .animation = LV_PM_ANIMA_POPUP
};

// 页面替换
lv_pm_open_options_t self_options = {
    .animation = LV_PM_ANIMA_SLIDE,
    .target = LV_PM_TARGET_SELF
};
```

### 页面生命周期回调

```c
typedef struct _lv_pm_page_t {
    lv_obj_t *page;                 // 页面对象指针
    lv_pm_lifecycle onLoad;         // 页面加载时回调
    lv_pm_lifecycle willAppear;     // 页面出现前回调
    lv_pm_lifecycle didAppear;      // 页面出现后回调
    lv_pm_lifecycle willDisappear;  // 页面消失前回调
    lv_pm_lifecycle didDisappear;   // 页面消失后回调
    lv_pm_lifecycle unLoad;         // 页面卸载时回调
} lv_pm_page_t;
```

## 快速开始

### 1. 环境准备
```bash
# 安装ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh
```

### 2. 编译和烧录
```bash
# 克隆项目
git clone <your-repo-url>
cd lcd_lvgl_pm

# 配置目标芯片
idf.py set-target esp32s3

# 编译项目
idf.py build

# 烧录到设备
idf.py flash monitor
```

### 3. 选择示例
在 `main/main.c` 中修改示例选择宏：
```c
#define START_PM 1       // 选择基础页面管理示例
#define POPUP_EX 0       // 其他示例设为0
#define TARGET_SELF_EX 0
```

## 软件架构

### 主要组件

1. **LCD驱动层** (`lcd.c/lcd.h`)
   - SPI总线初始化和配置
   - ST7789显示驱动程序
   - 背光PWM控制
   - 显示缓冲区管理

2. **页面管理层** (`components/lvgl_pm/`)
   - 页面路由管理
   - 页面历史记录
   - 动画效果处理
   - 生命周期管理

3. **应用层** (`main/main.c`)
   - 硬件初始化
   - 示例选择和配置
   - 页面创建和回调设置
   - 定时器管理

## 开发指南

### 创建新页面

1. **定义页面头文件**
```c
// page_new.h
#ifndef PAGE_NEW_H
#define PAGE_NEW_H

#include "lvgl.h"

void page_new_onLoad(lv_obj_t *page);
void page_new_unLoad(lv_obj_t *page);

#endif
```

2. **实现页面功能**
```c
// page_new.c
#include "page_new.h"
#include <stdio.h>

void page_new_onLoad(lv_obj_t *page) {
    printf("New page loaded\n");
    
    // 设置页面背景色
    lv_obj_set_style_bg_color(page, lv_color_make(100, 200, 100), LV_STATE_DEFAULT);
    
    // 创建页面内容
    lv_obj_t *label = lv_label_create(page);
    lv_label_set_text(label, "New Page");
    lv_obj_center(label);
}

void page_new_unLoad(lv_obj_t *page) {
    printf("New page unloaded\n");
}
```

3. **注册页面**
```c
// 在main.c中
lv_pm_page_t *new_page = lv_pm_create_page(2);
new_page->onLoad = page_new_onLoad;
new_page->unLoad = page_new_unLoad;
```

### 页面切换
```c
// 普通切换
lv_pm_open_page(2, &options);

// 返回
lv_pm_back();
```

## 常见问题

### Q: 编译时出现链接错误？
A: 确保在 `CMakeLists.txt` 中包含了所有的 `.c` 文件：
```cmake
idf_component_register(SRCS "main.c" "lcd.c" "lvgl-components.c" "page1.c" "page2.c" ...
                    INCLUDE_DIRS ".")
```

### Q: 页面切换没有动画效果？
A: 检查动画配置是否正确：
```c
lv_pm_open_options_t options = {
    .animation = LV_PM_ANIMA_SLIDE,  // 确保动画类型正确
};
```

### Q: 触摸不响应？
A: 检查I2C连接和触摸控制器配置。

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 联系方式

- 作者: honestliu
- 邮箱: [your-email@example.com]
- 项目地址: [https://github.com/your-username/lcd_lvgl_pm]

---

*最后更新时间: 2025-07-18*
   - FT5x06触摸控制器驱动
   - I2C通信接口
   - 触摸坐标转换

3. **IO扩展层**
   - PCA9557 I2C IO扩展芯片控制
   - LCD片选信号控制
   - 其他外设控制信号

4. **LVGL图形库**
   - 用户界面渲染
   - 触摸事件处理
   - 双缓冲显示

### 关键配置参数

```c
// 显示屏分辨率
#define BSP_LCD_H_RES              (320)
#define BSP_LCD_V_RES              (240)

// SPI时钟频率
#define BSP_LCD_PIXEL_CLOCK_HZ     (80 * 1000 * 1000)

// I2C频率
#define BSP_I2C_FREQ_HZ            100000

// LVGL缓冲区高度
#define BSP_LCD_DRAW_BUF_HEIGHT    (20)
```

## 环境配置

### 前置要求

1. **ESP-IDF**: v5.0或更高版本
2. **Python**: 3.7或更高版本
3. **CMake**: 3.5或更高版本

### 安装ESP-IDF

```bash
# 下载ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf

# 安装工具链
./install.sh

# 设置环境变量
. ./export.sh
```

## 编译和烧录

### 1. 克隆项目

```bash
git clone <project-url>
cd lcd_lvgl
```

### 2. 配置项目

```bash
idf.py menuconfig
```

重要配置项：
- 选择目标芯片: ESP32-S3
- 配置SPIRAM: 启用PSRAM支持
- 配置分区表: 根据需要调整分区大小

配置LVGL：
- 开启swao
- 设置12、16号字体
- 开启Example，并根据所使用示例都选对应示例

配置FLASH: 
- 配置为16MB

开启PSRAM: 
- 设置为Octal
- 频率给到80MHz

### 3. 编译项目

```bash
idf.py build
```

### 4. 烧录固件

```bash
# 烧录固件
idf.py -p /dev/ttyUSB0 flash

# 查看串口输出
idf.py -p /dev/ttyUSB0 monitor
```

## 使用说明

### 基本显示功能

```c
// 初始化显示系统
bsp_i2c_init();     // 初始化I2C
pca9557_init();     // 初始化IO扩展
bsp_lcd_init();     // 初始化LCD

// 显示纯色
lcd_set_color(0xF800);  // 红色

// 显示图片
lcd_draw_pictrue(0, 0, 320, 240, image_data);
```

### LVGL图形界面

```c
// 启动LVGL系统
bsp_lvgl_start();

// 运行LVGL演示
lv_demo_widgets();
```

### 背光控制

```c
// 设置背光亮度 (0-100%)
bsp_display_brightness_set(50);

// 开启背光
bsp_display_backlight_on();

// 关闭背光
bsp_display_backlight_off();
```

## 依赖组件

项目使用以下ESP组件：

- `espressif/esp_lcd_touch_ft5x06`: FT5x06触摸驱动
- `espressif/esp_lvgl_port`: LVGL移植层
- `lvgl/lvgl`: LVGL图形库

这些依赖在`main/idf_component.yml`中定义，会自动下载。

## 故障排除

### 常见问题

1. **屏幕不显示**
   - 检查SPI连接
   - 确认背光是否开启
   - 验证IO扩展芯片CS信号

2. **触摸不响应**
   - 检查I2C连接
   - 确认触摸控制器地址
   - 验证中断引脚配置

3. **编译错误**
   - 确认ESP-IDF版本兼容性
   - 检查组件依赖是否正确下载
   - 验证目标芯片配置

### 调试技巧

```c
// 启用调试日志
esp_log_level_set("esp32_s3_szp", ESP_LOG_DEBUG);

// 检查SPI传输
ESP_LOGD(TAG, "SPI transfer complete");

// 验证I2C通信
ESP_LOGD(TAG, "I2C device response: 0x%02X", data);
```

## 性能优化

1. **使用SPIRAM**: 启用外部PSRAM存储LVGL缓冲区
2. **DMA传输**: 使用SPI DMA加速显示刷新
3. **双缓冲**: 减少显示闪烁
4. **时钟优化**: 调整SPI和I2C时钟频率

## 贡献指南

1. Fork本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开Pull Request

## 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 致谢

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif物联网开发框架
- [LVGL](https://github.com/lvgl/lvgl) - 轻量级通用图形库
- [ESP LCD Touch](https://github.com/espressif/esp-bsp) - ESP LCD触摸驱动
- [lvgl-pm](https://github.com/LanFly/lvgl-pm) - 本工程使用的页面管理器

## 版本历史

- **v1.0.0** - 初始版本
  - 基本LCD显示功能
  - LVGL图形界面支持
  - 触摸输入处理
  - PCA9557 IO扩展控制
- **v1.0.1** - 优化版
  - 拆出了LVGL逻辑
  - 拆出句柄到main函数

## 联系方式

- 作者: honestliu
---

*最后更新时间: 2025-07-18*
