#include "lcd.h"
#include "spi.h"
#include "string.h"
#include "font_lcd.h"
#include "stdio.h"

static void LCD_ControlGPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = LCD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(LCD_CS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_RS_PIN;
    HAL_GPIO_Init(LCD_RS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LCD_BL_PIN;
    HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStruct);

    LCD_CS_RELEASE;
    LCD_RS_CMD;
    LCD_BL_ON;
}

void Lcd_WriteIndex(uint8_t Index)
{
    LCD_CS_SELECT;
    LCD_RS_CMD;
    HAL_SPI_Transmit(&hspi1, &Index, 1, 0xfff);
    LCD_CS_RELEASE;
}

void Lcd_WriteData(uint8_t Data)
{
    LCD_CS_SELECT;
    LCD_RS_DATA;
    HAL_SPI_Transmit(&hspi1, &Data, 1, 0xfff);
    LCD_CS_RELEASE;
}

void LCD_WriteData_16Bit(uint16_t Data)
{
    uint8_t Data_H = Data >> 8;
    uint8_t Data_L = Data & 0xFF;
    LCD_CS_SELECT;
    LCD_RS_DATA;
    HAL_SPI_Transmit(&hspi1, &Data_H, 1, 0xfff);
    HAL_SPI_Transmit(&hspi1, &Data_L, 1, 0xfff);
    LCD_CS_RELEASE;
}

void Lcd_Init(void)
{
    LCD_ControlGPIO_Init();

    Lcd_WriteIndex(0x01);
    HAL_Delay(120);

    Lcd_WriteIndex(0x11);
    HAL_Delay(120);

    Lcd_WriteIndex(0x36);
    Lcd_WriteData(0xC8);

    Lcd_WriteIndex(0x3A);
    Lcd_WriteData(0x05);

    Lcd_WriteIndex(0x29);
}

void Lcd_SetRegion(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    Lcd_WriteIndex(0x2a);
    Lcd_WriteData(0x00);
    Lcd_WriteData(x_start + 2);
    Lcd_WriteData(0x00);
    Lcd_WriteData(x_end + 2);

    Lcd_WriteIndex(0x2b);
    Lcd_WriteData(0x00);
    Lcd_WriteData(y_start + 3);
    Lcd_WriteData(0x00);
    Lcd_WriteData(y_end + 3);

    Lcd_WriteIndex(0x2c);
}

void Lcd_Clear(uint16_t Color)
{
    unsigned int i, m;
    Lcd_SetRegion(0, 0, X_MAX_PIXEL - 1, Y_MAX_PIXEL - 1);
    Lcd_WriteIndex(0x2C);
    for (i = 0; i < X_MAX_PIXEL; i++)
        for (m = 0; m < Y_MAX_PIXEL; m++)
        {
            LCD_WriteData_16Bit(Color);
        }
}

void Gui_DrawPoint(uint16_t x, uint16_t y, uint16_t Data)
{
    Lcd_SetRegion(x, y, x + 1, y + 1);
    LCD_WriteData_16Bit(Data);
}

void Gui_DrawFont_GBK16(uint16_t x0, uint16_t y0, uint16_t fc, uint16_t bc, uint8_t *s)
{
    int i, j, k, x, y;
    unsigned char qm;
    long int ulOffset;
    char ywbuf[32];

    for (i = 0; i < strlen((char*)s); i++)
    {
        if (((unsigned char)(*(s + i))) >= 161)
        {
            return;
        }
        else
        {
            qm = *(s + i);
            ulOffset = (long int)(qm) * 16;

            for (j = 0; j < 16; j++)
            {
                ywbuf[j] = Zk_ASCII8X16[ulOffset + j];
            }

            for (y = 0; y < 16; y++)
            {
                for (x = 0; x < 8; x++)
                {
                    k = x % 8;
                    if (ywbuf[y] & (0x80 >> k))
                    {
                        Gui_DrawPoint(x0 + x + i * 8, y + y0, fc);
                    }
                    else
                    {
                        Gui_DrawPoint(x0 + x + i * 8, y + y0, bc);
                    }
                }
            }
        }
    }
}

void Gui_DrawFont_data(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint32_t s)
{
    char buf1[16];
    snprintf(buf1, 16, "%d", s);
    Gui_DrawFont_GBK16(x, y, fc, bc, (uint8_t*)buf1);
    memset(buf1, 0, sizeof(buf1));
}
