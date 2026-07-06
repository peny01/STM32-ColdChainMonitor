#include "mpu6050.h"

/* Software I2C delay (~5us for ~100kHz) */
static void i2c_delay(void) { delay_us(5); }

static void i2c_start(void)
{
    MPU_SDA_H; MPU_SCL_H; i2c_delay();
    MPU_SDA_L; i2c_delay();
    MPU_SCL_L; i2c_delay();
}

static void i2c_stop(void)
{
    MPU_SDA_L; MPU_SCL_H; i2c_delay();
    MPU_SDA_H; i2c_delay();
}

static uint8_t i2c_wait_ack(void)
{
    uint8_t ack;
    MPU_SDA_H; i2c_delay();
    MPU_SCL_H; i2c_delay();
    ack = MPU_SDA_IN;
    MPU_SCL_L; i2c_delay();
    return ack;
}

static void i2c_send_ack(uint8_t ack)
{
    if (ack) MPU_SDA_H; else MPU_SDA_L;
    i2c_delay();
    MPU_SCL_H; i2c_delay();
    MPU_SCL_L; i2c_delay();
}

static void i2c_write_byte(uint8_t data)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (data & 0x80) MPU_SDA_H; else MPU_SDA_L;
        i2c_delay();
        MPU_SCL_H; i2c_delay();
        MPU_SCL_L; i2c_delay();
        data <<= 1;
    }
}

static uint8_t i2c_read_byte(uint8_t ack)
{
    uint8_t data = 0;
    int i;
    MPU_SDA_H;
    for (i = 0; i < 8; i++) {
        data <<= 1;
        MPU_SCL_H; i2c_delay();
        if (MPU_SDA_IN) data |= 1;
        MPU_SCL_L; i2c_delay();
    }
    i2c_send_ack(ack);
    return data;
}

static uint8_t mpu_write_reg(uint8_t reg, uint8_t val)
{
    i2c_start();
    i2c_write_byte(MPU6050_ADDR_WRITE);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    i2c_write_byte(reg);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    i2c_write_byte(val);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    i2c_stop();
    return 0;
}

static uint8_t mpu_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    i2c_start();
    i2c_write_byte(MPU6050_ADDR_WRITE);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    i2c_write_byte(reg);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    i2c_start();
    i2c_write_byte(MPU6050_ADDR_READ);
    if (i2c_wait_ack()) { i2c_stop(); return 1; }
    for (i = 0; i < len; i++) {
        buf[i] = i2c_read_byte(i < (len - 1) ? 0 : 1);
    }
    i2c_stop();
    return 0;
}

uint8_t MPU6050_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = MPU_SCL_PIN | MPU_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    MPU_SCL_H; MPU_SDA_H;
    delay_ms(10);

    if (mpu_write_reg(MPU6050_REG_PWR_MGMT, 0x00)) return 1;
    delay_ms(10);

    if (mpu_write_reg(MPU6050_REG_ACCEL_CFG, 0x00)) return 1;

    return 0;
}

uint8_t MPU6050_ReadAccel(float *ax, float *ay, float *az)
{
    uint8_t buf[6];
    int16_t raw_ax, raw_ay, raw_az;

    if (ax == NULL || ay == NULL || az == NULL) return 1;
    if (mpu_read_reg(MPU6050_REG_ACCEL_X, buf, 6)) return 2;

    raw_ax = ((int16_t)buf[0] << 8) | buf[1];
    raw_ay = ((int16_t)buf[2] << 8) | buf[3];
    raw_az = ((int16_t)buf[4] << 8) | buf[5];

    *ax = raw_ax / 16384.0f;
    *ay = raw_ay / 16384.0f;
    *az = raw_az / 16384.0f;
    return 0;
}

uint8_t MPU6050_ReadTemperature(float *temp)
{
    uint8_t buf[2];
    int16_t raw;
    if (temp == NULL) return 1;
    if (mpu_read_reg(0x41, buf, 2)) return 2;
    raw = ((int16_t)buf[0] << 8) | buf[1];
    *temp = raw / 340.0f + 36.53f;
    return 0;
}
