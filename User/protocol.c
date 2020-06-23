#include <string.h>
#include <stdio.h>
#include "protocol.h"

#define DEBUG_PRTCL_EN

/*command data buffer parsing*/
EN_RESP_TYPE prtcl_rcvPackParsing(ST_PRTCL_INFO* pstRcvInfo, uint8_t* RcvBuf)
{
	u16 u16RcvPackHeader,u16CalChecksum, u16BufOfst;
	EN_RESP_TYPE enRslt = EN_RESP_SUCCESS;
#ifdef DEBUG_PRTCL_EN
	u32 i;
#endif

	u16BufOfst = 0;
	/*package header*/
	u16RcvPackHeader 	= MergeU16(&RcvBuf[u16BufOfst]);
	u16BufOfst += 2; 
	if(IS_PACK_HEADER_INSTANCE(u16RcvPackHeader) == FALSE)
	{
	#ifdef DEBUG_PRTCL_EN
		DBG("recv package header invalid, recv header:%04X\r\n",u16RcvPackHeader );
	#endif
		enRslt = EN_RESP_CMO_ERR_PACK_HEADER;
		return enRslt;
	}

	/*platform type*/
	pstRcvInfo->u16PfrmType 	= MergeU16(&RcvBuf[u16BufOfst]);	
	u16BufOfst += 2; 
	if(IS_PFRM_INSTANCE(pstRcvInfo->u16PfrmType) == FALSE)
	{
	#ifdef DEBUG_PRTCL_EN
		DBG("recv platform type invalid, recv platform:%04X\r\n",pstRcvInfo->u16PfrmType);
	#endif
		enRslt = EN_RESP_CMO_ERR_PFRM_TYPE;
		return enRslt;
	}

	/*project type*/
	pstRcvInfo->u8PrjType    = RcvBuf[u16BufOfst];
	u16BufOfst += 1; 
	
#if 0
	if(IS_PRJ_INSTANCE(pstRcvInfo->u8PrjType) == FALSE)
	{
	#ifdef DEBUG_PRTCL_EN
		DBG("recv project type invalid, recv prj type:%02X\r\n",pstRcvInfo->u8PrjType );
	#endif
		enRslt = EN_RESP_CMO_ERR_PRJ_TYPE;
		return enRslt;
	}
#endif

	/*sub-project type*/
	pstRcvInfo->u8SubPrjType = RcvBuf[u16BufOfst];
	u16BufOfst += 1; 

	/*package identify*/
	pstRcvInfo->u8Identify = RcvBuf[u16BufOfst];
	u16BufOfst += 1; 
	if(IS_RCVE_PACK_ID_INSTANCE(pstRcvInfo->u8Identify) == FALSE)
	{
	#ifdef DEBUG_PRTCL_EN
		DBG("recv package identify invalid, recv package identify:%02X\r\n",pstRcvInfo->u8Identify);
	#endif
		enRslt = EN_RESP_CMO_ERR_PACK_ID;
		return enRslt;
	}

	/*package length*/
	pstRcvInfo->u16PackLen = MergeU16(&RcvBuf[u16BufOfst]);
	u16BufOfst += 2; 

	/*----------distinguish CMD and data package--------------*/
	if((pstRcvInfo->u8Identify == EN_PACK_ID_CMD) || \
		(pstRcvInfo->u8Identify == EN_PACK_ID_RESP))/*receive CMD*/
	{
		if(pstRcvInfo->u8Identify == EN_PACK_ID_CMD)/*CMD in pacakge from host to device*/
		{
			pstRcvInfo->u8Cmd= RcvBuf[u16BufOfst];
		}
		else if(pstRcvInfo->u8Identify == EN_PACK_ID_RESP)/*respond in pacakge from device to host*/
		{
			pstRcvInfo->u8Rslt= RcvBuf[u16BufOfst];
		}
		u16BufOfst += 1; 

		/*copy cmd parameter data*/
		pstRcvInfo->u16ParaLen = pstRcvInfo->u16PackLen-1-2;/*Cmd 1B,checksum 2B*/
		memcpy(pstRcvInfo->u8CmdParaBuf, &RcvBuf[u16BufOfst], pstRcvInfo->u16ParaLen);
		u16BufOfst += pstRcvInfo->u16ParaLen;
		u16CalChecksum = Checksum16(&RcvBuf[BUF_CAL_VERIFY_SHIFT], (u32)(pstRcvInfo->u16PackLen+1));
	}
	else/*receive data*/
	{
		pstRcvInfo->u16ParaLen = pstRcvInfo->u16PackLen-2;/*checksum 2B*/
		pstRcvInfo->pu8DataBufPointer = &RcvBuf[u16BufOfst];
		u16BufOfst += pstRcvInfo->u16ParaLen;
		/*data package use CRC checksum:2 bytes pack len + 1 byte pack identify*/
		u16CalChecksum = CRC16(&RcvBuf[BUF_CAL_VERIFY_SHIFT], (u32)(pstRcvInfo->u16PackLen+1),BASCAL_CRC_VAL);
	}
	
	pstRcvInfo->u16RcvChecksum = MergeU16(&RcvBuf[u16BufOfst]);
	if(u16CalChecksum != pstRcvInfo->u16RcvChecksum)
	{
	#ifdef DEBUG_PRTCL_EN
		DBG("crc check fail, recv crc:%04X, cal crc:%04X\r\n",pstRcvInfo->u16RcvChecksum, u16CalChecksum);
		DBG("cal ckecksum len:%d\r\n",(pstRcvInfo->u16PackLen+1+2));
		for(i=0;i < (pstRcvInfo->u16PackLen+1+2); i++ )
		{
			DBG("%02X ",RcvBuf[BUF_CAL_VERIFY_SHIFT+i] );
		}
		DBG("\r\n");
	#endif
		/*later optimization*/
		enRslt = EN_RESP_CMO_ERR_CHECKUM;
		return enRslt;
	}
	
	return enRslt;
}

