/***************************************************************************************************
** File name   : app_cmos.c
** Function    : App_cmos_cmdProc
** Author      : sun
** Date        : 2019
** Version     : v0.1
** Description : cmos 命令函数入口
** ModifyRecord: 
***************************************************************************************************/

/***************************************************************************************************
** Include Header Files
***************************************************************************************************/
#include <stdio.h>

#include "brd_app_cmos.h"
#include "lib_cmos.h"
#include "dev_usb.h"
#include "usbd_cdc_if.h"

#include "stm32f4xx_hal.h"


/***************************************************************************************************
** Local Macro Definition
***************************************************************************************************/
#define DEBUG_APP_CMOS		1

/***************************************************************************************************
** Local Type Definition
***************************************************************************************************/
extern volatile int g_readCurrent;

#pragma pack(1)
typedef struct _CMOS_REG_RW
{
	uint8_t RegAddr;
	uint8_t Value;
	uint16_t Wait_ms;
}ST_CMOS_REG_RW;

#pragma pack(1)
typedef struct _CMOS_REG_RW_ARG
{
	uint16_t cnt;
	ST_CMOS_REG_RW* pdata;
}ST_CMOS_REG_RW_ARG;

#pragma pack(1)
typedef struct _CMOS_WAIT_STATE_ARG
{
	uint8_t RegAddr;
	uint16_t RegValMask;
	uint16_t ChkRegVal;
	uint32_t TimeOver;
}ST_CMOS_WAIT_STATE_ARG;

#pragma pack(1)
typedef struct _CMOS_DMA_RX_ARG
{
	uint16_t length;
	uint8_t* RcvBuf;
}ST_CMOS_DMA_RX_ARG;


#define CMOS_RW_REG_SET(pdata, index, adr,val, delay)	\
		pdata[index].RegAddr=adr;\
		pdata[index].Value=val;\
		pdata[index].Wait_ms=delay;

#define CMOS_WAIT_STATE_SET(wait_state_arg, adr, mask, check_val, timeout)	\
		wait_state_arg.RegAddr=adr;\
		wait_state_arg.RegValMask=mask;\
		wait_state_arg.ChkRegVal=check_val;\
		wait_state_arg.TimeOver=timeout;


/***************************************************************************************************
** Constant Variable Declaration
***************************************************************************************************/

/***************************************************************************************************
** Local static Variable Declaration
***************************************************************************************************/
static EN_RESP_TYPE Brd_CMOS_setPmuPara(ST_PRTCL_INFO rcvDataInfo, ST_PRTCL_INFO rcvCmdInfo, u32 addrOffset)
{
	Dev_CMOS_setPmuPara(rcvDataInfo.pu8DataBufPointer,rcvDataInfo.u16ParaLen);
	
	return EN_RESP_SUCCESS;
}

static EN_RESP_TYPE Brd_CMOS_WriteRegs(ST_PRTCL_INFO rcvDataInfo, ST_PRTCL_INFO rcvCmdInfo, u32 addrOffset)
{
	u16 u16len  = rcvDataInfo.u16ParaLen;
	u16 i;
	
	for(i=0;i<u16len;i+=3)
	{
		_dev_CMOS_WriteReg(rcvDataInfo.pu8DataBufPointer[i], rcvDataInfo.pu8DataBufPointer[i+1]);
		HAL_Delay(rcvDataInfo.pu8DataBufPointer[i+2]);
	}
	return EN_RESP_SUCCESS;
}

static EN_RESP_TYPE Brd_CMOS_ReadRegs(ST_PRTCL_INFO rcvDataInfo, ST_PRTCL_INFO rcvCmdInfo, u32 addrOffset)
{
	u16 u16len  = rcvDataInfo.u16ParaLen, u16TmpRegVal;;
	u16 i;

	memcpy(g_pu8DataBuf+9,rcvDataInfo.pu8DataBufPointer, u16len);//前10个是包头

	for(i=0;i<u16len;i++)
	{
		//DBG("[%2x]\r\n",g_pu8DataBuf[i+9]);
		u16TmpRegVal = _dev_CMOS_ReadReg(g_pu8DataBuf[i+9]);
		g_pu8DataBuf[i+9] = u16TmpRegVal;
	}
	
	return EN_RESP_SUCCESS;
}

