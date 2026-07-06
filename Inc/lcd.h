#ifndef __LCD_H
#define __LCD_H

#include "stdint.h"
#include "stm32f0xx.h"

#define RED     0xf800
#define GREEN   0x07e0
#define BLUE    0x001f
#define WHITE   0xffff
#define BLACK   0x0000
#define YELLOW  0xFFE0
#define CYAN    0x07ff
#define BRIGHT_RED 0xf810
#define GRAY0   0xEF7D
#define GRAY1   0x8410
#define GRAY2   0x4208

#define X_MAX_PIXEL  128
#define Y_MAX_PIXEL  128

/* TFT ST7735R Pin Definition (ColdChainMonitor) */
/* SPI1: PB3=SCK, PA7=MOSI (K8 32-pin: no PB5) */
#define LCD_SCK_PORT         GPIOB
#define LCD_SCK_PIN          GPIO_PIN_3
#define LCD_MOSI_PORT        GPIOA
#define LCD_MOSI_PIN         GPIO_PIN_7

/* Control pins */
#define LCD_CS_PORT          GPIOA
#define LCD_CS_PIN           GPIO_PIN_15

#define LCD_RS_PORT          GPIOA
#define LCD_RS_PIN           GPIO_PIN_8

#define LCD_BL_PORT          GPIOA
#define LCD_BL_PIN           GPIO_PIN_4

#define LCD_BL_ON    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET)
#define LCD_BL_OFF   HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET)

#define LCD_CS_SELECT   HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET)
#define LCD_CS_RELEASE  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET)

#define LCD_RS_CMD   HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET)
#define LCD_RS_DATA  HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET)

void Lcd_Init(void);
void Lcd_Clear(uint16_t Color);
void Lcd_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void Lcd_WriteIndex(uint8_t Index);
void Lcd_WriteData(uint8_t Data);
void LCD_WriteData_16Bit(uint16_t Data);
void Gui_DrawPoint(uint16_t x, uint16_t y, uint16_t Data);
void Gui_DrawFont_GBK16(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t *s);
void Gui_DrawFont_data(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint32_t s);

#endif
