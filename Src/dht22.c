#include "dht22.h"

#define DHT22_TIMEOUT_US  200

/* Helper: wait while pin == state, with timeout; returns 0=ok, 1=timeout */
static uint8_t DHT22_WaitWhile(GPIO_PinState state, uint16_t timeout_us)
{
    while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == state) {
        if (--timeout_us == 0)
            return 1;
        delay_us(1);
    }
    return 0;
}

/* DHT22 init: GPIO open-drain output */
uint8_t DHT22_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
    return 0;
}

/* Read one byte with timeout; returns 0=ok, 2=low timeout, 3=high timeout */
static uint8_t DHT22_ReadByte(uint8_t *value)
{
    uint8_t val = 0;

    for (int i = 0; i < 8; i++) {
        val <<= 1;

        /* Wait for pin to go high (end of start-bit low) */
        if (DHT22_WaitWhile(GPIO_PIN_RESET, DHT22_TIMEOUT_US))
            return 2;

        /* Wait 40us then sample */
        delay_us(40);

        if (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET)
            val |= 1;

        /* Wait for pin to go low (end of data-bit high) */
        if (DHT22_WaitWhile(GPIO_PIN_SET, DHT22_TIMEOUT_US))
            return 3;
    }

    *value = val;
    return 0;
}

uint8_t DHT22_ReadData(float *temperature, float *humidity)
{
    uint8_t buf[5] = {0};
    uint8_t timeout;
    if (temperature == NULL || humidity == NULL) return 1;

    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_RESET);
    delay_us(20000);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
    delay_us(40);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);

    timeout = 100;
    while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_SET) {
        if (--timeout == 0) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
            HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
            return 2;
        }
        delay_us(2);
    }

    timeout = 100;
    while (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_GPIO_PIN) == GPIO_PIN_RESET) {
        if (--timeout == 0) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
            HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
            return 3;
        }
        delay_us(2);
    }

    for (int i = 0; i < 5; i++) {
        if (DHT22_ReadByte(&buf[i]) != 0) {
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
            HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);
            return 5;
        }
    }

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);

    uint8_t checksum = buf[0] + buf[1] + buf[2] + buf[3];
    if (checksum != buf[4]) return 4;

    uint16_t raw_hum = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t raw_temp = ((uint16_t)buf[2] << 8) | buf[3];
    *humidity = raw_hum * 0.1f;

    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        *temperature = -(raw_temp * 0.1f);
    } else {
        *temperature = raw_temp * 0.1f;
    }
    return 0;
}
