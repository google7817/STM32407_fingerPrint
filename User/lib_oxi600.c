#include "lib_oxi600.h"

const uint8_t u8LibVer[4] = {20,1,2,16};	 /*year NOTUSE release_sub_ver debug_ver*/



/* 
 *log enable control, not include err log
 *该log为函数运行中的参数及状态便于调试，不包含代码中的错误log，错误log必打印
 */
//#define DEBUG_DEV_OXI600


/*
*	DBG api define , differnt platform has differnt implemented
*/
#if 0	//(OXI_CONFIG_PLATFORM == PLATFORM_QSEE && OXI_CONFIG_BUILD == BUILD_TEE)
	#if 1
	#include <string.h>
	#include <stdio.h>                        
	#include "qsee_log.h"    
	#define DBG(fmt,...)         qsee_printf("INFO: [%s] " fmt, __FUNCTION__,  ##__VA_ARGS__)   
	#define DBG_OPTM(fmt,...)		qsee_printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

	#elif LIBUSB_OPEN_FD
	#include <android/log.h>

	#define LOG_TAG                        "_OXIFP_LISTDEV_"
	#define DBG(...)            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
	#define INFO(...)            __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
	#endif
#else 	//(stm32 pc)

#ifndef   __NO_DEBUG_MODE__
#include <stdio.h>
#include <string.h>
	
	#define	DBG(format, ...)		printf(format, ##__VA_ARGS__)
	#define DBG_OPTM(fmt,...)		printf("%s(%d)-<%s>: "##fmt,__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else
	
	#define	DBG(format, ...)		
	#define DBG_OPTM(fmt,...)		

#endif

#endif



