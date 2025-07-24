# ESP32 MQTT工具库

一个基于ESP-IDF的简单易用的MQTT客户端库，提供了MQTT连接、发布、订阅等基本功能的高级封装。

## 📋 目录

- [特性](#特性)
- [系统要求](#系统要求)
- [安装](#安装)
- [快速开始](#快速开始)
- [API参考](#api参考)
- [使用示例](#使用示例)
- [配置选项](#配置选项)
- [错误处理](#错误处理)
- [故障排除](#故障排除)
- [贡献](#贡献)
- [许可证](#许可证)

## ✨ 特性

- 🔌 **简单易用**: 提供简洁的API接口，几行代码即可实现MQTT功能
- 🔒 **线程安全**: 使用FreeRTOS互斥锁和信号量确保多线程环境下的安全性
- 🛡️ **错误处理**: 完善的错误处理机制和详细的日志输出
- ⚙️ **灵活配置**: 支持自定义代理地址、认证信息、客户端ID等
- 📊 **状态管理**: 实时的连接状态监控和管理
- 🔄 **自动重连**: 支持网络断开后的自动重连机制
- 📝 **详细日志**: 提供详细的调试和运行日志

## 🎯 系统要求

- **ESP-IDF**: v4.4 或更高版本
- **硬件**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3 等ESP系列芯片
- **内存**: 建议至少32KB可用RAM
- **网络**: WiFi或以太网连接

## 📦 安装

### 方法1: 作为ESP-IDF组件

1. 将 `mqtt_tool` 文件夹复制到您的项目的 `components` 目录下：
```bash
cp -r mqtt_tool /path/to/your/project/components/
```

2. 在您的 `CMakeLists.txt` 中添加依赖：
```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt_tool
)
```

### 方法2: 直接集成

将 `mqtt_tool.h` 和 `mqtt_tool.c` 文件添加到您的项目中，并确保在CMakeLists.txt中包含必要的依赖。

## 🚀 快速开始

### 基本用法

```c
#include "mqtt_tool.h"
#include "esp_log.h"

static const char *TAG = "app_main";

void app_main(void)
{
    // 1. 初始化MQTT工具
    uint8_t result = mqtt_tool_init();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT tool init failed: %d", result);
        return;
    }

    // 2. 连接到MQTT代理
    result = mqtt_tool_connect();
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT connect failed: %d", result);
        return;
    }

    // 3. 订阅主题
    result = mqtt_tool_subscribe("test/topic", 1);
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT subscribe failed: %d", result);
    }

    // 4. 发布消息
    result = mqtt_tool_publish("test/topic", "Hello MQTT!", 1);
    if (result != MQTT_TOOL_SUCCESS) {
        ESP_LOGE(TAG, "MQTT publish failed: %d", result);
    }

    ESP_LOGI(TAG, "MQTT setup completed successfully");
}
```

### 高级配置

```c
#include "mqtt_tool.h"

void app_main(void)
{
    // 1. 设置自定义MQTT代理
    mqtt_tool_set_broker_uri("mqtt://your-broker.com:1883");
    
    // 2. 设置认证信息
    mqtt_tool_set_credentials("your_username", "your_password");
    
    // 3. 设置客户端ID
    mqtt_tool_set_client_id("my_esp32_device_001");
    
    // 4. 初始化并连接
    mqtt_tool_init();
    mqtt_tool_connect();
    
    // ... 其他操作
}
```

## 📚 API参考

### 核心函数

#### `mqtt_tool_init()`
初始化MQTT工具库。

**返回值:**
- `MQTT_TOOL_SUCCESS`: 初始化成功
- `MQTT_TOOL_ERROR_INIT`: 初始化失败

**示例:**
```c
uint8_t result = mqtt_tool_init();
if (result != MQTT_TOOL_SUCCESS) {
    ESP_LOGE(TAG, "Init failed");
}
```

#### `mqtt_tool_connect()`
连接到MQTT代理服务器。

**返回值:**
- `MQTT_TOOL_SUCCESS`: 连接成功
- `MQTT_TOOL_ERROR_NOT_INIT`: 未初始化
- `MQTT_TOOL_ERROR_CONNECT`: 连接失败

**注意:** 此函数会阻塞最多10秒等待连接建立。

#### `mqtt_tool_publish(topic, message, qos)`
发布消息到指定主题。

**参数:**
- `topic`: 目标主题 (不能为空)
- `message`: 消息内容 (不能为空)
- `qos`: 服务质量等级 (0, 1, 或 2)

**返回值:**
- `MQTT_TOOL_SUCCESS`: 发布成功
- `MQTT_TOOL_ERROR_INVALID_PARAM`: 参数无效

#### `mqtt_tool_subscribe(topic, qos)`
订阅指定主题。

**参数:**
- `topic`: 要订阅的主题
- `qos`: 服务质量等级

#### `mqtt_tool_get_state()`
获取当前连接状态。

**返回值:**
- `MQTT_TOOL_STATE_DISCONNECTED`: 已断开
- `MQTT_TOOL_STATE_CONNECTING`: 正在连接
- `MQTT_TOOL_STATE_CONNECTED`: 已连接

### 配置函数

#### `mqtt_tool_set_broker_uri(uri)`
设置MQTT代理服务器地址。

**参数:**
- `uri`: 代理服务器URI (例如: "mqtt://broker.example.com:1883")

**注意:** 必须在 `mqtt_tool_init()` 之前调用。

#### `mqtt_tool_set_credentials(username, password)`
设置认证凭据。

**参数:**
- `username`: 用户名 (可以为NULL)
- `password`: 密码 (可以为NULL)

#### `mqtt_tool_set_client_id(client_id)`
设置客户端ID。

**参数:**
- `client_id`: 客户端标识符字符串

## 💡 使用示例

### 示例1: 温度传感器数据发布

```c
#include "mqtt_tool.h"
#include "driver/temperature_sensor.h"

void publish_temperature_task(void *pvParameters)
{
    char temp_str[32];
    float temperature;
    
    while (1) {
        // 读取温度
        temperature_sensor_get_celsius(temp_handle, &temperature);
        
        // 格式化温度数据
        snprintf(temp_str, sizeof(temp_str), "%.2f", temperature);
        
        // 发布到MQTT
        uint8_t result = mqtt_tool_publish("sensors/temperature", temp_str, 1);
        if (result != MQTT_TOOL_SUCCESS) {
            ESP_LOGE(TAG, "Failed to publish temperature");
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // 5秒间隔
    }
}
```

### 示例2: 远程控制LED

```c
#include "mqtt_tool.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_2

void setup_led_control(void)
{
    // 配置GPIO
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    // 订阅控制主题
    mqtt_tool_subscribe("device/led/control", 1);
}

// 在事件处理中添加LED控制逻辑
// 当收到"ON"消息时点亮LED，收到"OFF"时熄灭LED
```

### 示例3: 连接状态监控

```c
void connection_monitor_task(void *pvParameters)
{
    mqtt_tool_state_t last_state = MQTT_TOOL_STATE_DISCONNECTED;
    
    while (1) {
        mqtt_tool_state_t current_state = mqtt_tool_get_state();
        
        if (current_state != last_state) {
            switch (current_state) {
                case MQTT_TOOL_STATE_CONNECTED:
                    ESP_LOGI(TAG, "MQTT连接已建立");
                    // 重新订阅主题
                    mqtt_tool_subscribe("device/status", 1);
                    break;
                    
                case MQTT_TOOL_STATE_DISCONNECTED:
                    ESP_LOGW(TAG, "MQTT连接已断开");
                    break;
                    
                case MQTT_TOOL_STATE_CONNECTING:
                    ESP_LOGI(TAG, "正在连接MQTT...");
                    break;
            }
            last_state = current_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## ⚙️ 配置选项

### 默认配置

```c
#define MQTT_TOOL_DEFAULT_BROKER_URI "mqtt://test.mosquitto.org"
#define MQTT_TOOL_DEFAULT_PORT       1883
#define MQTT_TOOL_DEFAULT_CLIENT_ID  "esp32_mqtt_client"
```

### 自定义配置

您可以通过以下方式自定义配置：

1. **在代码中动态设置:**
```c
mqtt_tool_set_broker_uri("mqtt://your-broker.com:1883");
mqtt_tool_set_credentials("username", "password");
```

2. **修改默认值:** 直接修改头文件中的宏定义

3. **使用menuconfig:** 在ESP-IDF项目中通过menuconfig配置

## ❌ 错误处理

### 错误代码

| 错误代码 | 含义 | 解决方案 |
|---------|------|---------|
| `MQTT_TOOL_SUCCESS` | 操作成功 | - |
| `MQTT_TOOL_ERROR_INIT` | 初始化失败 | 检查内存是否足够，确保WiFi已连接 |
| `MQTT_TOOL_ERROR_CONNECT` | 连接失败 | 检查网络连接和代理地址 |
| `MQTT_TOOL_ERROR_PUBLISH` | 发布失败 | 确保已连接且参数有效 |
| `MQTT_TOOL_ERROR_INVALID_PARAM` | 参数无效 | 检查传入的参数是否正确 |
| `MQTT_TOOL_ERROR_NOT_INIT` | 未初始化 | 先调用 `mqtt_tool_init()` |

### 错误处理示例

```c
uint8_t result = mqtt_tool_publish("test/topic", "message", 1);
switch (result) {
    case MQTT_TOOL_SUCCESS:
        ESP_LOGI(TAG, "Message published successfully");
        break;
    case MQTT_TOOL_ERROR_NOT_INIT:
        ESP_LOGE(TAG, "MQTT tool not initialized");
        mqtt_tool_init();
        break;
    case MQTT_TOOL_ERROR_PUBLISH:
        ESP_LOGE(TAG, "Failed to publish message");
        // 重试或检查连接状态
        break;
    default:
        ESP_LOGE(TAG, "Unknown error: %d", result);
        break;
}
```

## 🔧 故障排除

### 常见问题

**Q: 连接超时怎么办？**
A: 
- 检查WiFi连接是否正常
- 验证MQTT代理服务器地址和端口
- 检查防火墙设置
- 尝试使用其他MQTT代理进行测试

**Q: 消息发布失败？**
A:
- 确保已经连接到MQTT代理
- 检查主题名称是否有效
- 验证QoS等级设置
- 检查消息内容是否过大

**Q: 无法接收订阅的消息？**
A:
- 确认订阅操作成功
- 检查主题匹配是否正确
- 验证QoS设置
- 查看事件处理函数是否正确实现

### 调试技巧

1. **启用详细日志:**
```c
esp_log_level_set("mqtt_tool", ESP_LOG_DEBUG);
```

2. **检查连接状态:**
```c
mqtt_tool_state_t state = mqtt_tool_get_state();
ESP_LOGI(TAG, "Current state: %d", state);
```

3. **使用MQTT客户端工具测试:** 推荐使用MQTT Explorer或mosquitto_pub/sub进行测试

## 🏗️ 项目结构

```
mqtt_tool/
├── include/
│   └── mqtt_tool.h          # 头文件
├── mqtt_tool.c              # 实现文件
├── CMakeLists.txt           # 构建配置
└── README.md               # 本文档
```

## 🤝 贡献

欢迎贡献代码！请遵循以下步骤：

1. Fork 本项目
2. 创建您的特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交您的更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开一个 Pull Request

### 代码规范

- 使用Doxygen风格的注释
- 遵循ESP-IDF编码规范
- 添加适当的错误处理
- 包含单元测试（如果适用）

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 📞 支持

如果您遇到问题或有建议，请：

1. 查看[故障排除](#故障排除)部分
2. 在GitHub上创建Issue
3. 查看ESP-IDF官方文档

---

**作者:** HonestLiu  
**版本:** 1.0  
**最后更新:** 2025-07-23

**⭐ 如果这个项目对您有帮助，请给个星标！**
