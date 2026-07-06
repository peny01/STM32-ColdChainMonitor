#include "main.h"
#include "dht22.h"
#include "mpu6050.h"
#include "gps.h"
#include "esp8266.h"
#include "mqtt.h"
#include "alarm.h"
#include "lcd.h"

SensorData_t g_sensor = {0};
SystemState_t g_state = {0};

UART_HandleTypeDef huart1;
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

static void MX_IWDG_Init(void);

static uint32_t last_sample_tick = 0;

#define WIFI_SSID    "YourWiFi"
#define WIFI_PASS    "YourPassword"

#define MQTT_CLIENT_ID    "ColdChainMonitor_001"
#define MQTT_BROKER_IP    "192.168.1.100"
#define MQTT_BROKER_PORT  1883

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
    RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
        Error_Handler();
}

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* DHT22 data - PA5 (open-drain with pullup, K8 32-pin no PB6) */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Buzzer - PA1 (moved from PB4) */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

    /* RGB LED - PB0/PB1/PB2 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

    /* TFT control pins are initialized in LCD_ControlGPIO_Init() -> lcd.c */
}

void MX_USART1_UART_Init_ESP(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
        Error_Handler();
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

/* SPI1 - TFT ST7735R (PB3=SCK, PB5=MOSI) */
void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
        Error_Handler();
}



static void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 72 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1000 - 1;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
        Error_Handler();
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
        Error_Handler();
}

uint32_t GetTick(void) { return HAL_GetTick(); }

void delay_ms(uint32_t ms) { HAL_Delay(ms); }

void delay_us(uint16_t us) {
    volatile uint16_t i;
    for (; us > 0; us--) {
        for (i = 0; i < 12; i++);
    }
}

void Error_Handler(void)
{
    Lcd_Clear(RED);
    while (1);
}

static void Base_Periph_Init(void)
{
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init_ESP();
    MX_SPI1_Init();
    MX_TIM2_Init();
}

static void System_Init(void)
{
    Base_Periph_Init();

    g_state.sample_interval = SAMPLE_INTERVAL_DEF;
    g_state.temp_alarm_high = TEMP_ALARM_HIGH_DEF;
    g_state.temp_alarm_low = TEMP_ALARM_LOW_DEF;
    g_state.hum_alarm_high = HUM_ALARM_HIGH_DEF;
    g_state.accel_thresh = ACCEL_ALARM_THRESH;

    DHT22_Init();
    MPU6050_Init();
    GPS_Init();
    Alarm_Init();

    /* TFT Init */
    Lcd_Init();
    Lcd_Clear(CYAN);
    Gui_DrawFont_GBK16(20, 20, RED, CYAN, (uint8_t*)"ColdChain");
    Gui_DrawFont_GBK16(20, 44, BLUE, CYAN, (uint8_t*)"Monitor v1.0");
    delay_ms(1500);
    Lcd_Clear(BLACK);

    MX_IWDG_Init();

    Alarm_SetLED(0, 1, 0);
    delay_ms(500);
    Alarm_SetLED(0, 0, 0);
}

static void Sample_Task(void)
{
    float temp = 0, hum = 0;
    float ax = 0, ay = 0, az = 0;

    if (DHT22_ReadData(&temp, &hum) == 0) {
        g_sensor.temperature = temp;
        g_sensor.humidity = hum;
    }

    if (MPU6050_ReadAccel(&ax, &ay, &az) == 0) {
        g_sensor.accel_x = ax;
        g_sensor.accel_y = ay;
        g_sensor.accel_z = az;
    }

    if (GPS_ParseBuffer() == 0) {
        g_state.gps_fixed = 1;
    }
}

static void MQTT_Task(void)
{
    char json_buf[256];
    snprintf(json_buf, sizeof(json_buf),
        "{\"t\":%.1f,\"h\":%.1f,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,"
        "\"lat\":%.4f,\"lon\":%.4f,\"alt\":%.1f,\"spd\":%.1f,"
        "\"fix\":%d,\"sat\":%d,\"ts\":%lu}",
        g_sensor.temperature, g_sensor.humidity,
        g_sensor.accel_x, g_sensor.accel_y, g_sensor.accel_z,
        g_sensor.latitude, g_sensor.longitude,
        g_sensor.altitude, g_sensor.speed,
        g_sensor.gps_fix, g_sensor.gps_satellites,
        g_state.uptime);

    if (MQTT_Publish("coldchain/data", json_buf, 1) == 0) {
        g_state.packet_count++;
    }
}

