/**
  * HAL Configuration File for ColdChainMonitor (STM32F051K8)
  */
#ifndef __STM32F0xx_HAL_CONF_H

/* CMSIS device header - defines basic types */
#include "stm32f0xx.h"
#include "stm32f0xx_hal_def.h"
#define __STM32F0xx_HAL_CONF_H

#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_SPI_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_USART_MODULE_ENABLED

#define HSE_VALUE    ((uint32_t)8000000)
#define HSI_VALUE    ((uint32_t)8000000)
#define LSE_VALUE    ((uint32_t)32768)
#define LSI_VALUE    ((uint32_t)40000)

#define TICK_INT_PRIORITY            0x00
#define USE_RTOS                     0
#define PREFETCH_ENABLE              1
#define USE_HAL_UART_REGISTER_CALLBACKS 0

#include "stm32f0xx_hal_rcc.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_dma.h"
#include "stm32f0xx_hal_cortex.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_i2c.h"
#include "stm32f0xx_hal_pwr.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_hal_usart.h"/* Timeout values */
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000)

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif /* __STM32F0xx_HAL_CONF_H */