/*
*   Internal function declaration 
*/
static EN_OXI600_ERR_TYPE _dev_Oxi600_WaitRegStatus(uint8_t u8RegAddr, uint16_t u16RegValMask, uint16_t u16ChkRegVal, uint32_t u32Timeout);
static void _dev_Oxi600_GetImageDataFromFIFO(uint8_t *pu8DataBuf, uint32_t u32ImageSize);
static void _dev_Oxi600_LayoutRegInit(ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo);
static EN_OXI600_ERR_TYPE _dev_Oxi600_ClrScan(uint8_t u8FrameCnt, uint16_t u16StvCovCnt,uint16_t u16CpvPeriod,uint16_t u16ScanRow,uint8_t u8IsCptGateoff,uint32_t u32Timeout);
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanWithoutGateOff(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout);
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanWithGateOff(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout);
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanLastFrame(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout);
static EN_OXI600_ERR_TYPE _dev_Oxi600_OutputGateoffLayOut(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK720_100umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK720_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK810_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK320_100umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MS001_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_MS006_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_SensorDistinguish(ST_CHNL600_CPT_PARA stChnl600CptPara);
static EN_OXI600_ERR_TYPE _dev_Oxi600_RoicRegInit(void);
static EN_OXI600_ERR_TYPE _dev_Oxi600_ClrFrame(uint8_t u8PrjType,Chl600_bool bIsCptGeteOff,uint32_t u32Timeout);
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptFrame(uint8_t u8PrjType,Chl600_bool bIsCptGeteOff,uint16_t u16InteLine ,uint32_t u32Timeout);

static int _dev_Oxi600_WriteReg(uint8_t u8RegAddr,uint8_t u8RegData); /*wirte 1 byte*/
static uint16_t _dev_Oxi600_ReadReg(uint8_t u8RegAddr); /*read 1 byte*/
static uint64_t _dev_Oxi600_GetLocalTime( ); /*计时*/
static void _delay_ms( uint32_t n); 

uint32_t g_u32WinImgDataSize = 0;
uint8_t g_u8SwitchVcomFlag = 0;
uint8_t g_u8SwitchVcomFramCnt = 0;
uint16_t g_u16RowMax = 0;
uint8_t g_u8Vcom2vcom;
uint8_t g_u8Vcom2Vdd;
uint8_t g_u8Vcom2Vee;
uint8_t g_u8VcomStaus;
static ST_CHNL600_EXTER_DRV *_g_stCh600ExterDrv = NULL;

EN_CHL600_CAPACITY g_enShtInteCapacity = EN_CAP_0_2PF;	/*short integration frame capacity*/	
EN_CHL600_CAPACITY g_enLongInteCapacity = EN_CAP_0_6PF; 	/*long integration frame capacity*/ 

#define GATEOFF_ROW_START_SHIFT 	100
#define GATEOFF_ROW_NUMBER			10
#define DOUBLE_FINGER_CUT_SIZE		124



ST_OXI600_CLR_PARA g_stOxi600clrpara =
{
	.bClrLagEn						= Chl600_TRUE, 	/*swtich to on/off clear lag operation*/

	.bVddEn							= Chl600_TRUE,		/*swtich Vcom to Vdd flag*/
	.u32SwVddDelay					= 5,		/*swtich Vcom voltage to Vdd delay, unit--ms*/
	.u8VddScnCnt					= 1,		/*frame scan number in VDD*/
	.u16VddPeriod					= 147,		/*CPV period !!not use now!!*/
	.u16VddFsStvCovCnt				= 20,		/*STV cover CPV number*/
	
	.bVeeEn							= Chl600_FALSE,	/*swtich Vcom to Vee flag*/
	.u32SwVeeDelay					= 30,		/*swtich Vcom voltage to Vee delay, unit--ms*/
	.u8VeeScnCnt					= 5,			/*frame scan number in VEE*/
	.u16VeePeriod					= 30,		/*CPV period !!not use now!!*/
	.u16VeeFsStvCovCnt				= 20,			/*STV cover CPV number*/

	.bVcomEn						= Chl600_TRUE,		/*swtich Vcom to Vcom flag*/
	.u32SwVcomDelay 				= 5,		/*swtich Vcom voltage to Vcom delay, unit--ms*/
	.u8VcomScnCnt					= 1, 		/*frame scan number in VCOM*/
	.u16VcomPeriod					= 147,		/*CPV period !!not use now!!*/
	.u16VcomFsStvCovCnt 			= 20,		/*STV cover CPV number*/	

	.u8CptImgCnt					= 2,		/*image capture counter*/
	//u16ShortIntlFramLineCnt	=			/*short integral lines*/
	.u16IntegralFramLineCnt 		= 600,		/*first frame integral frame line counter*/
	
};

const ST_OXI600_LAYOUT_REG_PARA g_stsensorLayout[] = 
{
	/*mode,colStr,border,colEnd,rowMax,colStr1/shift,colEnd1/shift,rowStr1,rowEnd1,FINGER 2,   adcMask*/
	
	{0x71, 35,    36,    465,   304,   36,   		 464, 		   6,      0,     36,464,6,0,  0xFFFE,}, /*MK720_80um*/
	{0x71, 34,	  35,	 458,	304,   99,	  		 457,		   38,	   0,	  99,457,38,0, 0xFFFF,}, /*MK720_100um*/
	{0x71, 31,	  10,	 568,	260,   38,	  		 538, 		   9,	   0,	  38,538,9,0,  0xFFFF,}, /*MK810_80um*/
	{0x71, 34,	  21,	 550,	252,   45,	   		 517,   	   1,	   0,	  45,517,1,0,  0xFFFF,}, /*MK320_100um*/	
	{0x71, 84,	  10,	 510,	304,   33,	  		 427, 		   6,	   0,	  33,427,0,0,  0xFFFF,}, /*MS001_80um_ES1-3*/
	{0x71, 84,	  10,	 494,	304,   33,	  		 411, 		   6,	   0,	  33,427,0,0,  0xFFFF,}, /*MS006_80um_ES1-3*/
};

/*			double finger layput init using, 								*/
/*			change the AA area from 138*138 to 124*124,image size < 64K		*/
/*			output 5 coloum covered pixel,no dark pixel						*/				 
const ST_OXI600_LAYOUT_REG_PARA g_stsensorDFLayout[] = 
{
	{0x71, 89,	   5,	 510,	304,   28,	  		 422, 		   6,	   0,	  28,422,0,0,  0xFFFF,}, /*MS001_80um_ES1-3*/
	{0x71, 89,	   5,	 494,	304,   28,			 406,		   6,	   0,	  28,406,0,0,  0xFFFF,}, /*MS006_80um_ES1-3*/
};

const ST_OXI600_LAYOUT_REG_PARA g_stsensorLayout_Test[] = 
{
	/*mode,colStr,border,colEnd,rowMax,colStr1/shift,colEnd1/shift,rowStr1,rowEnd1,FINGER 2,   adcMask*/
	
	{0x31, 4,    0,    600,   200,   0,   		 600, 		   0,      200,     36,464,6,0,  0xFFFF,}, /*Test_um*/
};



/*
*	
*	border area start at str, include str 
*   str1 is the relative coordinate of str.
*   end1 is the relative coordinate of str1
*   [str1 end1) include str1 not include end1 when capture
*/
uint8_t g_u8oxiWriteToRegBuf[315];
uint8_t g_u8Oxi600RegInitBuf[315] = 
{						 // offset
	0xAB,  0x71,  0x00,  // 0 ADC_shift	ADC_de_set	/	ADC_mode 70:mod0;31:mod1;72:mod2 
	0xAC,  0x23,  0x00,	 // 1  ADC_col_start
	0xAD , 0x24 , 0x00,  // 2  ADC_col_border 
	0xAE , 0xd1 , 0x00,  // 3  ADC_col_end 
	0xAF , 0x01 , 0x00,  // 4  ADC_col_end //0x258(include darkline) 
	0xB8 , 0x30 , 0x00,  // 5 num_row_max_1 
	0xB9 , 0x01 , 0x00,  // 6 num_row_max_1 
	0xB0 , 0x24 , 0x00,  // 7 ADC_col_start_1 
	0xB1 , 0x00 , 0x00,  // 8 ADC_col_start_1 startX 0x0064 
	0xB2 , 0xd0 , 0x00,  // 9 ADC_col_end_1 
	0xB3 , 0x01 , 0x00 , // 10 ADC_col_end_1	endX 0x00E4 
	0xE4 , 0x64 , 0x00,  // 11 ADC_row_start_1 
	0xE5 , 0x00 , 0x00,  // 12 ADC_row_start_1	startY 0x0064 
	0xE6 , 0xE4 , 0x00,  // 13 ADC_row_end_1 
	0xE7 , 0x00 , 0x00,  // 14 ADC_row_end_1	endY 0x00E4 
	0xB4 , 0x2C , 0x00,  // 15 ADC_col_start_2 
	0xB5 , 0x01 , 0x00,  // 16 ADC_col_start_2	startX2 0x012C 
	0xB6 , 0xAC , 0x00,  // 17 ADC_col_end_2 
	0xB7 , 0x01 , 0x00,  // 18 ADC_col_end_2	endX2	0x01AC 
	0xE8 , 0x2C , 0x00,  // 19 ADC_row_start_2 
	0xE9 , 0x01 , 0x00,  // 20 ADC_row_start_2	startY2	0x012C 
	0xEA , 0xAC , 0x00,  // 21 ADC_row_end_2 
	0xEB , 0x01 , 0x00,  // 22 ADC_row_end_2	endY2	0x1AC 
	0x80 , 0xd8 , 0x00,  //P1 
	0x81 , 0x02 , 0x00,  //P1 
	0x82 , 0x0b , 0x00,  //P2
	0x83 , 0x03 , 0x00,  // 26 P2
	0x84 , 0xc0 , 0x00,  //P3
	0x85 , 0x01 , 0x00,  //P3 
	0x86 , 0x00 , 0x00,  // 29 P3 
	0x87 , 0x28 , 0x00,  //P4 
	0x88 , 0x06 , 0x00,  // 31 P4 
	0x89 , 0x1F , 0x00,  // 32 p_STV_cycle_num 
	0x8A , 0x01 , 0x00,  // 33 p_STV_high_clk 
	0x8B , 0x01 , 0x00,  // 34 p_STV_low_clk 
	0x8C , 0xC8 , 0x00,  // 35 P5 
	0x8D , 0x00 , 0x00,  // 36 P5 
	0x8E , 0x00 , 0x00,  // 37 P5  
	0x8F , 0x54 , 0x00,  // 38 P6 
	0x90 , 0x00 , 0x00,  // 39 P6 
	0x91 , 0x18 , 0x00,  // 40 P7   17
	0x92 , 0x01 , 0x00,  // 41 P7   01
	0x93 , 0xf8 , 0x00,  //P8   2c
	0x94 , 0x01 , 0x00,  //P8		02
	0x95 , 0x46 , 0x00,  // 44 P9		45 
	0x96 , 0x00 , 0x00,  // 45 P9 	00
	0x97 , 0x38 , 0x00,  // 46 P10   88
	0x98 , 0x00 , 0x00,  // 47 P10 		00
	0x99 , 0x38 , 0x00,  // 48 P11    
	0x9A , 0x00 , 0x00,  // 49 P11 
	0x9B , 0x36 , 0x00,  //P12		57
	0x9C , 0x04 , 0x00,  //P12		04
	0x9D , 0xd2 , 0x00,  // 52 P13		d0
	0x9E , 0x00 , 0x00,  // 53 P13		00
	0x9F , 0x38 , 0x00,  // 54 P14 		37
	0xA0 , 0x00 , 0x00,  // 55 P14 
	0xA1 , 0x38 , 0x00,  // 56 P15 		37
	0xA2 , 0x00 , 0x00,  // 57 P15 
	0xA3 , 0x00 , 0x00,  // 58 P16 
	0xA4 , 0x00 , 0x00,  // 59 P16 
	0xA5 , 0x00 , 0x00,  // 60 P17 
	0xA6 , 0x00 , 0x00,  // 61 P17 
	0xA7 , 0x5c , 0x00,  //P18		f7
	0xA8 , 0x05 , 0x00,  //P18		05
	0xA9 , 0x28 , 0x00,  // 64 P19 
	0xAA , 0x01 , 0x00,  // 65 XAO_PIRST_CTRL 
	0xBA , 0xC8 , 0x00,  // 66 num_row_max_2 
	0xBB , 0x00 , 0x00,  // 67 num_row_max_2 
	0xC1 , 0x00 , 0x00,  // 68 /	/	/	/	/	/	pol_led_en	capture_forever 
	0xC2 , 0x02 , 0x00,  // 69 num_dark_led[7:4]	num_dark_led[3:0] 
	0xC3 , 0x00 , 0x00,  // 70 active_frame_h 
	0xC4 , 0x02 , 0x00,  // 71 active_frame_l 
	0xC5 , 0xFF , 0x00,  // 72 active_cap_h 
	0xC6 , 0xFF , 0x00,  // 73 active_cap_l 
	0xC7 , 0x00 , 0x00,  // 74 num_dly 
	0xC8 , 0x04 , 0x00,  // 75 value_std_fifo[15:8] 
	0xC9 , 0x00 , 0x00,  // 76 value_std_fifo[7:0] 
	0xCA , 0x05 , 0x00,  // 77 num_active_fire 
	0xCB , 0x05 , 0x00,  // 78 num_fm_dly 
	0xCC , 0x00 , 0x00,  // 79 hv_thres	half_fifo_shift 
	0xCD , 0x81 , 0x00,  // 80 D2A_REFINT_SEL 	D2A_AMP_SEL 
	0xCE , 0x08 , 0x00,  // 81 喔佮竵/	喔佮竵/	D2A_SLEEP_IRST	D2A_I_BUF 
	0xCF , 0x5F , 0x00,  // 82 D2P_VCOM2VEE  	D2P_VCOM2VDD   	D2P_VCOM2VCOM    	D2P_ENHV 	D2A_PD 	D2A_PDAD 	D2A_PDBGP  	D2A_PDCK 
	0xD0 , 0x41 , 0x00,  // 83 i2c_set	D2A_SWE     	D2A_TST   	D2A_EXREF 	D2A_EXVBG  	D2A_TSTADCB 
	0xD1 , 0x10 , 0x00,  // 84 debug_sel	D2A_TESTA 	D2A_RBG	D2A_TE 
	0xD2 , 0x4C , 0x00,  // 85 D2A_REFSEL	D2A_I_ADC 
	0xD3 , 0x78 , 0x00,  // 86 D2A_I_INT	D2A_I_REF 
	0xD4 , 0x10 , 0x00,  //D2A_CSELX	/	D2A_RSELB 
	0xD5 , 0x00 , 0x00,  // 88 dly_set	p_STV_cycle_flag	p_clk_fast_flag 
	0xD6 , 0x04 , 0x00,  // 89 clk_sw	D2A_TESTBG	/	/	/	D2A_BG_TRIM 
	0xD7 , 0x00 , 0x00,  // 90 FIRE_ENABLE	D2P_PU_EN18	/	/	D2A_OSC_ITUNE	D2A_OSC_TESTEN 
	0xD8 , 0x44 , 0x00,  // 91 \\ OSC FREQUNCE 55MHZ CLK 
	0xD9 , 0x40 , 0x00,  // 92 intn_ext_sw	D2A_IREF_TRIM	D2A_BIAS_TESTEN	D2A_BIASEN 
	0xDA , 0x00 , 0x00,  // 93 /	/	/	D2A_LDO18EN 	D2A_LDO18_ITUNE 
	0xDB , 0x23 , 0x00,  // 94 \\ LDO18 
	0xDC , 0xCB , 0x00,  // 95 num_fire_cmd	num_fire_normal_frame	num_fire_stv_frame 
	0xDD , 0xA6 , 0x00,  // 96 fire_cmd2	fire_cmd1 
	0xDE , 0x30 , 0x00,  // 97 fire_cmd4	fire_cmd3 
	0xDF , 0x10 , 0x00,  // 98 fire_cmd6	fire_cmd5 
	0xEC , 0xFF , 0x00,  //D2A_mask 
	0xED , 0xFF , 0x00,  // 100 D2A_mask 
	0xEE , 0x88 , 0x00,  // 101 VGG setting	VEE setting 
	0xEF , 0x88 , 0x00,  // 102 VCOM setting	I2C OSC setting
	0xFF , 0xFF , 0xFF,	 // 103
};

#if 1
// linetime NO. == 0  ---> lientime :107us  
uint8_t g_u8Oxi600RegInitBuf_linetime107us[315] = 
{						 // offset
	0xAB,  0x71,  0x00,  // 0 ADC_shift	ADC_de_set	/	ADC_mode 70:mod0;31:mod1;72:mod2 
	0xAC,  0x23,  0x00,	 // 1  ADC_col_start
	0xAD , 0x24 , 0x00,  // 2  ADC_col_border 
	0xAE , 0xd1 , 0x00,  // 3  ADC_col_end 
	0xAF , 0x01 , 0x00,  // 4  ADC_col_end //0x258(include darkline) 
	0xB8 , 0x30 , 0x00,  // 5 num_row_max_1 
	0xB9 , 0x01 , 0x00,  // 6 num_row_max_1 
	0xB0 , 0x24 , 0x00,  // 7 ADC_col_start_1 
	0xB1 , 0x00 , 0x00,  // 8 ADC_col_start_1 startX 0x0064 
	0xB2 , 0xd0 , 0x00,  // 9 ADC_col_end_1 
	0xB3 , 0x01 , 0x00 , // 10 ADC_col_end_1	endX 0x00E4 
	0xE4 , 0x64 , 0x00,  // 11 ADC_row_start_1 
	0xE5 , 0x00 , 0x00,  // 12 ADC_row_start_1	startY 0x0064 
	0xE6 , 0xE4 , 0x00,  // 13 ADC_row_end_1 
	0xE7 , 0x00 , 0x00,  // 14 ADC_row_end_1	endY 0x00E4 
	0xB4 , 0x2C , 0x00,  // 15 ADC_col_start_2 
	0xB5 , 0x01 , 0x00,  // 16 ADC_col_start_2	startX2 0x012C 
	0xB6 , 0xAC , 0x00,  // 17 ADC_col_end_2 
	0xB7 , 0x01 , 0x00,  // 18 ADC_col_end_2	endX2	0x01AC 
	0xE8 , 0x2C , 0x00,  // 19 ADC_row_start_2 
	0xE9 , 0x01 , 0x00,  // 20 ADC_row_start_2	startY2	0x012C 
	0xEA , 0xAC , 0x00,  // 21 ADC_row_end_2 
	0xEB , 0x01 , 0x00,  // 22 ADC_row_end_2	endY2	0x1AC 
	0x80 , 0xd8 , 0x00,  //P1 
	0x81 , 0x02 , 0x00,  //P1 
	0x82 , 0x0b , 0x00,  //P2
	0x83 , 0x03 , 0x00,  // 26 P2
	0x84 , 0xc0 , 0x00,  //P3
	0x85 , 0x01 , 0x00,  //P3 
	0x86 , 0x00 , 0x00,  // 29 P3 
	0x87 , 0x28 , 0x00,  //P4 
	0x88 , 0x06 , 0x00,  // 31 P4 
	0x89 , 0x1F , 0x00,  // 32 p_STV_cycle_num 
	0x8A , 0x01 , 0x00,  // 33 p_STV_high_clk 
	0x8B , 0x01 , 0x00,  // 34 p_STV_low_clk 
	0x8C , 0xC8 , 0x00,  // 35 P5 
	0x8D , 0x00 , 0x00,  // 36 P5 
	0x8E , 0x00 , 0x00,  // 37 P5  
	0x8F , 0x54 , 0x00,  // 38 P6 
	0x90 , 0x00 , 0x00,  // 39 P6 
	0x91 , 0x18 , 0x00,  // 40 P7   17
	0x92 , 0x01 , 0x00,  // 41 P7   01
	0x93 , 0xf8 , 0x00,  //P8   2c
	0x94 , 0x01 , 0x00,  //P8		02
	0x95 , 0x46 , 0x00,  // 44 P9		45 
	0x96 , 0x00 , 0x00,  // 45 P9 	00
	0x97 , 0x38 , 0x00,  // 46 P10   88
	0x98 , 0x00 , 0x00,  // 47 P10 		00
	0x99 , 0x38 , 0x00,  // 48 P11    
	0x9A , 0x00 , 0x00,  // 49 P11 
	0x9B , 0x36 , 0x00,  //P12		57
	0x9C , 0x04 , 0x00,  //P12		04
	0x9D , 0xd2 , 0x00,  // 52 P13		d0
	0x9E , 0x00 , 0x00,  // 53 P13		00
	0x9F , 0x38 , 0x00,  // 54 P14 		37
	0xA0 , 0x00 , 0x00,  // 55 P14 
	0xA1 , 0x38 , 0x00,  // 56 P15 		37
	0xA2 , 0x00 , 0x00,  // 57 P15 
	0xA3 , 0x00 , 0x00,  // 58 P16 
	0xA4 , 0x00 , 0x00,  // 59 P16 
	0xA5 , 0x00 , 0x00,  // 60 P17 
	0xA6 , 0x00 , 0x00,  // 61 P17 
	0xA7 , 0x5c , 0x00,  //P18		f7
	0xA8 , 0x05 , 0x00,  //P18		05
	0xA9 , 0x28 , 0x00,  // 64 P19 
	0xAA , 0x01 , 0x00,  // 65 XAO_PIRST_CTRL 
	0xBA , 0xC8 , 0x00,  // 66 num_row_max_2 
	0xBB , 0x00 , 0x00,  // 67 num_row_max_2 
	0xC1 , 0x00 , 0x00,  // 68 /	/	/	/	/	/	pol_led_en	capture_forever 
	0xC2 , 0x02 , 0x00,  // 69 num_dark_led[7:4]	num_dark_led[3:0] 
	0xC3 , 0x00 , 0x00,  // 70 active_frame_h 
	0xC4 , 0x02 , 0x00,  // 71 active_frame_l 
	0xC5 , 0xFF , 0x00,  // 72 active_cap_h 
	0xC6 , 0xFF , 0x00,  // 73 active_cap_l 
	0xC7 , 0x00 , 0x00,  // 74 num_dly 
	0xC8 , 0x04 , 0x00,  // 75 value_std_fifo[15:8] 
	0xC9 , 0x00 , 0x00,  // 76 value_std_fifo[7:0] 
	0xCA , 0x05 , 0x00,  // 77 num_active_fire 
	0xCB , 0x05 , 0x00,  // 78 num_fm_dly 
	0xCC , 0x00 , 0x00,  // 79 hv_thres	half_fifo_shift 
	0xCD , 0x81 , 0x00,  // 80 D2A_REFINT_SEL 	D2A_AMP_SEL 
	0xCE , 0x08 , 0x00,  // 81 喔佮竵/	喔佮竵/	D2A_SLEEP_IRST	D2A_I_BUF 
	0xCF , 0x5F , 0x00,  // 82 D2P_VCOM2VEE  	D2P_VCOM2VDD   	D2P_VCOM2VCOM    	D2P_ENHV 	D2A_PD 	D2A_PDAD 	D2A_PDBGP  	D2A_PDCK 
	0xD0 , 0x41 , 0x00,  // 83 i2c_set	D2A_SWE     	D2A_TST   	D2A_EXREF 	D2A_EXVBG  	D2A_TSTADCB 
	0xD1 , 0x10 , 0x00,  // 84 debug_sel	D2A_TESTA 	D2A_RBG	D2A_TE 
	0xD2 , 0x4C , 0x00,  // 85 D2A_REFSEL	D2A_I_ADC 
	0xD3 , 0x78 , 0x00,  // 86 D2A_I_INT	D2A_I_REF 
	0xD4 , 0x10 , 0x00,  //D2A_CSELX	/	D2A_RSELB 
	0xD5 , 0x00 , 0x00,  // 88 dly_set	p_STV_cycle_flag	p_clk_fast_flag 
	0xD6 , 0x04 , 0x00,  // 89 clk_sw	D2A_TESTBG	/	/	/	D2A_BG_TRIM 
	0xD7 , 0x00 , 0x00,  // 90 FIRE_ENABLE	D2P_PU_EN18	/	/	D2A_OSC_ITUNE	D2A_OSC_TESTEN 
	0xD8 , 0x44 , 0x00,  // 91 \\ OSC FREQUNCE 55MHZ CLK 
	0xD9 , 0x40 , 0x00,  // 92 intn_ext_sw	D2A_IREF_TRIM	D2A_BIAS_TESTEN	D2A_BIASEN 
	0xDA , 0x00 , 0x00,  // 93 /	/	/	D2A_LDO18EN 	D2A_LDO18_ITUNE 
	0xDB , 0x23 , 0x00,  // 94 \\ LDO18 
	0xDC , 0xCB , 0x00,  // 95 num_fire_cmd	num_fire_normal_frame	num_fire_stv_frame 
	0xDD , 0xA6 , 0x00,  // 96 fire_cmd2	fire_cmd1 
	0xDE , 0x30 , 0x00,  // 97 fire_cmd4	fire_cmd3 
	0xDF , 0x10 , 0x00,  // 98 fire_cmd6	fire_cmd5 
	0xEC , 0xFF , 0x00,  //D2A_mask 
	0xED , 0xFF , 0x00,  // 100 D2A_mask 
	0xEE , 0x88 , 0x00,  // 101 VGG setting	VEE setting 
	0xEF , 0x88 , 0x00,  // 102 VCOM setting	I2C OSC setting
	0xFF , 0xFF , 0xFF,	 // 103
};


// linetime NO. == 1  ---> lientime :121us  
uint8_t g_u8Oxi600RegInitBuf_linetime121us[315] = 
{						 // offset
	0xAB,  0x71,  0x00,  // 0 ADC_shift	ADC_de_set	/	ADC_mode 70:mod0;31:mod1;72:mod2 
	0xAC,  0x23,  0x00,	 // 1  ADC_col_start
	0xAD , 0x24 , 0x00,  // 2  ADC_col_border 
	0xAE , 0xd1 , 0x00,  // 3  ADC_col_end 
	0xAF , 0x01 , 0x00,  // 4  ADC_col_end //0x258(include darkline) 
	0xB8 , 0x30 , 0x00,  // 5 num_row_max_1 
	0xB9 , 0x01 , 0x00,  // 6 num_row_max_1 
	0xB0 , 0x24 , 0x00,  // 7 ADC_col_start_1 
	0xB1 , 0x00 , 0x00,  // 8 ADC_col_start_1 startX 0x0064 
	0xB2 , 0xd0 , 0x00,  // 9 ADC_col_end_1 
	0xB3 , 0x01 , 0x00 , // 10 ADC_col_end_1	endX 0x00E4 
	0xE4 , 0x64 , 0x00,  // 11 ADC_row_start_1 
	0xE5 , 0x00 , 0x00,  // 12 ADC_row_start_1	startY 0x0064 
	0xE6 , 0xE4 , 0x00,  // 13 ADC_row_end_1 
	0xE7 , 0x00 , 0x00,  // 14 ADC_row_end_1	endY 0x00E4 
	0xB4 , 0x2C , 0x00,  // 15 ADC_col_start_2 
	0xB5 , 0x01 , 0x00,  // 16 ADC_col_start_2	startX2 0x012C 
	0xB6 , 0xAC , 0x00,  // 17 ADC_col_end_2 
	0xB7 , 0x01 , 0x00,  // 18 ADC_col_end_2	endX2	0x01AC 
	0xE8 , 0x2C , 0x00,  // 19 ADC_row_start_2 
	0xE9 , 0x01 , 0x00,  // 20 ADC_row_start_2	startY2	0x012C 
	0xEA , 0xAC , 0x00,  // 21 ADC_row_end_2 
	0xEB , 0x01 , 0x00,  // 22 ADC_row_end_2	endY2	0x1AC 
	0x80 , 0xba , 0x00,  //P1 
	0x81 , 0x03 , 0x00,  //P1 
	0x82 , 0x85 , 0x00,  //P2
	0x83 , 0x04 , 0x00,  //P2
	0x84 , 0x38 , 0x00,  //P3
	0x85 , 0x03 , 0x00,  //P3 
	0x86 , 0x00 , 0x00,  //P3 
	0x87 , 0xa9 , 0x00,  //P4 
	0x88 , 0x08 , 0x00,  //P4 
	0x89 , 0x1F , 0x00,  //p_STV_cycle_num 
	0x8A , 0x01 , 0x00,  //p_STV_high_clk 
	0x8B , 0x01 , 0x00,  //p_STV_low_clk 
	0x8C , 0xC8 , 0x00,  //P5 
	0x8D , 0x00 , 0x00,  //P5 
	0x8E , 0x00 , 0x00,  //P5 
	0x8F , 0x68 , 0x00,  //P6 
	0x90 , 0x00 , 0x00,  //P6 
	0x91 , 0xae , 0x00,  //P7	17
	0x92 , 0x00 , 0x00,  //P7	01
	0x93 , 0x08 , 0x00,  //P8	2c
	0x94 , 0x02 , 0x00,  //P8		02
	0x95 , 0x57 , 0x00,  //P9		45 
	0x96 , 0x00 , 0x00,  //P9	00
	0x97 , 0xf3 , 0x00,  //P10	 88
	0x98 , 0x00 , 0x00,  //P10		00
	0x99 , 0x45 , 0x00,  //P11	  
	0x9A , 0x00 , 0x00,  //P11 
	0x9B , 0xd1 , 0x00,  //P12		57
	0x9C , 0x05 , 0x00,  //P12		04
	0x9D , 0x57 , 0x00,  //P13		d0
	0x9E , 0x00 , 0x00,  //P13		00
	0x9F , 0xa1 , 0x00,  //P14		37
	0xA0 , 0x01 , 0x00,  //P14 
	0xA1 , 0x45 , 0x00,  //P15		37
	0xA2 , 0x00 , 0x00,  //P15 
	0xA3 , 0x00 , 0x00,  //P16 
	0xA4 , 0x00 , 0x00,  //P16 
	0xA5 , 0x00 , 0x00,  //P17 
	0xA6 , 0x00 , 0x00,  //P17 
	0xA7 , 0x22 , 0x00,  //P18		f7
	0xA8 , 0x08 , 0x00,  //P18		05
	0xA9 , 0x28 , 0x00,  //P19 
	0xAA , 0x01 , 0x00,  // 65 XAO_PIRST_CTRL 
	0xBA , 0xC8 , 0x00,  // 66 num_row_max_2 
	0xBB , 0x00 , 0x00,  // 67 num_row_max_2 
	0xC1 , 0x00 , 0x00,  // 68 /	/	/	/	/	/	pol_led_en	capture_forever 
	0xC2 , 0x02 , 0x00,  // 69 num_dark_led[7:4]	num_dark_led[3:0] 
	0xC3 , 0x00 , 0x00,  // 70 active_frame_h 
	0xC4 , 0x02 , 0x00,  // 71 active_frame_l 
	0xC5 , 0xFF , 0x00,  // 72 active_cap_h 
	0xC6 , 0xFF , 0x00,  // 73 active_cap_l 
	0xC7 , 0x00 , 0x00,  // 74 num_dly 
	0xC8 , 0x04 , 0x00,  // 75 value_std_fifo[15:8] 
	0xC9 , 0x00 , 0x00,  // 76 value_std_fifo[7:0] 
	0xCA , 0x05 , 0x00,  // 77 num_active_fire 
	0xCB , 0x05 , 0x00,  // 78 num_fm_dly 
	0xCC , 0x00 , 0x00,  // 79 hv_thres	half_fifo_shift 
	0xCD , 0x81 , 0x00,  // 80 D2A_REFINT_SEL 	D2A_AMP_SEL 
	0xCE , 0x08 , 0x00,  // 81 喔佮竵/	喔佮竵/	D2A_SLEEP_IRST	D2A_I_BUF 
	0xCF , 0x5F , 0x00,  // 82 D2P_VCOM2VEE  	D2P_VCOM2VDD   	D2P_VCOM2VCOM    	D2P_ENHV 	D2A_PD 	D2A_PDAD 	D2A_PDBGP  	D2A_PDCK 
	0xD0 , 0x41 , 0x00,  // 83 i2c_set	D2A_SWE     	D2A_TST   	D2A_EXREF 	D2A_EXVBG  	D2A_TSTADCB 
	0xD1 , 0x10 , 0x00,  // 84 debug_sel	D2A_TESTA 	D2A_RBG	D2A_TE 
	0xD2 , 0x4C , 0x00,  // 85 D2A_REFSEL	D2A_I_ADC 
	0xD3 , 0x78 , 0x00,  // 86 D2A_I_INT	D2A_I_REF 
	0xD4 , 0x10 , 0x00,  // 87 D2A_CSELX	/	D2A_RSELB 
	0xD5 , 0x00 , 0x00,  // 88 dly_set	p_STV_cycle_flag	p_clk_fast_flag 
	0xD6 , 0x04 , 0x00,  // 89 clk_sw	D2A_TESTBG	/	/	/	D2A_BG_TRIM 
	0xD7 , 0x00 , 0x00,  // 90 FIRE_ENABLE	D2P_PU_EN18	/	/	D2A_OSC_ITUNE	D2A_OSC_TESTEN 
	0xD8 , 0x60 , 0x00,  // 91 \\ OSC FREQUNCE 55MHZ CLK 
	0xD9 , 0x40 , 0x00,  // 92 intn_ext_sw	D2A_IREF_TRIM	D2A_BIAS_TESTEN	D2A_BIASEN 
	0xDA , 0x00 , 0x00,  // 93 /	/	/	D2A_LDO18EN 	D2A_LDO18_ITUNE 
	0xDB , 0x23 , 0x00,  // 94 \\ LDO18 
	0xDC , 0xCB , 0x00,  // 95 num_fire_cmd	num_fire_normal_frame	num_fire_stv_frame 
	0xDD , 0xA6 , 0x00,  // 96 fire_cmd2	fire_cmd1 
	0xDE , 0x30 , 0x00,  // 97 fire_cmd4	fire_cmd3 
	0xDF , 0x10 , 0x00,  // 98 fire_cmd6	fire_cmd5 
	0xEC , 0xFE , 0x00,  // 99 D2A_mask 
	0xED , 0xFF , 0x00,  // 100 D2A_mask 
	0xEE , 0x88 , 0x00,  // 101 VGG setting	VEE setting 
	0xEF , 0x88 , 0x00,  // 102 VCOM setting	I2C OSC setting
	0xFF , 0xFF , 0xFF,	 // 103
};

#endif



static void _dev_Oxi600_logErrType(char *buf,EN_OXI600_ERR_TYPE errType)
{
	DBG("_OXIFP_IC_DRV %s ,sub err type:",buf);
	switch(errType)
	{
		case EN_OXI600_SUCCESS:	
			DBG("operation success\r\n");
			break;
			
		case EN_OXI600_ERR_BASE_VAL:
			DBG("base value error\r\n");
			break;
			
		case EN_OXI600_IMAGE_SIZE_ERR:
			DBG("image size error\r\n");
			break;
			
		case EN_OXI600_REG_INIT_ERR:
			DBG("register init error\r\n");
			break;
			
		case EN_OXI600_CLR_RUN_ERR:
			DBG("clear lag operation failed\r\n");
			break;
			
		case EN_OXI600_CLR_RESTAT_ERR:	
			DBG("restart in clear lag failed\r\n");
			break;
			
		case EN_OXI600_CPT_RUN_ERR:
			DBG("capture image failed\r\n");
			break;
			
		case EN_OXI600_CPT_RESTAT_ERR:
			DBG("restart in capture image failed\r\n");
			break;
		case EN_OXI600_DATA_READY_ERR:
			DBG("wait for image data failed\r\n");
			break;
			
		case EN_OXI600_SLEEP_ROIC_ERR:
			DBG("sleep RIOC failed\r\n");
			break;
			
		case EN_OXI600_PROJECT_TYPE_ERR:
			DBG("para project id err,not exist \r\n");
			break;
	
		default:
			DBG("there is no such err type\r\n");
			break;
	}
}


/**
 * @brief Write values to specified registers
 * @param u8RegAddr which register address will be wirte
 * @param u8RegData register value
 * @retval int
 */
static int _dev_Oxi600_WriteReg(uint8_t u8RegAddr,uint8_t u8RegData)/*wirte 1 byte*/
{
	int retVal;
	uint8_t u8regBuf[2]={0xff,0xff};
	u8regBuf[0] = u8RegAddr;
	u8regBuf[1] = u8RegData;
	retVal = _g_stCh600ExterDrv->SPI_Send(u8regBuf,2);
	return retVal;
}

/**
 * @brief Write data to ROIC
 * @param u8RegAddr which register address will be wirte
 * @param u8RegData register value
 * @retval int
 */
static int _dev_Oxi600_WriteData(uint8_t *u8DataBuf,uint8_t u8DataSize)/*wirte 1 byte*/
{
	int retVal;
	retVal = _g_stCh600ExterDrv->SPI_Send(u8DataBuf,u8DataSize);
	return retVal;
}

/*
 * @brief Read values in specified registers
 * @param u8RegAddr which register address will be read
 * @retval Register value
 */
static uint16_t _dev_Oxi600_ReadReg(uint8_t u8RegAddr) /*read 1 byte*/
{
	uint16_t u16RetVal ;	
	uint8_t u8RegReceBUf[2];
	u8RegAddr &= 0x7F;
	u8RegReceBUf[0] = u8RegAddr;
	u8RegReceBUf[1] = 0xff;
	_g_stCh600ExterDrv->SPI_Send(u8RegReceBUf,2);
	
	u8RegReceBUf[0] = 0xff;				/* Just in case, MOSI need 0xff when spi read reg*/
	_g_stCh600ExterDrv->SPI_Receive(u8RegReceBUf,2);

	u16RetVal = ((uint16_t)u8RegReceBUf[0])<<8 | u8RegReceBUf[1];
	return u16RetVal;
}

/**
 * @brief SPI send mass of data
 * @param pu8DataBuf 
 * @param u32Size
 * @retval error code
 */
static int _dev_Oxi600_SpiSendMass(uint8_t *pu8DataBuf,uint32_t u32Size)
{
	_g_stCh600ExterDrv->SPI_Send_mass(pu8DataBuf,u32Size);
    return 0;
}


/**
 * @brief SPI receive data
 * @param pu8DataBuf 
 * @param u32Size
 * @retval error code
 */
static int _dev_Oxi600_SpiReceive(uint8_t *pu8DataBuf,uint32_t u32Size)
{
	_g_stCh600ExterDrv->SPI_Receive(pu8DataBuf,u32Size);
    return 0;
}



/**
 * @brief SPI send mass of data
 * @param pu8DataBuf 
 * @param u32Size
 * @retval error code
 */
static int _dev_Oxi600_SpiReceiveMass(uint8_t *pu8DataBuf,uint32_t u32Size)
{
	_g_stCh600ExterDrv->SPI_Receive_mass(pu8DataBuf,u32Size);
    return 0;
}

/**
 * @brief get system local time 
 * @retval current time
 */
static uint64_t _dev_Oxi600_GetLocalTime(void) /*计时*/
{
	return _g_stCh600ExterDrv->getLocalTime();
}

/**
 * @brief delay ms
 * @param number of ms
 * @retval none 
 */
static void _delay_ms(uint32_t n)
{	
	_g_stCh600ExterDrv->delay_ms(n);
    return ;
}


/**
 * @brief Judgment of FSM
 * @param u8RegAddr Should be 0x3F 
 * @param u16RegValMask Mask
 * @param u16ChkRegVal Number corresponding to the current state
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_WaitRegStatus(uint8_t u8RegAddr, uint16_t u16RegValMask, uint16_t u16ChkRegVal, uint32_t u32Timeout)
{
	uint16_t u16RcvRegVal,u16AftMask;
	EN_OXI600_ERR_TYPE u8Result;
	uint32_t u32entryTime,u32localTime;
	u8Result = EN_OXI600_SUCCESS;			
	u32entryTime = _dev_Oxi600_GetLocalTime();
	while(1)
	{
		u16RcvRegVal = _dev_Oxi600_ReadReg(u8RegAddr);
		u16AftMask = u16RcvRegVal & u16RegValMask;
		if(u16AftMask == u16ChkRegVal)
		{
			u8Result = EN_OXI600_SUCCESS ;
			break;
		}
		_delay_ms(1);
		u32localTime = _dev_Oxi600_GetLocalTime();
		
		if(u32localTime -u32entryTime > u32Timeout)
		{
			DBG("_OXIFP_IC_DRV wait reg status timeout,reg = %#X,mask = %#X,check val = %#X, rev val = %#X \n",u8RegAddr,u16RegValMask,u16ChkRegVal,u16RcvRegVal);
			u8Result = EN_OXI600_CHECK_STATUS_TIMEOUT ;
			break;
		}
	}
	
	return u8Result;
}

/**
 * @brief Judgment of FSM ,waiting the ROIC running
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_WaitRoicRun(uint32_t u32Timeout)
{
	if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0700,u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;

}

/**
 * @brief Judgment of FSM,and waiting frame scan end 
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_WaitFrameEnd(uint32_t u32Timeout)
{
	if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0B00,u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get image data check FSM2 timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;

}

/**
 * @brief Judgment of FSM,and waiting FIFO read  
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_WaitFifoReady(uint32_t u32Timeout)
{
	if(_dev_Oxi600_WaitRegStatus(0x3f,0x8000,0x8000,u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get image data check FSM1 timeout\n");
		return EN_OXI600_DATA_READY_ERR;
	}
	return EN_OXI600_SUCCESS;

}


/**
 * @brief Receive amount of data from FIFO
 * @param pu8DataBuf pointer to data buffer
 * @param u32ImageSize amount of data to be received from FIFO
 * @retval none
 */
static void _dev_Oxi600_GetImageDataFromFIFO(uint8_t *pu8DataBuf, uint32_t u32ImageSize)
{
	//ROIC_CMD_TRANSFER();
	//ROIC_CMD_DUMMY();
	uint8_t u8TmpRegBuf[4]={0xfc,0xff,0xff,0xff};
	_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
 	_dev_Oxi600_SpiReceiveMass(pu8DataBuf,u32ImageSize);
	return ;
}

/**
 * @brief Set layout register
 * @param stChnl600LayInfo 
 * @retval none
 */
static void _dev_Oxi600_LayoutRegInit(ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo)
{
	uint16_t i;
	for(i=0; i<23; i++)
	{
		if(g_u8oxiWriteToRegBuf[3*i] == 0xAB)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = stChnl600LayInfo.u8mode;
		}

		if(g_u8oxiWriteToRegBuf[3*i] == 0xAC)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = stChnl600LayInfo.u8colStr;
		}

		if(g_u8oxiWriteToRegBuf[3*i] == 0xAD)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = stChnl600LayInfo.u8border;
		}

		if(g_u8oxiWriteToRegBuf[3*i] == 0xAE)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16colEnd & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xAF)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16colEnd >> 8) & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB8)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16rowMax & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB9)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16rowMax >> 8) & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB0)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16colStr1 & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB1)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16colStr1 >>8) & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB2)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16colEnd1 & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB3)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16colEnd1 >> 8) & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE4)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16rowStr1 & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE5)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16rowStr1 >> 8) & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE6)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16rowEnd1 & 0xFF);
		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE7)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16rowEnd1 >> 8) & 0xFF);
		}
	
		/*double finger*/
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB4)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16colStr2 & 0xFF);
	
		}	
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB5)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16colStr2 >> 8) & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB6)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16colEnd2 & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xB7)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16colEnd2 >> 8) & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE8)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16rowStr2 & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xE9)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16rowStr2 >> 8) & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xEA)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)(stChnl600LayInfo.u16rowEnd2 & 0xFF);

		}
		
		if(g_u8oxiWriteToRegBuf[3*i] == 0xEB)
		{
			g_u8oxiWriteToRegBuf[3*i+1] = (uint8_t)((stChnl600LayInfo.u16rowEnd2 >> 8) & 0xFF);

		}

				
	}

	g_u8oxiWriteToRegBuf[99*3+1] = (uint8_t)(stChnl600LayInfo.u16adcMask & 0xFF);
	g_u8oxiWriteToRegBuf[100*3+1] = (uint8_t)(stChnl600LayInfo.u16adcMask >> 8);	
	return ;
	
}



