#ifndef __ALARM_H
#define __ALARM_H
#include "main.h"

#define BUZZER_PORT           GPIOA
#define BUZZER_PIN            GPIO_PIN_1
#define LED_R_PORT            GPIOB
#define LED_R_PIN             GPIO_PIN_0
#define LED_G_PORT            GPIOB
#define LED_G_PIN             GPIO_PIN_1
#define LED_B_PORT            GPIOB
#define LED_B_PIN             GPIO_PIN_2

#define ALARM_NONE            0
#define ALARM_TEMP_HIGH       1
#define ALARM_TEMP_LOW        2
#define ALARM_HUM_HIGH        3
#define ALARM_ACCEL           4
#define ALARM_GPS_FENCE       5

void Alarm_Init(void);
void Alarm_Check(SensorData_t *data);
void Alarm_Trigger(uint8_t type);
void Alarm_Clear(void);
void Alarm_SetLED(uint8_t r, uint8_t g, uint8_t b);
void Alarm_Beep(uint16_t duration_ms);

#endif