/***************************************************************************************************
** Subroutine  : 
** Function    : _app_cmos_read_regs
** Input       : ST_PRTCL_INFO* pstRcvInfo
** Output      : none
** Description : usb 读寄存器预处理及响应
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
static void _app_cmos_read_regs(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	u16 u16len;
	
#if DEBUG_APP_CMOS
	DBG("%s\r\n",__FUNCTION__);
#endif

	u16len = MergeU16(&pstRcvInfo->u8CmdParaBuf[0]);
#if DEBUG_APP_CMOS
	DBG("arg len,%d\r\n",u16len);
#endif
	
	g_pu8DataBuf = malloc(u16len+80);
	if(g_pu8DataBuf == NULL)
	{
	#if DEBUG_APP_CMOS
		DBG("_app_cmos_read_regs malloc  failed\r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}

	enRlst = Dev_Usb_recvData(u16len, 60000,pstRcvInfo, Brd_CMOS_ReadRegs);	

	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);

	Dev_Usb_sendData(g_pu8DataBuf, u16len, 10000, pstRcvInfo, NULL);
	free(g_pu8DataBuf);
}


/***************************************************************************************************
** Subroutine  : 
** Function    : _app_cmos_write_regs
** Input       : ST_PRTCL_INFO* pstRcvInfo
** Output      : none
** Description : usb 写寄存器预处理及相应
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
static void _app_cmos_write_regs(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
	u32 u32ShouldSendDataSize;
	
#ifdef DEBUG_APP_CMOS
	DBG("cmos write regs, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	

	u32ShouldSendDataSize = MergeU16(&pstRcvInfo->u8CmdParaBuf[0]);
	
	enRlst = Dev_Usb_recvData(u32ShouldSendDataSize, 60000,pstRcvInfo, Brd_CMOS_WriteRegs);	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/***************************************************************************************************
** Subroutine  : 
** Function    : _app_cmos_errCmdProc
** Input       : ST_PRTCL_INFO* pstRcvInfo
** Output      : none
** Description : usb 错误命令处理及相应
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
static void _app_cmos_errCmdProc(ST_PRTCL_INFO* pstRcvInfo)
{
	ST_PRTCL_INFO stRespInfo;
	
#if DEBUG_APP_CMOS
	DBG("\r\nrecevie CMD err, CMD:%02X\r\n",pstRcvInfo->u8Cmd);
#endif
	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_CMO_ERR_INVALID_CMD, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
}

/***************************************************************************************************
** Global Variable Declaration
***************************************************************************************************/

/***************************************************************************************************
** Global Function Declaration
***************************************************************************************************/

static void _app_CMOS_cptWinImg(ST_PRTCL_INFO* pstRcvInfo)
{
	uint8_t u8TransBuf[4];
	uint32_t u32ImgDataLen;
	ST_PRTCL_INFO stRespInfo;

	u32ImgDataLen = 88*86*2+0;

	g_pu8DataBuf = malloc(u32ImgDataLen+11);
	if(g_pu8DataBuf == NULL)
	{
	#ifdef DEBUG_APP_CMOS
		DBG("_app_cmos_cptWinImg malloc 64k failed\r\n");
	#endif	
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SYS_ERR_MALLOC_FAIL, 0, NULL);
		Dev_Usb_resp(&stRespInfo);
		return;
	}

	IwdgFeed();
	if(Dev_Cmos_Cpt(&g_pu8DataBuf[9]))
	{
		prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_CPT_ERR, 0, NULL );
		Dev_Usb_resp(&stRespInfo);
		free(g_pu8DataBuf);
		return;
	}
	
#ifdef DEBUG_APP_CMOS
	DBG("pic send\r\n");
#endif	
	/*CAPTURE IMAGE CODE END*/
	SplitU32(u8TransBuf,g_readCurrent);
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 4, u8TransBuf );
	Dev_Usb_resp(&stRespInfo);
	Dev_Usb_sendData(g_pu8DataBuf, u32ImgDataLen, 10000, pstRcvInfo, NULL);
	free(g_pu8DataBuf);
}



