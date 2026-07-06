#include "main.h"

void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_Init = {0};
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_Init.Pin = GPIO_PIN_9; GPIO_Init.Mode = GPIO_MODE_AF_PP; GPIO_Init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_Init);
        GPIO_Init.Pin = GPIO_PIN_10; GPIO_Init.Mode = GPIO_MODE_INPUT; GPIO_Init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_Init);
        HAL_NVIC_SetPriority(USART1_IRQn, 6, 0); HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef GPIO_Init = {0};
    if (hspi->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /* TFT ST7735R: PB3=SPI1_SCK, PA7=SPI1_MOSI (K8 32-pin no PB5) */
        GPIO_Init.Pin = GPIO_PIN_3;
        GPIO_Init.Mode = GPIO_MODE_AF_PP;
        GPIO_Init.Pull = GPIO_NOPULL;
        GPIO_Init.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_Init.Alternate = GPIO_AF0_SPI1;
        HAL_GPIO_Init(GPIOB, &GPIO_Init);
        GPIO_Init.Pin = GPIO_PIN_7;
        HAL_GPIO_Init(GPIOA, &GPIO_Init);
    }
}

/* I2C1 removed - MPU6050 uses software I2C on PA2/PA3 */

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
        HAL_NVIC_SetPriority(TIM2_IRQn, 5, 0); HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
}
