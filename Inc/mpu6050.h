#ifndef __MPU6050_H
#define __MPU6050_H
#include "main.h"

#define MPU6050_ADDR          0x68
#define MPU6050_ADDR_WRITE    (MPU6050_ADDR << 1)
#define MPU6050_ADDR_READ     ((MPU6050_ADDR << 1) | 0x01)


/* Software I2C pins (K8 32-pin: no PB6, use PA2=SCL PA3=SDA) */
#define MPU_SCL_PORT          GPIOA
#define MPU_SCL_PIN           GPIO_PIN_2
#define MPU_SDA_PORT          GPIOA
#define MPU_SDA_PIN           GPIO_PIN_3

#define MPU_SCL_H  HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_SET)
#define MPU_SCL_L  HAL_GPIO_WritePin(MPU_SCL_PORT, MPU_SCL_PIN, GPIO_PIN_RESET)
#define MPU_SDA_H  HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_PIN, GPIO_PIN_SET)
#define MPU_SDA_L  HAL_GPIO_WritePin(MPU_SDA_PORT, MPU_SDA_PIN, GPIO_PIN_RESET)
#define MPU_SDA_IN HAL_GPIO_ReadPin(MPU_SDA_PORT, MPU_SDA_PIN)

#define MPU6050_REG_ACCEL_X   0x3B
#define MPU6050_REG_PWR_MGMT  0x6B
#define MPU6050_REG_CONFIG    0x1A
#define MPU6050_REG_GYRO_CFG  0x1B
#define MPU6050_REG_ACCEL_CFG 0x1C

uint8_t MPU6050_Init(void);
uint8_t MPU6050_ReadAccel(float *ax, float *ay, float *az);
uint8_t MPU6050_ReadTemperature(float *temp);

#endif
