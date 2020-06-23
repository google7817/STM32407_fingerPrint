#include <string.h>
#include <stdio.h>
#include "lib_cmos.h"

////////////////////////////////////

#define TOT_REG_NO			87 /* total CMOS registers number */	

#define DEBUG_DEV_CMOS	0
#ifdef GET_CURRENT	
	extern volatile int g_readCurrent;
	extern int BSP_ReadCurrent(uint8_t chanel);
#endif


u32 g_u32IntTimeMs;
u8 g_u8CmosRegInitBuf[] = 
{
	
	0x80,0x90,0x00,
	0x81,0x01,0x00,
	0x82,0x00,0x00,
	0x83,0x90,0x00,
	0x84,0x01,0x00,
	0x85,0x00,0x00,
	0x86,0x5b,0x00,
	0x87,0x09,0x00,
	0x88,0x00,0x00,
	0x89,0x68,0x00,
	0x8a,0x01,0x00,
	0x8b,0x00,0x00,
	0x8c,0x05,0x00,
	0x8d,0x00,0x00,
	0x8e,0x05,0x00,
	0x8f,0x00,0x00,
	0x90,0xfe,0x00,
	0x91,0x00,0x00,
	0x92,0x05,0x00,
	0x93,0x00,0x00,
	0x94,0xaf,0x00,  //// 100ms--0x271--time /// 110ms
	0x95,0x02,0x00,  ////
	0x96,0x00,0x00,
	0x97,0x68,0x00,
	0x98,0x01,0x00,
	0x99,0x00,0x00,
	0x9a,0x00,0x00,
	0x9b,0x00,0x00,
	0x9c,0x24,0x00,
	0x9d,0x00,0x00,
	0x9e,0x05,0x00,
	0x9f,0x00,0x00,
	0xa0,0x1f,0x00,
	0xa1,0x00,0x00,
	0xa2,0x25,0x00,
	0xa3,0x00,0x00,
	0xa4,0x40,0x00,
	0xa5,0x00,0x00,
	0xa6,0x15,0x00,
	0xa7,0x00,0x00,
	0xa8,0x02,0x00,
	0xa9,0x00,0x00,
	0xaa,0x7d,0x00,
	0xab,0x00,0x00,
	0xac,0x40,0x00,
	0xad,0x00,0x00,
	0xae,0xbd,0x00,
	0xaf,0x00,0x00,
	0xb0,0x40,0x00,
	0xb1,0x00,0x00,
	0xb2,0x1a,0x00,
	0xb3,0x00,0x00,
	0xb4,0x02,0x00,
	0xb5,0x00,0x00,
	0xb6,0x1a,0x00,
	0xb7,0x01,0x00,
	0xb8,0x02,0x00,
	0xb9,0x00,0x00,
	0xba,0x05,0x00,
	0xbb,0x00,0x00,
	0xc1,0x00,0x00,
	0xc2,0x01,0x00,
	0xc3,0x00,0x00,
	0xc4,0x01,0x00,
	0xc5,0x00,0x00,
	0xc6,0x03,0x00,
	0xc7,0x05,0x00,
	0xc8,0x10,0x00,
	0xc9,0x00,0x00,
	0xca,0x05,0x00,
	0xcb,0x01,0x00,
	//0xcc,0x00,0x00,
	0xcd,0x00,0x00,
	0xce,0x3f,0x00,
	0xd7,0x80,0x00,
	0xd8,0x78,0x00,
	0xd9,0x42,0x00,
	0xda,0x7a,0x00,
	0xdb,0x00,0x00,
	//0xdc,0x00,0x00,
	//0xdd,0x00,0x00,
	0xde,0x10,0x00,
	0xdf,0x10,0x0a,//delay
	0xd0,0x45,0x00,
	0xd1,0x10,0x00,
	0xd2,0x20,0x00,
	0xd3,0x0f,0x00,
	0xd4,0xf8,0x00,
	//0xd5,0x00,0x00,
	0xd6,0x24,0x00,
	0xcf,0x05,0x0a, //wakeup
	0xff,0xff,0x00,
};

static ST_CMOS_EXTER_DRV _g_stCmosExterDrv;

