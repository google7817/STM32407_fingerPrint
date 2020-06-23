#include <stdio.h>

#include "app_oxi600.h"

#include "dev_usb.h"


#include "usbd_cdc_if.h"
#include "usbd_cdc.h"

#include "heap_5.h"
#include "data.h"
#include "w25qxx.h" 


#define DEBUG_APP_OXI600


/*CMD --- 01H*/
static void _app_Oxi600_cptWinImg(ST_PRTCL_INFO* pstRcvInfo)
{
	uint8_t u8WinNum;
	uint8_t u8TransBuf[4];
	uint16_t u16W1Stat_X, u16W1Stat_Y;
	uint32_t u32ImgDataLen;
	ST_PRTCL_INFO stRespInfo;
	ST_CHNL600_CPT_PARA stChanl600Info;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;

	u8WinNum	  = pstRcvInfo->u8CmdParaBuf[0];
	u32ImgDataLen = MergeU32(&(pstRcvInfo->u8CmdParaBuf[1]));
	u16W1Stat_X   = MergeU16(&(pstRcvInfo->u8CmdParaBuf[5]));
	u16W1Stat_Y   = MergeU16(&(pstRcvInfo->u8CmdParaBuf[7]));

	
	/*CHECK PARAMETERS CODE BEGIN*/
	if((u8WinNum != 1)||(u32ImgDataLen > IMG_MAX_SIZE))
	{
	#ifdef DEBUG_APP_OXI600
		DBG("host send parameter invalid:u8WinNum--%d,imgsize-- %d\r\n",u8WinNum,u32ImgDataLen);
	#endif
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return; 
	}
	/*CHECK PARAMETERS CODE END*/
	
	g_pu8DataBuf = malloc(u32ImgDataLen+11+404*20);
	if(g_pu8DataBuf == NULL)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("_app_Oxi600_cptWinImg malloc 64k failed\r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}

	/*CAPTURE IMAGE CODE BEGIN*/
		/*==>save start in &g_pu8DataBuf[9]*/
	stChanl600Info.enCptMode = EN_MODE1_CPT_SGL_WIN;
	stChanl600Info.enCptType = EN_MODE1_CPT_WIN_IMG;
	stChanl600Info.FingerNo = u8WinNum;
	stChanl600Info.CptDataSize = u32ImgDataLen;
	stChanl600Info.W1ColStrt = u16W1Stat_X;
	stChanl600Info.W1RowStrt = u16W1Stat_Y ;
	stChanl600Info.PrjType = pstRcvInfo->u8PrjType;
	if((g_u8ShtOrLong != 0) && (g_u8ShtOrLong != 1) )
		stChanl600Info.shtOrLong = 2;
	else 	
		stChanl600Info.shtOrLong = g_u8ShtOrLong;
	stChanl600Info.XaoDelay = 0;
	stChanl600Info.isGetGateoff = Chl600_TRUE;
	stChanl600Info.u16TmpInte = g_u16ShtIntBack;
	DBG("**sht or long = %d \n",stChanl600Info.shtOrLong);
	IwdgFeed();
	enRetVal = Brd_Oxi600_CaptureWinImg(stChanl600Info,&g_pu8DataBuf[9],&u32ImgDataLen);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		enRetVal -= EN_OXI600_ERR_BASE_VAL; // !error code start at 3000,but respon result only one byte 
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, (uint8_t)enRetVal, 0, NULL );
		Dev_Usb_resp(&stRespInfo);
		free(g_pu8DataBuf);
		return;
	}
	/*CAPTURE IMAGE CODE END*/
	SplitU32(u8TransBuf,g_readCurrent);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 4, u8TransBuf );
	Dev_Usb_resp(&stRespInfo);
	Dev_Usb_sendData(g_pu8DataBuf, stChanl600Info.CptDataSize, 10000, pstRcvInfo, NULL);
	free(g_pu8DataBuf);
}

