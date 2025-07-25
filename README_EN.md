# ESP32 MQTT Tool with LCD Display

An ESP32-S3 based MQTT client tool featuring LCD touchscreen interface, supporting MQTT message publishing, subscription and real-time display functionality.

<img src="./img/image-20250725093126776.png" style="zoom: 25%;" />

## Features

- 🖥️ **LCD Touchscreen Interface** - User-friendly interface based on LVGL
- 📡 **MQTT Client** - Support for message publishing, subscription and receiving
- 🔧 **Modular Design** - Component-based architecture, easy to extend and maintain
- 📱 **Real-time Message Display** - Live display of received MQTT messages
- ⚡ **High Performance** - Multi-task processing based on FreeRTOS
- 🛠️ **Complete Error Handling** - Includes connection failure, reconnection control and other error handling mechanisms

## Hardware Requirements

- **MCU**: ESP32-S3
- **Display**: SPI interface LCD display (320x240)
- **Touch Controller**: FT5x06 series touch controller
- **IO Expander**: PCA9557 I2C IO expander chip
- **Interfaces**: I2C, SPI, PWM

### Pin Configuration

| Function | Pin | Description |
|----------|-----|-------------|
| LCD SPI CLK | GPIO12 | LCD clock signal |
| LCD SPI MOSI | GPIO11 | LCD data output |
| LCD SPI CS | GPIO10 | LCD chip select |
| LCD DC | GPIO13 | LCD data/command select |
| LCD RST | GPIO14 | LCD reset |
| LCD Backlight | GPIO2 | LCD backlight PWM control |
| I2C SDA | GPIO19 | I2C data line |
| I2C SCL | GPIO20 | I2C clock line |

## Software Architecture

### Core Components

1. **mqtt_tool** - MQTT client tool library
   - Connection management
   - Message publishing/subscription
   - Error handling

2. **mqtt_ui** - MQTT user interface component
   - Connection interface
   - Message display interface
   - Subscribe/publish interface

3. **ui_interface** - User interface interface
   - Inter-task communication
   - Message queue management

4. **mqtt_message_display** - Message display module
   - Real-time message display
   - System status display

### Task Architecture

```
┌─────────────────┐    Message Queue   ┌──────────────────┐
│   GUI Task      │ <─────────────────→ │ Main Logic Task  │
│ (UI Processing) │                     │ (Business Logic) │
└─────────────────┘                     └──────────────────┘
         │                                       │
         ▼                                       ▼
┌─────────────────┐                     ┌──────────────────┐
│   LVGL UI       │                     │   MQTT Client    │
│ (UI Rendering)  │                     │ (Network Comm)   │
└─────────────────┘                     └──────────────────┘
```

## Development Environment

- **ESP-IDF**: v5.4.2 or higher
- **Compiler**: GCC for Xtensa ESP32-S3
- **Build System**: CMake
- **Dependency Management**: ESP Component Manager

### Main Dependencies

```yaml
dependencies:
  idf: ">=4.1.0"
  espressif/esp_lcd_touch_ft5x06: "==1.0.0"
  lvgl/lvgl: "^8.3.0"
  espressif/esp_lvgl_port: "^1.4.0"
```

## Quick Start

### 1. Environment Setup

```bash
# Install ESP-IDF
git clone -b v5.4.2 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
source ./export.sh
```

### 2. Clone Project

```bash
git clone https://github.com/HonestLiu/MQTT_Tool.git
cd MQTT_Tool
```

### 3. Configure Project

```bash
# Set target chip
idf.py set-target esp32s3

# Configure project (optional)
idf.py menuconfig
```

### 4. Build and Flash

```bash
# Build project
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash

# View logs
idf.py -p /dev/ttyUSB0 monitor
```

## Usage Instructions

### MQTT Connection

1. **Set MQTT Server**
   - Enter MQTT broker address in the interface
   - Set client ID (optional)
   - Enter username and password (if required)

2. **Connect to Server**
   - Click "Connect" button
   - Button turns green and shows "Connected" status when successful

### Message Subscription

1. Enter topic name in subscription interface
2. Select QoS level (0, 1, 2)
3. Click "Subscribe" button

### Message Publishing

1. Enter topic name in publish interface
2. Enter message content
3. Select QoS level
4. Click "Publish" button

### Message Viewing

- Received messages are displayed in real-time in the message list
- Display content includes: topic, content, QoS level, timestamp
- System status messages are distinguished by different colors

## Performance Optimization

### Boot Speed Optimization

Boot speed and UI display optimization is a common requirement. Here are optimization recommendations for this project:

#### 1. Parallel Initialization Optimization

**Problem**: All hardware initialization is performed serially in one task, causing long startup times.

**Solution**: Split initialization into critical and non-critical paths, execute in parallel:

```c
// Optimized initialization strategy
void app_main(void) {
    // 1. Immediately start LCD display basic interface
    bsp_i2c_init();
    pca9557_init();
    bsp_lvgl_start(&io_handle, &panel_handle);
    
    // Display splash screen
    lv_obj_t *splash = lv_label_create(lv_scr_act());
    lv_label_set_text(splash, "Loading...");
    lv_obj_center(splash);
    
    // 2. Initialize other components in parallel
    xTaskCreate(wifi_init_task, "wifi_init", 4096, NULL, 3, NULL);
    xTaskCreate(ui_init_task, "ui_init", 8192, NULL, 4, NULL);
}
```

