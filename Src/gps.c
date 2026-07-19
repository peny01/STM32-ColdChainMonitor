#include "gps.h"

/* ========== Software UART (RX-only) for GPS @ 9600 baud ========== */
/* GPS TX -> MCU PA0 (EXTI line 0, falling edge triggers start bit detect) */

#define GPS_SW_RX_PORT       GPIOA
#define GPS_SW_RX_PIN        GPIO_PIN_0
#define GPS_BIT_US           104    /* 9600 baud: ~104us per bit */

volatile uint8_t gps_rx_buf[GPS_RX_BUF_SIZE];
volatile uint16_t gps_rx_len = 0;

static uint8_t parse_buf[GPS_RX_BUF_SIZE];

/* Simple busy-wait microsecond delay (48MHz: ~12 cycles/us) */
static void gps_delay_us(uint32_t us)
{
    uint32_t count = us * (SYSTEM_CLOCK_Hz / 1000000) / 4;
    while (count--) { __NOP(); }
}

/* Read one byte via software UART at 9600 baud (blocking) */
static uint8_t SWUART_ReadByte(void)
{
    uint8_t data = 0;

    /* Wait for start bit (falling edge) - already detected by EXTI */
    /* Wait 1.5 bit periods to sample at center of first data bit */
    gps_delay_us(GPS_BIT_US + GPS_BIT_US / 2);

    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (HAL_GPIO_ReadPin(GPS_SW_RX_PORT, GPS_SW_RX_PIN) == GPIO_PIN_SET)
            data |= 0x80;
        gps_delay_us(GPS_BIT_US);
    }

    /* Wait for stop bit (ignore it) */
    gps_delay_us(GPS_BIT_US);

    return data;
}

/* Software UART read line (up to max_len bytes, ends with \n) */
static uint16_t SWUART_ReadLine(uint8_t *buf, uint16_t max_len, uint32_t timeout_ms)
{
    uint16_t idx = 0;
    uint32_t start = HAL_GetTick();

    while (HAL_GetTick() - start < timeout_ms) {
        /* Wait for falling edge (start bit) */
        uint32_t pin_wait = HAL_GetTick();
        while (HAL_GPIO_ReadPin(GPS_SW_RX_PORT, GPS_SW_RX_PIN) == GPIO_PIN_SET) {
            if (HAL_GetTick() - pin_wait > 10)  /* 10ms timeout per byte */
                goto done;
        }

        uint8_t byte = SWUART_ReadByte();
        if (idx < max_len - 1) {
            buf[idx++] = byte;
            if (byte == '\n')
                break;
        }
    }
done:
    buf[idx] = '\0';
    return idx;
}

/* GPS Init: configure PA0 as input with falling edge interrupt */
uint8_t GPS_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPS_SW_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPS_SW_RX_PORT, &GPIO_InitStruct);

    gps_rx_len = 0;
    return 0;
}

void GPS_ClearBuffer(void)
{
    gps_rx_len = 0;
}

/* Extract NMEA field by index (0-based) */
static uint8_t GetField(const char *start, uint8_t field_num, char *out, uint8_t max_len)
{
    const char *p = start;
    uint8_t cur = 0, idx = 0;

    while (*p && cur < field_num) {
        if (*p == ',') cur++;
        p++;
    }
    if (*p == '\0' || *p == '\r' || *p == '\n')
        return 1;

    while (*p && *p != ',' && *p != '\r' && *p != '\n' && *p != '*' && idx < max_len - 1)
        out[idx++] = *p++;
    out[idx] = '\0';
    return 0;
}

/* Parse $GPGGA sentence */
static uint8_t ParseGPGGA(const char *buf)
{
    char field[16];

    if (GetField(buf, 2, field, sizeof(field))) return 1;
    float lat_raw = (float)atof(field);
    int32_t lat_deg = (int32_t)(lat_raw / 100.0f);
    float lat_min = lat_raw - lat_deg * 100.0f;
    g_sensor.latitude = lat_deg + lat_min / 60.0f;

    if (GetField(buf, 3, field, sizeof(field)) == 0)
        if (field[0] == 'S' || field[0] == 's')
            g_sensor.latitude = -g_sensor.latitude;

    if (GetField(buf, 4, field, sizeof(field))) return 1;
    float lon_raw = (float)atof(field);
    int32_t lon_deg = (int32_t)(lon_raw / 100.0f);
    float lon_min = lon_raw - lon_deg * 100.0f;
    g_sensor.longitude = lon_deg + lon_min / 60.0f;

    if (GetField(buf, 5, field, sizeof(field)) == 0)
        if (field[0] == 'W' || field[0] == 'w')
            g_sensor.longitude = -g_sensor.longitude;

    if (GetField(buf, 6, field, sizeof(field)) == 0)
        g_sensor.gps_fix = (uint8_t)atoi(field);

    if (GetField(buf, 7, field, sizeof(field)) == 0)
        g_sensor.gps_satellites = (uint8_t)atoi(field);

    /* Reject invalid fix: quality 0 means no GPS lock */
    if (g_sensor.gps_fix == 0) {
        g_sensor.latitude = 0;
        g_sensor.longitude = 0;
        g_sensor.altitude = 0;
        return 1;
    }

    if (GetField(buf, 9, field, sizeof(field)) == 0)
        g_sensor.altitude = (float)atof(field);

    return 0;
}

/* Parse GPS buffer for NMEA sentences */
uint8_t GPS_ParseBuffer(void)
{
    uint16_t len;
    uint16_t start = 0;

    len = SWUART_ReadLine(parse_buf, GPS_RX_BUF_SIZE, 1000);
    if (len == 0) return 1;

    gps_rx_len = len;
    memcpy((void *)gps_rx_buf, parse_buf, len);

    while (start < len) {
        if (parse_buf[start] == '$' &&
            strncmp((const char *)&parse_buf[start], "$GPGGA", 6) == 0) {
            return ParseGPGGA((const char *)&parse_buf[start]);
        }
        start++;
    }
    return 2;
}
