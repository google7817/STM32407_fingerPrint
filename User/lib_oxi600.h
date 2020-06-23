
#ifndef __LIB_OXI600_H
#define __LIB_OXI600_H

//#include "type.h"
#include <stdint.h>

typedef uint32_t Chl600_bool;
#define Chl600_TRUE   	((uint32_t) 1)
#define Chl600_FALSE	((uint32_t) 0)

#ifndef NULL	
#define	NULL			((void*)0)
#endif

#define CHNL600_ROIC_FIFO_SIZE					(64*1024)			/*ROIC FIFO SIZE 64K*/


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~ROIC command~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#define ROIC_CMD_RD_ST()						_dev_Oxi600_WriteReg(0x3F,0xFF)
#define ROIC_CMD_WAKEUP()						_dev_Oxi600_WriteReg(0xF1,0xFF)
#define ROIC_CMD_SLEEP()						_dev_Oxi600_WriteReg(0xF2,0xFF)
#define ROIC_CMD_RUN()							_dev_Oxi600_WriteReg(0xF3,0xFF)
#define ROIC_CMD_RAM_TEST()						_dev_Oxi600_WriteReg(0xF5,0xFF)
#define ROIC_CMD_RESTART()						_dev_Oxi600_WriteReg(0xF8,0xFF)
#define ROIC_CMD_CLEAR()						_dev_Oxi600_WriteReg(0xFB,0xFF)
#define ROIC_CMD_TRANSFER()						_dev_Oxi600_WriteReg(0xFC,0xFF)
#define ROIC_CMD_I2C()							_dev_Oxi600_WriteReg(0xFE,0xFF)
#define ROIC_CMD_DUMMY()						_dev_Oxi600_WriteReg(0xFF,0xFF)


/*
 *	REG 0xAA :XAO and PIRST control register
 *       bit: [7-4]  write 0,unused    
 *		 bit:	[3]  PIRST controled enable: 0: bit[1] cann't control PIRST
 *											 1: PIRST controled by bit[1]
 *		 bit:	[2]
 *		 bit:	[1]  PIRST control: when bit[3] setting 1,0:low level ,1:high level
 *		 bit:	[0]  xao control :  0:low level ,1:high level 
 */
#define PIRST_H()								_dev_Oxi600_WriteReg(0xAA,0x0B)		
#define PIRST_L()								_dev_Oxi600_WriteReg(0xAA,0x01)
#define XAO_H()									_dev_Oxi600_WriteReg(0xAA,0x01)
#define XAO_L()									_dev_Oxi600_WriteReg(0xAA,0x00)
#define XAO_H_PIRST_H()							_dev_Oxi600_WriteReg(0xAA,0x0B)
#define XAO_L_PIRST_H()							_dev_Oxi600_WriteReg(0xAA,0x0A)
#define VCOM_TO_VCOM(value)						_dev_Oxi600_WriteReg(0xCF,value)
#define VCOM_TO_VDD(value)						_dev_Oxi600_WriteReg(0xCF,value)
#define VCOM_TO_VEE(value)						_dev_Oxi600_WriteReg(0xCF,value)
#define DISABLE_ROIC_ADC(value)					_dev_Oxi600_WriteReg(0xCF,value)

#define Oxi600_ROIC_REG_P1_CPV_LOW				0x80
#define Oxi600_ROIC_REG_P2_CPV_HIGH				0x82
#define Oxi600_ROIC_REG_P3_STV_START			0x84
#define Oxi600_ROIC_REG_P4_STV_HIGH				0x87
#define Oxi600_ROIC_REG_ROW_MAX					0xB8
#define Oxi600_ROIC_REG_IS_WRITE_DATA 			0xC4
#define Oxi600_ROIC_REG_FRAME_CNT				0xC2
#define Oxi600_ROIC_REG_ROW_MAX2				0xBA
#define Oxi600_ROIC_REG_C6_ACTIVE_CAP			0xC6
typedef enum _EN_CHNL600_VCOM_MODE
{	
	EN_RESET_TO_DEFAULT							=0x00,
	EN_VCOM_TO_VDD								=0x01,
	EN_VCOM_TO_VEE								=0x02,
	EN_VCOM_TO_VCOM								=0x03,
}EN_CHNL600_VCOM_MODE;


typedef enum _EN_CHNL600_CPT_MODE
{
	EN_MODE0_CPT_WHL_IMG					= 0x70,/*capture whole image -- not support now*/
	EN_MODE1_CPT_SGL_WIN					= 0x71,/*capture single window */
	EN_MODE2_CPT_DBL_WIN					= 0x72,/*capture double window */
		
}EN_CHNL600_CPT_MODE;


typedef enum _EN_CHNL600_CPT_TYPE
{
	EN_MODE1_CPT_WIN_IMG					= 0x01,/*capture window image*/
	EN_MODE1_CPT_WHLE_ROW_IMG				= 0x02,/*capture whole row image*/
	
}EN_CHNL600_CPT_TYPE;