#### 2. WiFi Connection Optimization

**Problem**: WiFi connection usually takes 3-10 seconds, blocking interface display.

**Solution**: 
- Move WiFi initialization to background task
- Use WiFi fast connect feature
- Display connection progress

```c
// Add to sdkconfig:
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP32_WIFI_FAST_CONNECT=y
```

#### 3. LVGL Buffer Optimization

**Problem**: Current use of SPIRAM as display buffer is slow.

**Solution**: 
- Enable double buffering
- Optimize buffer size
- Use DMA transfer

```c
const lvgl_port_display_cfg_t disp_cfg = {
    .buffer_size = BSP_LCD_H_RES * 40,  // Reduce buffer size to reduce memory allocation time
    .double_buffer = true,              // Enable double buffering for smoother performance
    .flags = {
        .buff_dma = true,              // Use DMA for faster transfer
        .buff_spiram = false,          // Prefer internal RAM
    }
};
```

#### 4. Compilation Optimization

Enable the following optimization options in `sdkconfig`:

```
# Compilation optimization
CONFIG_COMPILER_OPTIMIZATION_SIZE=n
CONFIG_COMPILER_OPTIMIZATION_PERF=y

# Boot optimization  
CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP=y
CONFIG_BOOTLOADER_FAST_BOOT=y

# Memory optimization
CONFIG_SPIRAM_BOOT_INIT=y
CONFIG_SPIRAM_ALLOW_BSS_SEG_EXTERNAL_MEMORY=y

# WiFi optimization
CONFIG_ESP32_WIFI_STATIC_RX_BUFFER_NUM=16
CONFIG_ESP32_WIFI_DYNAMIC_RX_BUFFER_NUM=32
CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED=y
CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED=y
```

Through these optimizations, boot time can be reduced from 5-10 seconds to 1-3 seconds, with significantly improved UI response time.

## API Reference

### MQTT Tool API

```c
// Initialize MQTT tool
uint8_t mqtt_tool_init(mqtt_tool_handle_t* handle);

// Set broker address
uint8_t mqtt_tool_set_broker_uri(mqtt_tool_handle_t* handle, const char* uri);

// Set client ID
uint8_t mqtt_tool_set_client_id(mqtt_tool_handle_t* handle, const char* client_id);

// Connect to MQTT server
uint8_t mqtt_tool_connect(mqtt_tool_handle_t* handle);

// Publish message
uint8_t mqtt_tool_publish(mqtt_tool_handle_t* handle, const char* topic, 
                         const char* payload, int qos);

// Subscribe to topic
uint8_t mqtt_tool_subscribe(mqtt_tool_handle_t* handle, const char* topic, int qos);

// Disconnect
uint8_t mqtt_tool_disconnect(mqtt_tool_handle_t* handle);

// Clean up resources
uint8_t mqtt_tool_deinit(mqtt_tool_handle_t* handle);
```

### Return Values

- `MQTT_TOOL_SUCCESS` (0) - Operation successful
- `MQTT_TOOL_ERROR_INVALID_PARAM` (1) - Invalid parameter
- `MQTT_TOOL_ERROR_NOT_INITIALIZED` (2) - Not initialized
- `MQTT_TOOL_ERROR_INIT` (3) - Initialization failed
- `MQTT_TOOL_ERROR_CONNECT` (4) - Connection failed

## Configuration Options

### WiFi Configuration

Configure WiFi connection in `menuconfig`:

```
Example Configuration ->
    WiFi SSID
    WiFi Password
```

### MQTT Default Configuration

```c
#define MQTT_TOOL_DEFAULT_BROKER_URI "mqtt://mqtt.eclipseprojects.io"
#define MQTT_TOOL_DEFAULT_CLIENT_ID "esp32_mqtt_client"
#define MQTT_TOOL_DEFAULT_KEEPALIVE 60
```

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   ```
   Solution: Check ESP-IDF version is correct, ensure all dependencies are installed
   ```

2. **LCD Display Issues**
   ```
   Solution: Check SPI wiring, confirm LCD model configuration is correct
   ```

3. **MQTT Connection Failure**
   ```
   Solutions:
   - Check WiFi connection status
   - Verify MQTT broker address format
   - Confirm firewall settings
   ```

4. **Touch Not Responding**
   ```
   Solution: Check I2C wiring, confirm touch controller address configuration
   ```

### Debugging Tips

1. **Enable Verbose Logging**
   ```c
   esp_log_level_set("*", ESP_LOG_VERBOSE);
   ```

2. **Monitor Memory Usage**
   ```c
   ESP_LOGI(TAG, "Free heap: %d", esp_get_free_heap_size());
   ```

3. **Check Task Status**
   ```bash
   # In ESP-IDF monitor
   esp32> tasks
   ```

## Contributing

1. Fork this project
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## Version History

- **v1.0.0** - Initial release
  - Basic MQTT functionality
  - LCD interface display
  - Touch interaction

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

## Contact

- **Author**: HonestLiu
- **Email**: your.email@example.com
- **Project URL**: https://github.com/HonestLiu/MQTT_Tool

## Acknowledgments

- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif IoT Development Framework
- [LVGL](https://github.com/lvgl/lvgl) - Light and Versatile Graphics Library
- [ESP-MQTT](https://github.com/espressif/esp-mqtt) - ESP32 MQTT Client Library

---

If this project helps you, please give it a ⭐ Star!
