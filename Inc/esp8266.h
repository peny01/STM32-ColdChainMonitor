#ifndef __ESP8266_H
#define __ESP8266_H
#include "main.h"

#define ESP_RX_BUF_SIZE       512
#define ESP_TX_TIMEOUT        5000
#define ESP_AT_TIMEOUT        2000

uint8_t ESP8266_Init(void);
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password);
uint8_t ESP8266_TCP_Connect(const char *ip, uint16_t port);
uint8_t ESP8266_SendData(const uint8_t *data, uint16_t len);
uint8_t ESP8266_CloseTCP(void);
uint8_t ESP8266_Reset(void);
uint8_t ESP8266_IsConnected(void);

extern volatile uint8_t esp_rx_buf[ESP_RX_BUF_SIZE];
extern volatile uint16_t esp_rx_len;
extern volatile uint8_t esp_data_ready;

#endif
