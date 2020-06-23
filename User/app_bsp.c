#include "app_bsp.h"

#include "brd_app_cmos.h"

#include "dev_stflash.h"
#include "w25qxx.h" 
#include "dev_usb.h"
#include "bsp.h"


#include "data.h"




/* version */
const uint8_t g_u8Version[4]=
						{
							PLATFORM_TYPE,
						 	1,
						 	2,
						 	15
						 };



static EN_RESP_TYPE sApp_Bsp_cbSaveGain(ST_PRTCL_INFO rcvDataInfo, ST_PRTCL_INFO rcvCmdInfo, u32 addrOffset)
{
	u32 u32baseFlashAddr, u32currentFlashAddr;

	u32baseFlashAddr = MergeU32(rcvCmdInfo.u8CmdParaBuf);
	u32currentFlashAddr = u32baseFlashAddr+addrOffset;

	return W25QXX_writeGainImg(rcvDataInfo.pu8DataBufPointer ,rcvDataInfo.u16ParaLen ,u32currentFlashAddr);
}

static EN_RESP_TYPE sApp_Bsp_cbSaveFw(ST_PRTCL_INFO rcvDataInfo , ST_PRTCL_INFO rcvCmdInfo , u32 addrOffset)
{
	Dev_STFlash_programData(rcvDataInfo.pu8DataBufPointer, rcvDataInfo.u16ParaLen, APP_BACKUP_ADDR+addrOffset);

	return EN_RESP_SUCCESS;
}

extern ADC_HandleTypeDef hadc1;
static uint32_t sApp_Bsp_readADCVal(void)
{
	uint32_t val =0;
	uint32_t i;
	
	for(i=0;i<2;i++)
	{
		val +=BSP_getAdcVal(&hadc1, ADC_CHANNEL_14);
		HAL_ADC_Stop(&hadc1);
	}
	val /=2;
	
	
	return val;
}

