#include <stdio.h>

#include "dev_usb.h"
#include "bsp.h"

#include "data.h"
#include "heap_5.h"

#include "usbd_cdc_if.h"
#include "usbd_def.h" 

#define DEBUG_DEV_USB /*the switch to enable or disable debug log func*/

/**
  * @brief  Dev_Usb_remallocRxBuf
  *         malloc buffer with setting size for USB receive data;
  * @param  size			input	the buffer size of you want to malloc;  						
  *						
  * @retval the result of function run, false => malloc failed, true => run success.
  *
  * @note:  can not add any printf in this function,becasue it will run before UARST init!
  */
bool Dev_Usb_remallocRxBuf(u32 size)
{
	if(size == 0)
	{
		free(g_pu8UsbRcvDataBuf);
	}
	else
	{
		g_pu8UsbRcvDataBuf = malloc(size);
		if(!g_pu8UsbRcvDataBuf)
		{
			return FALSE;
		}
	}

	return TRUE;
}

bool Dev_Usb_switchUsbRxStat(EN_CDC_HS_STAT_TYPE enUsbRxStat, u32 u32RxSize)
{
	CDC_ClrRxLen_Hs();
	
	if(enUsbRxStat == EN_USB_STAT_RCV_DATA)
	{
		if(Dev_Usb_remallocRxBuf(u32RxSize) == FALSE)
		{
			return FALSE;
		}
		CDC_SetRxStat_Hs(EN_USB_STAT_RCV_DATA);
	}
	else
	{
		Dev_Usb_remallocRxBuf(0);/*free usb rx data buffer*/
		CDC_SetRxStat_Hs(EN_USB_STAT_RCV_CMD);
	}
	
	return TRUE;
}

/**
  * @brief  Dev_Usb_resp
  *         send to host that CMD respond or data by USB interface;
  * @param  *pstRespInfo	input	respond information;
  			*DataBuf		input   only used in ;
  			*pstRcvInfo		input	get the package header from CMD sent by host.
  						
  *						
  * @retval the result of function run, false => malloc failed, true => run success.
  */
bool Dev_Usb_resp(ST_PRTCL_INFO* pstRespInfo)
{
	uint8_t* pu8DstBuf = NULL;
	uint32_t u32GetBufLen, u32ShouldRespLen;
	
	/*
		header(2B) + plfm&prj(4B) + ID(1B) + packLen(2B) + pack len 
	*/
	u32ShouldRespLen = 2+4+1+2+pstRespInfo->u16PackLen;
	if(pstRespInfo->u8Identify == EN_PACK_ID_RESP)
	{
		pu8DstBuf = malloc(u32ShouldRespLen);
		if(!pu8DstBuf)
		{
		#ifdef DEBUG_DEV_USB
			DBG("Dev_Usb_resp malloc size %d failed, free ram:%d \r\n", \
									u32ShouldRespLen,xPortGetFreeHeapSize());
		#endif	
			return FALSE;
		}
	}
	else/*respond data*/
	{
	#ifdef DEBUG_DEV_USB
		DBG("Dev_Usb_resp==call wrong func, send data should call fun:Dev_Usb_sendData\r\n");
	#endif	
		return FALSE;
	}
	u32GetBufLen =prtcl_respPackPacketization(pu8DstBuf, pstRespInfo);
	if(u32GetBufLen != u32ShouldRespLen)
	{
	#ifdef DEBUG_DEV_USB
		DBG("buf len err, cal len:%d, get len:%d \r\n",u32ShouldRespLen,u32GetBufLen);
	#endif	
		free(pu8DstBuf);
	
		return FALSE;
	}
	
	/*send data from USB*/
	CDC_Transmit_HS_Timeout(pu8DstBuf, u32ShouldRespLen,5000);
	
	free(pu8DstBuf);
	
	return TRUE;
}

