/**
*
*
*
**/

#include "oxi_mk_fw.h"
#include "lib_oxi600.h"
#include "oxi_sensor.h"
#include "oxi_tee_memory.h"


typedef int (* SPI_FUN)(void *dataBuf,uint32_t dataLen); 

/**
 * @brief  tee paramter convert to ROIC lib paramter
 * @param  srcParam TEE structural paramter in
 * @param  destParam paramter out after convert
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_convert_param(void* pvSrcParam,ST_CHNL600_CPT_PARA *pstDestParam)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
   ST_CHNL600_CPT_PARA *psrcParam_temp  = (ST_CHNL600_CPT_PARA *)pvSrcParam;
    DBG("_dev_convert_param start\n");
    if(pvSrcParam == NULL || pstDestParam == NULL)
    {
        DBG("_OXIFP_IC_DRV_FW convert para err,param_in == NULL\n");
        return EN_OXI600_PARA_ERR;
    }

    memcpy(pstDestParam, pvSrcParam, sizeof(ST_CHNL600_CPT_PARA));
    switch((sensorType_t)psrcParam_temp->PrjType)
    {
        case SENSORMK720_80:
            pstDestParam->PrjType = EN_PRJ_OXI600_MK720_80UM;
            break;
        case SENSORMK720_100:
            pstDestParam->PrjType = EN_PRJ_OXI600_MK720_100UM;
            break;
        case SENSORMK810_80:
            pstDestParam->PrjType = EN_PRJ_OXI600_MK810_80UM;
            break;
        case SENSORMK320_100:
            pstDestParam->PrjType = EN_PRJ_OXI600_MK320_100UM;
            break;
        case SENSORMK810_80_04:
            pstDestParam->PrjType = EN_PRJ_OXI600_MK810_80UM_1_3;
            break;
        case SENSORMS001_80:
            pstDestParam->PrjType = EN_PRJ_OXI600_MS001_80UM_1_3;
            DBG("oxi project type %x \n",pstDestParam->PrjType);
            break;
        case SENSORM006:
			pstDestParam->PrjType = EN_PRJ_OXI600_MS006_80UM_1_3;
            DBG("oxi project type %x \n",pstDestParam->PrjType);
            break;

        case SENSORM006_V01:
			pstDestParam->PrjType = EN_PRJ_OXI600_MS006_80UM_V01;
            DBG("oxi project type %x \n",pstDestParam->PrjType);
            break;		
			
        default:
            enRetVal =EN_OXI600_PARA_ERR;
            DBG("_OXIFP_IC_DRV_FW convert para err,not support now,project type:%d",pstDestParam->PrjType);

            break;
    }
    DBG("_dev_convert_param end\n");
    return enRetVal;
}


/**
 * @brief  ROIC lib peripheral driver init
 * @param  tee structural   
 * @retval EN_OXI600_ERR_TYPE
**/
oxi_error_t oxi_fw_drv_init(void *phDevice)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    ST_CHNL600_EXTER_DRV *stCh600ExterDrv = NULL;

    DBG("oxi_fw_drv_init strat");
    stCh600ExterDrv = oxi_malloc(sizeof(ST_CHNL600_EXTER_DRV));

    if(phDevice == NULL || stCh600ExterDrv == NULL)
    {
        DBG("_OXIFP_IC_DRV_FW SPI drviver init para err,param_in == NULL\n");
        return (oxi_error_t)EN_OXI600_PARA_ERR;
    }

    oxi_device_handle_t *handle = (oxi_device_handle_t *)phDevice;
    stCh600ExterDrv->delay_ms = handle->protocol_iface.sleep_ms;        /*need user  assignment*/
    stCh600ExterDrv->getLocalTime = handle->protocol_iface.get_uptime;    /*need user  assignment*/
    stCh600ExterDrv->SPI_Receive = (SPI_FUN)handle->protocol_iface.read;
    stCh600ExterDrv->SPI_Send = (SPI_FUN)handle->protocol_iface.write;
    stCh600ExterDrv->SPI_Receive_mass = (SPI_FUN)handle->protocol_iface.read;/*stm32 use it, for tee same with receive*/
    stCh600ExterDrv->SPI_Send_mass = (SPI_FUN)handle->protocol_iface.write;/*stm32 use it, for tee same with send*/
    Dev_Oxi600_DrvInit(stCh600ExterDrv);

	Dev_Oxi600_SetPartPara(int *paraBuf,uint32_t u32bufLen);

	
    DBG("oxi_fw_drv_init end");
    return (oxi_error_t)enRetVal;
}



