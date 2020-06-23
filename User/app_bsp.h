
#ifndef __APP_BSP_H
#define __APP_BSP_H


#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "protocol.h"

//#define TEST_CODE_UPDATA /*swtich to enable/disable */		

void App_Bsp_cmdProc(ST_PRTCL_INFO* pstRcvInfo);


#endif