/*CMD --- 02H*/
static void _app_Oxi600_cptGainImg(ST_PRTCL_INFO* pstRcvInfo)
{
	uint8_t u8WhlRowImgNum;
	uint32_t u32ImgDataLen,u32datalen;
	uint8_t u8TransBuf[4];
	ST_PRTCL_INFO stRespInfo;
	ST_CHNL600_CPT_PARA stChanl600Info;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;


	u8WhlRowImgNum = pstRcvInfo->u8CmdParaBuf[0];
	u32ImgDataLen  = MergeU32(&pstRcvInfo->u8CmdParaBuf[1]);
	/*CHECK POSITION CODE BEGIN*/
	if((u8WhlRowImgNum > 5)||(u32ImgDataLen > IMG_MAX_SIZE))
	{
	#ifdef DEBUG_APP_OXI600	
		DBG("host send parameter invalid:u8WhlRowImgNum--%d,imgsize-- %d\r\n",u8WhlRowImgNum,u32ImgDataLen);
	#endif
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return; 
	}
	/*CHECK POSITION CODE END*/
	
	g_pu8DataBuf = malloc(u32ImgDataLen+11+404*20);
	if(g_pu8DataBuf == NULL)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("_app_Oxi600_cptGainImg malloc 63k+11 failed\r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}

	/*CAPTURE IMAGE CODE BEGIN*/
		/*==>save start in &g_pu8DataBuf[9]*/
	stChanl600Info.enCptMode = EN_MODE1_CPT_SGL_WIN;
	stChanl600Info.enCptType = EN_MODE1_CPT_WHLE_ROW_IMG;
	stChanl600Info.CptDataSize = u32ImgDataLen;
	stChanl600Info.PrjType = pstRcvInfo->u8PrjType;
	stChanl600Info.WhlImageNo = u8WhlRowImgNum;	
	if((g_u8ShtOrLong != 0) && (g_u8ShtOrLong != 1) )
		stChanl600Info.shtOrLong = 2;
	else 	
		stChanl600Info.shtOrLong = g_u8ShtOrLong;
	
	stChanl600Info.XaoDelay = 0;
	stChanl600Info.isGetGateoff = Chl600_TRUE;
	stChanl600Info.u16TmpInte = g_u16ShtIntBack;
	IwdgFeed();
	enRetVal = Brd_Oxi600_CaptureWhlImg(stChanl600Info,&g_pu8DataBuf[9],&u32datalen);
	if(enRetVal != EN_RESP_SUCCESS)
	{
		enRetVal -= EN_OXI600_ERR_BASE_VAL; // !error code start at 3000,but respon result only one byte 
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, (uint8_t)enRetVal, 0, NULL );
		Dev_Usb_resp(&stRespInfo);
		free(g_pu8DataBuf);
		return;
	}
	/*CAPTURE IMAGE CODE END*/
	
	SplitU32(u8TransBuf,g_readCurrent);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 4,u8TransBuf );
	Dev_Usb_resp(&stRespInfo);
	Dev_Usb_sendData(g_pu8DataBuf, u32ImgDataLen, 10000, pstRcvInfo, NULL);
	free(g_pu8DataBuf);
}