bool Dev_Usb_sendData(uint8_t* sendBuf, uint32_t dataSize, uint32_t timeout, ST_PRTCL_INFO* pstRcvInfo, clBckSendDataProcsFun FuncDataProc)
{
	uint32_t i, u32TotSendPackNum, u32ProcDataOffset;
	uint16_t u16SendDataLen, u16TrulySendLen;
	ST_PRTCL_INFO stTmpRespInfo; 
	uint8_t u8UsbComRslt;
	EN_PACK_ID enPackID; 
	bool bRslt = TRUE;

	u32ProcDataOffset = MergeU32(pstRcvInfo->u8CmdParaBuf);
	u32TotSendPackNum = getAlignTimes(dataSize, USB_HS_TX_PER_PACK_MAX_LEN);
	for(i=0; i< u32TotSendPackNum; i++)
	{
		IwdgFeed();
		if(i == u32TotSendPackNum-1)/*last package*/
		{
			enPackID = EN_PACK_ID_DATA_LAST;
			u16SendDataLen = dataSize % USB_HS_TX_PER_PACK_MAX_LEN;
		}
		else/*non-last package*/
		{
			enPackID = EN_PACK_ID_DATA;
			u16SendDataLen = USB_HS_TX_PER_PACK_MAX_LEN;
		}
		
		/* GET DATA CODE BEGAIN */
		if(FuncDataProc != NULL)
		{
			IwdgFeed();
			FuncDataProc(&sendBuf[9], u16SendDataLen, u32ProcDataOffset);
			u32ProcDataOffset += u16SendDataLen;
		}
		/* GET DATA CODE END */
		
		prtcl_respPreproc(&stTmpRespInfo, pstRcvInfo, enPackID, NULL, u16SendDataLen, &sendBuf[9]);
		u16TrulySendLen =prtcl_respPackPacketization(sendBuf, &stTmpRespInfo);
		u8UsbComRslt = CDC_Transmit_HS_Timeout(sendBuf, u16TrulySendLen,timeout);
		if(u8UsbComRslt != USBD_OK)
		{
		#ifdef DEBUG_DEV_USB
			DBG("usb send data pack num %d failed \r\n",i);
		#endif	
			bRslt = FALSE;
			break;
		}
	}

	return bRslt;
}

/**
  * @brief  sDev_Usb_recvPackData
  *         send to host that CMD respond or data by USB interface;
  * @param  RespType		input	respond type(CMD respond / non-last(last) data package;
  			*rslt			input   respond result,used only in CMD respond;
  			*paraBuf		input   respond parameters data buffer,them used in both Cmd;
  									respond and data;
  			*pstRcvInfo		input	get the package header from CMD sent by host.
  						
  *						
  * @retval the result of function run, false => malloc failed, true => run success.
  */
static EN_RESP_TYPE sDev_Usb_recvPackData(ST_PRTCL_INFO* pstRcvInf, u32 rcvSize, u32 timeout)
{
	volatile u32 vu32LocalTick, vu32wdgTick = 0;
	EN_RESP_TYPE enRslt = EN_RESP_SUCCESS;
	
	
	vu32LocalTick = HAL_GetTick();
	while(1)
	{
		if(CDC_GetRxLen_Hs() >= rcvSize)
		{
			break;
		}
		
		vu32wdgTick++;
		if(vu32wdgTick % 1000 == 0)
		{
			IwdgFeed();
		}

		if(HAL_GetTick() - vu32LocalTick > timeout)
		{
			enRslt = EN_RESP_CMO_ERR_RCV_TIMEOUT;
		}
	}
		
	if(enRslt == EN_RESP_CMO_ERR_RCV_TIMEOUT)
	{
	#ifdef DEBUG_DEV_USB
		DBG("rcv data timeout \r\n");
	#endif	
		return enRslt;
	}
	
#ifdef DEBUG_DEV_USB
	DBG("rcv data success,size:%d \r\n",CDC_GetRxLen_Hs());
#endif	
	CDC_ClrRxLen_Hs();
	enRslt = prtcl_rcvPackParsing(pstRcvInf, g_pu8UsbRcvDataBuf);
	
	return enRslt;
}