/**
 * @brief Configure clear frame parameters and generate waveforms
 * @param u8FrameCnt Frame number
 * @param u16StvCovCnt The number of STV coverage CPV
 * @param u16CpvPeriod Cycle of CPV
 * @param u16ScanRow Scanning rows
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_ClrScan(uint8_t u8FrameCnt, uint16_t u16StvCovCnt ,uint16_t u16CpvPeriod,uint16_t u16ScanRow,uint8_t u8IsCptGateoff,uint32_t u32Timeout)
{
	uint16_t u16STV_H;
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;

	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();

	u16STV_H =(((uint16_t)g_u8oxiWriteToRegBuf[24*3+1]<<8 | g_u8oxiWriteToRegBuf[23*3+1])+\
				((uint16_t)g_u8oxiWriteToRegBuf[26*3+1]<<8 | g_u8oxiWriteToRegBuf[25*3+1]))*u16StvCovCnt;
	g_u8oxiWriteToRegBuf[30*3+1] = u16STV_H & 0xff;    /*87 stv_l*/
	g_u8oxiWriteToRegBuf[31*3+1] = (u16STV_H>>8) & 0xff; /*87 stv_h*/
	g_u8oxiWriteToRegBuf[65*3+1] = 0x0b;			   /*aa pirst_H*/
	g_u8oxiWriteToRegBuf[69*3+1] = u8FrameCnt;		   /*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[71*3+1] = u8IsCptGateoff;	   /*c4 is write fifo*/	
	g_u8oxiWriteToRegBuf[5*3+1] = u16ScanRow & 0xff;     /*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (u16ScanRow>>8) & 0xff;  /*b9 row max_H*/
	enRetVal = _dev_Oxi600_RoicRegInit();
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV clr scan reg init err,type = %d\n",enRetVal);
		return enRetVal;
	}
#if 0	// log :read  all register setting 
	DBG("IC_DRVIVER :  clr scan reg log\n");
	for(int k = 0; k < 103;k++)
	{
		DBG(" %#x , %#x \n\n",g_u8oxiWriteToRegBuf[3*k],g_u8oxiWriteToRegBuf[3*k+1]);

	}
#endif
	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();

	ROIC_CMD_RUN();
	if(u8IsCptGateoff == 0) 
	{
		if(_dev_Oxi600_WaitFrameEnd(u32Timeout) != EN_OXI600_SUCCESS)
		{
			DBG("_OXIFP_IC_DRV clr scan no gateoff check FSM timeout\n");
			return EN_OXI600_CLR_RUN_ERR; 	
		}
	}
	else if(u8IsCptGateoff == 1)
	{
		if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
		{
			DBG("_OXIFP_IC_DRV clr scan with gateoff check FSM timeout\n");
			return EN_OXI600_CLR_RUN_ERR;	
		}
	}

	return EN_OXI600_SUCCESS;
}


/**
 * @brief Configure capture frame parameters and generate waveforms without gaetoff
 * @param u8FrameCnt Frame number
 * @param u16InteLines The number of integration lines
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanWithoutGateOff(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;


	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();
	g_u8oxiWriteToRegBuf[69*3+1] = u8FrameCnt;		   		/*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[71*3+1] = 1<<(u8FrameCnt-1);	          /*c4 is write fifo*/ 
	g_u8oxiWriteToRegBuf[5*3+1] = g_u16RowMax & 0xff;	 /*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (g_u16RowMax>>8) & 0xff; /*b9 row max_H*/
	g_u8oxiWriteToRegBuf[73*3+1] = 0xff ^ (1<<(u8FrameCnt-2));	/*c6 cap_l*/
	g_u8oxiWriteToRegBuf[66*3+1] = u16InteLines & 0xff;		/*ba row max2*/
	g_u8oxiWriteToRegBuf[67*3+1] = (u16InteLines>>8) & 0xff; /*bb row max2*/	
	g_u8oxiWriteToRegBuf[82*3+1] = g_u8VcomStaus;			/*cf*/
	
	enRetVal= _dev_Oxi600_RoicRegInit();	
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan reg init err,type = %d\n",enRetVal);
	}
	ROIC_CMD_RUN();

	if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;
}


/**
 * @brief Configure capture frame parameters and generate waveforms with gateoff(reset frame + integrate frame)
 * @param u8FrameCnt Frame number
 * @param u16InteLines The number of integration lines
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanWithGateOff(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;

	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();
	g_u8oxiWriteToRegBuf[69*3+1] = u8FrameCnt;		   		/*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[71*3+1] = 1<<(u8FrameCnt-1);	          /*c4 is write fifo*/ 
	g_u8oxiWriteToRegBuf[5*3+1] = g_u16RowMax & 0xff;	 /*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (g_u16RowMax>>8) & 0xff; /*b9 row max_H*/
	g_u8oxiWriteToRegBuf[73*3+1] = 0xff ^ (1<<(u8FrameCnt-1)) ;	/*c6 cap_l*/
	g_u8oxiWriteToRegBuf[66*3+1] = u16InteLines & 0xff;		/*ba row max2*/
	g_u8oxiWriteToRegBuf[67*3+1] = (u16InteLines>>8) & 0xff; /*bb row max2*/	
	g_u8oxiWriteToRegBuf[82*3+1] = g_u8VcomStaus;			/*cf*/
	//ROIC_CMD_WAKEUP();
	enRetVal= _dev_Oxi600_RoicRegInit();	
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan reg init err,type = %d\n",enRetVal);
	}
#if 0
	DBG("IC_DRVIVER :  capt scan reg log\n");
	for(int k = 0; k < 103;k++)
	{
		DBG(" %#x , %#x \n\n",g_u8oxiWriteToRegBuf[3*k],g_u8oxiWriteToRegBuf[3*k+1]);

	}
#endif
	
	//ROIC_CMD_RESTART();
	ROIC_CMD_RUN();
#if 0	
	if(IsWriteFIFO == 0)
	{
		if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0B00,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_CPT_RUN_ERR;
		}
		
		ROIC_CMD_RESTART();
		if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0400,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_CPT_RESTAT_ERR;
		}
	}
	else
	{
		if(_dev_Oxi600_WaitRegStatus(0x3f,0x8000,0x8000,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_DATA_READY_ERR;
		}
		
	//	g_readCurrent = BSP_ReadCurrent(1);
		
		_dev_Oxi600_GetImageDataFromFIFO(dataBuf,g_u32WinImgDataSize);
		if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0B00,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_CPT_RUN_ERR;
		}

		ROIC_CMD_RESTART();
		if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0400,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_CPT_RESTAT_ERR;
		}

		XAO_L();
		ROIC_CMD_SLEEP();
		DISABLE_ROIC_ADC();
		if(_dev_Oxi600_WaitRegStatus(0x3f,0xFF00,0x7000,u32Timeout) != EN_OXI600_SUCCESS)
		{
			return EN_OXI600_SLEEP_ROIC_ERR;
		}
			

	}
