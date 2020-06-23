
#include "brd_app_oxi600.h"
#include "stm32f4xx_hal.h"
//#include "dev_oxi600_driver.h"
#include "bsp.h"
uint16_t g_u16integraLines = 400;
extern  SPI_HandleTypeDef hspi1;
extern EN_CHL600_CAPACITY g_enShtInteCapacity ;	/*short integration frame capacity*/	
extern EN_CHL600_CAPACITY g_enLongInteCapacity ; 	/*long integration frame capacity*/ 

ST_CHNL600_EXTER_DRV g_stChnl600Drv;
uint16_t g_u16ShtIntBack = 0;
uint8_t g_u8ShtOrLong = 2;
uint32_t g_u32XaoDelay = 0;
void delayMs(uint32_t n)
{
	if( n!= 0)
	HAL_Delay(n-1);
}


static int _brd_Oxi600_SpiWrite(uint8_t *pu8DataBuf,uint32_t u32Size)
{		
	return Dev_SPI_SendData(&hspi1, pu8DataBuf,u32Size,200);
}


static int _brd_Oxi600_SpiRead(uint8_t *pu8DataBuf,uint32_t u32size)
{
	return 	Dev_SPI_ReceiveData(&hspi1, pu8DataBuf,u32size,200);
}

static int _brd_Oxi600_SPI_ReceiveMass(uint8_t *pu8DataBuf,uint32_t u32size)
{
 	return Dev_SPI_ReceiveDataDMA(&hspi1,pu8DataBuf,u32size);
}

static int _brd_Oxi600_SPI_SendMass(uint8_t *pu8DataBuf,uint32_t u32size)
{
 	return Dev_SPI_SendDataContinus(&hspi1,pu8DataBuf,u32size);
}


void Brd_Oxi600_DrvInit(void)
{
	g_stChnl600Drv.delay_ms = delayMs;
	g_stChnl600Drv.getLocalTime = HAL_GetTick;
	g_stChnl600Drv.SPI_Receive = _brd_Oxi600_SpiRead;
	g_stChnl600Drv.SPI_Send = _brd_Oxi600_SpiWrite;
	g_stChnl600Drv.SPI_Receive_mass = _brd_Oxi600_SPI_ReceiveMass;
	g_stChnl600Drv.SPI_Send_mass = _brd_Oxi600_SPI_SendMass;
	
	Dev_Oxi600_DrvInit(&g_stChnl600Drv);
}


void Brd_Oxi600_SetIntegrationLine(uint16_t u16InteLIne)
{
	g_u16integraLines = u16InteLIne;
}

uint16_t Brd_Oxi600_GetIntergrationLine(uint8_t *pu8Databuf)
{
	uint16_t u16Linetime;
	u16Linetime = (((uint16_t)g_u8Oxi600RegInitBuf[24*3+1]<<8 | g_u8Oxi600RegInitBuf[23*3+1])+\
				((uint16_t)g_u8Oxi600RegInitBuf[26*3+1]<<8 | g_u8Oxi600RegInitBuf[25*3+1]));
	u16Linetime /= 14;
	SplitU16(pu8Databuf,g_u16integraLines);
	SplitU16(&pu8Databuf[2],u16Linetime);
	
}

EN_OXI600_ERR_TYPE Brd_Oxi600_setPmuPara(ST_PRTCL_INFO stRcvDataInfo, ST_PRTCL_INFO stRcvCmdInfo, u32 u32AddrOffset)
{
	EN_RESP_TYPE enRetVal = EN_RESP_SUCCESS;

	//memcpy(&g_u8Oxi600RegInitBuf[addrOffset],rcvDataInfo.pu8DataBufPointer,rcvDataInfo.u16ParaLen);
	Dev_Oxi600_setPmuPara(stRcvDataInfo.pu8DataBufPointer,stRcvDataInfo.u16ParaLen);

	return enRetVal;
}

EN_OXI600_ERR_TYPE Brd_Oxi600_setClrPara(ST_PRTCL_INFO stRcvDataInfo, ST_PRTCL_INFO stRcvCmdInfo, u32 u32AddrOffset)
{
	EN_RESP_TYPE enRetVal = EN_RESP_SUCCESS;

	Dev_Oxi600_setCptPara(stRcvDataInfo.pu8DataBufPointer,stRcvDataInfo.u16ParaLen);
	if(stRcvDataInfo.pu8DataBufPointer[0] == 2)
	{
		return EN_RESP_SUCCESS;
	}
	else
	{
		g_u32XaoDelay =  ((uint32_t)stRcvDataInfo.pu8DataBufPointer[25]<<24)|((uint32_t)stRcvDataInfo.pu8DataBufPointer[26]<<16)|\
							((uint32_t)stRcvDataInfo.pu8DataBufPointer[27]<<8)|((uint32_t)stRcvDataInfo.pu8DataBufPointer[28]);
	}
	g_enShtInteCapacity = stRcvDataInfo.pu8DataBufPointer[30];
	g_enLongInteCapacity = stRcvDataInfo.pu8DataBufPointer[31];
	g_u16ShtIntBack = ((uint16_t)stRcvDataInfo.pu8DataBufPointer[32]<<8)|((uint16_t)stRcvDataInfo.pu8DataBufPointer[33]);
	g_u8ShtOrLong = stRcvDataInfo.pu8DataBufPointer[34];
	DBG("CAP SHT= %X,LONG =%X,INTE =%X,sht long = %x",g_enShtInteCapacity,g_enLongInteCapacity,g_u16ShtIntBack,g_u8ShtOrLong);
	return enRetVal;
}