/*CMD --- 21H*/
static void _app_Oxi600_setIntegralLineCnt(ST_PRTCL_INFO* pstRcvInfo)
{
	u16 u16IntegralLineCnt;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	ST_PRTCL_INFO stRespInfo;

	u16IntegralLineCnt = MergeU16(pstRcvInfo->u8CmdParaBuf);
	/* CHECK INTEGRAL PARAMETER VALID+ SAVE INTEGRAL LINE CNT  CODE BEGAIN */

	Brd_Oxi600_SetIntegrationLine(u16IntegralLineCnt);
	
	/* CHECK INTEGRAL PARAMETER VALID+ SAVE INTEGRAL LINE CNT  CODE BEGAIN */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 28H*/
static void _app_Oxi600_switchVcomVol(ST_PRTCL_INFO* pstRcvInfo)
{
	uint8_t u8volMode,u8cptFramCnt;
	ST_PRTCL_INFO stRespInfo;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;

	/* switch vcom  CODE BEGAIN */
	u8volMode = pstRcvInfo->u8CmdParaBuf[0];
	u8cptFramCnt = pstRcvInfo->u8CmdParaBuf[1];
	enRetVal = Dev_Oxi600_SwitchVcomVol(u8volMode,u8cptFramCnt);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		enRetVal -= EN_OXI600_ERR_BASE_VAL; // !error code start at 3000,but respon result only one byte 
	}
	/* switch vcom CNT  CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, (uint8_t)enRetVal, 0 ,NULL);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 22H*/
static void _app_Oxi600_getIntegralLineCnt(ST_PRTCL_INFO* pstRcvInfo)
{
	uint16_t u16IntegralLineCnt;
	uint8_t u8ParaBuf[4];
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;

	/* GET INTEGRAL LINE CNT  CODE BEGAIN */
	
	Brd_Oxi600_GetIntergrationLine(u8ParaBuf);
	
	/* GET INTEGRAL LINE CNT  CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 4, u8ParaBuf);
	Dev_Usb_resp(&stRespInfo);

}


/*CMD --- 24H*/
static void _app_Oxi600_setClrFlow(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;	
	uint32_t u32ShouldSendDataSize;
	
#ifdef DEBUG_APP_OXI600
	DBG("==OXI600 set clr flow, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	
	/* SET CLEAR FLOW  CODE BEGAIN */

	u32ShouldSendDataSize = MergeU16(&pstRcvInfo->u8CmdParaBuf[0]);
	
	enRlst = Dev_Usb_recvData(u32ShouldSendDataSize, 60000,pstRcvInfo, Brd_Oxi600_setClrPara);	

	/* SET CLEAR FLOW   CODE END */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);


}


/*CMD --- 29*/
static void _app_Oxi600_setPmuPara(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	uint32_t u32ShouldSendDataSize;
	
#ifdef DEBUG_APP_OXI600
	DBG("==OXI600 set para, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	

	u32ShouldSendDataSize = MergeU16(&pstRcvInfo->u8CmdParaBuf[0]);
	
	enRlst = Dev_Usb_recvData(u32ShouldSendDataSize, 60000,pstRcvInfo, Brd_Oxi600_setPmuPara);	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);


}

/*CMD --- 2AH*/
static void _app_Oxi600_rdPmuRegVal(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	uint8_t u8RegAddr, u8RegValBuf[2];
	uint16_t u16RegVal;
	


	u8RegAddr = pstRcvInfo->u8CmdParaBuf[0];
	u16RegVal = Dev_Oxi600_ReadRegVal(u8RegAddr);
#ifdef DEBUG_APP_OXI600
	DBG("==OXI600 read reg addr :%02X,reg val:%04X\r\n",pstRcvInfo->u8CmdParaBuf[0],u16RegVal);
#endif	

	SplitU16(u8RegValBuf, u16RegVal);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 2, u8RegValBuf);
	Dev_Usb_resp(&stRespInfo);
}

/*CMD --- 2BH*/
static void _app_Oxi600_lowPowerMode(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	ST_CHNL600_CPT_PARA stChanl600Info;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	
	stChanl600Info.PrjType = pstRcvInfo->u8PrjType;

	
	enRetVal = Dev_Oxi600_SleepROIC(stChanl600Info,1000);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		enRetVal -= EN_OXI600_ERR_BASE_VAL; // !error code start at 3000,but respon result only one byte 
	}
	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, (uint8_t)enRetVal, 0,NULL );
	Dev_Usb_resp(&stRespInfo);
}



/*CMD --- F0H*/
static void _app_Oxi600_CheckModule(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t u8CheckValue = 0, u8CheckDelay = 0, u8SpiVlaue[2];
	
	u8CheckValue = pstRcvInfo->u8CmdParaBuf[0];
	u8CheckDelay = pstRcvInfo->u8CmdParaBuf[1];

	HAL_Delay(u8CheckDelay);
	u8SpiVlaue[0] = 0x3e;u8SpiVlaue[1] = 0xff;
	Dev_SPI_SendData(&hspi1, u8SpiVlaue, 2, 200);
	u8SpiVlaue[0] = 0xff;u8SpiVlaue[1] = 0xff;
	Dev_SPI_ReceiveData(&hspi1, u8SpiVlaue, 2, 200);

#if(PLATFORM_TYPE == PLATFORM_6500)
	if (u8CheckValue == u8SpiVlaue[0])
		HAL_GPIO_WritePin(GPIOA, DIR_BUF1_Pin, GPIO_PIN_SET);
	else
		enRetVal = EN_OXI600_MODULE_ERR - EN_OXI600_ERR_BASE_VAL;
#endif	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, (uint8_t)enRetVal, 0,NULL );
	Dev_Usb_resp(&stRespInfo);
}

/*CMD ERROR*/
static void _app_Oxi600_errCmdProc(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	
#ifdef DEBUG_APP_OXI600
	DBG("OXI600 recevie CMD err, CMD:%02X\r\n",pstRcvInfo->u8Cmd);
#endif
	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_CMO_ERR_INVALID_CMD, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
}