#else
		if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
		{
			DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
			return EN_OXI600_CPT_RUN_ERR;
		}

#endif
	return EN_OXI600_SUCCESS;
}



/**
 * @brief Configure capture frame parameters and generate waveforms (last frame)
 * @param u8FrameCnt Frame number
 * @param u16InteLines The number of integration lines
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanLastFrame(uint8_t u8FrameCnt, uint16_t u16InteLines ,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;


	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();
	g_u8oxiWriteToRegBuf[69*3+1] = 1;		   		/*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[71*3+1] = 1;	          /*c4 is write fifo*/ 
	g_u8oxiWriteToRegBuf[5*3+1] = g_u16RowMax & 0xff;	 /*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (g_u16RowMax>>8) & 0xff; /*b9 row max_H*/
	g_u8oxiWriteToRegBuf[73*3+1] = 0xff ;				/*c6 cap_l*/
	g_u8oxiWriteToRegBuf[66*3+1] = g_u16RowMax & 0xff;		/*ba row max2*/
	g_u8oxiWriteToRegBuf[67*3+1] = (g_u16RowMax>>8) & 0xff; /*bb row max2*/	
	g_u8oxiWriteToRegBuf[82*3+1] = g_u8VcomStaus;			/*cf*/
	
	enRetVal= _dev_Oxi600_RoicRegInit();	
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan reg init err,type = %d\n",enRetVal);
	}
	ROIC_CMD_RUN();

	if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;
}


/**
 * @brief Configure capture frame parameters and generate waveforms(reclear frame, and short inte frame,Ability to choose gateoff) 
 * 		  reclear  sht-inte(select gateoff) 	
 *	       __		__  
 * clr + _|  |_____|  |________________
 * @param u8FrameCnt Frame number
 * @param u16InteLines The number of integration lines
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScanFrame_Reclr_Inte(uint8_t u8FrameCnt,uint16_t u16clrScanNo , uint16_t u16shtInteLines ,Chl600_bool isGetGateoff, uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV _dev_Oxi600_CptScanFrame_Reclr_Inte para: frmcnt=%#X,clrscnarow=%#X,inte=%#X,gateoff=%#X \n",u8FrameCnt,u16clrScanNo,u16shtInteLines,isGetGateoff);
	#endif


	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();
	g_u8oxiWriteToRegBuf[69*3+1] = u8FrameCnt;				/*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[5*3+1] = u16clrScanNo & 0xff;	 	/*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (u16clrScanNo>>8) & 0xff;  /*b9 row max_H*/
	g_u8oxiWriteToRegBuf[73*3+1] = 0xff ^ (1<<(u8FrameCnt-1));	/*c6 cap_l*/
	g_u8oxiWriteToRegBuf[66*3+1] = u16shtInteLines & 0xff; 	/*ba row max2*/
	g_u8oxiWriteToRegBuf[67*3+1] = (u16shtInteLines>>8) & 0xff; /*bb row max2*/	
	g_u8oxiWriteToRegBuf[82*3+1] = g_u8VcomStaus;			/*cf*/
	//g_u8oxiWriteToRegBuf[87*3+1] = enCapacity; 			 	/*attention:  IC capactior*/
	//g_u8oxiWriteToRegBuf[65*3+1] = 0x11;			   /*aa pirst_H*/
	if(isGetGateoff == Chl600_TRUE)
	{	
		g_u8oxiWriteToRegBuf[71*3+1] = 1<<(u8FrameCnt-1);		/*c4 is write fifo*/ 
	}
	else
	{
		g_u8oxiWriteToRegBuf[71*3+1] = 0;		/*c4 is write fifo*/ 
	}
	
	enRetVal= _dev_Oxi600_RoicRegInit();	
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan reg init err,type = %d\n",enRetVal);
	}
	ROIC_CMD_RUN();

	if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;
}






/**
 * @brief Configure capture frame parameters and generate waveforms(get short image frame, long inte frame and get long inte frame) 
 * 				  get-sht  long-inte 		 get-long
 *	               __		__				  __
 * clr + sht-inte_|  |_____|  |______________|  |____
 * @param u16InteLines The number of integration lines
 * @param u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptScan_GetImg_Inte_GetImg(uint16_t u16getImgScanNo, EN_CHL600_CAPACITY enCapacity, uint16_t u16shtInteLines , uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV _dev_Oxi600_CptScan_GetImg_Inte_GetImg para: scanrow=%#X,cap=%#X,inte=%#X \n",u16getImgScanNo,enCapacity,u16shtInteLines);
	#endif


	//ROIC_CMD_RESTART();
	//ROIC_CMD_WAKEUP();
	g_u8oxiWriteToRegBuf[69*3+1] = g_stOxi600clrpara.u8CptImgCnt+1;			/*c2 frame cnt*/
	g_u8oxiWriteToRegBuf[71*3+1] = 1+(1<<g_stOxi600clrpara.u8CptImgCnt);	/*c4 is write fifo*/ 
	g_u8oxiWriteToRegBuf[5*3+1] = u16getImgScanNo & 0xff;	/*b8 row max_l*/
	g_u8oxiWriteToRegBuf[6*3+1] = (u16getImgScanNo>>8) & 0xff; /*b9 row max_H*/
	g_u8oxiWriteToRegBuf[73*3+1] = 0xff - (1<<(g_stOxi600clrpara.u8CptImgCnt-1))  ;					/*c6 cap_l*/
	g_u8oxiWriteToRegBuf[66*3+1] = u16shtInteLines & 0xff;		/*ba row max2*/
	g_u8oxiWriteToRegBuf[67*3+1] = (u16shtInteLines>>8) & 0xff; /*bb row max2*/	
	g_u8oxiWriteToRegBuf[82*3+1] = g_u8VcomStaus;			/*cf*/
	g_u8oxiWriteToRegBuf[87*3+1] = enCapacity; 			 	/*attention:  IC capactior*/
	//g_u8oxiWriteToRegBuf[65*3+1] = 0x11;			   /*aa pirst_H*/
	
	enRetVal= _dev_Oxi600_RoicRegInit();	
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan reg init err,type = %d\n",enRetVal);
	}
	ROIC_CMD_RUN();

	if(_dev_Oxi600_WaitRoicRun(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV cpt scan check FSM timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	return EN_OXI600_SUCCESS;
}




/**
 * @brief output gateoff initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_OutputGateoffLayOut(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

	
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	switch(stChnl600CptPara.PrjType)
	{
		case EN_PRJ_TEST_OUTPUT:
			g_u16RowMax = g_stsensorLayout_Test[0].u16rowMax;
			break;
		case EN_PRJ_OXI600_MK720_80UM:
		case EN_PRJ_OXI600_MK720_80UM_1_3:
			g_u16RowMax = g_stsensorLayout[0].u16rowMax;
			break;

		case EN_PRJ_OXI600_MK720_100UM:
		case EN_PRJ_OXI600_MK720_100UM_1_3:
			g_u16RowMax = g_stsensorLayout[1].u16rowMax;
			break;

		case EN_PRJ_OXI600_MK810_80UM:
		case EN_PRJ_OXI600_MK810_80UM_1_3:
			g_u16RowMax = g_stsensorLayout[2].u16rowMax;
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			g_u16RowMax = g_stsensorLayout[3].u16rowMax;
			break;

		case EN_PRJ_OXI600_MS001_80UM_1_3:
			g_u16RowMax = g_stsensorLayout[4].u16rowMax;
			break;

		case EN_PRJ_OXI600_MS006_80UM_1_3:
		case EN_PRJ_OXI600_MS006_80UM_V01:
			g_u16RowMax = g_stsensorLayout[5].u16rowMax;
			break;
			
		default:
			enRetVal = EN_OXI600_PROJECT_TYPE_ERR;
			DBG("_OXIFP_IC_DRV dev gateoff layout para in err, type: %#X\n",stChnl600CptPara.PrjType);
			break;
	}
	stChnl600LayInfo.u8mode = 0x31;			/*reg: 0xAB*/
	stChnl600LayInfo.u8colStr = 0x04;		/*reg: 0xAC*/
	stChnl600LayInfo.u8border = 0;			/*reg: 0xAD*/		
	stChnl600LayInfo.u16colEnd = 0x258; 		/*reg: 0xAE,0xAF*/
//	stChnl600LayInfo.u16rowMax = g_u16RowMax+20;//CHNL600_MK810_80UM_ROW_MAX; 		/*reg: 0xB8,0xB9*/
	stChnl600LayInfo.u16adcMask = 0xFFFF;   /*reg: 0xEC,0xED*/
	
	stChnl600LayInfo.u16colStr1 = 0;
	stChnl600LayInfo.u16colEnd1 = 0x258;

	/*to get gateoff ,incease 18 rows when scanning ,and get the last 10 rows (8 - 18 )*/
	stChnl600LayInfo.u16rowStr1 = g_u16RowMax;								/*reg: 0xE4,0xE5*/
	stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1;

	g_u32WinImgDataSize = 600*10*2;
				
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}

/**
 * @brief Test_um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_Test_umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

#if 0
	stChnl600LayInfo.u8mode = EN_MODE1_CPT_SGL_WIN; 				/*reg: 0xAB*/
	stChnl600LayInfo.u8colStr = CHNL600_MK810_80UM_COL_STR;			/*reg: 0xAC*/
	stChnl600LayInfo.u8border = CHNL600_MK810_80UM_BORDER;			/*reg: 0xAD*/		
	stChnl600LayInfo.u16colEnd = CHNL600_MK810_80UM_COL_END; 		/*reg: 0xAE,0xAF*/
	stChnl600LayInfo.u16rowMax = CHNL600_MK810_80UM_ROW_MAX; 		/*reg: 0xB8,0xB9*/
	stChnl600LayInfo.u16adcMask = CHNL600_MK810_80UM_ADC_MASK;    	/*reg: 0xEC,0xED*/
	g_u16RowMax = CHNL600_MK810_80UM_ROW_MAX;
#endif
	
	memcpy(&stChnl600LayInfo,&g_stsensorLayout_Test[0],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout_Test[0].u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		return EN_OXI600_COOR_ERR;
//		stChnl600CptPara.W1ColNo = 138;
//		stChnl600CptPara.W1RowNo = 138;
//		
//		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 394)\
//			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 292))
//		{
//			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
//			return EN_OXI600_COOR_ERR;
//		}

//		stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[4].u16colStr1; /*reg: 0xB0,0xB1*/
//		stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
//		stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[4].u16rowStr1; /*reg: 0xE4,0xE5*/
//		stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/

//		if(stChnl600CptPara.enCptMode == EN_MODE1_CPT_SGL_WIN)
//		{
//			g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
//										(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
//			#if 0
//			if(g_u32WinImgDataSize > 64*1024)
//			{
//				DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
//				return EN_OXI600_IMAGE_SIZE_ERR;
//			}
//			#endif
//		}
//		else if(stChnl600CptPara.enCptMode == EN_MODE2_CPT_DBL_WIN)
//		{
//			stChnl600CptPara.W2ColNo = 138;
//			stChnl600CptPara.W2RowNo = 138;
//			if((stChnl600CptPara.W2ColStrt + stChnl600CptPara.W2ColNo > 394)\
//				|| (stChnl600CptPara.W2RowStrt + stChnl600CptPara.W2RowNo > 292)\
//				|| (stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1RowNo > stChnl600CptPara.W2ColStrt ))
//			{
//				DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
//				return EN_OXI600_COOR_ERR;
//			}
//			
//			stChnl600LayInfo.u8mode = EN_MODE2_CPT_DBL_WIN;
//			stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorLayout[4].u16colStr1; /*reg: 0xB4,0xB5*/
//			stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB6,0xB7*/
//			stChnl600LayInfo.u16rowStr2 = stChnl600CptPara.W2RowStrt + g_stsensorLayout[4].u16rowStr1; /*reg: 0xE8,0xE9*/
//			stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + stChnl600CptPara.W2RowNo;	 /*reg: 0xEA,0xEB*/

//			if(stChnl600CptPara.W1RowStrt < stChnl600CptPara.W2RowStrt)	/*case: y1 < y2*/
//			{
//				g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt\
//									+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
//									+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
//			}
//			else
//			{
//				g_u32WinImgDataSize =  stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt +\
//										stChnl600CptPara.W1ColNo)+(stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2*2);

//			}
//		}	
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{		
		stChnl600LayInfo.u16colStr1 = g_stsensorLayout_Test[0].u16colStr1;			/*reg: 0xB0,0xB1*/
		stChnl600LayInfo.u16colEnd1 = g_stsensorLayout_Test[0].u16colEnd1;			/*reg: 0xB2,0xB3*/
		stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout_Test[0].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
		stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
		
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1 )*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}


/**
 * @brief MK720_80um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK720_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

		
	memcpy(&stChnl600LayInfo,&g_stsensorLayout[0],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout[0].u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		stChnl600CptPara.W1ColNo = 138;
		stChnl600CptPara.W1RowNo = 138;

		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 415)\
			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 292))
		{
			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
			return EN_OXI600_COOR_ERR;
		}
		
		if(stChnl600CptPara.W1ColStrt <= 22)
		{
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[0].u16colStr1;
		}
		else
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt - 22 + g_stsensorLayout[0].u16colStr1;
		}
		stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;

		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[0].u16rowStr1; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
		}
		else
		{
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[0].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	 /*reg: 0xE6,0xE7*/
		}

		
		g_u32WinImgDataSize = (stChnl600CptPara.W1ColNo+g_stsensorLayout[0].u8border+10)*\
									(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{

		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[0].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[0].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[0].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[0].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[0].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[0].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}
				
		g_u32WinImgDataSize =(stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + 1+ 10)*\
			(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}

/**
 * @brief MK720_100um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK720_100umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

	memcpy(&stChnl600LayInfo,&g_stsensorLayout[1],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = stChnl600LayInfo.u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		stChnl600CptPara.W1ColNo = 128;
		stChnl600CptPara.W1RowNo = 128;
		
		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 324)\
			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 228))
		{
			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
			return EN_OXI600_COOR_ERR;
		}
		
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[1].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[1].u16rowStr1; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
		}
		else
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[1].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[1].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	 /*reg: 0xE6,0xE7*/
		}
		g_u32WinImgDataSize = (g_stsensorLayout[1].u8border+10+stChnl600CptPara.W1ColNo)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#if 0
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
	#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
	#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#endif	
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{
		
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[1].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[1].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[1].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[1].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[1].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[1].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}

													
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + 1+ 10)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
	#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
	#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}

/**
 * @brief MK810_80um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK810_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;
	
	memcpy(&stChnl600LayInfo,&g_stsensorLayout[2],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout[2].u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		stChnl600CptPara.W1ColNo = 138;
		stChnl600CptPara.W1RowNo = 138;
		
		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 500)\
			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 250))
		{
			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
			return EN_OXI600_COOR_ERR;
		}

		
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[2].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[2].u16rowStr1; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
		}
		else 
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[2].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[2].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	 /*reg: 0xE6,0xE7*/
		}
		if(stChnl600CptPara.enCptMode == EN_MODE1_CPT_SGL_WIN)
		{
			g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
										(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
			#if 0
			if(g_u32WinImgDataSize > 64*1024)
			{
				DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
				return EN_OXI600_IMAGE_SIZE_ERR;
			}
			#endif
		}
		else if(stChnl600CptPara.enCptMode == EN_MODE2_CPT_DBL_WIN)
		{
			stChnl600CptPara.W2ColNo = 138;
			stChnl600CptPara.W2RowNo = 138;
			if((stChnl600CptPara.W2ColStrt + stChnl600CptPara.W2ColNo > 500)\
				|| (stChnl600CptPara.W2RowStrt + stChnl600CptPara.W2RowNo > 250)\
				|| (stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1RowNo > stChnl600CptPara.W2ColStrt ))
			{
				DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
				return EN_OXI600_COOR_ERR;
			}
			
			stChnl600LayInfo.u8mode = EN_MODE2_CPT_DBL_WIN;
			
			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{	
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorLayout[2].u16colStr1; /*reg: 0xB4,0xB5*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB6,0xB7*/
				stChnl600LayInfo.u16rowStr2 = stChnl600CptPara.W2RowStrt + g_stsensorLayout[2].u16rowStr1; /*reg: 0xE8,0xE9*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + stChnl600CptPara.W2RowNo;	 /*reg: 0xEA,0xEB*/
			}
			else
			{
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorLayout[2].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr2 = g_stsensorLayout[2].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
			}

			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				if(stChnl600CptPara.W1RowStrt < stChnl600CptPara.W2RowStrt)	/*case: y1 < y2*/
				{
					g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt\
										+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
				}
				else
				{
					g_u32WinImgDataSize =  stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt +\
											stChnl600CptPara.W1ColNo)+stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
				}
			}
			else
			{
				g_u32WinImgDataSize = GATEOFF_ROW_NUMBER* (stChnl600LayInfo.u8border + stChnl600CptPara.W1ColNo + stChnl600CptPara.W2ColNo) * 2;
			}
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{
	
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[2].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[2].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[2].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[2].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[2].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[2].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}

		
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1 )*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}