typedef enum _EN_OXI600_ERROR_TYPE
{
	EN_OXI600_SUCCESS						=0x00,
	EN_OXI600_ERR_BASE_VAL 					=3000,
	EN_OXI600_IMAGE_SIZE_ERR,
	EN_OXI600_REG_INIT_ERR,
	EN_OXI600_CLR_RUN_ERR,
	EN_OXI600_CLR_RESTAT_ERR,	
	EN_OXI600_CPT_RUN_ERR,
	EN_OXI600_CPT_RESTAT_ERR,
	EN_OXI600_DATA_READY_ERR,
	EN_OXI600_SLEEP_ROIC_ERR,
	EN_OXI600_PROJECT_TYPE_ERR,				
	EN_OXI600_CHECK_STATUS_TIMEOUT,	
	EN_OXI600_PARA_ERR,
	EN_OXI600_COOR_ERR,
	EN_OXI600_READ_ID_ERR,
	EN_OXI600_MODULE_ERR,
	EN_OXI600_VCOM_STATUS_ERR,
}EN_OXI600_ERR_TYPE;

/*ROIC capacity value*/
typedef enum _EN_CHL600_CAPACITY
{
	EN_CAP_0_1PF = 0x00	,	
	EN_CAP_0_2PF = 0x10 ,
	EN_CAP_0_3PF = 0x20 ,
	EN_CAP_0_4PF = 0x30,
	EN_CAP_0_5PF = 0x40,
	EN_CAP_0_6PF = 0x50,
	EN_CAP_0_7PF = 0x60,
	EN_CAP_0_8PF = 0x80,
	EN_CAP_0_9PF = 0x90,
	EN_CAP_1_0PF = 0xA0,
	EN_CAP_1_1PF = 0xB0,
	EN_CAP_1_2PF = 0xC0,
	EN_CAP_1_3PF = 0xD0,
	EN_CAP_1_4PF = 0xE0,
	EN_CAP_1_5PF = 0xF0,

}EN_CHL600_CAPACITY;

/*project type*/
typedef enum _EN_CHL600_PRJ_TYPE
{
	EN_PRJ_TEST_OUTPUT 		= 0x00,
	EN_PRJ_OXI600_MK720_80UM= 0x40, 
	EN_PRJ_OXI600_MK720_100UM, 
	EN_PRJ_OXI600_MK810_80UM,
	EN_PRJ_OXI600_MK320_100UM,
	EN_PRJ_OXI600_MK810_80UM_1_3,
	EN_PRJ_OXI600_MK720_80UM_1_3,
	EN_PRJ_OXI600_MK720_100UM_1_3,
	EN_PRJ_OXI600_MS001_80UM_1_3,
	EN_PRJ_OXI600_MS006_80UM_1_3,
	EN_PRJ_OXI600_MS006_80UM_V01,
	
}EN_CHL600_PRJ_TYPE;


typedef struct 
{
	void (*delay_ms)(uint32_t ms);
	uint64_t (*getLocalTime)(void);

	int (*SPI_Send)(void *dataBuf,uint32_t length);
	int (*SPI_Receive)(void *dataBuf,uint32_t length);

	int (*SPI_Send_mass)(void *dataBuf,uint32_t length);		/*mass of data*/
	int (*SPI_Receive_mass)(void *dataBuf,uint32_t length);  /*mass of data*/

}ST_CHNL600_EXTER_DRV;

typedef struct
{										/* 注释以-分隔，第一个代表使用环境，第二个代表参数内容详情  */
	uint32_t CptDataSize;				/* stm32f404 - */
	uint8_t FingerNo;					/* no use */
	uint8_t WhlImageNo;					/* stm32f404 - using in capture whole */
	EN_CHNL600_CPT_MODE enCptMode;		/* all - singal finger: 0x71,double finger :0x72 */
	EN_CHL600_PRJ_TYPE PrjType;			/* all - MK720/MK810.... */
	EN_CHNL600_CPT_TYPE enCptType;		/* all - whole:2 or window image:1 */
	uint16_t W1ColStrt;					/* all - X1 */
	uint16_t W1RowStrt;					/* all - Y1 */	
	uint16_t W1ColNo;					/* all - no use */
	uint16_t W1RowNo;					/* all - whole image row number */
	uint16_t W2ColStrt; 				/* all - X2 */																					
	uint16_t W2RowStrt;					/* all - Y2 */
	uint16_t W2ColNo; 					/* no use */
	uint16_t W2RowNo;					/* no use */
	uint16_t integraLine;				/* all - integral lines */ 
	uint8_t shtOrLong;					/* all - capture short:0 and long image:1 esle : use default*/
	uint16_t XaoDelay;					/* all - using in capture whole image,between clr and capture*/
	Chl600_bool 	 isGetGateoff;		/* all - using in clear frame 1:capture gateoff, 0: dont capture gateoff*/
	uint16_t u16TmpInte;				/* TEE - short integration lines,when cail long integration, should capture short integration fisrt*/
}ST_CHNL600_CPT_PARA;