/**
 * @brief Write values to specified registers
 * @param u8RegAddr which register address will be wirte
 * @param u8RegData register value
 * @retval int
 */
void _dev_CMOS_WriteReg(uint8_t u8RegAddr,uint8_t u8RegData)/*wirte 1 byte*/
{
	uint8_t u8regBuf[2]={0xff,0xff};
	
	u8regBuf[0] = u8RegAddr;
	u8regBuf[1] = u8RegData;
	_g_stCmosExterDrv.SPI_Send(u8regBuf,2);
}

/**
 * @brief Read values in specified registers
 * @param u8RegAddr which register address will be read
 * @retval Register value
 */
uint16_t _dev_CMOS_ReadReg(uint8_t u8RegAddr) /*read 1 byte*/
{
	uint16_t u16RetVal ;	
	uint8_t u8RegReceBUf[2];
	u8RegAddr &= 0x7F;
	u8RegReceBUf[0] = u8RegAddr;
	u8RegReceBUf[1] = 0xff;
	_g_stCmosExterDrv.SPI_Send(u8RegReceBUf,2);
	
	u8RegReceBUf[0] = 0xff;				/* Just in case, MOSI need 0xff when spi read reg*/
	_g_stCmosExterDrv.SPI_Receive(u8RegReceBUf,2);

	u16RetVal = ((uint16_t)u8RegReceBUf[0])<<8 | u8RegReceBUf[1];
	return u16RetVal;
}

/**
 * @brief SPI send mass of data
 * @param pu8DataBuf 
 * @param u32Size
 * @retval error code
 */
int _dev_CMOS_SpiSendMass(uint8_t *pu8DataBuf,uint32_t u32Size)
{
	_g_stCmosExterDrv.SPI_Send_mass(pu8DataBuf,u32Size);
    return 0;
}

/**
 * @brief SPI send mass of data
 * @param pu8DataBuf 
 * @param u32Size
 * @retval error code
 */
int _dev_CMOS_SpiReceiveMass(uint8_t *pu8DataBuf,uint32_t u32Size)
{
	_g_stCmosExterDrv.SPI_Receive_mass(pu8DataBuf,u32Size);
    return 0;
}

/**
 * @brief get system local time 
 * @retval current time
 */
uint32_t _dev_CMOS_GetLocalTime(void) /*��ʱ*/
{
	return _g_stCmosExterDrv.getLocalTime();
}
/**
 * @brief delay ms
 * @param number of ms
 * @retval none 
 */
void _cmos_delay_ms(uint32_t n)
{	
	_g_stCmosExterDrv.delay_ms(n);
    return ;
}


/**
 * @brief  ROIC peripheral driver init
 * @param  pstCmosDrv 
 * @retval oxi_cmos_status_t value
**/
oxi_cmos_status_t Dev_CMOS_DrvInit(ST_CMOS_EXTER_DRV *pstCmosDrv)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV drv inint\n");
	#endif
	_g_stCmosExterDrv = *pstCmosDrv;
	return OXI_Success;
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
oxi_cmos_status_t Dev_SPI_CmosRecvRegVal(u8 u8RegAddr, u16 u16RegValMask, u16 u16ChkRegVal, u32 u32TimeOver)
{
	volatile u8 vu8RcvRegVal;
	volatile u32 vu32RecordTime = 0;
	u8 u8DestRegVal = (u16ChkRegVal>>8)&0xff;
	u8 u8RegValMask = (u16RegValMask>>8)&0xff;

#if DEBUG_DEV_CMOS	
	DBG("S %02X %02X %02X\r\n",u8RegAddr, u8RegValMask, u8DestRegVal);
#endif
	
	while(1)
	{
		vu8RcvRegVal = (uint8_t)(_dev_CMOS_ReadReg(u8RegAddr)>>8);
		
#ifdef GET_CURRENT
		if(u16ChkRegVal == 0x8B0B)//cmos 采图等完成
		{
			g_readCurrent = BSP_ReadCurrent(1);
#if DEBUG_DEV_CMOS
			DBG("cuurent = %d \n",g_readCurrent);
#endif
		}
#endif
		
		
	#if DEBUG_DEV_CMOS
		DBG("Reg Val:%02X\r\n",vu8RcvRegVal);
	#endif

		if((vu8RcvRegVal & u8RegValMask) == u8DestRegVal)
		{
			return OXI_Success;
		}
		
		vu32RecordTime++;
		
		if(vu32RecordTime > u32TimeOver)
		{
		#if DEBUG_DEV_CMOS
			DBG("check reg %02X, mask %02X, val %02X failed\r\n" \
									,u8RegAddr,u8RegValMask,u8DestRegVal );
		#endif
			return OXI_Timeout;
		}
		_cmos_delay_ms(1);
	}
}