void App_Oxi600_cmdProc(ST_PRTCL_INFO* pstRcvInfo)
{
	
	Brd_Oxi600_DrvInit();
	switch(pstRcvInfo->u8Cmd)
	{

		case EN_OXI600_CMD_CAPTURE_WIN_IMAGE:
			_app_Oxi600_cptWinImg(pstRcvInfo);
			break;

		case EN_OXI600_CMD_CAPTURE_GAIN_IMAGE:
			_app_Oxi600_cptGainImg(pstRcvInfo);

			break;

		case EN_OXI600_CMD_SET_INTEGRAL_LINE_CNT:
			_app_Oxi600_setIntegralLineCnt(pstRcvInfo);
			break;

		case EN_OXI600_CMD_GET_INTEGRAL_LINE_CNT:
			_app_Oxi600_getIntegralLineCnt(pstRcvInfo);
			break;
			
		case EN_OXI600_CMD_SET_CLR_LAG_FLOW:
			_app_Oxi600_setClrFlow(pstRcvInfo);
			break;

		case EN_OXI600_CMD_SWITCH_VCOM_VOL:
			_app_Oxi600_switchVcomVol(pstRcvInfo);
			break;

		case EN_OXI600_CMD_SET_PMU_PARA:
			_app_Oxi600_setPmuPara(pstRcvInfo);
			break;

		case EN_OXI600_CMD_RD_PMU_REG:
			_app_Oxi600_rdPmuRegVal(pstRcvInfo);
			break;

		case EN_OXI600_CMD_LOW_POWER_MODE:
			_app_Oxi600_lowPowerMode(pstRcvInfo);
			break;		
		case EN_OXI600_CMD_CHECK_MODULE:
			_app_Oxi600_CheckModule(pstRcvInfo);
			break;
			
		default:
			_app_Oxi600_errCmdProc(pstRcvInfo);
			break;
	}
}

void App_Oxi600_ageingProc(void)
{
	uint8_t u8WinNum;
	uint16_t u16W1Stat_X, u16W1Stat_Y;
	uint32_t u32ImgDataLen,i;
	ST_PRTCL_INFO stRespInfo;
	ST_CHNL600_CPT_PARA stChanl600Info;
	static bool sbPowerSet = FALSE;

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET); /* lingt on */
	
 	if(sbPowerSet == FALSE)
 	{
 		sbPowerSet = TRUE;
		BSP_VoltAndSpiSetting(0x0CE4, 0x0E,0x01, 0x02);/*set voltage and SPI speed*/
		BSP_PowerControl(0x80,0x00);/*power on*/
		Brd_Oxi600_DrvInit();
 	}
	
	u8WinNum	  = 1;
//	u32ImgDataLen = 138*(138+10)*2;
	u32ImgDataLen = 173*128*2;
	u16W1Stat_X   = 50;
	u16W1Stat_Y   = 50;

	g_pu8DataBuf = malloc(u32ImgDataLen+11);
	if(g_pu8DataBuf == NULL)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("App_Oxi600_ageingProc malloc %d bytes failed\r\n",u32ImgDataLen+11);
	#endif	
		return;
	}


	/*CAPTURE IMAGE CODE BEGIN*/
		/*==>save start in &g_pu8DataBuf[9]*/
	stChanl600Info.enCptMode = EN_MODE1_CPT_SGL_WIN;
	stChanl600Info.enCptType = EN_MODE1_CPT_WIN_IMG;
	stChanl600Info.FingerNo = u8WinNum;
	stChanl600Info.CptDataSize = u32ImgDataLen;
	stChanl600Info.W1ColStrt = u16W1Stat_X;
	stChanl600Info.W1RowStrt = u16W1Stat_Y ;
//	stChanl600Info.PrjType = EN_PRJ_OXI600_MK810_80UM_1_3;
	stChanl600Info.PrjType = EN_PRJ_OXI600_MK720_100UM;
	stChanl600Info.shtOrLong = 2;
	stChanl600Info.XaoDelay = 0;
	
	IwdgFeed();
	if(Brd_Oxi600_CaptureWinImg(stChanl600Info,&g_pu8DataBuf[9],&u32ImgDataLen) != EN_OXI600_SUCCESS)
	{
		DBG("cpture image failed\r\n");
		free(g_pu8DataBuf);
		return;
	}
	
	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET); /* lingt off */
	free(g_pu8DataBuf);
}