//Dev_Oxi600_CaptureWinImage(stChanl600Info,&g_pu8DataBuf[9],&u32ImgDataLen)
EN_OXI600_ERR_TYPE Brd_Oxi600_CaptureWinImg(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen)
{
	ST_CHNL600_CPT_PARA stTmp;
	uint16_t u16inteBackup;
	stChnl600CptPara.integraLine = g_u16integraLines; 
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	
	enRetVal = Dev_Oxi600_ClrAndGetGateoff(stChnl600CptPara,pu8DataBuf+stChnl600CptPara.CptDataSize,pu32Datalen,FALSE,1000);
	if(enRetVal != EN_OXI600_SUCCESS )
	{
		return enRetVal;
	}
	delayMs(g_u32XaoDelay);

	if(stChnl600CptPara.shtOrLong != 1)
	{
		enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		enRetVal = Dev_Oxi600_capShtLastFrame(stChnl600CptPara);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		g_readCurrent = BSP_ReadCurrent(1);
		enRetVal = Dev_Oxi600_GetImageData(pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}

	}
	else
	{
		
		u16inteBackup = stChnl600CptPara.integraLine;		// backup long integraion lines
		stChnl600CptPara.shtOrLong = 0;						// notice , for correct capacity
		stChnl600CptPara.integraLine = stChnl600CptPara.u16TmpInte;	// for genrate short waveform

		enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		
		stChnl600CptPara.integraLine = u16inteBackup; 		// change to long integration
		enRetVal = Dev_Oxi600_getShtAndCptLong(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);	
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		g_readCurrent = BSP_ReadCurrent(1);
		//HAL_Delay(900);	// test match time > 400ms
		enRetVal = Dev_Oxi600_GetLongImageData(pu8DataBuf,pu32Datalen,2000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
	}

	enRetVal = Dev_Oxi600_SleepROIC(stChnl600CptPara,1000);
	if(enRetVal != EN_OXI600_SUCCESS )
	{
		return enRetVal;
	}
	
	return enRetVal;
}


EN_OXI600_ERR_TYPE Brd_Oxi600_CaptureWhlImg(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen)
{
	ST_CHNL600_CPT_PARA stTmp;

	static u8 u8rowOfst = 0;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	if(stChnl600CptPara.WhlImageNo == 1)
	{
		u8rowOfst = 0;
	}

	switch(stChnl600CptPara.PrjType)
	{
		case EN_PRJ_OXI600_MK720_80UM:
		case EN_PRJ_OXI600_MK720_80UM_1_3:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(439*2);  
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;
			break;
			
		case EN_PRJ_OXI600_MK720_100UM:
		case EN_PRJ_OXI600_MK720_100UM_1_3:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(369*2);
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;  
			break;

		case EN_PRJ_OXI600_MK810_80UM:
		case EN_PRJ_OXI600_MK810_80UM_1_3:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(510*2);  
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;
			break;

		case EN_PRJ_OXI600_MK320_100UM:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(493*2);  
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;
			break;

		case EN_PRJ_OXI600_MS001_80UM_1_3:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(404*2);  
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;
			break;		

		case EN_PRJ_OXI600_MS006_80UM_1_3:
		case EN_PRJ_OXI600_MS006_80UM_V01:
			stChnl600CptPara.W1RowNo = stChnl600CptPara.CptDataSize/(388*2);  
			stChnl600CptPara.W1RowStrt = u8rowOfst;
			u8rowOfst += stChnl600CptPara.W1RowNo;
			break;		
			
		default :
			break;	
	}
	
	stChnl600CptPara.integraLine = g_u16integraLines; 
	enRetVal = Dev_Oxi600_ClrAndGetGateoff(stChnl600CptPara,pu8DataBuf+stChnl600CptPara.CptDataSize,pu32Datalen,FALSE,1000);
	if(enRetVal != EN_OXI600_SUCCESS )
	{
		return enRetVal;
	}
	delayMs(g_u32XaoDelay);

	if(stChnl600CptPara.shtOrLong != 1)
	{
		enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		enRetVal = Dev_Oxi600_capShtLastFrame(stChnl600CptPara);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		g_readCurrent = BSP_ReadCurrent(1);
		enRetVal = Dev_Oxi600_GetImageData(pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}

	}
	else
	{
		stChnl600CptPara.shtOrLong = 0;	// notice , for correct capacity
		stChnl600CptPara.integraLine = stChnl600CptPara.u16TmpInte;

		enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		//g_readCurrent = BSP_ReadCurrent(1);
		
		stChnl600CptPara.integraLine = g_u16integraLines; 
		enRetVal = Dev_Oxi600_getShtAndCptLong(stChnl600CptPara,pu8DataBuf,pu32Datalen,1000);	
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
		g_readCurrent = BSP_ReadCurrent(1);
		//HAL_Delay(500);
		enRetVal = Dev_Oxi600_GetLongImageData(pu8DataBuf,pu32Datalen,2000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			return enRetVal;
		}
	}

	enRetVal = Dev_Oxi600_SleepROIC(stChnl600CptPara,1000);
	if(enRetVal != EN_OXI600_SUCCESS )
	{
		return enRetVal;
	}
	
	return enRetVal;
}