static void _app_CMOS_setIntegralLineCnt(ST_PRTCL_INFO* pstRcvInfo)
{
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	ST_PRTCL_INFO stRespInfo;

#ifdef DEBUG_APP_CMOS
	DBG("cmos CMD:%02X \r\n",pstRcvInfo->u8Cmd);
#endif	
	Dev_Cmos_SetIntTime(pstRcvInfo->u8CmdParaBuf);
	
	/* CHECK INTEGRAL PARAMETER VALID+ SAVE INTEGRAL LINE CNT  CODE BEGAIN */
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL );
	Dev_Usb_resp(&stRespInfo);
}


static void _app_CMOS_setPmuPara(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	u32 u32ShouldSendDataSize;
	
#ifdef DEBUG_APP_CMOS
	DBG("cmos set para, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	

	u32ShouldSendDataSize = MergeU16(&pstRcvInfo->u8CmdParaBuf[0]);
	
	enRlst = Dev_Usb_recvData(u32ShouldSendDataSize, 60000,pstRcvInfo, Brd_CMOS_setPmuPara);

DBG("cmos set para end==\r\n");	
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}


static void _app_CMOS_set_status(ST_PRTCL_INFO* pstRcvInfo)
{	
	ST_PRTCL_INFO stRespInfo;
	EN_RESP_TYPE enRlst = EN_RESP_SUCCESS;
	
#ifdef DEBUG_APP_CMOS
	DBG("cmos set para, CMD:%02X==\r\n",pstRcvInfo->u8Cmd);
#endif	
		
	uint8_t u8status = pstRcvInfo->u8CmdParaBuf[0];
	uint16_t u16delay = MergeU16(pstRcvInfo->u8CmdParaBuf+1);

	enRlst = EN_RESP_SUCCESS;
	switch(u8status)
	{
		case CMOS_RESET:
			Dev_Cmos_Reset( u16delay);
			break;
		
		case CMOS_WAKEUP:
			Dev_Cmos_WakeUp();
			break;
		
		case CMOS_SLEEP:
			Dev_CMOS_SleepROIC(0);
			break;
	
		case CMOS_RESTART:
			Dev_Cmos_Restart();
			break;
		
		default:
			enRlst = EN_RESP_CMO_ERR_INVALID_PARA;
#ifdef DEBUG_APP_CMOS
	DBG(" para err :%02X==\r\n",u8status);
#endif	
			break;				
	}
		
	prtcl_respPreproc(&stRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRlst, 0, NULL);
	Dev_Usb_resp(&stRespInfo);
}

/***************************************************************************************************
** Subroutine  : 
** Function    : App_cmos_cmdProc
** Input       : ST_PRTCL_INFO* pstRcvInfo
** Output      : none
** Description : cmos 命令函数入口
** Date        : 2019
** ModifyRecord:
***************************************************************************************************/
void App_cmos_cmdProc(ST_PRTCL_INFO* pstRcvInfo)
{
#if DEBUG_APP_CMOS
	DBG("\r\n\r\n %s CMD:%02X\r\n",__FUNCTION__,pstRcvInfo->u8Cmd);
#endif
	IwdgFeed();
	Brd_CMOS_DrvInit();
	switch(pstRcvInfo->u8Cmd)
	{
		case EN_CMOS_CMD_CAPTURE_WIN_IMAGE:
			_app_CMOS_cptWinImg(pstRcvInfo);
			break;

		case EN_CMOS_CMD_SET_INTEGRAL_LINE_CNT:
			_app_CMOS_setIntegralLineCnt(pstRcvInfo);
			break;

		case EN_CMOS_CMD_SET_PMU_PARA:
			_app_CMOS_setPmuPara(pstRcvInfo);
			break;

		case EN_CMOS_CMD_SET_CMOS_STATUS:
			_app_CMOS_set_status(pstRcvInfo);
			break;

		
		case EN_CMOS_CMD_READ_REGS:
			_app_cmos_read_regs(pstRcvInfo);
			break;

		case EN_CMOS_CMD_WRITE_REGS:
			_app_cmos_write_regs(pstRcvInfo);
			break;

		default:
			_app_cmos_errCmdProc(pstRcvInfo);
			break;
	}
}
