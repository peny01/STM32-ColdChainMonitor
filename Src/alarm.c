#include "alarm.h"
#include "mqtt.h"

static uint8_t current_alarm = ALARM_NONE;
static const char *Alarm_TypeName(uint8_t type)
{
    switch (type)
    {
        case ALARM_TEMP_HIGH: return "TEMP_HIGH";
        case ALARM_TEMP_LOW:  return "TEMP_LOW";
        case ALARM_HUM_HIGH:  return "HUM_HIGH";
        case ALARM_ACCEL:     return "ACCEL";
        case ALARM_GPS_FENCE: return "GPS_FENCE";
        default:              return "UNKNOWN";
    }
}

void Alarm_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Buzzer PA1 */
    GPIO_InitStruct.Pin = BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);

    /* RGB LED PB0(R), PB1(G), PB2(B) */
    GPIO_InitStruct.Pin = LED_R_PIN | LED_G_PIN | LED_B_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_R_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN | LED_G_PIN | LED_B_PIN, GPIO_PIN_RESET);

    current_alarm = ALARM_NONE;
    g_state.alarm_active = 0;
}

void Alarm_Check(SensorData_t *data)
{
    uint8_t new_alarm = ALARM_NONE;

    if (data == NULL) return;

    if (data->temperature > g_state.temp_alarm_high)
        new_alarm = ALARM_TEMP_HIGH;
    else if (data->temperature < g_state.temp_alarm_low)
        new_alarm = ALARM_TEMP_LOW;
    else if (data->humidity > g_state.hum_alarm_high)
        new_alarm = ALARM_HUM_HIGH;
    else if (data->accel_x > g_state.accel_thresh ||
             data->accel_y > g_state.accel_thresh ||
             data->accel_z > g_state.accel_thresh)
        new_alarm = ALARM_ACCEL;

    if (current_alarm == ALARM_NONE && new_alarm != ALARM_NONE)
    {
        Alarm_Trigger(new_alarm);
    }
    else if (current_alarm != ALARM_NONE && new_alarm == ALARM_NONE)
    {
        Alarm_Clear();
    }
}

void Alarm_Trigger(uint8_t type)
{
    current_alarm = type;

    switch (type)
    {
        case ALARM_TEMP_HIGH:
            Alarm_SetLED(1, 0, 0);
            break;
        case ALARM_TEMP_LOW:
            Alarm_SetLED(0, 0, 1);
            break;
        case ALARM_HUM_HIGH:
            Alarm_SetLED(0, 1, 0);
            break;
        case ALARM_ACCEL:
            Alarm_SetLED(1, 1, 0);
            break;
        case ALARM_GPS_FENCE:
            Alarm_SetLED(1, 0, 1);
            break;
        default:
            Alarm_SetLED(1, 1, 1);
            break;
    }

    g_state.alarm_active = 1;
    Alarm_Beep(200);
    MQTT_PublishAlarm(Alarm_TypeName(type), "Alarm triggered");
}

void Alarm_Clear(void)
{
    current_alarm = ALARM_NONE;
    g_state.alarm_active = 0;
    Alarm_SetLED(0, 0, 0);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}

void Alarm_SetLED(uint8_t r, uint8_t g, uint8_t b)
{
    HAL_GPIO_WritePin(LED_R_PORT, LED_R_PIN,
        (r) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_PORT, LED_G_PIN,
        (g) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_B_PORT, LED_B_PIN,
        (b) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Alarm_Beep(uint16_t duration_ms)
{
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
    HAL_Delay(duration_ms);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}
