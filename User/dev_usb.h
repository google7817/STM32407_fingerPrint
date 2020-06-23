#ifndef __DEV_USB_H
#define __DEV_USB_H

#include "type.h"
#include "protocol.h"
#include "stm32f4xx_hal.h"
#include "usbd_cdc_if.h"

#define USB_HS_TX_UNIT_LEN					(512U)
#define USB_HS_TX_PER_PACK_MAX_LEN			(63*1024U)
#define USB_HS_RX_PER_PACK_MAX_LEN			(63*1024U)

/*3 parameter: data buffer , data size , buffer offset*/
typedef EN_RESP_TYPE (*clBckSendDataProcsFun)(u8* , u32 , u32);
typedef EN_RESP_TYPE (*clBckRcvDataProcsFun)(ST_PRTCL_INFO , ST_PRTCL_INFO , u32);


extern bool Dev_Usb_switchUsbRxStat(EN_CDC_HS_STAT_TYPE enUsbRxStat, u32 u32RxSize);
extern bool Dev_Usb_resp(ST_PRTCL_INFO* pstRespInfo);
bool Dev_Usb_sendData(u8* sendBuf, u32 dataSize, u32 timeout, ST_PRTCL_INFO* pstRcvInfo, clBckSendDataProcsFun FuncDataProc);
EN_RESP_TYPE Dev_Usb_recvData(u32 rcvSize, u32 timeout, ST_PRTCL_INFO* pstRcvInfo, clBckRcvDataProcsFun FuncDataProc);

extern bool Dev_Usb_remallocRxBuf(u32 size);

#endif /* __DEV_OXI_H */

