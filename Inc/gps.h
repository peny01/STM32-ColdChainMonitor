#ifndef __GPS_H
#define __GPS_H
#include "main.h"

#define GPS_RX_BUF_SIZE       256

uint8_t GPS_Init(void);
uint8_t GPS_ParseBuffer(void);
void GPS_ClearBuffer(void);

extern volatile uint8_t gps_rx_buf[GPS_RX_BUF_SIZE];
extern volatile uint16_t gps_rx_len;

#endif