typedef struct
{	
	Chl600_bool bClrLagEn;					/*swtich to on/off clear lag operation*/

	Chl600_bool bVddEn;						/*swtich Vcom to Vdd flag*/
	uint16_t u8VddScnCnt;					/*frame scan number in VDD*/
	uint32_t u32SwVddDelay;					/*swtich Vcom voltage to Vee delay, unit--ms*/
	uint16_t u16VddPeriod;					/*CPV period*/
	uint16_t u16VddFsStvCovCnt;				/*STV cover CPV number*/
	
	Chl600_bool bVeeEn;						/*swtich Vcom to Vee flag*/
	uint32_t u32SwVeeDelay;					/*swtich Vcom voltage to Vee delay, unit--ms*/
	uint16_t u8VeeScnCnt;					/*frame scan number in VDD*/
	uint16_t u16VeePeriod;					/*CPV period*/
	uint16_t u16VeeFsStvCovCnt;				/*STV cover CPV number*/

	Chl600_bool bVcomEn;					/*swtich Vcom to Vcom flag*/
	uint32_t u32SwVcomDelay;				/*swtich Vcom voltage to Vcom delay, unit--ms*/
	uint16_t u8VcomScnCnt;					/*frame scan number in VDD*/
	uint16_t u16VcomPeriod;					/*CPV period*/
	uint16_t u16VcomFsStvCovCnt;			/*STV cover CPV number*/	

	uint8_t u8CptImgCnt;					/*image capture counter*/
	//u16 u16ShortIntlFramLineCnt;			/*short integral lines*/
	uint16_t u16IntegralFramLineCnt; 		/*first frame integral frame line counter*/
	
}ST_OXI600_CLR_PARA;

typedef struct
{
	uint8_t u8mode;				/*reg: 0xAB*/
	uint8_t u8colStr; 			/*reg: 0xAC*/
	uint8_t u8border;			/*reg: 0xAD*/		
	uint16_t u16colEnd;			/*reg: 0xAE,0xAF*/
	uint16_t u16rowMax;			/*reg: 0xB8,0xB9*/
	uint16_t u16colStr1;		/*reg: 0xB0,0xB1*/
	uint16_t u16colEnd1;		/*reg: 0xB2,0xB3*/
	uint16_t u16rowStr1;		/*reg: 0xE4,0xE5*/
	uint16_t u16rowEnd1;		/*reg: 0xE6,0xE7*/
	uint16_t u16colStr2;		/*reg: 0xB4,0xB5*/
	uint16_t u16colEnd2;		/*reg: 0xB6,0xB7*/
	uint16_t u16rowStr2;		/*reg: 0xE8,0xE9*/
	uint16_t u16rowEnd2;		/*reg: 0xEA,0xEB*/
	uint16_t u16adcMask;	    /*reg: 0xEC,0xED*/

}ST_OXI600_LAYOUT_REG_PARA;


/*
*  Provided functions of OXI600
*/
EN_OXI600_ERR_TYPE Dev_Oxi600_DrvInit(ST_CHNL600_EXTER_DRV *pstChnl600Drv);
EN_OXI600_ERR_TYPE Dev_Oxi600_ReadRoicId(void);
EN_OXI600_ERR_TYPE Dev_Oxi600_GetImageData(uint8_t *pu8DataBuf,uint32_t *pu32Datalen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_ClrAndGetGateoff(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen,Chl600_bool bIsCptGateOff,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_CaptureShtInte(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t * pu8DataBuf,uint32_t * pu32Datalen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_capShtLastFrame(ST_CHNL600_CPT_PARA stChnl600CptPara);
EN_OXI600_ERR_TYPE Dev_Oxi600_getShtAndCptLong(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32DataLen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_GetLongImageData(uint8_t *pu8DataBuf,uint32_t *pu32DataLen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_TeeCptWholeRowImage(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_TeeCptWholeRowImage_ShtAndLong(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8ShtDataBuf,uint8_t *pu8LongDataBuf,uint32_t *pu32Datalen,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_SleepROIC(ST_CHNL600_CPT_PARA stChnl600CptPara,uint32_t u32Timeout);
EN_OXI600_ERR_TYPE Dev_Oxi600_setPmuPara(uint8_t *pu8DataBuf, uint32_t u32DataLen);
EN_OXI600_ERR_TYPE Dev_Oxi600_getPmuPara(uint8_t *pu8DataBuf, uint32_t *pu32DataLen);
EN_OXI600_ERR_TYPE Dev_Oxi600_setCptPara(uint8_t *pu8DataBuf, uint32_t u32DataLen);
EN_OXI600_ERR_TYPE Dev_Oxi600_SwitchVcomVol(EN_CHNL600_VCOM_MODE enVcomMod,uint8_t u8FramNo);
EN_OXI600_ERR_TYPE Dev_Oxi600_getLibVer(uint8_t* pu8DataBuf);
uint16_t Dev_Oxi600_ReadRegVal(uint8_t u8RegAddr);
EN_OXI600_ERR_TYPE Dev_Oxi600_SetPartPara(int *paraBuf,uint32_t u32bufLen);
#if 0
EN_OXI600_ERR_TYPE Dev_Oxi600_Exit();
uint16_t W25QXX_ReadID(void);
void W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);
#endif



#endif

