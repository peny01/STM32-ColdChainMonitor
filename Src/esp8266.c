#include "esp8266.h"

/* 全局接收缓冲区 */
volatile uint8_t esp_rx_buf[ESP_RX_BUF_SIZE];
volatile uint16_t esp_rx_len = 0;
volatile uint8_t esp_data_ready = 0;

static uint8_t wifi_connected = 0;
static uint8_t tcp_connected = 0;

/* 内部：发送AT指令并等待响应 */
static uint8_t ESP8266_SendCmd(const char *cmd, uint32_t timeout_ms)
{
    esp_rx_len = 0;
    esp_data_ready = 0;

    /* 发送AT指令 */
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), ESP_TX_TIMEOUT);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, ESP_TX_TIMEOUT);

    /* 等待响应 */
    uint32_t tick_start = HAL_GetTick();
    while (HAL_GetTick() - tick_start < timeout_ms)
    {
        if (esp_data_ready)
        {
            /* 检查是否收到"OK" */
            if (strstr((const char *)esp_rx_buf, "OK") != NULL)
                return 0;   /* 成功 */
            /* 检查是否收到"ERROR"或"FAIL" */
            if (strstr((const char *)esp_rx_buf, "ERROR") != NULL ||
                strstr((const char *)esp_rx_buf, "FAIL") != NULL)
                return 2;   /* 指令执行失败 */
        }
        delay_ms(10);
    }

    return 1;   /* 超时 */
}

/* ESP8266初始化 */
uint8_t ESP8266_Init(void)
{
    uint8_t retry = 3;

    /* 先复位模块 */
    ESP8266_Reset();
    HAL_Delay(1000);

    /* AT测试 */
    while (retry--)
    {
        if (ESP8266_SendCmd("AT", ESP_AT_TIMEOUT) == 0)
            break;
    }
    if (retry == 0) return 1;

    HAL_Delay(200);

    /* 设置Station模式 */
    if (ESP8266_SendCmd("AT+CWMODE=1", ESP_AT_TIMEOUT) != 0)
        return 2;

    return 0;
}

/* 连接WiFi */
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", ssid, password);

    /* WiFi连接可能需要较长时间 */
    if (ESP8266_SendCmd(cmd, 10000) != 0)
        return 1;

    wifi_connected = 1;
    return 0;
}

/* 建立TCP连接 */
uint8_t ESP8266_TCP_Connect(const char *ip, uint16_t port)
{
    char cmd[128];

    /* 先关闭已有连接 */
    if (tcp_connected)
        ESP8266_CloseTCP();

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u", ip, port);

    if (ESP8266_SendCmd(cmd, 8000) != 0)
        return 1;

    tcp_connected = 1;
    return 0;
}

/* 发送数据（AT+CIPSEND模式） */
uint8_t ESP8266_SendData(const uint8_t *data, uint16_t len)
{
    char cmd[32];

    esp_rx_len = 0;
    esp_data_ready = 0;

    /* 发送AT+CIPSEND=len */
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u", len);
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), ESP_TX_TIMEOUT);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, ESP_TX_TIMEOUT);

    /* 等待">"提示符 */
    uint32_t tick_start = HAL_GetTick();
    while (HAL_GetTick() - tick_start < 3000)
    {
        if (esp_data_ready)
        {
            if (strstr((const char *)esp_rx_buf, ">") != NULL)
                break;
        }
        delay_ms(10);
    }

    if (!esp_data_ready)
        return 1;

    /* 发送实际数据 */
    esp_rx_len = 0;
    esp_data_ready = 0;
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, ESP_TX_TIMEOUT);

    /* 等待"SEND OK" */
    tick_start = HAL_GetTick();
    while (HAL_GetTick() - tick_start < 5000)
    {
        if (esp_data_ready)
        {
            if (strstr((const char *)esp_rx_buf, "SEND OK") != NULL)
                return 0;
        }
        delay_ms(10);
    }

    return 2;
}

/* 关闭TCP连接 */
uint8_t ESP8266_CloseTCP(void)
{
    if (ESP8266_SendCmd("AT+CIPCLOSE", 3000) != 0)
        return 1;

    tcp_connected = 0;
    return 0;
}

/* 复位模块 */
uint8_t ESP8266_Reset(void)
{
    ESP8266_SendCmd("AT+RST", 3000);
    wifi_connected = 0;
    tcp_connected = 0;
    return 0;
}

/* 检查连接状态 */
uint8_t ESP8266_IsConnected(void)
{
    return (wifi_connected && tcp_connected) ? 1 : 0;
}