static void Alarm_Task(void)
{
    Alarm_Check(&g_sensor);
}

/* TFT display update - draws on 128x128 ST7735R color display */
void LCD_UpdateDisplay(void)
{
    char line_buf[24];
    uint16_t bg = BLACK;

    /* Clear top portion */
    Lcd_SetRegion(0, 0, 127, 95);
    Lcd_Clear(BLACK);

    if (g_state.alarm_active) {
        bg = RED;
    }

    /* Line 1: Temperature & Humidity */
    snprintf(line_buf, sizeof(line_buf), "T:%.1fC H:%.1f%%",
             g_sensor.temperature, g_sensor.humidity);
    Gui_DrawFont_GBK16(0, 0, GREEN, bg, (uint8_t*)line_buf);

    /* Line 2: Alarm status or uptime */
    if (g_state.alarm_active) {
        Gui_DrawFont_GBK16(0, 20, RED, YELLOW, (uint8_t*)"!! ALARM !!");
    } else if (g_state.wifi_connected && g_state.gps_fixed) {
        snprintf(line_buf, sizeof(line_buf), "WiFi+GPS %lus", g_state.uptime);
        Gui_DrawFont_GBK16(0, 20, CYAN, BLACK, (uint8_t*)line_buf);
    } else if (g_state.wifi_connected) {
        Gui_DrawFont_GBK16(0, 20, YELLOW, BLACK, (uint8_t*)"WiFi OK NoGPS");
    } else {
        snprintf(line_buf, sizeof(line_buf), "Uptime: %lus", g_state.uptime);
        Gui_DrawFont_GBK16(0, 20, GRAY1, BLACK, (uint8_t*)line_buf);
    }

    /* Line 3: GPS coordinates if fixed */
    if (g_state.gps_fixed) {
        snprintf(line_buf, sizeof(line_buf), "Lat:%.4f", g_sensor.latitude);
        Gui_DrawFont_GBK16(0, 40, BLUE, BLACK, (uint8_t*)line_buf);
        snprintf(line_buf, sizeof(line_buf), "Lon:%.4f", g_sensor.longitude);
        Gui_DrawFont_GBK16(0, 56, BLUE, BLACK, (uint8_t*)line_buf);
    }

    /* Line 4: Packet count */
    snprintf(line_buf, sizeof(line_buf), "Pkts:%lu", g_state.packet_count);
    Gui_DrawFont_GBK16(0, 76, WHITE, BLACK, (uint8_t*)line_buf);

    /* Status bar at bottom */
    Lcd_SetRegion(0, 112, 127, 127);
    if (g_state.alarm_active) {
        Lcd_Clear(RED);
    } else if (g_state.mqtt_connected) {
        Lcd_Clear(GREEN);
    } else {
        Lcd_Clear(GRAY1);
    }
}

static void MX_IWDG_Init(void)
{
    IWDG->KR = 0x5555;
    IWDG->PR = 1;
    IWDG->RLR = 4095;
    while (IWDG->SR);
    IWDG->KR = 0xCCCC;
}

int main(void)
{
    System_Init();

    g_state.wifi_connected = (ESP8266_Init() == 0) ? 1 : 0;
    if (g_state.wifi_connected) {
        g_state.wifi_connected = (ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASS) == 0) ? 1 : 0;
    }

    if (g_state.wifi_connected) {
        g_state.mqtt_connected = (MQTT_Connect(MQTT_CLIENT_ID, "", "") == 0) ? 1 : 0;
    }

    while (1)
    {
        uint32_t now = GetTick();

        if ((now - last_sample_tick) >= (g_state.sample_interval * 1000)) {
            last_sample_tick = now;
            Sample_Task();
            Alarm_Task();

            if (g_state.mqtt_connected) {
                MQTT_Task();
            }

            g_state.uptime += g_state.sample_interval;

            LCD_UpdateDisplay();

            Alarm_SetLED(0, 0, 1);
            delay_ms(50);
            Alarm_SetLED(0, 0, 0);
        }

        IWDG->KR = 0xAAAA;

        static uint32_t last_heartbeat = 0;
        if ((now - last_heartbeat) >= 1000) {
            last_heartbeat = now;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }
}