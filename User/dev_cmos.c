/***************************************************************************************************
** File name   : dev_cmos.c
** Function    : 
** Author      : sun
** Date        : 2019
** Version     : v0.1
** Description : cmos 读写寄存器、等状态、收数据
** ModifyRecord: 
***************************************************************************************************/

/***************************************************************************************************
** Include Header Files
***************************************************************************************************/
#include <stdio.h>
#include "dev_cmos.h"

/***************************************************************************************************
** Local Macro Definition
***************************************************************************************************/
#define DEBUG_DEV_CMOS	1

/***************************************************************************************************
** Local Type Definition
***************************************************************************************************/

/***************************************************************************************************
** Constant Variable Declaration
***************************************************************************************************/

/***************************************************************************************************
** Local static Variable Declaration
***************************************************************************************************/
    
/***************************************************************************************************
** Global Variable Declaration
***************************************************************************************************/

/***************************************************************************************************
** Global Function Declaration
***************************************************************************************************/

/***************************************************************************************************
** Subroutine  : HAL_Delay
** Function    : Dev_cmos_delay_ms
** Input       : u32 u32delay	(ms)
** Output      : none
** Description : 延时
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
void Dev_cmos_delay_ms(u32 u32delay)
{
	if(u32delay==0)
		return;
	
	HAL_Delay(u32delay-1);//需要-1
	return;
}

/***************************************************************************************************
** Subroutine  : Dev_SPI_SendData
** Function    : Dev_SPI_CmosWrtReg
** Input       : u8 u8RegAddr; u8 u8Value
** Output      : 0
** Description : 批量写寄存器
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
int Dev_SPI_CmosWrtReg(u8 u8RegAddr, u8 u8Value)
{
#if DEBUG_DEV_CMOS
	DBG("W %02X = %02X\r\n",u8RegAddr, u8Value);
#endif

	u8 u8tx_data[2]={u8RegAddr,u8Value};
	Dev_SPI_SendData(&hspi1, u8tx_data,2,400);
	return 0;
}


/***************************************************************************************************
** Subroutine  : Dev_SPI_SendData、Dev_SPI_ReceiveData
** Function    : Dev_SPI_CmosRdReg
** Input       : u8 *pu8RegAddr
** Output      : 0; pu8RegAddr 会写上读到的值
** Description : 批量读寄存器
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
int Dev_SPI_CmosRdReg(u8 *pu8RegAddr)
{
	u8 u8tx_data[4]={(*pu8RegAddr)&0x7F,0xFF,0xFF,0xFF};
	//u8 u8rx_data[4]={0};

	Dev_SPI_SendData(&hspi1, u8tx_data,2,400);
	Dev_SPI_ReceiveData(&hspi1, u8tx_data,2,400);
	
#if DEBUG_DEV_CMOS	
	DBG("R %02X = %02X\r\n",(*pu8RegAddr)&0x7F, u8tx_data[0]);
#endif
	*pu8RegAddr = u8tx_data[0];
	return 0;
}


/***************************************************************************************************
** Subroutine  : Dev_SPI_CmosRdReg
** Function    : Dev_SPI_CmosRecvRegVal
** Input       : u8 u8RegAddr, u16 u16RegValMask, u16 u16ChkRegVal, u32 u32TimeOver
** Output      : 0:成功；其他：失败
** Description : 等一个状态
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
int Dev_SPI_CmosRecvRegVal(u8 u8RegAddr, u16 u16RegValMask, u16 u16ChkRegVal, u32 u32TimeOver)
{
	//bool bRes;
	volatile u8 vu8RcvRegVal;
	volatile u32 vu32RecordTime = 0;
	int ret;
	u8 u8DestRegVal = (u16ChkRegVal>>8)&0xff;
	u8 u8RegValMask = (u16RegValMask>>8)&0xff;

#if DEBUG_DEV_CMOS	
	DBG("S %02X %02X %02X\r\n",u8RegAddr, u8RegValMask, u8DestRegVal);
#endif
	
	while(1)
	{
		vu8RcvRegVal = u8RegAddr;
		ret = Dev_SPI_CmosRdReg((u8*)&vu8RcvRegVal);
		CHECK_RET_RETURN("Dev_SPI_CmosRdReg",ret,ret);
		
		if(u16ChkRegVal == 0x8B0B)//cmos 采图等完成
		{
			g_readCurrent = BSP_ReadCurrent(1);
#if DEBUG_DEV_CMOS
			DBG("cuurent = %d \n",g_readCurrent);
#endif
		}
		
	#if DEBUG_DEV_CMOS
		DBG("Reg Val:%02X\r\n",vu8RcvRegVal);
	#endif

		if((vu8RcvRegVal & u8RegValMask) == u8DestRegVal)
		{
			return 0;
		}
		
		vu32RecordTime++;
		if(vu32RecordTime%500 == 0)
		{
			IwdgFeed();
		}
		
		if(vu32RecordTime > u32TimeOver)
		{
		#if DEBUG_DEV_CMOS
			DBG("check reg %02X, mask %02X, val %02X failed\r\n" \
									,u8RegAddr,u8RegValMask,u8DestRegVal );
		#endif
			return -1;
		}
		Dev_cmos_delay_ms(1);
	}
	//return -2;
}


/***************************************************************************************************
** Subroutine  : Dev_SPI_ReceiveDataDMA
** Function    : Dev_SPI_CmosDmaRcv
** Input       : u8* pu8RcvBuf, u16 u16length
** Output      : 0
** Description : cmos 收fifo数据
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
int Dev_SPI_CmosDmaRcv(u8* pu8RcvBuf, u16 u16length)
{
 	Dev_SPI_ReceiveDataDMA(&hspi1,pu8RcvBuf,u16length);
	return 0;
}
