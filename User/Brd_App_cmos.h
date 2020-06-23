#ifndef __BRD_APP_CMOS_H
#define __BRD_APP_CMOS_H

#include "type.h"
#include "stm32f4xx_hal.h"
#include "protocol.h"
#include "lib_cmos.h"



extern  SPI_HandleTypeDef hspi1;

void Brd_CMOS_DrvInit(void);



#endif

