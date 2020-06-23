#include <stdio.h>

#include "app_usb.h"
#include "app_bsp.h"
#include "app_oxi600.h"
#include "app_cmos.h"

#include "dev_usb.h"

#include "protocol.h"
#include "usbd_cdc_if.h"

#define DEBUG_APP_USB



static void sApp_Usb_respErr(ST_PRTCL_INFO* pstRcvInfo, u8 rslt) 
{
	ST_PRTCL_INFO stTmpRespInfo;
	
	prtcl_respPreproc(&stTmpRespInfo, pstRcvInfo,EN_PACK_ID_RESP, rslt, 0, NULL);
	DBG("u16PfrmType=%04X,u8PrjType=%02X ,u8SubPrjType=%02X u8Identify=%02X\r\n",
		stTmpRespInfo.u16PfrmType, stTmpRespInfo.u8PrjType, stTmpRespInfo.u8SubPrjType, stTmpRespInfo.u8Identify);
	Dev_Usb_resp(&stTmpRespInfo);
	
}

/**
  * @brief  App_Usb_rcvProc
  *         application that USB interface communication process
  * @param  none
  * @retval none
  */
void App_Usb_intfcProc(void)
{
	ST_PRTCL_INFO stRcvInfo;
	EN_RESP_TYPE enRslt;
	
	if(CDC_GetRxStat_Hs() == EN_USB_STAT_RCV_CMD)
	{
		/*
			the shorst CMD:
				header(2B) + platType(2B) + prjType(1B) + subPrjType(1B) + ID(1B) + 
				packLen(2B) + Cmd(1B)+ checksum(2B) = 12B
		*/ 
		if(CDC_GetRxLen_Hs() >= 12)
		{
			CDC_ClrRxLen_Hs();
			enRslt = prtcl_rcvPackParsing(&stRcvInfo, g_u8UsbCmdRcvBuf);
			if(enRslt == EN_RESP_SUCCESS)
			{
				/*test board CMD*/
				if((stRcvInfo.u8Cmd >= EN_BSP_CMD_BASE) && \
					(stRcvInfo.u8Cmd <= EN_BSP_CMD_MAX))
				{
					App_Bsp_cmdProc(&stRcvInfo);
				}
				else
				{
					if(((stRcvInfo.u8PrjType >= EN_PRJ_OXI600_MIN) && \
						(stRcvInfo.u8PrjType <= EN_PRJ_OXI600_MAX)) || \
						(stRcvInfo.u8PrjType == 0x00))						/*(stRcvInfo.u8PrjType == 0x00) is the project type "EN_PRJ_TEST_OUTPUT"*/
					{
						App_Oxi600_cmdProc(&stRcvInfo);
					}
						
					if((stRcvInfo.u8PrjType >= EN_PRJ_CMOS_MIN) && \
						(stRcvInfo.u8PrjType <= EN_PRJ_CMOS_MAX))
					{
						App_cmos_cmdProc(&stRcvInfo);
					}
				}
			}
			else
			{
			#ifdef DEBUG_APP_USB
				DBG("invalid CMD package,err code:%02X\r\n",enRslt);
			#endif
				sApp_Usb_respErr(&stRcvInfo, enRslt);
			}
		}
	}
	else
	{
		
	}
}