/**
  * @brief  prtcl_respPreproc
  *         preprocess for responding to host by filling in variable stRespInfo for neccessry vaule;
  *
  * @param  pstCmdInfo	input	respond type(CMD respond / non-last(last) data package;
  			plfmType			input   respond result,used only in CMD respond;
  			prjType		input   respond parameters data buffer,them used in both Cmd;
  			subprjType	input					respond and data;
  			packID		input	get the package header from CMD sent by host.
  			command		input
  			paraLen		input
  			*paraBuf	input
  						
  *						
  * @retval the result of function run, false => malloc failed, true => run success.
  */
void prtcl_hostCmdPreproc(ST_PRTCL_INFO* pstCmdInfo, uint16_t plfmType, uint8_t prjType, uint8_t subprjType,
								EN_PACK_ID packID, uint8_t command, uint16_t paraLen, uint8_t* paraBuf )
{
	pstCmdInfo->u16PfrmType = plfmType;
	pstCmdInfo->u8PrjType = prjType;
	pstCmdInfo->u8SubPrjType = subprjType;
	pstCmdInfo->u16ParaLen = paraLen;
	pstCmdInfo->u8Identify = packID;
	if((pstCmdInfo->u8Identify == EN_PACK_ID_RESP) || \
		(pstCmdInfo->u8Identify == EN_PACK_ID_CMD))/*CMD respond*/
	{
		DBG("cmd respond\r\n");
		pstCmdInfo->u8Cmd = command;
		memcpy(pstCmdInfo->u8CmdParaBuf, paraBuf, paraLen);/*merge respond parameters*/
		pstCmdInfo->u16PackLen = 1+paraLen+2;/*rslt(1B)+paraLen + checksum(2B)*/
	}
	else/*data respond*/
	{
		pstCmdInfo->pu8DataBufPointer = paraBuf;
		pstCmdInfo->u16PackLen = paraLen+2;/*there is no rslt for CMD respond */
	}
}

/**
  * @brief  prtcl_respPreproc
  *         preprocess for responding to host by filling in variable stRespInfo for neccessry vaule;
  * @param  RespType		input	respond type(CMD respond / non-last(last) data package;
  			*rslt			input   respond result,used only in CMD respond;
  			*paraBuf		input   respond parameters data buffer,them used in both Cmd;
  									respond and data;
  			*pstRcvInfo		input	get the package header from CMD sent by host.
  						
  *						
  * @retval the result of function run, false => malloc failed, true => run success.
  */