void Dev_Cmos_Reset(uint16_t KpLowTim)
{
	_g_stCmosExterDrv.reset_pin_ctrl(0);
	_cmos_delay_ms((uint32_t)KpLowTim);
	_g_stCmosExterDrv.reset_pin_ctrl(1);
}

oxi_cmos_status_t Dev_Cmos_setRamReg(uint8_t reg, uint8_t val)
{
	uint16_t i;

	for(i=0; i<TOT_REG_NO; i++)
	{
		if(g_u8CmosRegInitBuf[3*i] == reg)
		{
			g_u8CmosRegInitBuf[3*i+1] = val;
			break;
		}
	}

	if(i< TOT_REG_NO)
	{
		return OXI_Success;
	}
	else
	{
		return OXI_InvalidArgument;
	}
}


oxi_cmos_status_t Dev_Cmos_Regs_Update(void)
{
	u16 i;
	u16 u16Tmp;
	u8 u8Reg94Val,u8Reg95Val;

	for(i=0; i<4096; i +=3)
	{
		if((g_u8CmosRegInitBuf[i] == 0xff)&&(g_u8CmosRegInitBuf[i+1] == 0xff))
		{
		#if DEBUG_DEV_CMOS
			DBG("regster init finish\r\n");
		#endif
			break;
		}
		
		_dev_CMOS_WriteReg(g_u8CmosRegInitBuf[i], g_u8CmosRegInitBuf[i+1]);
		_cmos_delay_ms(g_u8CmosRegInitBuf[i+2]);
	}	
	
	u8Reg94Val = (uint8_t)_dev_CMOS_ReadReg(0x94);
	u8Reg95Val = (uint8_t)_dev_CMOS_ReadReg(0x95);

#if DEBUG_DEV_CMOS
	DBG("reg 94:%02X, reg 95:%02X\r\n",u8Reg94Val,u8Reg95Val);
#endif
	u16Tmp = (u8Reg95Val<<8)|u8Reg94Val;
#if DEBUG_DEV_CMOS
	DBG("tmp:%04X\r\n",u16Tmp);
#endif
	g_u32IntTimeMs = ((u16Tmp>>3) + (u16Tmp>>5) >= 15)?((u16Tmp>>3) + (u16Tmp>>5) - 15):0;
#if DEBUG_DEV_CMOS
	DBG("integral time:%d\r\n",g_u32IntTimeMs);
#endif 
	
	return OXI_Success;
}

oxi_cmos_status_t Dev_Cmos_WakeUp(void)
{
	oxi_cmos_status_t ret=OXI_Success;
	/*wake up*/
	_dev_CMOS_WriteReg(0xCF, 0x05);/*it is set in init flow*/
	_dev_CMOS_WriteReg(0xF1, 0xFF);
	_cmos_delay_ms(2);
	ret=Dev_SPI_CmosRecvRegVal(0x3F, 0x0400, 0x0400, 500);/*check wether wake up success */
	
	return ret;
}


oxi_cmos_status_t Dev_Cmos_Restart(void)
{
	oxi_cmos_status_t ret=OXI_Success;
	
	_dev_CMOS_WriteReg(0xF8, 0xFF);/*restart CMD*/
	_cmos_delay_ms(2);

	ret=Dev_SPI_CmosRecvRegVal(0x3F, 0x0004, 0x0004, 500);/*check capture option success */
	
	return ret;
}

oxi_cmos_status_t Dev_Cmos_GetChipID(u8* Buf)
{	
	u8 u8ChipId;
	
	u8ChipId = (uint8_t)_dev_CMOS_ReadReg(0x3e);

	*Buf = u8ChipId;
	return OXI_Success;
}

