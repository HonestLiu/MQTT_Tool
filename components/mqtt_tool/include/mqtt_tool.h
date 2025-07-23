#ifndef _MQTT_TOOL_H_
#define _MQTT_TOOL_H_
#include <stdint.h>
#include "mqtt_client.h"

uint8_t mqtt_tool_init(void);
uint8_t mqtt_tool_deinit(void);
uint8_t mqtt_tool_connect(void);
uint8_t mqtt_tool_disconnect(void);
uint8_t mqtt_tool_publish(const char* topic, const char* message, int qos);
uint8_t mqtt_tool_subscribe(const char* topic, int qos);
uint8_t mqtt_tool_unsubscribe(const char* topic);

#endif