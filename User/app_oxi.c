#include <string.h>
#include <stdio.h>

#include "main.h"

#include "app_oxi.h"
#include "app_oxi600.h"
#include "app_usb.h"

#include "lib_cmos.h"
#include "w25Qxx.h"
#include "dev_usb.h"

#include "heap_5.h"

#include "usbd_cdc_if.h"

#define LOG_APP_OXI

void App_Oxi_Task(void)
{
	App_Usb_intfcProc();
}

void App_Oxi_showCodeRun(void)
{
	volatile static u32 su32tick =0;
	volatile static bool sbFlagLedOn = TRUE;

	HAL_Delay(1);
	su32tick++;
	if(su32tick% 500 == 0)
	{		
		if(sbFlagLedOn == TRUE)
		{
			HAL_GPIO_WritePin(GPIOE,GPIO_PIN_12,GPIO_PIN_RESET);
			sbFlagLedOn = FALSE;
		}
		else
		{
			HAL_GPIO_WritePin(GPIOE,GPIO_PIN_12,GPIO_PIN_SET);			
			sbFlagLedOn = TRUE;
		}
	}
}

void App_Oxi_testGPIOBacklightCtrl(void)
{
	volatile static u32 su32tick =0;
	volatile static bool sbFlagLedOn = TRUE;

	HAL_Delay(1);
	su32tick++;
	if(su32tick% 1000 == 0)
	{		
		if(sbFlagLedOn == TRUE)
		{
			DBG("light off\r\n");
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_RESET);
			sbFlagLedOn = FALSE;
		}
		else
		{
			DBG("light on\r\n");
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_2,GPIO_PIN_SET);			
			sbFlagLedOn = TRUE;
		}
	}
}


void App_Oxi_testBacklightCtrl(void)
{
	volatile static u32 su32tick =0, su32FlowCnt = 0;
	static bool sbFlagIncres = TRUE;
	static bool sbFlagWhileLedOn = TRUE;
	static bool sbFlagFstRun = TRUE;

	HAL_Delay(1);
	su32tick++;
	if(su32tick% 500 == 0)
	{
		static int su32tick_xx=0;
	#ifdef LOG_APP_OXI
		printf("WhileLedOn -- %d,Incres -- %d, tick -- %d\r\n",sbFlagWhileLedOn,sbFlagIncres,su32tick_xx);
	#endif
		if(sbFlagWhileLedOn == TRUE)
		{
			pwm_led_set(1,su32tick_xx);
			pwm_led_set(2,0);
		}
		else if(sbFlagWhileLedOn == FALSE)
		{
			pwm_led_set(2,su32tick_xx);
			pwm_led_set(1,0);
		}

		if(sbFlagFstRun == FALSE)
		{
			if((su32tick_xx == 100)||(su32tick_xx == 0))
			{
				sbFlagIncres = !sbFlagIncres;
				su32FlowCnt++;
			}
		}

		sbFlagFstRun = FALSE;

		if(su32FlowCnt == 2)
		{
			su32FlowCnt = 0;
			sbFlagWhileLedOn = !sbFlagWhileLedOn;
			sbFlagFstRun  = TRUE;
			return ;
		}

		if(sbFlagIncres == TRUE)
		{
			su32tick_xx++;
		}
		else if(sbFlagIncres == FALSE)
		{
			su32tick_xx--;
		}
	}
}

extern ADC_HandleTypeDef hadc1;
void adc_test(void)
{
	uint32_t val =0;
	uint32_t i;
	for(i=0;i<2;i++)
	{
		val +=BSP_getAdcVal(&hadc1, ADC_CHANNEL_14);
		HAL_ADC_Stop(&hadc1);
	}
	val /=2;
	
	printf("=======adc val: %d==============\r\n", val);
	
}

void App_Oxi_testADC(void)
{
	volatile static u32 su32tick =0;
	
	HAL_Delay(1);
	su32tick++;
	if(su32tick% 500 == 0)
	{		
		adc_test();
	}
}


void App_Oxi_TestUSBCommunication(void)
{
	static bool sbFlagFrstRun = TRUE;
	uint32_t i;
	uint8_t u8UsbSendBuf[] = {0xf0, 0x00 ,0xff, 0xff};
	uint8_t UsbComResult;

	if(sbFlagFrstRun == TRUE)
	{
		sbFlagFrstRun = FALSE;
		Dev_Usb_remallocRxBuf(70*1024);
	#ifdef LOG_APP_OXI
		DBG("malloc 70k for usb rx buffer,then MCU free ram:%d\r\n",xPortGetFreeHeapSize());
	#endif
	}
	
	if((g_u32UsbRcvLen != 0)||(g_UsbRcvEpPackCnt != 0))
	{
		g_UsbRcvEpPackCnt = 0;
	#ifdef LOG_APP_OXI
		DBG("rcv en pack cnt:%d,u32RcvLen = %d\r\n",g_UsbRcvEpPackCnt,g_u32UsbRcvLen);
	
		for(i=0; i<g_u32UsbRcvLen; i++ )
		{
			DBG("%02X ", g_u8UsbCmdRcvBuf[i]);
		}
		DBG("\r\n");
	#endif
		g_u32UsbRcvLen = 0;
		UsbComResult = CDC_Transmit_HS(u8UsbSendBuf, 64);
	#ifdef LOG_APP_OXI
		DBG("result:%d\r\n",UsbComResult);
	#endif
	}

}

