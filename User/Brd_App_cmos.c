#include "brd_app_cmos.h"
#include "stm32f4xx_hal.h"
#include "bsp.h"

extern  SPI_HandleTypeDef hspi1;

void delayMs_CMOS(uint32_t n)
{
	if( n!= 0)
	HAL_Delay(n-1);
}

static uint64_t _brd_CMOS_getTick(void)
{		
	return HAL_GetTick();
}

static int _brd_CMOS_SpiWrite(void *pu8DataBuf,uint32_t u32Size)
{		
	return Dev_SPI_SendData(&hspi1, pu8DataBuf,u32Size,200);
}

static int _brd_CMOS_SpiRead(void *pu8DataBuf,uint32_t u32size)
{
	return 	Dev_SPI_ReceiveData(&hspi1, pu8DataBuf,u32size,200);
}

static int _brd_CMOS_SPI_ReceiveMass(void *pu8DataBuf,uint32_t u32size)
{
 	return Dev_SPI_ReceiveDataDMA(&hspi1,pu8DataBuf,u32size);
}

static int _brd_CMOS_SPI_SendMass(void *pu8DataBuf,uint32_t u32size)
{
 	return Dev_SPI_SendDataContinus(&hspi1,pu8DataBuf,u32size);
}

static int _brd_CMOS_ResetPinCtrl(uint8_t status)
{
	return BSP_CTRLResetPin(status);
}

void Brd_CMOS_DrvInit(void)
{
	ST_CMOS_EXTER_DRV stCmosDrv;
	
	stCmosDrv.delay_ms = delayMs_CMOS;
	stCmosDrv.getLocalTime = _brd_CMOS_getTick;
	stCmosDrv.SPI_Receive = _brd_CMOS_SpiRead;
	stCmosDrv.SPI_Send = _brd_CMOS_SpiWrite;
	stCmosDrv.SPI_Receive_mass = _brd_CMOS_SPI_ReceiveMass;
	stCmosDrv.SPI_Send_mass = _brd_CMOS_SPI_SendMass;
	stCmosDrv.reset_pin_ctrl = _brd_CMOS_ResetPinCtrl;
	
	Dev_CMOS_DrvInit(&stCmosDrv);
}


