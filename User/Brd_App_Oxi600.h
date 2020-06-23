#ifndef __BRD_APP_OXI600_H
#define __BRD_APP_OXI600_H

#include "type.h"
#include "stm32f4xx_hal.h"
#include "protocol.h"
#include "lib_oxi600.h"
#include "lib_oxi600_jigsaw.h"


extern uint8_t g_u8Oxi600RegInitBuf[315];
extern  SPI_HandleTypeDef hspi1;
extern uint16_t g_u16ShtIntBack;
extern uint8_t g_u8ShtOrLong;

void Brd_Oxi600_DrvInit(void);
void Brd_Oxi600_SetIntegrationLine(uint16_t InteLIne);
uint16_t Brd_Oxi600_GetIntergrationLine(uint8_t *pu8Databuf);
EN_OXI600_ERR_TYPE Brd_Oxi600_setPmuPara(ST_PRTCL_INFO stRcvDataInfo, ST_PRTCL_INFO stRcvCmdInfo, u32 u32AddrOffset);
EN_OXI600_ERR_TYPE Brd_Oxi600_setClrPara(ST_PRTCL_INFO stRcvDataInfo, ST_PRTCL_INFO stRcvCmdInfo, u32 u32AddrOffset);
EN_OXI600_ERR_TYPE Brd_Oxi600_CaptureWinImg(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu8Datalen);
EN_OXI600_ERR_TYPE Brd_Oxi600_CaptureWhlImg(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen);


#endif