/*
	aec =  v95 << 8 |  v94
	delay = (aec >> 3) + (aec >>5) - 15
*/
oxi_cmos_status_t Dev_Cmos_SetIntTime(u8* Buf)
{	
	u8 u8Reg94Val,u8Reg95Val;
	u32 u32ReferVal,u32Fac,u32SetIntTime;/*intergral factor*/

	/*cal integral time referent value: (reg val02H~00H + reg05H~03H)*0.2 --unit: us*/
	u32ReferVal = (((u32)g_u8CmosRegInitBuf[4])<<8)|g_u8CmosRegInitBuf[1];
	u32ReferVal += (((u32)g_u8CmosRegInitBuf[13])<<8)|g_u8CmosRegInitBuf[10];
	u32ReferVal /= 5;/*unit us -- 160us*/
	u32ReferVal = 160;
	/*u32SetIntTime*1000(ms -> us) = referent vaule* reg 96H~94H*/
	u32SetIntTime = (((u16)Buf[0])<<8) | Buf[1];
#if DEBUG_DEV_CMOS
	DBG("set int time = %dMs\r\n",u32SetIntTime);
#endif
	u32SetIntTime *=1000;/*unit ms -> us*/
	u32Fac = u32SetIntTime/u32ReferVal;
	u8Reg95Val = (u8)(u32Fac>>8);
	u8Reg94Val = (u8)(u32Fac&0xFF);
	
	uint8_t u8regSetCnt = 0;
	
	for(int i=0; i< sizeof(g_u8CmosRegInitBuf); i +=3){
		if(g_u8CmosRegInitBuf[i] == 0x95){
			g_u8CmosRegInitBuf[i+1] = u8Reg95Val;
			u8regSetCnt++;
		}

		if(g_u8CmosRegInitBuf[i] == 0x94){
			g_u8CmosRegInitBuf[i+1] = u8Reg94Val;
			u8regSetCnt++;
		}

		if(u8regSetCnt >= 2)
			break;
	}

	if(u8regSetCnt != 2){
		return OXI_Failed;
	}
	
	u16 u16Tmp = (u8Reg95Val<<8)|u8Reg94Val;
	g_u32IntTimeMs = ((u16Tmp>>3) + (u16Tmp>>5) >= 15)?((u16Tmp>>3) + (u16Tmp>>5) - 15):0;
	#if DEBUG_DEV_CMOS
		DBG("register set success,int time:%d\r\n",g_u32IntTimeMs);
	#endif
	
	return OXI_Success;
}



/*
	func describe	: Cmos capture image(without init) flow:
								step1. CMOS wake up;
								step2. send CMOS capture CMD ,then wait for 
									   capture complete of CMOS;
								step3. send get image data CMD ,then get image data;
								step4. sleep CMOS.
	input para 		: none
	output para		: image data(88x86x2, col: 88, row: 86)
	return 			: init result => TRUE -- capture success, FALSE -- capture failed
*/
oxi_cmos_status_t Dev_Cmos_Cpt(u8* ParaBuf)
{
	volatile u16 vu16RecvRegVal;
	oxi_cmos_status_t ret=OXI_Success;
#if DEBUG_DEV_CMOS
	DBG("Cmos Capture\r\n");
#endif
	ret=Dev_Cmos_WakeUp();
	if(ret != OXI_Success)
	{
		goto EXIT;
	}
	
#if DEBUG_DEV_CMOS
	DBG("Cmos wake up success\r\n");
#endif

	/*send Capture image CMD*/
	_dev_CMOS_WriteReg(0xF3, 0xFF);
	_cmos_delay_ms(g_u32IntTimeMs);
	_dev_CMOS_WriteReg(0xD2, 0x28);
	_dev_CMOS_WriteReg(0xD0, 0x40);
	_dev_CMOS_WriteReg(0xCF, 0x00);
	

	ret=Dev_SPI_CmosRecvRegVal(0x3F, 0xFFFF, 0x8B0B, 5000);/*check capture option success */
	if(ret != OXI_Success)
	{
		goto EXIT;
	}
	
#if DEBUG_DEV_CMOS
	DBG("cpt image success, image ready\r\n");
#endif
	/*Receive data*/
	_dev_CMOS_WriteReg(0xFC, 0xFF);
	_dev_CMOS_WriteReg(0xFF, 0xFF);
	_dev_CMOS_SpiReceiveMass(ParaBuf,88*86*2);
	
#if DEBUG_DEV_CMOS
	DBG("Cmos send cpt image success\r\n");
#endif

	ret=Dev_Cmos_Restart();
	if(ret != OXI_Success)
	{
		goto EXIT;
	}
	
	ret=Dev_CMOS_SleepROIC(0);

EXIT:
	return ret;
}