/**
 * @brief  set low power mode , sleep ROIC and turn off HV
 * @param  phDevice TEE structural 
 * @param  timeout
 * @retval oxi_error_t
 
  EXAMPLE:
    stChnl600CptPara.PrjType = XX;
**/
oxi_error_t oxi_fw_sleep_roic(void *phDevice, void* CptPara, uint32_t timeout)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    DBG("oxi_fw_sleep_roic start");
        
    
    ST_CHNL600_CPT_PARA stChnl600CptPara;
    if(phDevice == NULL )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW sleep roic para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = _dev_convert_param(CptPara,&stChnl600CptPara);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW sleep ROIC convert para err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = Dev_Oxi600_SleepROIC(stChnl600CptPara,timeout);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW sleep roic err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    } 
    DBG("oxi_fw_sleep_roic end ");

    return (oxi_error_t)enRetVal;
}



oxi_error_t  oxi_fw_set_power_mode(void *phDevice, oxiUint8_t pwrMode)
{
    /*same withoxi_fw_sleep_roic, using oxi_fw_sleep_roic()*/
    return (oxi_error_t)EN_OXI600_SUCCESS;
}

oxi_error_t oxi_fw_soft_reset(void *phDevice)
{
    /*It needs to be implemented, but not yet */
    return (oxi_error_t)EN_OXI600_SUCCESS;
}

