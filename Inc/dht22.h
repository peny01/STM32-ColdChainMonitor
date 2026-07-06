#ifndef __DHT22_H
#define __DHT22_H
#include "main.h"

#define DHT22_GPIO_PORT       GPIOA
#define DHT22_GPIO_PIN        GPIO_PIN_5

uint8_t DHT22_Init(void);
uint8_t DHT22_ReadData(float *temperature, float *humidity);

#endif