EN_RESP_TYPE Dev_Usb_recvData(u32 rcvSize, u32 timeout, ST_PRTCL_INFO* pstRcvInfo, clBckRcvDataProcsFun FuncDataProc)
{
	EN_RESP_TYPE enRslt;
	u32 u32PerPackRcvSize,i, u32TotRcvPackNum, u32ProcDataOffset;
	ST_PRTCL_INFO stTmpRcvInfo, stTmpRespInfo;
#ifdef DEBUG_DEV_USB	
	DBG("recv truly data:%d\r\n",rcvSize);
#endif
	if(rcvSize < USB_HS_RX_PER_PACK_MAX_LEN)
	{
		u32PerPackRcvSize = rcvSize + 11;
	}
	else
	{
		u32PerPackRcvSize = USB_HS_RX_PER_PACK_MAX_LEN + 11;
	}
	
	if(Dev_Usb_switchUsbRxStat(EN_USB_STAT_RCV_DATA, u32PerPackRcvSize) == FALSE)
	{
	#ifdef DEBUG_APP_OXI600
		DBG("Dev_Usb_recvData malloc usb rx buffer fail\r\n");
	#endif	
		Dev_Usb_switchUsbRxStat(EN_USB_STAT_RCV_CMD, NULL);
		return EN_RESP_SYS_ERR_MALLOC_FAIL; 
	}
	
	prtcl_respPreproc(&stTmpRespInfo, pstRcvInfo,EN_PACK_ID_RESP, EN_RESP_SUCCESS, 0, NULL);
	Dev_Usb_resp(&stTmpRespInfo);
	
	u32TotRcvPackNum = getAlignTimes(rcvSize, USB_HS_RX_PER_PACK_MAX_LEN);
	u32ProcDataOffset = 0;
	for(i=0; i<u32TotRcvPackNum; i++)
	{
		IwdgFeed();
		if(i == u32TotRcvPackNum-1)/*last data package*/
		{
			u32PerPackRcvSize = rcvSize % USB_HS_RX_PER_PACK_MAX_LEN + 11;
			enRslt = sDev_Usb_recvPackData(&stTmpRcvInfo, u32PerPackRcvSize, timeout);
			if(enRslt == EN_RESP_SUCCESS)
			{
				if(stTmpRcvInfo.u8Identify == EN_PACK_ID_DATA_LAST)
				{
					/* SAVE DATA PROCESS BEGAIN */
					if(FuncDataProc != NULL)
					{
						IwdgFeed();
						enRslt = FuncDataProc(stTmpRcvInfo, *pstRcvInfo, u32ProcDataOffset);
						if(enRslt !=  EN_RESP_SUCCESS)
						{
							prtcl_respPreproc(&stTmpRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRslt, 0, NULL);
							Dev_Usb_resp(&stTmpRespInfo);
							break;
						}
						u32ProcDataOffset += stTmpRcvInfo.u16ParaLen;
					}
					/* SAVE DATA PROCESS END */ 
				}
				else
				{
					enRslt = EN_RESP_CMO_ERR_RCV_LAST_DATA_PACK;
				}
			}
			else
			{
			#ifdef DEBUG_APP_OXI600
				DBG("recv last data pack:%d failed, err code:%d\r\n",i,enRslt);
			#endif
				break;
			}
		}
		else
		{
			enRslt = sDev_Usb_recvPackData(&stTmpRcvInfo, u32PerPackRcvSize, timeout);
			if(enRslt == EN_RESP_SUCCESS)
			{
				if(stTmpRcvInfo.u8Identify == EN_PACK_ID_DATA)
				{
					/* SAVE DATA PROCESS BEGAIN */
					if(FuncDataProc != NULL)
					{
						IwdgFeed();
						enRslt = FuncDataProc(stTmpRcvInfo, *pstRcvInfo, u32ProcDataOffset);
						if(enRslt !=  EN_RESP_SUCCESS)
						{
							prtcl_respPreproc(&stTmpRespInfo, pstRcvInfo,EN_PACK_ID_RESP, enRslt, 0, NULL);
							Dev_Usb_resp(&stTmpRespInfo);
							break;
						}
						u32ProcDataOffset += stTmpRcvInfo.u16ParaLen;
					}
					/* SAVE DATA PROCESS END */ 
					prtcl_respPreproc(&stTmpRespInfo, &stTmpRcvInfo,EN_PACK_ID_RESP, enRslt, 0, NULL);
					Dev_Usb_resp(&stTmpRespInfo);
				#ifdef DEBUG_DEV_USB	
					DBG("resp finish,i=%d\r\n",i);
				#endif	
				}
				else
				{
				#ifdef DEBUG_DEV_USB
					DBG("rcv last pack err,pack num:%d\r\n",i);
				#endif	
					enRslt = EN_RESP_CMO_ERR_RCV_LAST_DATA_PACK;
					break;
				}
			}
			else
			{
			#ifdef DEBUG_APP_OXI600
				DBG("recv non-last data pack:%d failed, err code:%d\r\n",i,enRslt);
			#endif
				break;
			}
		}		
	}
	
	Dev_Usb_switchUsbRxStat(EN_USB_STAT_RCV_CMD, NULL);
	return enRslt;
}