char temppmu_data[1024]={0xAB,0x71,0x00,0xAC,0x1F,0x00,0xAD,0x0A,0x00,0xAE,0x38,0x00,
0xAF,0x02,0x00,0xB8,0x04,0x00,0xB9,0x01,0x00,0xB0,0x25,0x00,
0xB1,0x00,0x00,0xB2,0x19,0x00,0xB3,0x02,0x00,0xE4,0x09,0x00,
0xE5,0x00,0x00,0xE6,0x04,0x00,0xE7,0x01,0x00,0xB4,0x2C,0x00,
0xB5,0x01,0x00,0xB6,0xAC,0x00,0xB7,0x01,0x00,0xE8,0x2C,0x00,
0xE9,0x01,0x00,0xEA,0xAC,0x00,0xEB,0x01,0x00,0x80,0x36,0x00,
0x81,0x04,0x00,0x82,0xdd,0x00,0x83,0x03,0x00,0x84,0x1e,0x00,
0x85,0x03,0x00,0x86,0x00,0x00,0x87,0xfa,0x00,0x88,0x06,0x00,
0x89,0x1F,0x00,0x8A,0x01,0x00,0x8B,0x01,0x00,0x8C,0xC8,0x00,
0x8D,0x00,0x00,0x8E,0x00,0x00,0x8F,0x54,0x00,0x90,0x00,0x00,
0x91,0x18,0x00,0x92,0x01,0x00,0x93,0x56,0x00,0x94,0x03,0x00,
0x95,0x46,0x00,0x96,0x00,0x00,0x97,0x38,0x00,0x98,0x00,0x00,
0x99,0x38,0x00,0x9A,0x00,0x00,0x9B,0x94,0x00,0x9C,0x05,0x00,
0x9D,0xd2,0x00,0x9E,0x00,0x00,0x9F,0x38,0x00,0xA0,0x00,0x00,
0xA1,0x38,0x00,0xA2,0x00,0x00,0xA3,0xb4,0x00,0xA4,0x05,0x00,
0xA5,0x90,0x00,0xA6,0x01,0x00,0xA7,0xac,0x00,0xA8,0x06,0x00,
0xA9,0x28,0x00,0xAA,0x01,0x00,0xBA,0xC8,0x00,0xBB,0x00,0x00,
0xC1,0x00,0x00,0xC2,0x02,0x00,0xC3,0x00,0x00,0xC4,0x02,0x00,
0xC5,0xFF,0x00,0xC6,0xFF,0x00,0xC7,0x00,0x00,0xC8,0x04,0x00,
0xC9,0x00,0x00,0xCA,0x05,0x00,0xCB,0x05,0x00,0xCC,0x02,0x00,
0xCD,0x81,0x00,0xCE,0x08,0x00,0xCF,0x5F,0x00,0xD0,0x41,0x00,
0xD1,0x10,0x00,0xD2,0x4C,0x00,0xD3,0x78,0x00,0xD4,0x30,0x00,
0xD5,0x00,0x00,0xD6,0x04,0x00,0xD7,0x00,0x00,0xD8,0x44,0x00,
0xD9,0x40,0x00,0xDA,0x00,0x00,0xDB,0x23,0x00,0xDC,0xCB,0x00,
0xDD,0xA6,0x00,0xDE,0x30,0x00,0xDF,0x10,0x00,0xEC,0xFF,0x00,
0xED,0xFF,0x00,0xEE,0x88,0x00,0xEF,0x88,0x00};
/**
 * @brief  set ROIC's PMU table 
 * @param  phDevice TEE structural 
 * @param  pmuParaBlob data buffer to save PMU's data and data length
 * @retval oxi_error_t
**/
oxi_error_t oxi_fw_set_pmu_para(void *phDevice, oxiBlobData_t *pmuParaBlob)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    if(phDevice == NULL || pmuParaBlob == NULL )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW set PMU para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

    DBG("oxi_fw_set_pmu_para start\n");
    pmuParaBlob->pData = oxi_malloc(sizeof(temppmu_data));
//    oxi_tee_read(file_name, tempBuff, cali_size);
    if(pmuParaBlob->pData != NULL){
        DBG("param pmu malloc success");
        memcpy(pmuParaBlob->pData,temppmu_data,sizeof(temppmu_data));
   }
    pmuParaBlob->length= sizeof(temppmu_data);
    DBG("pmuParaBlob->length:%d",pmuParaBlob->length);
    enRetVal = Dev_Oxi600_setPmuPara(pmuParaBlob->pData,pmuParaBlob->length);    
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        /*this function cannot return err*/
        DBG("_OXIFP_IC_DRV_FW set PMU err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    } 
    if(pmuParaBlob->pData)
        oxi_free(pmuParaBlob->pData);
    return (oxi_error_t)enRetVal;
}

char temocapture_data[32]={0x01,0x01,0x01,0x00,
0x14,0x00,0x00,0x00,
0x0A,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
0x0A,0x01,0x01,0x00,
0x14,0x00,0x00,0x00,
0x0A,0x00,0x00,0x00,
0x00,0x03,0xFF,0xFF};
/**
 * @brief  set clear frame flow and capture flow 
 * @param  phDevice TEE structural 
 * @param  cptParaBlob data buffer to save paramter and data length
 * @retval oxi_error_t
**/
oxi_error_t oxi_fw_set_capture_para(void *phDevice, oxiBlobData_t *cptParaBlob)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    if(phDevice == NULL || cptParaBlob == NULL )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW set capture para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

    cptParaBlob->pData = oxi_malloc(sizeof(temocapture_data));
    if(cptParaBlob->pData != NULL){
        DBG("capture malloc success");
        memcpy(cptParaBlob->pData,temocapture_data,sizeof(temocapture_data));
        cptParaBlob->length= sizeof(temocapture_data);
    }
    DBG("cptParaBlob->length %d",cptParaBlob->length);
    enRetVal = Dev_Oxi600_setCptPara(cptParaBlob->pData,cptParaBlob->length);    
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        /*this function cannot return err*/
        DBG("_OXIFP_IC_DRV_FW set capture para err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    if(cptParaBlob->pData)
        oxi_free(cptParaBlob->pData);
    return (oxi_error_t)enRetVal;
}