/**
 * @brief MK320_100um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MK320_100umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

	memcpy(&stChnl600LayInfo,&g_stsensorLayout[3],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout[3].u16rowMax;


	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		stChnl600CptPara.W1ColNo = 128;
		stChnl600CptPara.W1RowNo = 128;

		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 472)\
			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 250))
		{
			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
			return EN_OXI600_COOR_ERR;
		}

		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[3].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[3].u16rowStr1; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
		}
		else
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[3].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[3].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	 /*reg: 0xE6,0xE7*/
		}

		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
							(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#if 0	
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#endif	
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{

		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[3].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[3].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[3].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[3].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[3].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[3].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}


		
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1 )*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#if 0	
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	#endif	
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}


/**
 * @brief MS001_80um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MS001_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;
	
	memcpy(&stChnl600LayInfo,&g_stsensorLayout[4],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout[4].u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		if(stChnl600CptPara.enCptMode == EN_MODE1_CPT_SGL_WIN)
		{
			stChnl600CptPara.W1ColNo = 138;
			stChnl600CptPara.W1RowNo = 138;
			
			if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 394)\
				|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 292))
			{
				DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
				return EN_OXI600_COOR_ERR;
			}
			
			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[4].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[4].u16rowStr1; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
			}
			else
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[4].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[4].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	 /*reg: 0xE6,0xE7*/
			}
			

			g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
										(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
			#if 0
			if(g_u32WinImgDataSize > 64*1024)
			{
				DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
				return EN_OXI600_IMAGE_SIZE_ERR;
			}
			#endif
		}
		else if(stChnl600CptPara.enCptMode == EN_MODE2_CPT_DBL_WIN)
		{
			memcpy(&stChnl600LayInfo,&g_stsensorDFLayout[0],sizeof(ST_OXI600_LAYOUT_REG_PARA));
			stChnl600CptPara.W1ColNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W1RowNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W2ColNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W2RowNo = DOUBLE_FINGER_CUT_SIZE;//138;
			if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 394)|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 292) \
				|| (stChnl600CptPara.W2ColStrt + stChnl600CptPara.W2ColNo > 394)|| (stChnl600CptPara.W2RowStrt + stChnl600CptPara.W2RowNo > 292) \
				|| (stChnl600CptPara.W1ColStrt > stChnl600CptPara.W2ColStrt)|| ((stChnl600CptPara.W1RowStrt > stChnl600CptPara.W2RowStrt)?\
				((stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt < stChnl600CptPara.W1RowNo) && (stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt <stChnl600CptPara.W1ColNo))\
				:((stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt < stChnl600CptPara.W1RowNo) && (stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt <stChnl600CptPara.W1ColNo))))
			{
				DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
				return EN_OXI600_COOR_ERR;
			}
			
			stChnl600LayInfo.u8mode = EN_MODE2_CPT_DBL_WIN;


			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorDFLayout[0].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorDFLayout[0].u16rowStr1; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorDFLayout[0].u16colStr1; /*reg: 0xB4,0xB5*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB6,0xB7*/
				stChnl600LayInfo.u16rowStr2 = stChnl600CptPara.W2RowStrt + g_stsensorDFLayout[0].u16rowStr1; /*reg: 0xE8,0xE9*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + stChnl600CptPara.W2RowNo;	 /*reg: 0xEA,0xEB*/
			}
			else
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorDFLayout[0].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = g_stsensorDFLayout[0].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorDFLayout[0].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr2 = g_stsensorDFLayout[0].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
			}
			

			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				if(stChnl600CptPara.W1RowStrt < stChnl600CptPara.W2RowStrt)	/*case: y1 < y2*/
				{
					if((stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt) < stChnl600CptPara.W2ColNo) /*y1 y2 overlap  重合*/
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt\
										+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
					else
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowNo + stChnl600CptPara.W2RowNo)\
										+ stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
				}
				else
				{
					if((stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt) < stChnl600CptPara.W2ColNo)
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt\
										+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
					else
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowNo + stChnl600CptPara.W2RowNo)\
										+ stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
				}
			}
			else
			{
				if((stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt) < stChnl600CptPara.W1ColNo)	/*case: x 重合*/
				{
					g_u32WinImgDataSize = GATEOFF_ROW_NUMBER* (stChnl600LayInfo.u8border + stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt\
						+ stChnl600CptPara.W1ColNo) * 2;
				}
				else
				{
					g_u32WinImgDataSize = GATEOFF_ROW_NUMBER* (stChnl600LayInfo.u8border + stChnl600CptPara.W1ColNo + stChnl600CptPara.W2ColNo) * 2;
				}
			}
		}
		
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{
		
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[4].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[4].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[4].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[4].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[4].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[4].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1 )*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#if 0		
		if(g_u32WinImgDataSize != pstChnl600CptPara.CptDataSize)
		{
		#ifdef DEBUG_DEV_OXI600
			DBG("capture whole rece size = %d , cac size =%d\n",pstChnl600CptPara.CptDataSize,g_u32WinImgDataSize);
		#endif
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
#endif		
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}




/**
 * @brief MS006_80um initialize configuration
 * @param pstChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_MS006_80umSensorLayoutInit(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	ST_OXI600_LAYOUT_REG_PARA stChnl600LayInfo;

	memcpy(&stChnl600LayInfo,&g_stsensorLayout[5],sizeof(ST_OXI600_LAYOUT_REG_PARA));
	g_u16RowMax = g_stsensorLayout[5].u16rowMax;

	if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WIN_IMG)
	{
		if(stChnl600CptPara.enCptMode == EN_MODE1_CPT_SGL_WIN)
	{
		stChnl600CptPara.W1ColNo = 138;
		stChnl600CptPara.W1RowNo = 138;
		
		if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 378)\
			|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 288))
		{
			DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
			return EN_OXI600_COOR_ERR;
		}

		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[5].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[5].u16rowStr1; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
		}
		else
		{
			stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorLayout[5].u16colStr1; /*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[5].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
		}

			g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
										(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1)*2;
			#if 0
			if(g_u32WinImgDataSize > 64*1024)
			{
				DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
				return EN_OXI600_IMAGE_SIZE_ERR;
			}
			#endif
		}
		else if(stChnl600CptPara.enCptMode == EN_MODE2_CPT_DBL_WIN)
		{
			memcpy(&stChnl600LayInfo,&g_stsensorDFLayout[1],sizeof(ST_OXI600_LAYOUT_REG_PARA));
			stChnl600CptPara.W1ColNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W1RowNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W2ColNo = DOUBLE_FINGER_CUT_SIZE;//138;
			stChnl600CptPara.W2RowNo = DOUBLE_FINGER_CUT_SIZE;//138;
			if((stChnl600CptPara.W1ColStrt + stChnl600CptPara.W1ColNo > 378)|| (stChnl600CptPara.W1RowStrt + stChnl600CptPara.W1RowNo > 288) \
				|| (stChnl600CptPara.W2ColStrt + stChnl600CptPara.W2ColNo > 378)|| (stChnl600CptPara.W2RowStrt + stChnl600CptPara.W2RowNo > 288) \
				|| (stChnl600CptPara.W1ColStrt > stChnl600CptPara.W2ColStrt)|| ((stChnl600CptPara.W1RowStrt > stChnl600CptPara.W2RowStrt)?\
				((stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt < stChnl600CptPara.W1RowNo) && (stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt <stChnl600CptPara.W1ColNo))\
				:((stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt < stChnl600CptPara.W1RowNo) && (stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt <stChnl600CptPara.W1ColNo))))
			{
				DBG("_OXIFP_IC_DRV coordinate out of range,x=%d,y=%d\n",stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt);
				return EN_OXI600_COOR_ERR;
			}
			
			stChnl600LayInfo.u8mode = EN_MODE2_CPT_DBL_WIN;


			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorDFLayout[1].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorDFLayout[1].u16rowStr1; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	 /*reg: 0xE6,0xE7*/
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorDFLayout[1].u16colStr1; /*reg: 0xB4,0xB5*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB6,0xB7*/
				stChnl600LayInfo.u16rowStr2 = stChnl600CptPara.W2RowStrt + g_stsensorDFLayout[1].u16rowStr1; /*reg: 0xE8,0xE9*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + stChnl600CptPara.W2RowNo;	 /*reg: 0xEA,0xEB*/
			}
			else
			{
				stChnl600LayInfo.u16colStr1 = stChnl600CptPara.W1ColStrt + g_stsensorDFLayout[1].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd1 = stChnl600LayInfo.u16colStr1 + stChnl600CptPara.W1ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr1 = g_stsensorDFLayout[1].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
				stChnl600LayInfo.u16colStr2 = stChnl600CptPara.W2ColStrt + g_stsensorDFLayout[1].u16colStr1; /*reg: 0xB0,0xB1*/
				stChnl600LayInfo.u16colEnd2 = stChnl600LayInfo.u16colStr2 + stChnl600CptPara.W2ColNo;	 /*reg: 0xB2,0xB3*/
				stChnl600LayInfo.u16rowStr2 = g_stsensorDFLayout[1].u16rowMax + GATEOFF_ROW_START_SHIFT; /*reg: 0xE4,0xE5*/
				stChnl600LayInfo.u16rowEnd2 = stChnl600LayInfo.u16rowStr2 + GATEOFF_ROW_NUMBER;  /*reg: 0xE6,0xE7*/
			}
			

			if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
			{
				if(stChnl600CptPara.W1RowStrt < stChnl600CptPara.W2RowStrt)	/*case: y1 < y2*/
				{
					if((stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt) < stChnl600CptPara.W2ColNo) /*y1 y2 overlap  重合*/
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W2RowStrt - stChnl600CptPara.W1RowStrt\
										+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
					else
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowNo + stChnl600CptPara.W2RowNo)\
										+ stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
				}
				else
				{
					if((stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt) < stChnl600CptPara.W2ColNo)
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowStrt - stChnl600CptPara.W2RowStrt\
										+ stChnl600CptPara.W2ColNo) + stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
					else
					{
						g_u32WinImgDataSize = stChnl600LayInfo.u8border*2*(stChnl600CptPara.W1RowNo + stChnl600CptPara.W2RowNo)\
										+ stChnl600CptPara.W1ColNo*stChnl600CptPara.W1RowNo*2 \
										+ stChnl600CptPara.W2ColNo*stChnl600CptPara.W2RowNo*2;
					}
				}
			}
			else
			{
				if((stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt) < stChnl600CptPara.W1ColNo)	/*case: x 重合*/
				{
					g_u32WinImgDataSize = GATEOFF_ROW_NUMBER* (stChnl600LayInfo.u8border + stChnl600CptPara.W2ColStrt - stChnl600CptPara.W1ColStrt\
						+ stChnl600CptPara.W1ColNo) * 2;
				}
				else
				{
					g_u32WinImgDataSize = GATEOFF_ROW_NUMBER* (stChnl600LayInfo.u8border + stChnl600CptPara.W1ColNo + stChnl600CptPara.W2ColNo) * 2;
				}
			}
		}
		
	}
	else if(stChnl600CptPara.enCptType == EN_MODE1_CPT_WHLE_ROW_IMG)
	{
	
		if(stChnl600CptPara.isGetGateoff == Chl600_FALSE)
		{
			
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[5].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[5].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = stChnl600CptPara.W1RowStrt + g_stsensorLayout[5].u16rowStr1;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + stChnl600CptPara.W1RowNo;	/*reg: 0xE6,0xE7*/
			
		}
		else
		{		
			stChnl600LayInfo.u16colStr1 = g_stsensorLayout[5].u16colStr1;			/*reg: 0xB0,0xB1*/
			stChnl600LayInfo.u16colEnd1 = g_stsensorLayout[5].u16colEnd1;			/*reg: 0xB2,0xB3*/
			stChnl600LayInfo.u16rowStr1 = g_stsensorLayout[5].u16rowMax + GATEOFF_ROW_START_SHIFT;//u16rowOffset;								/*reg: 0xE4,0xE5*/
			stChnl600LayInfo.u16rowEnd1 = stChnl600LayInfo.u16rowStr1 + GATEOFF_ROW_NUMBER;	/*reg: 0xE6,0xE7*/

		}
		g_u32WinImgDataSize = (stChnl600LayInfo.u16colEnd1 - stChnl600LayInfo.u16colStr1 + stChnl600LayInfo.u8border)*\
								(stChnl600LayInfo.u16rowEnd1 - stChnl600LayInfo.u16rowStr1 )*2;
		if(g_u32WinImgDataSize > 64*1024)
		{
			DBG("_OXIFP_IC_DRV cac size >64K = %d\n",g_u32WinImgDataSize);
			return EN_OXI600_IMAGE_SIZE_ERR;
		}
	
	}
	
	_dev_Oxi600_LayoutRegInit(stChnl600LayInfo);
	
	return EN_OXI600_SUCCESS;

}



/**
 * @brief Distinguish module type and initialize configuration
 * @param stChnl600CptPara 
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_SensorDistinguish(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	switch(stChnl600CptPara.PrjType)
	{
		case EN_PRJ_TEST_OUTPUT:
			enRetVal = _dev_Oxi600_Test_umSensorLayoutInit(stChnl600CptPara);
			break;

		case EN_PRJ_OXI600_MK720_80UM:
		case EN_PRJ_OXI600_MK720_80UM_1_3:
			enRetVal = _dev_Oxi600_MK720_80umSensorLayoutInit(stChnl600CptPara);
			break;

		case EN_PRJ_OXI600_MK720_100UM:
		case EN_PRJ_OXI600_MK720_100UM_1_3:
			enRetVal = _dev_Oxi600_MK720_100umSensorLayoutInit(stChnl600CptPara);
			break;

		case EN_PRJ_OXI600_MK810_80UM:
		case EN_PRJ_OXI600_MK810_80UM_1_3:
			enRetVal = _dev_Oxi600_MK810_80umSensorLayoutInit(stChnl600CptPara);
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			enRetVal =_dev_Oxi600_MK320_100umSensorLayoutInit(stChnl600CptPara);
			break;

		case EN_PRJ_OXI600_MS001_80UM_1_3:
			enRetVal =_dev_Oxi600_MS001_80umSensorLayoutInit(stChnl600CptPara);
			break;
			
		case EN_PRJ_OXI600_MS006_80UM_1_3:
		case EN_PRJ_OXI600_MS006_80UM_V01:
			enRetVal = _dev_Oxi600_MS006_80umSensorLayoutInit(stChnl600CptPara);
			break;
			
		default:
			DBG("_OXIFP_IC_DRV distinguish sensor err, type = %#X\n",stChnl600CptPara.PrjType);
			enRetVal = EN_OXI600_PROJECT_TYPE_ERR;
			break;
	}
	if(stChnl600CptPara.shtOrLong == 0)
	{
		g_u8oxiWriteToRegBuf[87*3+1] = g_enShtInteCapacity;		/*attention:  IC capactior*/
	}
	else if(stChnl600CptPara.shtOrLong == 1)
	{
		g_u8oxiWriteToRegBuf[87*3+1] = g_enLongInteCapacity;		/*attention:  IC capactior*/
	}
	return enRetVal;
}



/**
 * @brief All register writes corresponding data
 * @param none
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_RoicRegInit(void)
{
	uint32_t i;
	uint8_t u8ConvBuf[105*2+4];		// MOVE THE RESTART AND WAKEUP TO THIS BUF 
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV capture capacity = %#X \n",g_u8oxiWriteToRegBuf[87*3+1]);
	#endif
	//ROIC_CMD_RD_ST();
	//ROIC_CMD_DUMMY();
	u8ConvBuf[0] = 0xf8;
	u8ConvBuf[1] = 0xff;
	u8ConvBuf[2] = 0xf1;
	u8ConvBuf[3] = 0xff;
	for(i=0; i<sizeof(g_u8oxiWriteToRegBuf); i+=3)
	{
		if((g_u8oxiWriteToRegBuf[i] == 0xff)&&(g_u8oxiWriteToRegBuf[i+1] == 0xff))
		{
			break;
		}
		u8ConvBuf[i/3*2+4] = g_u8oxiWriteToRegBuf[i];		// +4 because restat and wakeup cmd
		u8ConvBuf[i/3*2+1+4] = g_u8oxiWriteToRegBuf[i+1];
	}	
	
	_dev_Oxi600_SpiSendMass(u8ConvBuf,(i/3)*2+4);
	
	//if(_dev_Oxi600_WaitRegStatus(0x3e, 0xFF00,  0x5E00, 100) != EN_OXI600_SUCCESS)
	//{
	//	DBG("_OXIFP_IC_DRV ROIC reg init check FSM timeout\n");
	//	return EN_OXI600_REG_INIT_ERR;		// error code
	//}
	
	return EN_OXI600_SUCCESS;
}


/**
 * @brief Clear frame in VDD VEE or VCOM
 * @param u8PrjType  project type
 * @param bIsCptGeteOff  is get gateoff data
 * @param 32Timeout  
 * @retval EN_OXI600_ERR_TYPE
 */