/*
 * Sensor读图->写Flash->读Sensor寄存器-->读Flash数据->读图
 * 
 */
 extern uint8_t g_u8CmosRegInitBuf[];
static bool testLenovoCmos_cptImgFlow(uint8_t* dstBuf, uint16_t intTime)
{
	uint8_t u8IntTimeBuf[2];
	
	Dev_CMOS_RoicRegInit();
	SplitU16(u8IntTimeBuf, intTime);
	
	if(Dev_Cmos_SetIntTime(u8IntTimeBuf) != OXI_Success)
	{
	#ifdef LOG_APP_OXI
		printf("set intragel time failed\r\n");
	#endif
		return FALSE;
	}
	
	if(Dev_Cmos_Cpt(dstBuf) != OXI_Success)
	{
		return FALSE;
	}

	return TRUE;
}

void App_Oxi_testlenovoCmos(void)
{
	volatile uint32_t vu32LocalTick;
	static bool sbFlagPwrOn = FALSE;
	uint8_t* pu8FlashWrtBuf = malloc(4096);
	uint8_t* pu8FlashRdBuf = malloc(4096);
	uint16_t u16IntTime = 100; /* uint:ms*/
	uint32_t i;
	static uint32_t su32TstCnt = 0;
	uint32_t u32FlashTstSize = 0x10000; /* 1M */
	
	g_pu8DataBuf = malloc(88*86*2*2);
	vu32LocalTick = HAL_GetTick();
	su32TstCnt++;
	
	/* power on begin */
	if(sbFlagPwrOn == FALSE)
	{
		sbFlagPwrOn = TRUE;
		BSP_PowerControl(0x80, 100);
	}
	/* power on end */
	
	/* capture image begin */
	if(testLenovoCmos_cptImgFlow(g_pu8DataBuf, u16IntTime) == TRUE)
	{
	#ifdef LOG_APP_OXI
		printf("before flash option, %dst cpt img success\r\n", su32TstCnt);
	#endif
	}
	else
	{
	#ifdef LOG_APP_OXI
		printf("before flash option, %dst cpt img failed\r\n", su32TstCnt);
	#endif
		goto EXIT;
	}
	/* cpature image end */

	/*write flash begin*/
	HAL_GPIO_WritePin(CS0_N_SPI_GPIO_Port,CS0_N_SPI_Pin,GPIO_PIN_RESET);
	for(i= 0; i<4096 ; i++)
	{
		pu8FlashWrtBuf[i] = i%256;
	}
	
	W25QXX_Erase_multi_Sector(FLASH_BASE_ADDR, u32FlashTstSize);
	for(i=0; i<u32FlashTstSize/4096; i++)
	{
		W25QXX_Write(pu8FlashWrtBuf ,FLASH_BASE_ADDR+i*4096, 4096);
	}
	HAL_GPIO_WritePin(CS0_N_SPI_GPIO_Port,CS0_N_SPI_Pin,GPIO_PIN_SET);
	/* write flash end */

	

	/* read flash and verify begin */
	HAL_GPIO_WritePin(CS0_N_SPI_GPIO_Port,CS0_N_SPI_Pin,GPIO_PIN_RESET);
	for(i=0; i<u32FlashTstSize/4096; i++)
	{
		IwdgFeed();
		memset(pu8FlashRdBuf, 0x00, 4096);
		W25QXX_Read(pu8FlashRdBuf, FLASH_BASE_ADDR+i*4096, 4096);
		if(memcmp(pu8FlashWrtBuf, pu8FlashRdBuf, 4096) != 0)
		{
		#ifdef LOG_APP_OXI
			printf("%dst W/R flash:%dst sector flash data change,test failed\r\n", su32TstCnt, i);
		#endif
			break;
		}
	}

	if(i == u32FlashTstSize/4096)
	{
	#ifdef LOG_APP_OXI
		printf("flash test success\r\n");
	#endif
	}
	else
	{
		goto EXIT;
	}
	HAL_GPIO_WritePin(CS0_N_SPI_GPIO_Port,CS0_N_SPI_Pin,GPIO_PIN_SET);
	/* read flash and verify end */
	
	/* capture image begin */
	if(testLenovoCmos_cptImgFlow(&g_pu8DataBuf[88*86*2], u16IntTime) == TRUE)
	{
	#ifdef LOG_APP_OXI
		printf("after flash option, %dst cpt img success\r\n", su32TstCnt);
	#endif
	}
	else
	{
	#ifdef LOG_APP_OXI
		printf("after flash option, %dst cpt img failed\r\n", su32TstCnt);
	#endif
		goto EXIT;
	}
	/* cpature image end */

EXIT:
	free(g_pu8DataBuf);
	free(pu8FlashWrtBuf);
	free(pu8FlashRdBuf);
	
	IwdgFeed();
	if(HAL_GetTick() - vu32LocalTick < 3000)/* 1 cycle 1s */
	{
		HAL_Delay(3000 - (HAL_GetTick() - vu32LocalTick));
	}

#ifdef LOG_APP_OXI
	printf("\r\n\r\n");
#endif
}

void App_Oxi_ageingProc(void)
{
	volatile u32 vu32LocalTick;
	u32 u32CptCostTim;
	
	vu32LocalTick = HAL_GetTick();
	
	App_Oxi600_ageingProc();
	
	u32CptCostTim = HAL_GetTick() - vu32LocalTick;
	DBG("cpt:%d\r\n", u32CptCostTim);
	
	if(u32CptCostTim < 1000)/*1s*/
	{
		HAL_Delay(1000 - u32CptCostTim);
	}	
}


