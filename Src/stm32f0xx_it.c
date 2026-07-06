#include "main.h"
#include "esp8266.h"

void NMI_Handler(void) {}
void HardFault_Handler(void) { while(1); }
void SVC_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) { HAL_IncTick(); }

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim2;

void TIM2_IRQHandler(void) { HAL_TIM_IRQHandler(&htim2); }

void USART1_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)(huart1.Instance->RDR & 0xFF);
        if (esp_rx_len < ESP_RX_BUF_SIZE - 1) {
            esp_rx_buf[esp_rx_len++] = byte;
            if (byte == '\n') {
                esp_rx_buf[esp_rx_len] = '\0';
                esp_data_ready = 1;
            }
        }
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
}

void USART2_IRQHandler(void) { /* USART2 not used on F051K8 */ }