static EN_OXI600_ERR_TYPE _dev_Oxi600_ClrFrame(uint8_t u8PrjType,Chl600_bool bIsCptGeteOff,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetval = EN_OXI600_SUCCESS;
	uint8_t u8TmpRegBuf[6] = {0xf1,0xff,0xcf,0,0xaa,0x0b};	//write WAKEUP + REG_AA + REG_CF cf need modify
	
	//ROIC_CMD_WAKEUP();			/// this  is  nesscearyy   at  the begining


	if(u8PrjType == EN_PRJ_TEST_OUTPUT)
	{
		g_u8Vcom2vcom = 0xC0;
		g_u8Vcom2Vdd= 0xA0;
		g_u8Vcom2Vee= 0x60; 	
	}
	else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM)
	{
		g_u8Vcom2vcom = 0xD0;
		g_u8Vcom2Vdd= 0xB0;
		g_u8Vcom2Vee= 0x70;	
	}
	else if(u8PrjType == EN_PRJ_OXI600_MK720_80UM || u8PrjType == EN_PRJ_OXI600_MK720_100UM ||u8PrjType == EN_PRJ_OXI600_MK320_100UM  )
	{
		g_u8Vcom2vcom = 0x50;
		g_u8Vcom2Vdd = 0x30;
		g_u8Vcom2Vee = 0xD0;
	}
	else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MK720_80UM_1_3|| u8PrjType == EN_PRJ_OXI600_MK720_100UM_1_3 ||\
			u8PrjType == EN_PRJ_OXI600_MS001_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MS006_80UM_1_3)
	{
		g_u8Vcom2vcom = 0xC0;
		g_u8Vcom2Vdd= 0xA0;
		g_u8Vcom2Vee= 0x60; 
	}
	else if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
	{		
		g_u8Vcom2vcom = 0xC0;
		g_u8Vcom2Vdd= 0xA0;
		if(g_u8SwitchVcomFlag == EN_VCOM_TO_VEE)
		{
			DBG("_OXIFP_IC_DRV not suppot vocm2vee, sensor : %#X\n",u8PrjType);
			return EN_OXI600_VCOM_STATUS_ERR ;
		}
	}
	else
	{
		DBG("_OXIFP_IC_DRV clr frame para prj type err, type = %#X \n",u8PrjType);
		enRetval = EN_OXI600_PROJECT_TYPE_ERR;
		return enRetval;
	}

	if(g_u8SwitchVcomFlag == EN_VCOM_TO_VDD || g_u8SwitchVcomFlag == EN_VCOM_TO_VEE || g_u8SwitchVcomFlag == EN_VCOM_TO_VCOM)
	{
		switch(g_u8SwitchVcomFlag)
		{
			
			case EN_VCOM_TO_VDD:
				//XAO_H_PIRST_H();
				//VCOM_TO_VDD(g_u8Vcom2Vdd);
				if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
				{
					u8TmpRegBuf[3] = g_u8Vcom2Vdd;
				_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
					_delay_ms(7);
				}
				else
				{
					u8TmpRegBuf[3] = g_u8Vcom2Vdd;
					_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
				}
				g_u8VcomStaus = g_u8Vcom2Vdd;
				_delay_ms(g_stOxi600clrpara.u32SwVddDelay);
				break;

			case EN_VCOM_TO_VEE:
				//XAO_H_PIRST_H();
				//VCOM_TO_VEE(g_u8Vcom2Vee);
				u8TmpRegBuf[3] = g_u8Vcom2Vee;
				_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
				g_u8VcomStaus = g_u8Vcom2Vee;
				_delay_ms(g_stOxi600clrpara.u32SwVeeDelay);
				break;

			case EN_VCOM_TO_VCOM:
				//XAO_H_PIRST_H();
				//VCOM_TO_VCOM(g_u8Vcom2vcom);
				u8TmpRegBuf[3] = g_u8Vcom2vcom;
				_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
				g_u8VcomStaus = g_u8Vcom2vcom;
				_delay_ms(g_stOxi600clrpara.u32SwVcomDelay);
				break;
			default:
				g_u8VcomStaus = g_u8Vcom2vcom;
				break;
		}
		return enRetval;
	}


	if(g_stOxi600clrpara.bClrLagEn == Chl600_TRUE)
	{
		if(g_stOxi600clrpara.bVddEn == Chl600_TRUE )
		{
			//XAO_H_PIRST_H();
			//VCOM_TO_VDD(g_u8Vcom2Vdd);
			if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
			{
				u8TmpRegBuf[3] = g_u8Vcom2Vdd;
				_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
				_delay_ms(7);
			}
			else
			{
				u8TmpRegBuf[3] = g_u8Vcom2Vdd;
				_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
			}
			g_u8VcomStaus = g_u8Vcom2Vdd;
			_delay_ms(g_stOxi600clrpara.u32SwVddDelay);			
			g_u8oxiWriteToRegBuf[82*3+1] = g_u8Vcom2Vdd;


			
			enRetval = _dev_Oxi600_ClrScan(g_stOxi600clrpara.u8VddScnCnt,g_stOxi600clrpara.u16VddFsStvCovCnt,\
									g_stOxi600clrpara.u16VddPeriod,g_u16RowMax,0,u32Timeout);
			if(enRetval != EN_OXI600_SUCCESS)
			{
				DBG("_OXIFP_IC_DRV vdd clr scan err, err: %d\n",enRetval);
				return enRetval;
			}
		}
		
		if(g_stOxi600clrpara.bVeeEn == Chl600_TRUE )
		{
			if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
			{
				DBG("_OXIFP_IC_DRV not suppot vocm2vee, sensor : %#X\n",u8PrjType);
				return EN_OXI600_VCOM_STATUS_ERR ;
			}
			//XAO_H_PIRST_H();
			//VCOM_TO_VEE(g_u8Vcom2Vee);
			u8TmpRegBuf[3] = g_u8Vcom2Vee;
			_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
			g_u8VcomStaus = g_u8Vcom2Vee;
			_delay_ms(g_stOxi600clrpara.u32SwVeeDelay);
			g_u8oxiWriteToRegBuf[82*3+1] = g_u8Vcom2Vee;
			enRetval = _dev_Oxi600_ClrScan(g_stOxi600clrpara.u8VeeScnCnt,g_stOxi600clrpara.u16VeeFsStvCovCnt,\
									g_stOxi600clrpara.u16VeePeriod,g_u16RowMax,0,u32Timeout);
			if(enRetval != EN_OXI600_SUCCESS)
			{
				DBG("_OXIFP_IC_DRV vee clr scan err, err: %d\n",enRetval);
				return enRetval;
			}
		}
		
		if(g_stOxi600clrpara.bVcomEn == Chl600_TRUE )
		{
			//XAO_H_PIRST_H();
			//VCOM_TO_VCOM(g_u8Vcom2vcom);
			u8TmpRegBuf[3] = g_u8Vcom2vcom;
			_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
			g_u8VcomStaus = g_u8Vcom2vcom;
			_delay_ms(g_stOxi600clrpara.u32SwVcomDelay);
			g_u8oxiWriteToRegBuf[82*3+1] = g_u8Vcom2vcom;
			enRetval = _dev_Oxi600_ClrScan(g_stOxi600clrpara.u8VcomScnCnt,g_stOxi600clrpara.u16VcomFsStvCovCnt,\
									g_stOxi600clrpara.u16VcomPeriod,g_u16RowMax,bIsCptGeteOff,u32Timeout);	
			if(enRetval != EN_OXI600_SUCCESS)
			{
				DBG("_OXIFP_IC_DRV vcom clr scan err, err: %d\n",enRetval);
				return enRetval;
			}
		}
		else
		{
			//XAO_H_PIRST_H();
			//VCOM_TO_VCOM(g_u8Vcom2vcom);
			u8TmpRegBuf[3] = g_u8Vcom2vcom;
			_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
			g_u8VcomStaus = g_u8Vcom2vcom;
			_delay_ms(g_stOxi600clrpara.u32SwVcomDelay);
		}

	}
	else
	{
		//XAO_H_PIRST_H();
		//VCOM_TO_VCOM(g_u8Vcom2vcom);
		u8TmpRegBuf[3] = g_u8Vcom2vcom;
		_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
		g_u8VcomStaus = g_u8Vcom2vcom;
		_delay_ms(g_stOxi600clrpara.u32SwVcomDelay);
	}

	
	
	return enRetval;
}

/**
 * @brief star capture frame 
 * @param u8PrjType  project type
 * @param bIsCptGeteOff  is get gateoff data
 * @param u16InteLine  integratal linesu32Timeout
 * @param 32Timeout  
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptFrame(uint8_t u8PrjType,Chl600_bool bIsCptGeteOff,uint16_t u16InteLine,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t u8cptFrameNo;
	if(g_u8SwitchVcomFlag == EN_VCOM_TO_VDD || g_u8SwitchVcomFlag == EN_VCOM_TO_VEE || g_u8SwitchVcomFlag == EN_VCOM_TO_VCOM)
	{
		u8cptFrameNo = g_u8SwitchVcomFramCnt;
	}
	else
	{
		u8cptFrameNo = g_stOxi600clrpara.u8CptImgCnt;
		if(u8PrjType == EN_PRJ_TEST_OUTPUT)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM)
		{
			g_u8Vcom2vcom = 0xD0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK720_80UM || u8PrjType == EN_PRJ_OXI600_MK720_100UM ||u8PrjType == EN_PRJ_OXI600_MK320_100UM	)
		{
			g_u8Vcom2vcom = 0x50;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MK720_80UM_1_3|| u8PrjType == EN_PRJ_OXI600_MK720_100UM_1_3 ||\
				u8PrjType == EN_PRJ_OXI600_MS001_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MS006_80UM_1_3)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else
		{
			DBG("_OXIFP_IC_DRV cpt frame para prj type err, type = %#X \n",u8PrjType);
			enRetVal = EN_OXI600_PROJECT_TYPE_ERR;
			return enRetVal;
		}
		
		g_u8VcomStaus = g_u8Vcom2vcom;
	}

	if(bIsCptGeteOff == Chl600_TRUE)
	{
		u8cptFrameNo -= 1; 							/*don't want genrate last frame ,only reset+inte*/
		
		if((enRetVal = _dev_Oxi600_CptScanWithGateOff(u8cptFrameNo,u16InteLine,u32Timeout)) != EN_OXI600_SUCCESS)
		{
			return enRetVal;
		}	
		
	}
	else
	{
		if((enRetVal = _dev_Oxi600_CptScanWithoutGateOff(u8cptFrameNo,u16InteLine,u32Timeout)) != EN_OXI600_SUCCESS)
		{
			return enRetVal;
		}
	}	


	
	return enRetVal;
}

/**
 * @brief star capture frame 
 * @param u8PrjType  project type
 * @param bIsCptGeteOff  is get gateoff data
 * @param u16InteLine  integratal linesu32Timeout
 * @param 32Timeout  
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_OXI600_ERR_TYPE _dev_Oxi600_CptShtFrame(uint8_t u8PrjType,Chl600_bool bIsCptGeteOff,uint16_t u16InteLine,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t u8cptFrameNo;
	if(g_u8SwitchVcomFlag == EN_VCOM_TO_VDD || g_u8SwitchVcomFlag == EN_VCOM_TO_VEE || g_u8SwitchVcomFlag == EN_VCOM_TO_VCOM)
	{
		u8cptFrameNo = g_u8SwitchVcomFramCnt;
	}
	else
	{
		u8cptFrameNo = g_stOxi600clrpara.u8CptImgCnt;
		if(u8PrjType == EN_PRJ_TEST_OUTPUT)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM)
		{
			g_u8Vcom2vcom = 0xD0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK720_80UM || u8PrjType == EN_PRJ_OXI600_MK720_100UM ||u8PrjType == EN_PRJ_OXI600_MK320_100UM	)
		{
			g_u8Vcom2vcom = 0x50;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MK810_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MK720_80UM_1_3|| u8PrjType == EN_PRJ_OXI600_MK720_100UM_1_3 ||\
				u8PrjType == EN_PRJ_OXI600_MS001_80UM_1_3 || u8PrjType == EN_PRJ_OXI600_MS006_80UM_1_3)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else if(u8PrjType == EN_PRJ_OXI600_MS006_80UM_V01)
		{
			g_u8Vcom2vcom = 0xC0;
		}
		else
		{
			DBG("_OXIFP_IC_DRV cpt frame para prj type err, type = %#X \n",u8PrjType);
			enRetVal = EN_OXI600_PROJECT_TYPE_ERR;
			return enRetVal;
		}
		
		g_u8VcomStaus = g_u8Vcom2vcom;
	}

	u8cptFrameNo -= 1; 			/*don't want genrate last frame ,only reset+inte*/
	
	if((enRetVal = _dev_Oxi600_CptScanFrame_Reclr_Inte(u8cptFrameNo,g_u16RowMax,u16InteLine,bIsCptGeteOff,u32Timeout)) != EN_OXI600_SUCCESS)
	{
		return enRetVal;
	}	
	
	return enRetVal;
}

/**
 * @brief delay Ms convert to rows 
 * @param u16delayMs  need delay time
 * @retval EN_OXI600_ERR_TYPE
**/
static uint16_t _dev_Oxi600_caclIncreaseRows(uint16_t u16delayMs)
{
	uint16_t u16CPV_L,u16CPV_H,u16linetime,u16covtRow;
	float fClkFreq,fLinetime;
	if(g_u8oxiWriteToRegBuf[91*3+1] == 0x44)
		fClkFreq = 14;
	else if(g_u8oxiWriteToRegBuf[91*3+1] == 0x60)
		fClkFreq = 17.5;
	fLinetime =(float)(((uint16_t)g_u8oxiWriteToRegBuf[24*3+1]<<8 | g_u8oxiWriteToRegBuf[23*3+1])+\
				((uint16_t)g_u8oxiWriteToRegBuf[26*3+1]<<8 | g_u8oxiWriteToRegBuf[25*3+1])) / fClkFreq ;
	u16covtRow = u16delayMs * 1000 / (uint16_t)fLinetime;
	DBG("covert row = %d\n",u16covtRow);
	return u16covtRow;
	
}

/**
 * @brief delay Ms convert to rows 
 * @param u16delayMs  need delay time
 * @retval EN_OXI600_ERR_TYPE
**/
static EN_CHL600_CAPACITY _dev_Oxi600_convertCapacity(uint8_t u8number)
{
	EN_CHL600_CAPACITY enChlCap = 0xFF;
	switch(u8number)
	{
		case 1:
			enChlCap = EN_CAP_0_1PF;
		break;
		case 2:
			enChlCap = EN_CAP_0_2PF;
		break;
		case 3:
			enChlCap = EN_CAP_0_3PF;
		break;
		case 4:
			enChlCap = EN_CAP_0_4PF; 		
		break;
		case 5:
			enChlCap = EN_CAP_0_5PF; 		
		break;
		case 6:
			enChlCap = EN_CAP_0_6PF; 		
		break;
		case 7:
			enChlCap = EN_CAP_0_7PF; 		
		break;
		case 8:
			enChlCap = EN_CAP_0_8PF; 		
		break;
		case 9:
			enChlCap = EN_CAP_0_9PF; 		
		break;
		case 10:
			enChlCap = EN_CAP_1_0PF; 		
		break;
		case 11:
			enChlCap = EN_CAP_1_1PF; 		
		break;
		case 12:
			enChlCap = EN_CAP_1_2PF;
		break;
		case 13:
			enChlCap = EN_CAP_1_3PF; 		
		break;
		case 14:
			enChlCap = EN_CAP_1_4PF; 		
		break;
		case 15:
			enChlCap = EN_CAP_1_5PF;
		break;
		
		default :
			return enChlCap;
		break;
	}
	return enChlCap;

}


/**
 * @brief  ROIC peripheral driver init
 * @param  pstChnl600Drv 
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_DrvInit(ST_CHNL600_EXTER_DRV *pstChnl600Drv)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV drv inint\n");
	#endif
	_g_stCh600ExterDrv = pstChnl600Drv;
	return EN_OXI600_SUCCESS;
}



/**
 * @brief  read ROIC id,check the ROIC communication
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_ReadRoicId(void)
{
	if(_dev_Oxi600_WaitRegStatus(0x3e, 0xFF00,  0x5E00, 100) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV read ROIC id timeout \n");
		return EN_OXI600_READ_ID_ERR;		// error code
	}
    return EN_OXI600_SUCCESS;
}


/**
 * @brief  get image data from FIFO
 * @param  pu8DataBuf the pointer to data buffer
 * @param  pu32DataLen the pointer to data length, Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_GetImageData(uint8_t *pu8DataBuf,uint32_t *pu32DataLen,uint32_t u32Timeout)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV get image data,data size : %d  \n",g_u32WinImgDataSize);
	#endif

	
	if(_dev_Oxi600_WaitFifoReady(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get image data check FSM1 timeout\n");
		return EN_OXI600_DATA_READY_ERR;
	}
	
	_dev_Oxi600_GetImageDataFromFIFO(pu8DataBuf,g_u32WinImgDataSize);
	*pu32DataLen = g_u32WinImgDataSize;

	
	return EN_OXI600_SUCCESS;

}


/**
 * @brief  clear frame and output gateoff signal(output 600 chanel signal)
 * @param  stChnl600CptPara 
 * @param  pu8DataBuf  the pointer to data buffer
 * @param  pu32DataLen the pointer to data length, Reassign datalen indicating effective data length
 * @param  bIsCptGateOff is capture gateoff pixel
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE value

 EXAMPLE: 	
 	stChnl600CptPara.PrjType = XX;
 	stChnl600CptPara.isGetGateoff = TRUE / FALSE;
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_ClrAndGetGateoff(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32DataLen,Chl600_bool bIsCptGateOff,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV clr clear frame and getgateoff, is get gateoff = %d,project type = %#X\n",bIsCptGateOff,stChnl600CptPara.PrjType);
	#endif

	/*!!!notice!!! because of reg 0xcf default value is 0, the HV will be enable after power on, 
	 *MK810_1_3 capture image data is 40, So HV off before capture .
	 */
	if( (stChnl600CptPara.PrjType ==EN_PRJ_OXI600_MK810_80UM_1_3 || \
		stChnl600CptPara.PrjType ==EN_PRJ_OXI600_MK720_100UM_1_3 || stChnl600CptPara.PrjType ==EN_PRJ_OXI600_MK720_80UM_1_3 || \
		stChnl600CptPara.PrjType ==EN_PRJ_OXI600_MS001_80UM_1_3 || stChnl600CptPara.PrjType ==EN_PRJ_OXI600_MS006_80UM_1_3 || \
		stChnl600CptPara.PrjType == EN_PRJ_TEST_OUTPUT))
	{
		DISABLE_ROIC_ADC(0x1f); /*ENHV OFF , IF NOT THE SIGNAL MIGHT BE 40*/
		_delay_ms(5);
	}
	
	memcpy(g_u8oxiWriteToRegBuf,g_u8Oxi600RegInitBuf,sizeof(g_u8Oxi600RegInitBuf));

	enRetVal = _dev_Oxi600_OutputGateoffLayOut(stChnl600CptPara);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		_dev_Oxi600_logErrType("clr farme layout",enRetVal);
		return enRetVal;
	}	

		bIsCptGateOff = Chl600_FALSE;	/*NOTICE: capture gateoff when integrate,so write to FALSE*/

	enRetVal = _dev_Oxi600_ClrFrame(stChnl600CptPara.PrjType,bIsCptGateOff,u32Timeout);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		_dev_Oxi600_logErrType("clr farme start",enRetVal);
		return enRetVal;
	}
	if(bIsCptGateOff == Chl600_TRUE)
	{
		enRetVal = Dev_Oxi600_GetImageData(pu8DataBuf,pu32DataLen,u32Timeout);
		if(enRetVal != EN_OXI600_SUCCESS)
		{
			_dev_Oxi600_logErrType("clr farme get image data",enRetVal);
			return enRetVal;
		}
	}
	if(g_stOxi600clrpara.u8CptImgCnt > 2 && g_stOxi600clrpara.bClrLagEn == Chl600_TRUE)
	{	
		//XAO_L_PIRST_H();
	}
	return enRetVal;
	
}

 
 /**
  * @brief	start capture frame scan (Start running and reply success immediately)
  * @param	stChnl600CptPara  
  * @param	pu8DataBuf	the pointer to data buffer
  * @param	pu32Datalen the pointer to data length, Reassign datalen indicating effective data length
  * @param	u32Timeout 

  * @retval EN_OXI600_ERR_TYPE

  EXAMPLE:
  	 stChnl600CptPara.enCptMode = EN_CHNL600_CPT_MODE;
	 stChnl600CptPara.enCptType = EN_MODE1_CPT_WIN_IMG;
	 stChnl600CptPara.integraLine = x;
	 stChnl600CptPara.shtOrLong = X;
	 stChnl600CptPara.PrjType = X;
	 stChnl600CptPara.W1ColStrt = X1;
	 stChnl600CptPara.W1RowStrt = Y1;
     stChnl600CptPara.isGetGateoff = Chl600_TRUE/Chl600_FALSE;
 **/
