
#ifndef __APP_OXI600_H
#define __APP_OXI600_H

#include "type.h"
#include "protocol.h"
#include "stm32f4xx_hal.h"
#include "brd_app_oxi600.h"

#define GAIN_SIZE				50*1024
#define IMG_MAX_SIZE			(63*1024)

extern void App_Oxi600_cmdProc(ST_PRTCL_INFO* pstRcvInfo);




#endif /* __APP_OXI_H */