oxi_error_t oxi_fw_read_flash(void *phDevice, oxiUint8_t partId ,oxiBlobData_t *imgBlob)
{    
    /*It needs to be implemented, but not yet */
    return (oxi_error_t)EN_OXI600_SUCCESS;
}

oxi_error_t oxi_fw_write_flash(void *phDevice, oxiUint8_t partId ,oxiBlobData_t *imgBlob)
{
    /*It needs to be implemented, but not yet */
    return (oxi_error_t)EN_OXI600_SUCCESS;
}

/**
 * @brief  clear frame and get gateoff piexl  
 * @param  phDevice  TEE structural 
 * @param  CptPara ST_CHNL600_CPT_PARA  structural,paramter in
 * @param  imgBlob  data buffer to save piexl data and length
 * @param  timeout
 * @retval oxi_error_t

  EXAMPLE:     
     stChnl600CptPara.PrjType = XX;
     stChnl600CptPara.isGetGateoff = FALSE;  // must FALSE,capture gateoff when integrate frame 
**/
oxi_error_t oxi_fw_clear_frame(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob, uint32_t timeout)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    ST_CHNL600_CPT_PARA pstChnl600CptPara;
    if(phDevice == NULL || CptPara == NULL || imgBlob == NULL)
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW clr frame para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = _dev_convert_param(CptPara,&pstChnl600CptPara);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW clr frame convert para err:%d",enRetVal);
        
        return (oxi_error_t) enRetVal;
    }
    
    enRetVal = Dev_Oxi600_ClrAndGetGateoff(pstChnl600CptPara,imgBlob->pData,&imgBlob->length,pstChnl600CptPara.isGetGateoff,timeout);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW clr frame err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    return (oxi_error_t)enRetVal;
    
}


/**
 * @brief  start capture window image and get the gateoff data
 * @param  phDevice  TEE structural 
 * @param  CptPara ST_CHNL600_CPT_PARA  structural,paramter in 
 * @param  imgBlob  data buffer ,save the gateoff data
 * @param  timeout
 * @retval oxi_error_t

   EXAMPLE:
     stChnl600CptPara.enCptMode = EN_CHNL600_CPT_MODE;  //single finger or double finger
     stChnl600CptPara.enCptType = EN_MODE1_CPT_WIN_IMG; //window image or whole image
     stChnl600CptPara.integraLine = x;
     stChnl600CptPara.shtOrLong = X;
     stChnl600CptPara.PrjType = X;
     stChnl600CptPara.W1ColStrt = X1;
     stChnl600CptPara.W1RowStrt = Y1;
     stChnl600CptPara.W2ColStrt = X2;
     stChnl600CptPara.W2RowStrt = Y2;
	 stChnl600CptPara.isGetGateoff = TRUE/FALSE; // SHORT write TRUE, LONG write FALSE 

**/
oxi_error_t oxi_fw_capture_fast_image(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob,oxiUint32_t timeout)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    ST_CHNL600_CPT_PARA pstChnl600CptPara;
    if(phDevice == NULL || CptPara == NULL)
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW cpture fast image para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = _dev_convert_param(CptPara,&pstChnl600CptPara);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW cpture fast image convert para err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = Dev_Oxi600_CaptureShtInte(pstChnl600CptPara,imgBlob->pData,&imgBlob->length,timeout);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW cpture fast image err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

    return (oxi_error_t)enRetVal;    
}



oxi_error_t oxi_fw_capture_full_partial_image(void *phDevice, void* CptPara,oxiUint32_t timeout)
{
    /*TEE not use now*/
    return (oxi_error_t)EN_OXI600_SUCCESS;
}