EN_OXI600_ERR_TYPE Dev_Oxi600_CaptureShtInte(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t * pu8DataBuf,uint32_t * pu32Datalen,uint32_t u32Timeout)
{

	 
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;

	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV Dev_Oxi600_CaptureShtInte,project type:%#X,cptmode=%#X,cpttype=%#X,x1=%d,y1=%d,x2=%d,y2=%d,shtorlong:%d,inteline:%d,\n",
			stChnl600CptPara.PrjType,stChnl600CptPara.enCptMode,stChnl600CptPara.enCptType,stChnl600CptPara.W1ColStrt,stChnl600CptPara.W1RowStrt,\
			stChnl600CptPara.W2ColStrt,stChnl600CptPara.W2RowStrt,stChnl600CptPara.shtOrLong,stChnl600CptPara.integraLine);
	#endif

	memcpy(g_u8oxiWriteToRegBuf,g_u8Oxi600RegInitBuf,sizeof(g_u8Oxi600RegInitBuf));
	enRetVal = _dev_Oxi600_SensorDistinguish(stChnl600CptPara);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		_dev_Oxi600_logErrType("cpt win distinguish sensor",enRetVal);
		return enRetVal;
	}

	_delay_ms(10);

	/** reset farme + integration_gateoff + get gateoff data  **/
	enRetVal = _dev_Oxi600_CptShtFrame(stChnl600CptPara.PrjType,stChnl600CptPara.isGetGateoff,stChnl600CptPara.integraLine,u32Timeout);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		_dev_Oxi600_logErrType("_dev_Oxi600_CptShtFrame ",enRetVal);
		return enRetVal;
	}
	if(stChnl600CptPara.isGetGateoff == Chl600_TRUE)
	{
		enRetVal = Dev_Oxi600_GetImageData(pu8DataBuf ,pu32Datalen,u32Timeout);
		if(enRetVal != EN_OXI600_SUCCESS)
		{
			_dev_Oxi600_logErrType("Dev_Oxi600_GetImageData ",enRetVal);
			return enRetVal;
		}
	}
	if(_dev_Oxi600_WaitFrameEnd(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get image data check FSM2 timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}

	

	return enRetVal;
	
}






/**
 * @brief  get image data from FIFO
 * @param  pu8DataBuf the pointer to data buffer
 * @param  pu32DataLen the pointer to data length, Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_capShtLastFrame(ST_CHNL600_CPT_PARA stChnl600CptPara)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	stChnl600CptPara.isGetGateoff = Chl600_FALSE;
	enRetVal = _dev_Oxi600_SensorDistinguish(stChnl600CptPara);

	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV Dev_Oxi600_capShtLastFrame \n");
	#endif

	enRetVal = _dev_Oxi600_CptScanLastFrame(1,g_u16RowMax,1000);
	if(enRetVal != EN_OXI600_SUCCESS )
	{
		return enRetVal;
	}
	return EN_OXI600_SUCCESS;

}


/**
 * @brief  get image data from FIFO
 * @param  pu8DataBuf the pointer to data buffer
 * @param  pu32DataLen the pointer to data length, Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_getShtAndCptLong(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32DataLen,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint16_t u16covRow;

	stChnl600CptPara.isGetGateoff = Chl600_FALSE;	// for setting capture iamge paramter, otherwise, it will be set to gateoff.
	enRetVal = _dev_Oxi600_SensorDistinguish(stChnl600CptPara);
	if(enRetVal != EN_OXI600_SUCCESS)
	{
		_dev_Oxi600_logErrType("cpt win distinguish sensor",enRetVal);
		return enRetVal;
	}
	u16covRow = _dev_Oxi600_caclIncreaseRows(10);
	if(_dev_Oxi600_CptScan_GetImg_Inte_GetImg(g_u16RowMax+u16covRow,g_enShtInteCapacity,stChnl600CptPara.integraLine,u32Timeout)!= EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV _dev_Oxi600_CptScan_GetImg_Inte_GetImg check FSM1 timeout\n");
		return EN_OXI600_DATA_READY_ERR;
	}

	if(Dev_Oxi600_GetImageData(pu8DataBuf,pu32DataLen,u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get_short_image check FSM1 timeout\n");
		return EN_OXI600_DATA_READY_ERR;
	}
	
	_dev_Oxi600_WriteReg(0xd4,g_enLongInteCapacity);
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV write long integrate capacity= %#X \n",g_enLongInteCapacity);
	#endif
	return EN_OXI600_SUCCESS;

}





/**
 * @brief  get image data from FIFO
 * @param  pu8DataBuf the pointer to data buffer
 * @param  pu32DataLen the pointer to data length, Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE value
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_GetLongImageData(uint8_t *pu8DataBuf,uint32_t *pu32DataLen,uint32_t u32Timeout)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV get long image data,data size = %d \n",g_u32WinImgDataSize);
	#endif
	uint64_t u64localTime;
	u64localTime = _dev_Oxi600_GetLocalTime();
	while(1)
	{
		_dev_Oxi600_SpiReceive(pu8DataBuf,2);
		if(pu8DataBuf[0] != 0xff || pu8DataBuf[1] != 0xff)
			break;
		if((_dev_Oxi600_GetLocalTime() - u64localTime) > u32Timeout)
		{
			DBG("_OXIFP_IC_DRV get long image timeout \n");
			return EN_OXI600_CHECK_STATUS_TIMEOUT ;
		}
		_delay_ms(1);
	}
	_dev_Oxi600_SpiReceiveMass(&pu8DataBuf[2],g_u32WinImgDataSize-2);
	*pu32DataLen = g_u32WinImgDataSize;
	if(_dev_Oxi600_WaitFrameEnd(u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV get image data check FSM2 timeout\n");
		return EN_OXI600_CPT_RUN_ERR;
	}
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV get long image data,buf[0] = %#X, buf[1] = %#x \n",pu8DataBuf[0],pu8DataBuf[1]);
	#endif
	
	return EN_OXI600_SUCCESS;

}
/**
 * @brief  capture whoel image and get data (using in TEE)
 * @param  stChnl600CptPara  
 * @param  pu8DataBuf  the pointer to data buffer
 * @param  pu32Datalen the pointer to data length,Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE

EXAMPLE:
  stChnl600CptPara.enCptMode = 0x71;
  stChnl600CptPara.enCptType = EN_MODE1_CPT_WHLE_ROW_IMG;
  stChnl600CptPara.integraLine = x;
  stChnl600CptPara.shtOrLong = X;
  stChnl600CptPara.PrjType = X;
  stChnl600CptPara.XaoDelay = X;
  stChnl600CptPara.isGetGateoff = Chl600_TRUE/Chl600_FALSE;
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_TeeCptWholeRowImage(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8DataBuf,uint32_t *pu32Datalen,uint32_t u32Timeout)
{	
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t i,u8CptWhlFrmNo,u8CptRowNo,u8CptLastRowNo;
	uint32_t u32RowOfset,u32BufOfset,u32GateOffSize,u32orgImgSize;
	ST_CHNL600_CPT_PARA stChnl600Tmp;
	uint16_t u16inteBackup;
	stChnl600Tmp = stChnl600CptPara;
	u32RowOfset = 0;
	u32BufOfset = 0;
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV capture tee whole image \n
		para:mode=%#X,prjtype=%#X,cpttype=%#X,gateoff=%#X,inte=%#X,tmp=%#X\n",stChnl600CptPara.enCptMode,stChnl600CptPara.PrjType,stChnl600CptPara.enCptType,
		stChnl600CptPara.isGetGateoff,stChnl600CptPara.integraLine,stChnl600CptPara.u16TmpInte);
	#endif	
	
	switch(stChnl600CptPara.PrjType)
	{
		case EN_PRJ_OXI600_MK720_80UM:
		case EN_PRJ_OXI600_MK720_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 73;
			u8CptLastRowNo = 73;
			u32orgImgSize = 439*292*2;
			break;
			
		case EN_PRJ_OXI600_MK720_100UM:
		case EN_PRJ_OXI600_MK720_100UM_1_3:
			u8CptWhlFrmNo = 3;
			u8CptRowNo = 87;
			u8CptLastRowNo = 54;
			u32orgImgSize = 369*228*2;
			break;
			
		case EN_PRJ_OXI600_MK810_80UM:
		case EN_PRJ_OXI600_MK810_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 63;
			u8CptLastRowNo = 61;
			u32orgImgSize = 510*250*2;
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 63;
			u8CptLastRowNo = 61;
			u32orgImgSize = 493*250*2;
			break;

		case EN_PRJ_OXI600_MS001_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 79;
			u8CptLastRowNo = 55;
			u32orgImgSize = 404*292*2;
			break;

		case EN_PRJ_OXI600_MS006_80UM_1_3:
		case EN_PRJ_OXI600_MS006_80UM_V01:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 79;
			u8CptLastRowNo = 51;
			u32orgImgSize = 388*288*2;
			break;

		default:
			DBG("_OXIFP_IC_DRV capture tee whole image para err type = %d \n",stChnl600CptPara.PrjType);
			return EN_OXI600_PARA_ERR;
			break;
		
	}
	
	for(i = 0;i < u8CptWhlFrmNo;i++)
	{
		stChnl600CptPara = stChnl600Tmp;	// cause next will change the struct , need reassignment
		if(i == u8CptWhlFrmNo -1)
		{
			stChnl600CptPara.W1RowStrt = u32RowOfset;
			stChnl600CptPara.W1RowNo = u8CptLastRowNo;
			u32RowOfset += stChnl600CptPara.W1RowNo;
		}
		else
		{
			stChnl600CptPara.W1RowStrt = u32RowOfset;
			stChnl600CptPara.W1RowNo = u8CptRowNo;
			u32RowOfset += u8CptRowNo;
		}
		/*  gateoff paramter must FALSE, gateoff captured when integrate */
		enRetVal = Dev_Oxi600_ClrAndGetGateoff(stChnl600CptPara,pu8DataBuf+u32BufOfset,pu32Datalen,Chl600_FALSE,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			_dev_Oxi600_logErrType("tee cpt whole clr",enRetVal);
			return enRetVal;
		}
		if(stChnl600CptPara.shtOrLong == 0)
		{
			// calculate orignal image size ,then capture the gateoff to bottom
			enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf+u32orgImgSize,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			if((stChnl600CptPara.isGetGateoff == Chl600_TRUE) &&(i == u8CptWhlFrmNo -1))
			{
				u32GateOffSize = *pu32Datalen;
			}
			enRetVal = Dev_Oxi600_capShtLastFrame(stChnl600CptPara);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			enRetVal = Dev_Oxi600_GetImageData(pu8DataBuf+u32BufOfset,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			
			u32BufOfset += *pu32Datalen;
	
		}
		else
		{
			u16inteBackup = stChnl600CptPara.integraLine;		// backup long integraion lines
			stChnl600CptPara.shtOrLong = 0; 					// notice , for correct capacity
			// genrate short waveform ,using short integration lines
			stChnl600CptPara.integraLine = stChnl600CptPara.u16TmpInte;
	
			enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8DataBuf+u32orgImgSize,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			if((stChnl600CptPara.isGetGateoff == Chl600_TRUE) &&(i == u8CptWhlFrmNo -1))
			{
				u32GateOffSize = *pu32Datalen;
			}
			stChnl600CptPara.integraLine = u16inteBackup; 		// change to long integration
			enRetVal = Dev_Oxi600_getShtAndCptLong(stChnl600CptPara,pu8DataBuf+u32BufOfset,pu32Datalen,1000);	
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			enRetVal = Dev_Oxi600_GetLongImageData(pu8DataBuf+u32BufOfset,pu32Datalen,2000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			
			u32BufOfset += *pu32Datalen;
		}


	}
	if(stChnl600CptPara.isGetGateoff == Chl600_TRUE)
	{
		u32BufOfset += u32GateOffSize;
	}
	*pu32Datalen = u32BufOfset;

    return enRetVal;
}


