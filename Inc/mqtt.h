#ifndef __MQTT_H
#define __MQTT_H
#include "main.h"

#define MQTT_BUF_SIZE         1024

uint8_t MQTT_Connect(const char *client_id, const char *username, const char *password);
uint8_t MQTT_Disconnect(void);
uint8_t MQTT_Subscribe(const char *topic);
uint8_t MQTT_Unsubscribe(const char *topic);
uint8_t MQTT_Publish(const char *topic, const char *payload, uint8_t qos);
uint8_t MQTT_PublishSensorData(SensorData_t *data);
uint8_t MQTT_PublishAlarm(const char *alarm_type, const char *message);
uint8_t MQTT_ProcessIncoming(void);

#endif