void prtcl_respPreproc(ST_PRTCL_INFO* pstRespInfo, ST_PRTCL_INFO* pstRcvInfo,
								EN_PACK_ID packID, uint8_t rslt, uint16_t paraLen, uint8_t* paraBuf )
{
	memcpy((uint8_t* )pstRespInfo, (uint8_t* )pstRcvInfo, 4);/*merge protocol header*/
	pstRespInfo->u16ParaLen = paraLen;
	pstRespInfo->u8Identify = packID;
	if((pstRespInfo->u8Identify == EN_PACK_ID_RESP) || \
		(pstRespInfo->u8Identify == EN_PACK_ID_CMD))/*CMD respond*/
	{
		pstRespInfo->u8Rslt = rslt;
		memcpy(pstRespInfo->u8CmdParaBuf, paraBuf, paraLen);/*merge respond parameters*/
		pstRespInfo->u16PackLen = 1+paraLen+2;/*rslt(1B)+paraLen + checksum(2B)*/
	}
	else/*data respond*/
	{
		pstRespInfo->pu8DataBufPointer = paraBuf;
		pstRespInfo->u16PackLen = paraLen+2;/*there is no rslt for CMD respond */
	}
}

/*respond packetization*/
u16 prtcl_respPackPacketization(uint8_t* DstBuf, ST_PRTCL_INFO* pstRespInfo)
{
	u16 u16BufOfst, u16Checksum;
	
	u16BufOfst = 0;
	/*package header*/
	SplitU16(&DstBuf[u16BufOfst], (u16)PACK_HEADER);
	u16BufOfst += 2;
	
	/*platform type*/
	SplitU16(&DstBuf[u16BufOfst], pstRespInfo->u16PfrmType);
	u16BufOfst += 2;
	
	/*project type*/
	DstBuf[u16BufOfst] = pstRespInfo->u8PrjType;
	u16BufOfst += 1;
	
	/*sub-project type, useless in some projects*/
	DstBuf[u16BufOfst] = pstRespInfo->u8SubPrjType;
	u16BufOfst += 1;
	
	/*respond type,it contain CMD RESPOND, NON-LAST DATA PACKAGE, LAST DATA PACKAGE*/
	DstBuf[u16BufOfst] = pstRespInfo->u8Identify;
	u16BufOfst += 1;

	/*package length*/
	SplitU16(&DstBuf[u16BufOfst], pstRespInfo->u16PackLen);
	u16BufOfst += 2;
	
	if((pstRespInfo->u8Identify == EN_PACK_ID_RESP) || \
		(pstRespInfo->u8Identify == EN_PACK_ID_CMD))
	{
		/*save result*/
		DstBuf[u16BufOfst] = pstRespInfo->u8Rslt;
		u16BufOfst += 1;

		/*save CMD parameters or data*/
		memcpy(&DstBuf[u16BufOfst], pstRespInfo->u8CmdParaBuf, pstRespInfo->u16ParaLen);
		
	}
	else
	{
		memcpy(&DstBuf[u16BufOfst], pstRespInfo->pu8DataBufPointer, pstRespInfo->u16ParaLen);
	}
	u16BufOfst += pstRespInfo->u16ParaLen;

	/*cal checksum*/
	if((pstRespInfo->u8Identify == EN_PACK_ID_RESP) || \
		(pstRespInfo->u8Identify == EN_PACK_ID_CMD))
	{
		/*1 byte identify + 2 bytes packlen - checksum self*/
		u16Checksum = Checksum16(&DstBuf[BUF_CAL_VERIFY_SHIFT], (u32)(1+pstRespInfo->u16PackLen+2-2));
	}
	else
	{
		u16Checksum = CRC16(&DstBuf[BUF_CAL_VERIFY_SHIFT], (u32)(1+pstRespInfo->u16PackLen+2-2),BASCAL_CRC_VAL);
	}
	SplitU16(&DstBuf[u16BufOfst], u16Checksum);
	u16BufOfst += 2;
	
	return u16BufOfst;
}