/*CMD --- 50H*/
static void _app_Bsp_handShake(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	
#ifdef DEBUG_APP_OXI600
	DBG("OXI600 handshake\r\n");
#endif	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 51H*/
static void _app_Bsp_volSetting(ST_PRTCL_INFO* pstRcvInfo)
{
	uint16_t u16mod1Vol;
	uint8_t u8buVol,u8mod2Vol,u8SPIspeed;
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;

	u16mod1Vol = MergeU16(pstRcvInfo->u8CmdParaBuf);
	u8buVol = pstRcvInfo->u8CmdParaBuf[2];
	u8mod2Vol = pstRcvInfo->u8CmdParaBuf[3];
	u8SPIspeed = pstRcvInfo->u8CmdParaBuf[4];
	/* Voltage Setting CODE BEGAIN */
	
	//BSP_VoltAndSpiSetting(u16mod1Vol,u8buVol,u8mod2Vol,u8SPIspeed);
	
	/* Voltage Setting CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 52H*/
static void _app_Bsp_readBoardVol(ST_PRTCL_INFO* pstRcvInfo)
{
	uint8_t u8chnlSel;
	uint32_t u32volResult;
	uint8_t u8resBuf[4];
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;

	u8chnlSel = pstRcvInfo->u8CmdParaBuf[0];
	/* READ VOLTAGE  CODE BEGAIN */
	
	u32volResult = BSP_ReadVoltage(u8chnlSel);
	SplitU32(u8resBuf,u32volResult);
	
	/* READ VOLTAGE CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 4, u8resBuf);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 53H*/
static void _app_Bsp_boardCurCali(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;

	/* BOARD CURRENT CALIBRATION  CODE BEGAIN */
	
	BSP_MOD1MOD2Calibration();
	
	/* BOARD CURRENT CALIBRATION CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 54H*/
static void _app_Bsp_readBoardCur(ST_PRTCL_INFO* pstRcvInfo)
{	
	uint8_t u8chnlSel,u8resBuf[4];
	uint32_t u32curResult;
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
	u8chnlSel = pstRcvInfo->u8CmdParaBuf[0];
	uint8_t u8mode = pstRcvInfo->u8CmdParaBuf[1];
	/* READ BOARD CURRENT  CODE BEGAIN */
	
	if(u8mode ==  1)	//采图电流
	{
		SplitU32(u8resBuf,g_readCurrent);
	}
	else if(u8mode == 2) 	//当前电流
	{
		u32curResult = BSP_ReadCurrent(u8chnlSel);
		SplitU32(u8resBuf,u32curResult);
	}
	
	/* READ BOARD CURRENT CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 4, u8resBuf);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 55H*/
static void _app_Bsp_powerCtrl(ST_PRTCL_INFO* pstRcvInfo)
{	
	uint8_t u8modSel;
	uint16_t u16PowerDelay;
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
	u8modSel = pstRcvInfo->u8CmdParaBuf[0];
	u16PowerDelay =  pstRcvInfo->u8CmdParaBuf[1];
	/* POWERON OR POWEROFF CODE BEGAIN */
	
	enRlst = BSP_PowerControl(u8modSel,u16PowerDelay);
	
	//cmos 需要单独初始化，上电时
	if((pstRcvInfo->u8PrjType >= EN_PRJ_CMOS_MIN) && \
					(pstRcvInfo->u8PrjType <= EN_PRJ_CMOS_MAX))
	{
		if(u8modSel == 0x80)
			Dev_CMOS_RoicRegInit();
	}
		
	/* POWERON OR POWEROFF CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 56H*/
static void _app_Bsp_backlightSet(ST_PRTCL_INFO* pstRcvInfo)
{	
	uint8_t u8curLed,u8PWMled1,u8PWMled2;
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;

	u8curLed = pstRcvInfo->u8CmdParaBuf[0];
	u8PWMled1 = pstRcvInfo->u8CmdParaBuf[1];
	u8PWMled2 = pstRcvInfo->u8CmdParaBuf[2];
	/* LED BACKLIGHT CONTROL CODE BEGAIN */
	
	enRlst = BSP_SetBacklight(u8curLed,u8PWMled1,u8PWMled2);
	
	/* LED BACKLIGHT CONTROL CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}



/*CMD --- 58H*/
static void _app_Bsp_writeFlash(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	u32 u32ShouldSendDataSize;
	u32 u32flashID;
	
#ifdef DEBUG_APP_OXI600
	DBG("==OXI600 write gain, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	
	u32flashID = W25QXX_getFlashID();
	if(u32flashID == 0 || u32flashID == 0xffffffff)
	{
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_PERI_FLASH_NOT_EXSIT, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}

	u32ShouldSendDataSize = MergeU32(&pstRcvInfo->u8CmdParaBuf[4]);
	
	enRlst = Dev_Usb_recvData(u32ShouldSendDataSize, 60000,pstRcvInfo, sApp_Bsp_cbSaveGain);	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 59H*/
static void _app_Bsp_readFlash(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	u32 u32RcvDataSize;
	u32 u32flashID;

#ifdef DEBUG_APP_OXI600
	DBG("OXI600 read gain, CMD:%02X\r\n",pstRcvInfo->u8Cmd);
	DBG("free ram:%d \r\n",xPortGetFreeHeapSize());
#endif	
	
	u32flashID = W25QXX_getFlashID();
	if(u32flashID == 0 || u32flashID == 0xffffffff)
	{
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_PERI_FLASH_NOT_EXSIT, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}
	
	g_pu8DataBuf = malloc(64*1024);
	if(g_pu8DataBuf == NULL)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("_app_Bsp_readFlash malloc 64k failed\r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}
	
	u32RcvDataSize = MergeU32(&pstRcvInfo->u8CmdParaBuf[4]);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
	Dev_Usb_sendData(g_pu8DataBuf, u32RcvDataSize, 10000, pstRcvInfo, W25QXX_readGainImg);
	free(g_pu8DataBuf);
}


/*CMD --- 5AH*/
static void _app_Bsp_getFlashID(ST_PRTCL_INFO* pstRcvInfo)
{
	uint32_t u32flashID;
	uint8_t u8DataBuf[4];
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
	u32flashID = pstRcvInfo->u8CmdParaBuf[0];
	/* GET FLASH ID CODE BEGAIN */
	
	u32flashID = W25QXX_getFlashID();
	
	/* GET FLASH ID CODE END */
	SplitU32(u8DataBuf,u32flashID);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 4, u8DataBuf);
	Dev_Usb_resp(&stRespInfo);
}
/*CMD --- 5cH*/
static void _app_Bsp_eraseFlash(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	uint32_t u32flashID,u32startAddr,u32size;
	u32startAddr = MergeU32(&pstRcvInfo->u8CmdParaBuf[0]);
	u32size =  MergeU32(&pstRcvInfo->u8CmdParaBuf[4]);
	
	/* GET FLASH ID CODE BEGAIN */
	u32flashID = W25QXX_getFlashID();
	if(u32flashID == 0 || u32flashID == 0xffffffff)
	{
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_PERI_FLASH_NOT_EXSIT, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}
	if(W25QXX_Erase_multi_Sector(u32startAddr, u32size) != EN_RESP_SUCCESS)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("_app_Bsp_eraseFlash paramter err \r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_CMO_ERR_INVALID_PARA, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}
	
	/* GET FLASH ID CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}


/*CMD --- 5eH*/
static void _app_Bsp_getFwVersion(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 4, (uint8_t *)g_u8Version);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 5fH*/
static void _app_Bsp_updateTestBoardFw(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	u16 u16rcvCrc, u16calCrc;
	u32 u32codeLength;
	EN_RESP_TYPE enRlst;
		
	u32codeLength = MergeU32(pstRcvInfo->u8CmdParaBuf);
	u16rcvCrc = MergeU16(&(pstRcvInfo->u8CmdParaBuf[4]));

	IwdgFeed();
	if(Dev_STFlash_eraseSector(APP_BACKUP_ADDR, u32codeLength) == FALSE)
	{
		DBG("flash erase err\r\n");
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ST_FLASH_OPT_ERR , 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}
	
	enRlst = Dev_Usb_recvData(u32codeLength, 60000,pstRcvInfo, sApp_Bsp_cbSaveFw);		
	if(enRlst == EN_RESP_SUCCESS)
	{
		u16calCrc = CRC16((uint8_t* )APP_BACKUP_ADDR, u32codeLength, 0);
		if(u16calCrc != u16rcvCrc)
		{
			DBG("Fw data CRC failed,rcv CRC:%04X, cal CRC:%04X\r\n",u16rcvCrc, u16calCrc);
			prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_CMO_ERR_CHECKUM, 0, NULL);
			Dev_Usb_resp(&stRespInfo);
			return;
		}
		
		IwdgFeed();
		Dev_STFlash_setAppInfo(APP_INFO_ADDR, INFO_APP_NEW);
		Dev_STFlash_setAppInfo(FW_LENGTH_ADDR, u32codeLength-1);
		Dev_STFlash_setAppInfo(FW_CRC_ADDR, u16rcvCrc);
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		HAL_Delay(100);
		NVIC_SystemReset();
	}
}

/*CMD --- 60H*/
static void _app_Bsp_readADCVal(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	uint32_t u32ADCVal;
	uint8 u8ParaBuf[4];
	
	u32ADCVal = sApp_Bsp_readADCVal();		
	SplitU32(u8ParaBuf, u32ADCVal);
		
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 4, u8ParaBuf);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 61H*/
static void _app_Bsp_simplyLEDCtrl(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRslt = EN_RESP_SUCCESS;
	
	if(pstRcvInfo->u8CmdParaBuf[0] == 1)
	{
		GPIO_LED_ON();
	}
	else if(pstRcvInfo->u8CmdParaBuf[0] == 0)
	{
		GPIO_LED_OFF();
	}
	else
	{
		enRslt = EN_RESP_CMO_ERR_INVALID_PARA;
	}
		
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRslt, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}



void App_Bsp_cmdProc(ST_PRTCL_INFO* pstRcvInfo)
{
	switch(pstRcvInfo->u8Cmd)
	{
		/*  tset board universal peripherals part 0x50 start */
		case EN_BSP_CMD_HANDSHAKE:
			_app_Bsp_handShake(pstRcvInfo);
			break;
		
		case EN_BSP_SET_VOL:
			_app_Bsp_volSetting(pstRcvInfo);
			break;

		case EN_BSP_READ_VOL:
			_app_Bsp_readBoardVol(pstRcvInfo);
			break;

		case EN_BSP_ELEC_CALI:
			_app_Bsp_boardCurCali(pstRcvInfo);
			break;

		case EN_BSP_READ_CRUURENT:
			_app_Bsp_readBoardCur(pstRcvInfo);
			break;

		case EN_BSP_POWERON_OFF:
			_app_Bsp_powerCtrl(pstRcvInfo);
			break;

		case EN_BSP_BACKLIGHT_CTL:
			_app_Bsp_backlightSet(pstRcvInfo);
			break;

		case EN_BSP_WRITE_FLASH:
			_app_Bsp_writeFlash(pstRcvInfo);
			break;
			
		case EN_BSP_READ_FLASH:
			_app_Bsp_readFlash(pstRcvInfo);
			break;
			
		case EN_BSP_GET_FLASH_ID:
			_app_Bsp_getFlashID(pstRcvInfo);
			break;

		case EN_BSP_ERASE_FLASH:
			_app_Bsp_eraseFlash(pstRcvInfo);
			break;
			
		case EN_BSP_GET_FW_VERSION:
			_app_Bsp_getFwVersion(pstRcvInfo);
			break;

		case EN_BSP_UPDATE_FW:
			_app_Bsp_updateTestBoardFw(pstRcvInfo);
			break;

		case EN_BSP_READ_ADC_VAL:
			_app_Bsp_readADCVal(pstRcvInfo);
			break;

		case EN_BSP_SIMPLY_LED_CTRL:
			_app_Bsp_simplyLEDCtrl(pstRcvInfo);
			break;
			
		/*  tset board universal peripherals part 0x50 end */
		default:
			break;
	}
}