/**
 * @brief  capture whole image(include clear frame and get image data)
 * @param  phDevice  TEE structural 
 * @param  CptPara ST_CHNL600_CPT_PARA  structural,paramter in
 * @param  imgBlob  data buffer to save piexl data and length
 * @param  timeout
 * @retval oxi_error_t

 EXAMPLE:
  stChnl600CptPara.enCptType = EN_MODE1_CPT_WHLE_ROW_IMG; // window image or whole image
  stChnl600CptPara.integraLine = x;
  stChnl600CptPara.shtOrLong = X;
  stChnl600CptPara.PrjType = X;
  stChnl600CptPara.XaoDelay = X;
  stChnl600CptPara.u16TmpInte = SHORT_INTE;  // using in Long cail
  stChnl600CptPara.isGetGateoff = Chl600_TRUE/Chl600_FALSE;
**/
oxi_error_t oxi_fw_capture_full_image(void *phDevice, void* CptPara, oxiBlobData_t *imgBlob ,oxiUint32_t timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	ST_CHNL600_CPT_PARA pstChnl600CptPara;
	if(phDevice == NULL || CptPara == NULL)
	{
		enRetVal = EN_OXI600_PARA_ERR;
		DBG("_OXIFP_IC_DRV_FW cpture full para in err:%d",enRetVal);
		
		return (oxi_error_t)enRetVal;
	}
	
	enRetVal = _dev_convert_param(CptPara,&pstChnl600CptPara);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV_FW cpture full convert para err:%d",enRetVal);
		
		return (oxi_error_t)enRetVal;
	}

	enRetVal = Dev_Oxi600_TeeCptWholeRowImage(pstChnl600CptPara,imgBlob->pData,&imgBlob->length,timeout);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV_FW cpture full err:%d",enRetVal);
		
		return (oxi_error_t)enRetVal;
	}
	return (oxi_error_t)enRetVal;
}




/**
 * @brief  capture whole image(include clear frame and get image data)
 * @param  phDevice  TEE structural 
 * @param  CptPara ST_CHNL600_CPT_PARA  structural,paramter in
 * @param  imgBlob  data buffer to save piexl data and length
 * @param  timeout
 * @retval oxi_error_t

 EXAMPLE:
  stChnl600CptPara.enCptType = EN_MODE1_CPT_WHLE_ROW_IMG; // window image or whole image
  stChnl600CptPara.integraLine = x;
  stChnl600CptPara.shtOrLong = X;
  stChnl600CptPara.PrjType = X;
  stChnl600CptPara.XaoDelay = X;
  stChnl600CptPara.u16TmpInte = SHORT_INTE;  // using in Long cail
  stChnl600CptPara.isGetGateoff = Chl600_TRUE/Chl600_FALSE;
**/
														//									short image buffer				long image buffer
