#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f0xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lcd.h"

/* LCD display update */
void LCD_UpdateDisplay(void);

/* ?????? */
#define SYSTEM_CLOCK_Hz       48000000
#define USART1_BAUDRATE       115200

/* ????(?) */
#define SAMPLE_INTERVAL_DEF   10
#define SAMPLE_INTERVAL_MIN   1
#define SAMPLE_INTERVAL_MAX   300

/* ??????? */
#define TEMP_ALARM_HIGH_DEF   25.0f
#define TEMP_ALARM_LOW_DEF    -5.0f
#define HUM_ALARM_HIGH_DEF    85.0f
#define ACCEL_ALARM_THRESH    2.0f

/* ???????? */
typedef struct {
    float temperature;
    float humidity;
    float accel_x;
    float accel_y;
    float accel_z;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    uint8_t gps_fix;
    uint8_t gps_satellites;
    uint32_t timestamp;
} SensorData_t;

/* ??????? */
typedef struct {
    uint8_t wifi_connected;
    uint8_t mqtt_connected;
    uint8_t gps_fixed;
    uint8_t alarm_active;
    uint16_t sample_interval;
    float temp_alarm_high;
    float temp_alarm_low;
    float hum_alarm_high;
    float accel_thresh;
    uint32_t uptime;
    uint32_t packet_count;
} SystemState_t;

extern SensorData_t g_sensor;
extern SystemState_t g_state;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi1;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init_ESP(void);
void MX_SPI1_Init(void);
void Error_Handler(void);
void delay_us(uint16_t us);
void delay_ms(uint32_t ms);

#endif




