#ifndef __LIB_CMOS_H__
#define __LIB_CMOS_H__

#include "type.h"
#define OXIStatusGroup_Generic  -800
typedef enum _oxi_status
{
	/* the generic status following HAL_StatusTypeDef */
	OXI_Success = 0,//为方便统一，错误改为负数
	OXI_Failed = OXIStatusGroup_Generic - 1,
	OXI_Busy = OXIStatusGroup_Generic - 2,
	OXI_Timeout = OXIStatusGroup_Generic - 3,
	/* the generic status new adding */
	OXI_OutOfRange = OXIStatusGroup_Generic - 4,
	OXI_InvalidArgument = OXIStatusGroup_Generic - 5,

	OXI_Write_CMD_Failed = OXIStatusGroup_Generic - 10,
	OXI_READ_CMD_RES_Failed = OXIStatusGroup_Generic - 11,
	OXI_RES_CRC_err = OXIStatusGroup_Generic - 12,
	OXI_RES_LEN_err = OXIStatusGroup_Generic - 13,
	OXI_RES_state_err = OXIStatusGroup_Generic - 14,
	OXI_Write_DATA_Failed = OXIStatusGroup_Generic - 15,
	OXI_READ_DATA_Failed = OXIStatusGroup_Generic - 16,
	OXI_READ_DATA_check_Failed = OXIStatusGroup_Generic - 17,

	OXI_unsupport_fun = OXIStatusGroup_Generic - 18,
}oxi_cmos_status_t;

typedef struct 
{
	void (*delay_ms)(uint32_t ms);
	uint64_t (*getLocalTime)(void);

	int (*SPI_Send)(void *dataBuf,uint32_t length);
	int (*SPI_Receive)(void *dataBuf,uint32_t length);

	int (*SPI_Send_mass)(void *dataBuf,uint32_t length);		/*mass of data*/
	int (*SPI_Receive_mass)(void *dataBuf,uint32_t length);  /*mass of data*/

	/*special cmos hardware operation*/
	int (*reset_pin_ctrl)(uint8_t status);

}ST_CMOS_EXTER_DRV;

extern u8 g_u8CmosRegInitBuf[];

oxi_cmos_status_t Dev_CMOS_DrvInit(ST_CMOS_EXTER_DRV *pstCmosDrv);



/*
*   Internal function declaration 
*/
void _dev_CMOS_WriteReg(uint8_t u8RegAddr,uint8_t u8RegData);
uint16_t _dev_CMOS_ReadReg(uint8_t u8RegAddr);

oxi_cmos_status_t Dev_Cmos_Regs_Update(void);
oxi_cmos_status_t Dev_Cmos_WakeUp(void);
oxi_cmos_status_t Dev_Cmos_Restart(void);
oxi_cmos_status_t Dev_Cmos_SetIntTime(u8* Buf);
oxi_cmos_status_t Dev_Cmos_Cpt(u8* ParaBuf);

oxi_cmos_status_t Dev_CMOS_setPmuPara(uint8_t *dataBuf, uint32_t dataLen);
oxi_cmos_status_t Dev_CMOS_SleepROIC(uint32_t timeout);
oxi_cmos_status_t Dev_CMOS_CaptureWinImage(uint32_t timeout);
oxi_cmos_status_t Dev_CMOS_RoicRegInit(void);
void Dev_Cmos_Reset(uint16_t KpLowTim);

#endif