oxi_error_t oxi_fw_capture_full_image_ShtAndLong(void *phDevice, void* CptPara, oxiBlobData_t *shtImgBlob ,oxiBlobData_t *longImgBlob,oxiUint32_t timeout)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    ST_CHNL600_CPT_PARA pstChnl600CptPara;
    if(phDevice == NULL || CptPara == NULL)
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW cpture full para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    
    enRetVal = _dev_convert_param(CptPara,&pstChnl600CptPara);
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW cpture full convert para err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

    enRetVal = Dev_Oxi600_TeeCptWholeRowImage_ShtAndLong(pstChnl600CptPara,shtImgBlob->pData,longImgBlob->pData,&shtImgBlob->length,timeout);
	longImgBlob->length = shtImgBlob->length;	// capture same image , same size.
    if(enRetVal != EN_OXI600_SUCCESS)
    {
        DBG("_OXIFP_IC_DRV_FW cpture full err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }
    return (oxi_error_t)enRetVal;
}



/**
 * @brief  receive data when ROCI ready
 * @param  CptPara ST_CHNL600_CPT_PARA  structural,paramter in
 * @param  phDevice  TEE structural 
 * @param  imgBlob  data buffer to save piexl data and length
 * @param  timeout
 * @retval oxi_error_t
**/
oxi_error_t  oxi_fw_get_image_data(void *phDevice, uint16_t int_type, oxiBlobData_t *imgBlob,void* CptPara,uint32_t timeout)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;	
    ST_CHNL600_CPT_PARA stChnl600CptPara;
	
    if(phDevice == NULL || imgBlob == NULL || (int_type == 0 && CptPara == NULL) )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW get image para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

	if(int_type == 0)
	{
		enRetVal = _dev_convert_param(CptPara,&stChnl600CptPara);
		if(enRetVal != EN_OXI600_SUCCESS)
		{
			DBG("_OXIFP_IC_DRV_FW cpture fast image convert para err:%d",enRetVal);
			return (oxi_error_t)enRetVal;
		}

		enRetVal = Dev_Oxi600_getShtAndCptLong(stChnl600CptPara,imgBlob->pData,&imgBlob->length,timeout);
		if(enRetVal != EN_OXI600_SUCCESS)
		{
			DBG("_OXIFP_IC_DRV_FW cpture fast image err:%d",enRetVal);
			return (oxi_error_t)enRetVal;
		}

	}
	else
	{
		enRetVal = Dev_Oxi600_GetLongImageData(imgBlob->pData,&imgBlob->length,timeout);
   	    if(enRetVal != EN_OXI600_SUCCESS)
	    {
	        DBG("_OXIFP_IC_DRV_FW get image  err:%d",enRetVal);
	        
	        return (oxi_error_t)enRetVal;
	    }
			
	}


    return (oxi_error_t) enRetVal;
}

/**
 * @brief  get lib version
 * @param  phDevice  TEE structural 
 * @retval oxi_error_t
**/
oxi_error_t oxi_fw_get_lib_ver(void *phDevice)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    oxi_device_handle_t *handle = (oxi_device_handle_t *)phDevice;
    
    if(phDevice == NULL  )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW get lib para in err:%d",enRetVal);
        
        return (oxi_error_t)enRetVal;
    }

    enRetVal = Dev_Oxi600_getLibVer(handle->device_info.fw_version);
    return (oxi_error_t)enRetVal;
}
oxi_error_t oxi_fw_deinit(void *phDevice)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    oxi_device_handle_t *handle = (oxi_device_handle_t *)phDevice;
    
    if(phDevice == NULL  )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        DBG("_OXIFP_IC_DRV_FW get lib para in err:%d",enRetVal);

        return (oxi_error_t)enRetVal;
    } 

    enRetVal = Dev_Oxi600_Exit();
    return (oxi_error_t)enRetVal;

}

/**
 * @brief  reading flash id
 * @param  phDevice  TEE structural 
 * @retval oxi_error_t
**/
oxi_error_t oxi_fw_flash_read_id(void *phDevice,uint16_t *flashId)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    oxi_device_handle_t *handle = (oxi_device_handle_t *)phDevice;
    
    
    if(phDevice == NULL  )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        return (oxi_error_t)enRetVal;
    }

    *flashId = W25QXX_ReadID();
    return (oxi_error_t)enRetVal;
}

/**
 * @brief  reading flash sn
 * @param  phDevice  TEE structural 
 * @retval oxi_error_t
**/
oxi_error_t oxi_fw_flash_read_sn(void *phDevice, char *pBuffer,uint32_t readAddr,uint16_t numByteToRead)
{
    EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
    oxi_device_handle_t *handle = (oxi_device_handle_t *)phDevice;
    
    if(phDevice == NULL  )
    {
        enRetVal = EN_OXI600_PARA_ERR;
        return (oxi_error_t)enRetVal;
    }

    W25QXX_Read((uint8_t*)pBuffer,readAddr,numByteToRead);
    return (oxi_error_t)enRetVal;
}