/**
 * @brief  capture whoel image and get data (using in TEE)
 * @param  stChnl600CptPara  
 * @param  pu8DataBuf  the pointer to data buffer
 * @param  pu32Datalen the pointer to data length,Reassign datalen indicating effective data length
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE

EXAMPLE:
  stChnl600CptPara.enCptMode = 0x71;
  stChnl600CptPara.enCptType = EN_MODE1_CPT_WHLE_ROW_IMG;
  stChnl600CptPara.integraLine = x;
  stChnl600CptPara.shtOrLong = X;
  stChnl600CptPara.PrjType = X;
  stChnl600CptPara.XaoDelay = X;
  stChnl600CptPara.isGetGateoff = Chl600_TRUE/Chl600_FALSE;
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_TeeCptWholeRowImage_ShtAndLong(ST_CHNL600_CPT_PARA stChnl600CptPara,uint8_t *pu8ShtDataBuf,uint8_t *pu8LongDataBuf,uint32_t *pu32Datalen,uint32_t u32Timeout)
{	
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t i,u8CptWhlFrmNo,u8CptRowNo,u8CptLastRowNo;
	uint32_t u32RowOfset,u32BufOfset,u32GateOffSize,u32orgImgSize;
	ST_CHNL600_CPT_PARA stChnl600Tmp;
	uint16_t u16inteBackup;
	stChnl600Tmp = stChnl600CptPara;
	u32RowOfset = 0;
	u32BufOfset = 0;
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV capture tee whole image test\n
		para:mode=%#X,prjtype=%#X,cpttype=%#X,gateoff=%#X,inte=%#X,tmp=%#X\n",stChnl600CptPara.enCptMode,stChnl600CptPara.PrjType,stChnl600CptPara.enCptType,
		stChnl600CptPara.isGetGateoff,stChnl600CptPara.integraLine,stChnl600CptPara.u16TmpInte);
	#endif	
	
	switch(stChnl600CptPara.PrjType)
	{
		case EN_PRJ_OXI600_MK720_80UM:
		case EN_PRJ_OXI600_MK720_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 73;
			u8CptLastRowNo = 73;
			u32orgImgSize = 439*292*2;
			break;
			
		case EN_PRJ_OXI600_MK720_100UM:
		case EN_PRJ_OXI600_MK720_100UM_1_3:
			u8CptWhlFrmNo = 3;
			u8CptRowNo = 87;
			u8CptLastRowNo = 54;
			u32orgImgSize = 369*228*2;
			break;
			
		case EN_PRJ_OXI600_MK810_80UM:
		case EN_PRJ_OXI600_MK810_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 63;
			u8CptLastRowNo = 61;
			u32orgImgSize = 510*250*2;
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 63;
			u8CptLastRowNo = 61;
			u32orgImgSize = 493*250*2;
			break;

		case EN_PRJ_OXI600_MS001_80UM_1_3:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 79;
			u8CptLastRowNo = 55;
			u32orgImgSize = 404*292*2;
			break;

		case EN_PRJ_OXI600_MS006_80UM_1_3:	
		case EN_PRJ_OXI600_MS006_80UM_V01:
			u8CptWhlFrmNo = 4;
			u8CptRowNo = 79;
			u8CptLastRowNo = 51;
			u32orgImgSize = 388*288*2;
			break;

		default:
			DBG("_OXIFP_IC_DRV capture tee whole image para err type = %d \n",stChnl600CptPara.PrjType);
			return EN_OXI600_PARA_ERR;
			break;
		
	}
	
	for(i = 0;i < u8CptWhlFrmNo;i++)
	{
		stChnl600CptPara = stChnl600Tmp;	// cause next will change the struct , need reassignment
		if(i == u8CptWhlFrmNo -1)
		{
			stChnl600CptPara.W1RowStrt = u32RowOfset;
			stChnl600CptPara.W1RowNo = u8CptLastRowNo;
			u32RowOfset += stChnl600CptPara.W1RowNo;
		}
		else
		{
			stChnl600CptPara.W1RowStrt = u32RowOfset;
			stChnl600CptPara.W1RowNo = u8CptRowNo;
			u32RowOfset += u8CptRowNo;
		}
		/*  gateoff paramter must FALSE, gateoff captured when integrate */
		enRetVal = Dev_Oxi600_ClrAndGetGateoff(stChnl600CptPara,pu8ShtDataBuf+u32BufOfset,pu32Datalen,Chl600_FALSE,1000);
		if(enRetVal != EN_OXI600_SUCCESS )
		{
			_dev_Oxi600_logErrType("tee cpt whole clr",enRetVal);
			return enRetVal;
		}
		if(stChnl600CptPara.shtOrLong == 0)
		{
			// calculate orignal image size ,then capture the gateoff to bottom
			enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8ShtDataBuf+u32orgImgSize,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			if((stChnl600CptPara.isGetGateoff == Chl600_TRUE) &&(i == u8CptWhlFrmNo -1))
			{
				u32GateOffSize = *pu32Datalen;
			}
			enRetVal = Dev_Oxi600_capShtLastFrame(stChnl600CptPara);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			enRetVal = Dev_Oxi600_GetImageData(pu8ShtDataBuf+u32BufOfset,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			
			u32BufOfset += *pu32Datalen;
	
		}
		else
		{
			u16inteBackup = stChnl600CptPara.integraLine;		// backup long integraion lines
			stChnl600CptPara.shtOrLong = 0; 					// notice , for correct capacity
			// genrate short waveform ,using short integration lines
			stChnl600CptPara.integraLine = stChnl600CptPara.u16TmpInte;
	
			enRetVal = Dev_Oxi600_CaptureShtInte(stChnl600CptPara,pu8ShtDataBuf+u32orgImgSize,pu32Datalen,1000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			if((stChnl600CptPara.isGetGateoff == Chl600_TRUE) &&(i == u8CptWhlFrmNo -1))
			{
				u32GateOffSize = *pu32Datalen;
			#ifdef DEBUG_DEV_OXI600
				DBG("_OXIFP_IC_DRV tee_capture_whole copy gateoff,buffer shift = %d,copy size = %d\n",u32orgImgSize,u32GateOffSize);
			#endif
				memcpy(pu8LongDataBuf+u32orgImgSize,pu8ShtDataBuf+u32orgImgSize,u32GateOffSize);
			}
			stChnl600CptPara.integraLine = u16inteBackup; 		// change to long integration
			enRetVal = Dev_Oxi600_getShtAndCptLong(stChnl600CptPara,pu8ShtDataBuf+u32BufOfset,pu32Datalen,1000);	
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			enRetVal = Dev_Oxi600_GetLongImageData(pu8LongDataBuf+u32BufOfset,pu32Datalen,2000);
			if(enRetVal != EN_OXI600_SUCCESS )
			{
				return enRetVal;
			}
			
			u32BufOfset += *pu32Datalen;
		}


	}
	if(stChnl600CptPara.isGetGateoff == Chl600_TRUE)
	{
		u32BufOfset += u32GateOffSize;
	}
	*pu32Datalen = u32BufOfset;

    return enRetVal;
}






/**
 * @brief  switch Vcom to XXX and disable clear frame 
 * @param  enVcomMod  Vcom to xxx  
 * @param  u8FramNo  captrue scan frame count
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_SwitchVcomVol(EN_CHNL600_VCOM_MODE enVcomMod,uint8_t u8FramNo)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV switch vcom mode = %d,frame = %#X\n",enVcomMod,u8FramNo);
	#endif

	if(enVcomMod == EN_RESET_TO_DEFAULT || enVcomMod == EN_VCOM_TO_VDD || \
		enVcomMod == EN_VCOM_TO_VEE || enVcomMod == EN_VCOM_TO_VCOM)
	{
		g_u8SwitchVcomFlag = enVcomMod;
		g_u8SwitchVcomFramCnt = u8FramNo;
	}
	else
	{
		DBG("_OXIFP_IC_DRV switch vcom para err:mod = %d,framNo = %d\n",enVcomMod,u8FramNo);
		enRetVal = EN_OXI600_PARA_ERR;
	}
	

	return enRetVal;
}

/**
 * @brief  sleep ROIC disale ADC 
 * @param  stChnl600CptPara  just use stChnl600CptPara.PrjType
 * @param  u32Timeout 
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_SleepROIC(ST_CHNL600_CPT_PARA stChnl600CptPara,uint32_t u32Timeout)
{
	EN_OXI600_ERR_TYPE enRetVal = EN_OXI600_SUCCESS;
	uint8_t u8TmpRegBuf[12] = {0xAA,0,0xfc,0xff,0xff,0xff,0xcf,0,0xf8,0xff,0xf2,0xff};
#ifdef DEBUG_DEV_OXI600
	DBG("_OXIFP_IC_DRV sleep ROIC,project type = %#X \n",stChnl600CptPara.PrjType);
#endif
	//XAO_L();

	//ROIC_CMD_TRANSFER(); /*if FIFO ready(INT_H),F8FF restart just clear FIFO,but the INT still high level, FCFF will change the INT to low level */

	if(stChnl600CptPara.PrjType == EN_PRJ_OXI600_MK810_80UM_1_3 || stChnl600CptPara.PrjType == EN_PRJ_OXI600_MK720_100UM_1_3\
		|| stChnl600CptPara.PrjType == EN_PRJ_OXI600_MK720_80UM_1_3 ||stChnl600CptPara.PrjType == EN_PRJ_OXI600_MS001_80UM_1_3\
		|| stChnl600CptPara.PrjType == EN_PRJ_OXI600_MS006_80UM_1_3 ||stChnl600CptPara.PrjType == EN_PRJ_OXI600_MS006_80UM_V01 )
	{
		//DISABLE_ROIC_ADC(0x1f);	/*ENHV LOW LEVEL WORKING*/
		u8TmpRegBuf[7] = 0x1f;
	}
	else
	{
		//DISABLE_ROIC_ADC(0x0f);	/*ENHV HIGH LEVEL WORKING*/
		u8TmpRegBuf[7] = 0x0f;
	}

	
	//ROIC_CMD_RESTART();

	//ROIC_CMD_SLEEP();

	
	_dev_Oxi600_WriteData(u8TmpRegBuf,sizeof(u8TmpRegBuf));
	if(_dev_Oxi600_WaitRegStatus(0x3f,0x0F00,0x0000,u32Timeout) != EN_OXI600_SUCCESS)
	{
		DBG("_OXIFP_IC_DRV sleep ROIC check FSM timeout\n");
		return EN_OXI600_SLEEP_ROIC_ERR;
	}
	return enRetVal;	
}

/**
 * @brief  set PMU parameter
 * @param  pu8DataBuf the pointer to data buffer
 * @param  u32DataLen  the data length
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_setPmuPara(uint8_t *pu8DataBuf, uint32_t u32DataLen)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("_OXIFP_IC_DRV set PMU paramter\n");
	#endif
	memcpy(g_u8Oxi600RegInitBuf,pu8DataBuf,u32DataLen);
	return EN_OXI600_SUCCESS;
}


/**
 * @brief  get PMU parameter
 * @param  pu8DataBuf  the pointer to data buffer 
 * @param  u32DataLen  the data length,Reassign datalen indicating effective data length 
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_getPmuPara(uint8_t *pu8DataBuf, uint32_t *pu32DataLen)
{
        int i = 0;
		DBG("_OXIFP_IC_DRV get PMU paramter\n");
        for(i = 0;i <= 103 ;i++ )
		{
			DBG("%X %X %X \n",g_u8Oxi600RegInitBuf[i*3+0],g_u8Oxi600RegInitBuf[i*3+1],g_u8Oxi600RegInitBuf[i*3+2]);
		}
		DBG("_OXIFP_IC_DRV get PMU para end\n");
		
		*pu32DataLen = sizeof(g_u8Oxi600RegInitBuf);
		memcpy(pu8DataBuf,g_u8Oxi600RegInitBuf,*pu32DataLen);
		return EN_OXI600_SUCCESS;
}


/**
 * @brief  set capture flow:Vdd Vee Vcom capture count ....
 * @param  pu8DataBuf  the pointer to data buffer 
 * @param  u32DataLen  the data length
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_setCptPara(uint8_t *pu8DataBuf, uint32_t u32DataLen)
{
	int i = 0;
	ST_OXI600_CLR_PARA stCptPara;
	if(pu8DataBuf[0] == 2)
	{
		#ifdef DEBUG_DEV_OXI600
			DBG("_OXIFP_IC_DRV set cpt paramter flag choose disable\n");
		#endif
		return EN_OXI600_SUCCESS;
	}
		
	stCptPara.bClrLagEn = pu8DataBuf[0];
	stCptPara.bVddEn= pu8DataBuf[1];
	stCptPara.u8VddScnCnt = pu8DataBuf[2];
	stCptPara.u16VddFsStvCovCnt = ((uint16_t)pu8DataBuf[3]<<8)|((uint16_t)pu8DataBuf[4]);
	stCptPara.u32SwVddDelay = ((uint32_t)pu8DataBuf[5]<<24)|((uint32_t)pu8DataBuf[6]<<16)|\
							((uint32_t)pu8DataBuf[7]<<8)|((uint32_t)pu8DataBuf[8]);
	
	stCptPara.bVeeEn= pu8DataBuf[9];
	stCptPara.u8VeeScnCnt = pu8DataBuf[10];
	stCptPara.u16VeeFsStvCovCnt = ((uint16_t)pu8DataBuf[11]<<8)|((uint16_t)pu8DataBuf[12]);
	stCptPara.u32SwVeeDelay = ((uint32_t)pu8DataBuf[13]<<24)|((uint32_t)pu8DataBuf[14]<<16)|\
							((uint32_t)pu8DataBuf[15]<<8)|((uint32_t)pu8DataBuf[16]);

	stCptPara.bVcomEn= pu8DataBuf[17];
	stCptPara.u8VcomScnCnt = pu8DataBuf[18];
	stCptPara.u16VcomFsStvCovCnt = ((uint16_t)pu8DataBuf[19]<<8)|((uint16_t)pu8DataBuf[20]);
	stCptPara.u32SwVcomDelay = ((uint32_t)pu8DataBuf[21]<<24)|((uint32_t)pu8DataBuf[22]<<16)|\
							((uint32_t)pu8DataBuf[23]<<8)|((uint32_t)pu8DataBuf[24]);

	stCptPara.u8CptImgCnt = pu8DataBuf[29];
	if(!(pu8DataBuf[30] == 0xff && pu8DataBuf[31] == 0xff))
	{
		g_enShtInteCapacity = pu8DataBuf[30];
		g_enLongInteCapacity = pu8DataBuf[31];
	}
	
	g_stOxi600clrpara = stCptPara;

	#ifdef DEBUG_DEV_OXI600
	DBG("_OXIFP_IC_DRV set cpt paramter\n");
	DBG("clr en:%d, vdd en:%d, vdd cnt:%d, vdd cov:%d, vdd delya:%d , vee en:%d, vee cnt:%d, vee cov:%d,\
	vee delya:%d, vcom en:%d, vcom cnt:%d, vcom cov%d, vcom delya:%d,cpt cnt:%d,sht cap:%#X,long cap:%#X \n",stCptPara.bClrLagEn,\
			stCptPara.bVddEn,stCptPara.u8VddScnCnt,stCptPara.u16VddFsStvCovCnt,stCptPara.u32SwVddDelay,stCptPara.bVeeEn,stCptPara.u8VeeScnCnt,\
			stCptPara.u16VeeFsStvCovCnt,stCptPara.u32SwVeeDelay,stCptPara.bVcomEn,stCptPara.u8VcomScnCnt,stCptPara.u16VcomFsStvCovCnt,\
			stCptPara.u32SwVcomDelay,stCptPara.u8CptImgCnt,g_enShtInteCapacity,g_enLongInteCapacity);
	#endif

    return EN_OXI600_SUCCESS;
}




/**
 * @brief  set capture flow:Vdd Vee Vcom capture count ....
 * @param  pu8paraBuf  the pointer to data buffer 
 * @param  u32bufLen  the data length
 * @param  u8Linetime	linetime select , 0: 107us, 1:121us
 * @retval EN_OXI600_ERR_TYPE
**/
EN_OXI600_ERR_TYPE Dev_Oxi600_SetPartPara(int *paraBuf,uint32_t u32bufLen)
{
	/*
	*	buf[0]: linetime select
	*	buf[1]: vcom frame count
	*	buf[2]: short inte capacity
	*	buf[3]: long inte capacity
	*/
#ifdef DEBUG_DEV_OXI600
	DBG("_OXIFP_IC_DRV set parameter linetime = %d, vcomCnt = %d,shtCap= %d,longCap= %d\n",
		paraBuf[0],paraBuf[1],paraBuf[2],paraBuf[3]);
#endif

	// paraBuf[0] :  linetime select 
	if (0 == paraBuf[0])
	{
		memcpy(g_u8Oxi600RegInitBuf, g_u8Oxi600RegInitBuf_linetime107us, sizeof(g_u8Oxi600RegInitBuf));
	}
	else if (1 == paraBuf[0])
	{
		memcpy(g_u8Oxi600RegInitBuf, g_u8Oxi600RegInitBuf_linetime121us, sizeof(g_u8Oxi600RegInitBuf)); 	   
	}
    else
    {        
        DBG("_OXIFP_IC_DRV set parameter linetime error\n");
        return EN_OXI600_PARA_ERR;
    }


	// paraBuf[1] clear frame vcom count setting
	if(paraBuf[1] > 9)	// in fact ,this is no limit ,but for save 
	{		 
		DBG("_OXIFP_IC_DRV set parameter frame count error\n");
		return EN_OXI600_PARA_ERR;
	}
	g_stOxi600clrpara.u8VcomScnCnt = paraBuf[1];


	//paraBuf[2],paraBuf[3] ROIC capacity setting
	if((paraBuf[2] <= 15 && paraBuf[2] > 0) &&(paraBuf[3] <= 15 && paraBuf[3] > 0))
	{
		g_enShtInteCapacity = _dev_Oxi600_convertCapacity(paraBuf[2]);
		g_enLongInteCapacity = _dev_Oxi600_convertCapacity(paraBuf[3]);
	}
	else
	{        
        DBG("_OXIFP_IC_DRV set parameter capacity error\n");
        return EN_OXI600_PARA_ERR;
    }
    return EN_OXI600_SUCCESS;

	
	
}



uint16_t Dev_Oxi600_ReadRegVal(uint8_t u8RegAddr)
{
	return _dev_Oxi600_ReadReg(u8RegAddr);
}


/*
* @brief  get lib version 
* @param  pu8DataBuf pointer to buffer 4 bytes 
* @retval EN_OXI600_ERR_TYPE
*/
EN_OXI600_ERR_TYPE Dev_Oxi600_getLibVer(uint8_t* pu8DataBuf)
{
	#ifdef DEBUG_DEV_OXI600
		DBG("get ROIC drv lib version:%d,%d,%d,%d\n",u8LibVer[0],u8LibVer[1],u8LibVer[2],u8LibVer[3]);
	#endif
	memcpy(pu8DataBuf,u8LibVer,4);
	return EN_OXI600_SUCCESS;
}
#if 0
EN_OXI600_ERR_TYPE Dev_Oxi600_Exit()
{
    if (_g_stCh600ExterDrv)
        oxi_free(_g_stCh600ExterDrv);

    return EN_OXI600_SUCCESS;
}
uint16_t W25QXX_ReadID(void)
{
	uint8_t sendData[4]={0x90,0,0,0};
	uint8_t receData=0;  
    uint16_t  temp = 0;      
    DBG("W25QXX_ReadID  start\n");
	_g_stCh600ExterDrv->SPI_Send(sendData,4); //发送读取ID命令	    
	_g_stCh600ExterDrv->SPI_Receive(&receData,1);  
    temp|=(uint16_t)receData<<8;
	_g_stCh600ExterDrv->SPI_Receive(&receData,1);  
    temp|=receData;
    DBG("W25QXX_ReadID  end\n");
    return temp;
}   
void W25QXX_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{ 
	uint8_t sendData=0x03;
    DBG("W25QXX_Read  start\n");
    _g_stCh600ExterDrv->SPI_Send(&sendData,1);       //发送读取命令   
    sendData = ((uint8_t)((ReadAddr)>>16));  
     _g_stCh600ExterDrv->SPI_Send(&sendData,1);
    sendData = ((uint8_t)((ReadAddr)>>8));  
	_g_stCh600ExterDrv->SPI_Send(&sendData,1);  
    sendData = ((uint8_t)ReadAddr);   
	_g_stCh600ExterDrv->SPI_Send(&sendData,1);  
    _g_stCh600ExterDrv->SPI_Receive(pBuffer,NumByteToRead);
    DBG("W25QXX_Read  end\n");
}  
#endif