//////////////////
/**
 * @brief  sleep ROIC disale ADC  
 * @param  timeout 
 * @retval EN_OXI600_ERR_TYPE
**/
oxi_cmos_status_t Dev_CMOS_SleepROIC(uint32_t timeout)
{
	_dev_CMOS_WriteReg(0xD0, 0x45);  ///reference closed
	_dev_CMOS_WriteReg(0xD2, 0x20);  ///ADC closed
	_dev_CMOS_WriteReg(0xCF, 0x27/*0x05*/);  ///ADC clock closed
	_cmos_delay_ms(3);    

	_dev_CMOS_WriteReg(0xF2, 0xFF);/*cmd sleep*/
	_cmos_delay_ms(2);
	return OXI_Success;
}

/**
 * @brief  set PMU parameter
 * @param   dataBuf the pointer to data buffer
 * @param   dataLen the pointer to data length, indicate effective data length
 * @retval EN_OXI600_ERR_TYPE
**/
oxi_cmos_status_t Dev_CMOS_setPmuPara(uint8_t *dataBuf, uint32_t dataLen)
{
	
#if DEBUG_DEV_CMOS
	DBG("Dev_CMOS_setPmuPara\r\n");
#endif

//	for(int i=0;i<dataLen;i+=3)
//	{
//		printf("w %2x=%2x %2x\r\n",dataBuf[i],dataBuf[i+1],dataBuf[i+2]);
//	}
	
	memcpy(g_u8CmosRegInitBuf,dataBuf,dataLen);
	return OXI_Success;
}

/**
  * @brief	start capture frame scan (Start running and reply success immediately)
  * @param	stChnl600CptPara  
  * @param	timeout 
  * @retval EN_OXI600_ERR_TYPE

  EXAMPLE:
  	 stChnl600CptPara.enCptMode = EN_CHNL600_CPT_MODE;
	 stChnl600CptPara.enCptType = EN_MODE1_CPT_WIN_IMG;
	 stChnl600CptPara.FingerNo = X;
	 stChnl600CptPara.integraLine = x;
	 stChnl600CptPara.shtOrLong = X;
	 stChnl600CptPara.PrjType = X;
	 stChnl600CptPara.W1ColStrt = X1;
	 stChnl600CptPara.W1RowStrt = Y1;
 **/
oxi_cmos_status_t Dev_CMOS_CaptureWinImage(uint32_t timeout)
{
	oxi_cmos_status_t ret=OXI_Success;
	
	ret=Dev_CMOS_RoicRegInit();//仅为了编译通过
	if(ret != OXI_Success)
	{
		goto EXIT;
	}

	ret=Dev_Cmos_SetIntTime(NULL);//仅为了编译通过
	if(ret != OXI_Success)
	{
		goto EXIT;
	}

	ret=Dev_Cmos_Cpt( NULL);//仅为了编译通过

EXIT:
	return ret;
}

/**
 * @brief All register writes corresponding data
 * @param none
 * @retval 1 or error number
 */
oxi_cmos_status_t Dev_CMOS_RoicRegInit(void)
{
	oxi_cmos_status_t ret=OXI_Success;
	u8 u8ChipID;
	
	Dev_Cmos_Reset(10);/*reset IC*/
	
	ret=Dev_Cmos_Regs_Update();/*init IC register*/
	if(ret != OXI_Success)
	{
		goto EXIT;
	}

	ret = Dev_Cmos_GetChipID(&u8ChipID);
	if(ret != OXI_Success)
	{
		goto EXIT;
	}
	
#if DEBUG_DEV_CMOS	
	DBG("chip id:%02x\r\n",u8ChipID);
#endif

EXIT:	
	return ret;
}






