#ifndef __DEV_OXI600_DRIVER_H
#define __DEV_OXI600_DRIVER_H

#include "type.h"
#include "stm32f4xx_hal.h"

extern uint8_t g_u8Oxi600RegInitBuf[370];
extern  SPI_HandleTypeDef hspi1;

void delay_ms(uint32_t n);
uint32_t Dev_Oxi600_GetLocalTime();
void Dev_Oxi600_WriteReg(uint8_t regAddr,uint8_t regData);
uint16_t Dev_Oxi600_ReadReg(uint8_t RegAddr);



#endif

