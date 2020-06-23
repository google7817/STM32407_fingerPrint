#include "bsp.h"
#include <string.h>
#include "dev_stflash.h"
#include "main.h"
#include "heap_5.h"

#include "lib_oxi600.h"

#define DEBUG_BSP

volatile uint8_t g_u8SpiRXCpltFlag = 0;

volatile uint8_t g_u8Vmod2 = 0x01;
volatile uint8_t g_u8Buf =  0x0E;
volatile uint16_t g_u16Vmod1 = 3300;
volatile uint8_t g_u8SpiSped = 2;
volatile uint16_t g_u16Mod1CaliVal;
volatile uint16_t g_u16Mod2CaliVal;
volatile int g_readCurrent=0;


extern IWDG_HandleTypeDef hiwdg;

extern void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);
extern void Error_Handler(void);

void IwdgFeed(void)
{
	HAL_IWDG_Refresh(&hiwdg);
}

/* platform for special function of some project */
void BSP_prjSpecialInit(void)
{
#ifdef  PLATFORM_4940_CMSO_LENOVO
	#ifdef DEBUG_BSP
	DBG("lenovo cmos\r\n");
	#endif

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	//浠呯敤浜庨绾挎祴璇曪紝鎶婂師鏉lash cs 鑴氭敼涓鸿緭鍏ユā寮?

	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = FLASH_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);
	
#endif
}

void BSP_DacVolSetting(uint16_t vol)
{

	uint16_t u16voltage;
	uint8_t u8receBuf[2],i;
	uint16_t u16tmp,u16setVal;
	i = 10;
	if(vol <1250)
	{		
		VMOD1_DISEN();
	}
	else
	{
		VMOD1_EN();
		u16setVal = (vol-1250)*512/495;
		do
		{
			
			HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R,u16setVal);//12位右对齐数据格式设置DAC值
			HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VOLTAGE_ADDR,1,u8receBuf,2,1000);	
			HAL_Delay(50);
			
			HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VOLTAGE_ADDR,1,u8receBuf,2,1000);	
			u16voltage = ((uint16_t)u8receBuf[1])|((uint16_t)u8receBuf[0]<<8);
			u16voltage = (uint32_t)u16voltage*125/100;	

			u16tmp = (u16voltage > vol) ? (u16voltage - vol):(vol - u16voltage);
			u16setVal = (u16voltage > vol) ? (u16setVal - u16tmp):(u16setVal + u16tmp);

			
		}while((u16tmp>=10)&&(--i));
	}

	VMOD1_DISEN();



}



/*
*   brief :  init voltage module U1
*	param 1 : VMOD1
*	param 2 : VMOD2
*	param 3 : BUf
*/
void BSP_VoltageInit(uint16_t mode1,uint8_t buf,uint8_t mode2)	
{
#ifdef DEBUG_BSP
	printf("vol init mod1 =%d buf = %d mod2 = %d spi = %d \n",mode1,g_u8Buf,g_u8Vmod2,g_u8SpiSped);
#endif

	uint8_t tmp = 0;

	RT9367_EN();  			/*RT9367 CHIP enable (active high) PE0*/
	
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_OFF,1,&mode2,1,1000);	// LDO1 off  0x20 ->
	HAL_Delay(1);
	
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_OFF,1,&buf,1,1000);    // LDO2 off
	HAL_Delay(1);	

	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,0x40,1,&tmp,1,1000);	// RT9367 LED1-4 off
	HAL_Delay(1);
#endif	

	BSP_DacVolSetting(mode1);		

}


EN_RESP_TYPE BSP_SpiSpeedSetting(uint8_t prescal)
{
	if((prescal < 1) || (prescal > 7))
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}
	switch(prescal)
	{
		case 1: 		
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
			break;
		case 2: 		
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
			break;
		case 3:
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
			break;
		case 4:
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
			break;
		case 5:
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
			break;
		case 6:
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
			break;
		case 7:
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
			break;
		default:
			//							send error code 
			hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
			break;
	}
	
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK)
	{
//		Error_Handler();
	}

	return EN_RESP_SUCCESS;
}

void BSP_VoltAndSpiSetting(uint16_t mode1,uint8_t buf,uint8_t mode2,uint8_t spiSpeed)	
{
#ifdef DEBUG_BSP
	printf("vol spi setting mod1 =%d buf = %d mod2 = %d spi = %d \n",mode1,buf,mode2,spiSpeed);
#endif

	g_u16Vmod1 = mode1;
	g_u8Buf = buf;
	g_u8Vmod2 = mode2;
	g_u8SpiSped = spiSpeed;
	
	VMOD1_EN();																/* power en */
	BSP_DacVolSetting(g_u16Vmod1);												/* DAC -> BUF */
	HAL_Delay(1);
	
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_ON,1,(u8* )(&g_u8Vmod2),1,1000);	/* LDO1 -> MOD2 */
	HAL_Delay(1);
	
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_ON,1,(u8* )(&g_u8Buf),1,1000);		/* LDO2 -> BUF */
	HAL_Delay(1);	
#endif	
	BSP_SpiSpeedSetting(spiSpeed);
	
	VMOD1_DISEN();
	
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_OFF,1,(u8* )(&g_u8Vmod2),1,1000);	
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_OFF,1,(u8* )(&g_u8Buf),1,1000);
#endif
	
}

void BSP_INA230Init(uint16_t mod1CaliVal,uint16_t mod2Calival)
{
#ifdef DEBUG_BSP
	printf("mod1 cali val = %d , mod2 cali val =%d \n",mod1CaliVal,mod2Calival);
#endif
	uint8_t data[2]={0x42,0x67};
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CONFIG_ADDR,1,data,2,1000);
	HAL_Delay(1);
	
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,PMOD2_DET_ADDR,CONFIG_ADDR,1,data,2,1000);
	HAL_Delay(1);
#endif

	data[0]= (uint8_t)(mod1CaliVal>>8);
	data[1]= (uint8_t)mod1CaliVal;
#ifdef DEBUG_BSP
	printf("write INA230 cali reg val:%d\r\n",MergeU16(data));
#endif
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CAL_ADDR,1,data,2,1000);
		
#if(PLATFORM_TYPE == PLATFORM_4940)
	data[0]= (uint8_t)(mod2Calival>>8);
	data[1]= (uint8_t)mod2Calival;
	HAL_I2C_Mem_Write(&hi2c1,PMOD2_DET_ADDR,CAL_ADDR,1,data,2,1000);
#endif
	
}

int BSP_ReadCurrent(uint8_t chanel)
{
	uint8_t u8receBuf[4];
	uint16_t u16regVal = 0;
	int current[10]={0};
	int temp = 0;
	int finalCur = 0;
	uint8_t i,j;
#if (PLATFORM_TYPE == PLATFORM_6500)
	uint8_t u8TmpBuf[2];
	short s16OrgCurRegVal;
#endif

	if(chanel == 1)
	{
	
	#if(PLATFORM_TYPE == PLATFORM_6500) 
		HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VSHUNT_ADDR,1,u8receBuf,2,1000);
	#else
		HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,CURRENT_ADDR,1,u8receBuf,2,1000);
	#endif
		for(i = 0;i < 10;i++)

		{
		#if(PLATFORM_TYPE == PLATFORM_6500) 
			HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VSHUNT_ADDR,1,u8receBuf,2,1000);
		#else
			HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,CURRENT_ADDR,1,u8receBuf,2,1000);
		#endif
			
			u16regVal =((uint16_t)u8receBuf[0]<<8)|((uint16_t)u8receBuf[1]);
			current[i] = (short)u16regVal*10;
		}
		

	}
	else if(chanel == 2)
	{
	
	#if(PLATFORM_TYPE == PLATFORM_6500)
		DBG("board 6500 have no channel 2 read current\r\n");
		return FALSE;
	#else
		HAL_I2C_Mem_Read(&hi2c1,PMOD2_DET_ADDR,CURRENT_ADDR,1,u8receBuf,2,1000);
		for(i = 0;i < 10; i++)
		{
			HAL_I2C_Mem_Read(&hi2c1,PMOD2_DET_ADDR,CURRENT_ADDR,1,u8receBuf,2,1000);
			u16regVal =((uint16_t)u8receBuf[1])| ((uint16_t)u8receBuf[0]<<8);
			current[i] = (uint32_t)u16regVal*10;
		}
	#endif	

	}
	/*Removing the Maximum and Minimum to Average*/
	for(i = 0;i < 10; i++)
	{
		for(j = 0;j < 10 - i; j++)
		{
			if(current[j]>current[j+1])
			{
				temp = current[j+1];
				current[j+1] = current[j];
				current[j] = temp;
			}
		}
	}
	for(i = 0;i < 10-2; i++)
	{
		finalCur += current[i+1];
	}
	finalCur /= 8;
	
#if(PLATFORM_TYPE == PLATFORM_6500)
	memcpy(u8TmpBuf, (uint8_t* )(ADDR_FLASH_SECTOR_8 + 64), 2);
	s16OrgCurRegVal = MergeU16(u8TmpBuf);
	#ifdef DEBUG_BSP
//	printf("before process finalCur =%d\r\n",finalCur);
//	printf("s16OrgCurRegVal = %d\r\n",s16OrgCurRegVal);
	#endif
	finalCur -= (s16OrgCurRegVal*10); /* current register LSB is 10uA */
#endif

#ifdef DEBUG_BSP
	printf("avg cur = %d \n",finalCur);
#endif
	return finalCur;

}

uint32_t BSP_ReadVoltage(uint8_t chanel)		// need confirm
{
	uint16_t u16voltage = 0;
	uint32_t u32result;
	uint8_t buf[2];
	
	if(chanel == 1)		// MOD1
	{
		HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VOLTAGE_ADDR,1,buf,2,1000);
		u16voltage = ((uint16_t)buf[1])|((uint16_t)buf[0]<<8);
		u32result = (uint32_t)u16voltage*125/100;
	
	#ifdef DEBUG_BSP
		printf("mod1 = %d\n",u32result);
	#endif
	}
	else if(chanel == 2)	// MOD2
	{
	#if(PLATFORM_TYPE == PLATFORM_6500)
		DBG("board 6500 have no channel 2 read current\r\n");
		return FALSE;
	#endif
	
		HAL_I2C_Mem_Read(&hi2c1,PMOD2_DET_ADDR,VOLTAGE_ADDR,1,buf,2,1000);
		u16voltage = ((uint16_t)buf[1])|((uint16_t)buf[0]<<8);
		u32result= (uint32_t)u16voltage*125/100;

		#ifdef DEBUG_BSP
		printf("mod2 = %d\n",u32result);
		#endif

	}
	return u32result;
}

uint16_t BSP_getAdcVal(ADC_HandleTypeDef* hadc,uint32_t ch)   
{
    ADC_ChannelConfTypeDef ADC_ChanConf;
    
    ADC_ChanConf.Channel=ch;                                   //閫氶亾
    ADC_ChanConf.Rank=1;                                       //绗?1涓簭鍒楋紝搴忓垪1
    ADC_ChanConf.SamplingTime=ADC_SAMPLETIME_480CYCLES;        //閲囨牱鏃堕棿
    ADC_ChanConf.Offset=0;                 
    HAL_ADC_ConfigChannel(hadc,&ADC_ChanConf);        //閫氶亾閰嶇疆
	
    HAL_ADC_Start(hadc);                               //寮?鍚疉DC
	
    HAL_ADC_PollForConversion(hadc,10);                //杞杞崲
 
	return (uint16_t)HAL_ADC_GetValue(hadc);	        //杩斿洖鏈?杩戜竴娆DC1瑙勫垯缁勭殑杞崲缁撴灉
}


uint8_t gu8_whlite_led_light=0;
uint8_t gu8_red_led_light=0;
extern TIM_HandleTypeDef htim4;

// 1 white  2：red  led_light 0/0xff  off 
int pwm_led_set(uint8_t led_num,uint8_t led_light)
{		
	TIM_OC_InitTypeDef sConfigOC = {0};
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	if((led_light == 0xff)||(led_light == 0))//0%需要关pwm
	{
		if(led_num == 1)//led1 
		{
			HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_1);

			GPIO_InitStruct.Pin = GPIO_PIN_12;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_RESET);
			gu8_whlite_led_light=0;			
		}
		else			//led2
		{
			HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_2);
			
			GPIO_InitStruct.Pin = GPIO_PIN_13;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13,GPIO_PIN_RESET);
			gu8_red_led_light=0;
		}
		return 0;
	}
	
	if(led_light == 100)//100%需要关pwm
	{
		if(led_num == 1)//led1 
		{
			HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_1);
			
			GPIO_InitStruct.Pin = GPIO_PIN_12;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_SET);	
		}
		else			//led2
		{
			HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_2);
			
			GPIO_InitStruct.Pin = GPIO_PIN_13;
			GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
			HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_13,GPIO_PIN_SET);
		}
		return 0;
	}

	HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_2);
	
	if(led_num == 1)//led1 
	{
		gu8_whlite_led_light=led_light;
	}
	else
	{
		gu8_red_led_light=led_light;
	}
	
	
	
	if(gu8_whlite_led_light)//led1 
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
		/**TIM4 GPIO Configuration    
		PD12     ------> TIM4_CH1
		PD13     ------> TIM4_CH2 
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_12;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

		/*
			TIM_OCPOLARITY_HIGH 时不需要100-led_light,但是会出现不能100%，即不能全亮
			TIM_OCPOLARITY_LOW 需要100-led_light,但是会出现不能0%，即不能全暗，可以通过已有的关函数实现
		*/
		sConfigOC.OCMode = TIM_OCMODE_PWM1;
 		sConfigOC.Pulse = htim4.Init.Period*(gu8_whlite_led_light)/100;
 		sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
		{
		    Error_Handler();
		}
	}
		
	if(gu8_red_led_light)//led1 
	{
		__HAL_RCC_GPIOD_CLK_ENABLE();
		/**TIM4 GPIO Configuration    
		PD12     ------> TIM4_CH1
		PD13     ------> TIM4_CH2 
		*/
		GPIO_InitStruct.Pin = GPIO_PIN_13;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_PULLDOWN;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		
		
		sConfigOC.OCMode = TIM_OCMODE_PWM1;
 		sConfigOC.Pulse = htim4.Init.Period*(gu8_red_led_light)/100;
 		sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
		sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
		if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
		{
		    Error_Handler();
		}
	}
	
  
	if(gu8_whlite_led_light)//led1 
	{
		HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
	}
	
	if(gu8_red_led_light)//led1 
	{
		HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_2);
	}
	
	return 0;
}


#define LED_White_Pin GPIO_PIN_12
#define LED_White_GPIO_Port GPIOD
#define LED_Red_Pin GPIO_PIN_13
#define LED_Red_GPIO_Port GPIOD


EN_RESP_TYPE BSP_SetBacklight(uint8_t curLed,uint8_t PWMled1,uint8_t PWMled2)
{
	uint8_t data;

#if(PLATFORM_TYPE == PLATFORM_4940)	
	if((curLed>0x1f) && (curLed<0xff))
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}
	
	if(curLed == 0xff)
	{
		data = 0;
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,0x40,1,&data,1,1000);			/* 4 LED ALL OFF */
	}
	else
	{
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,0x4f,1,&curLed,1,1000);		/* 4 LED ALL ON */
	}
#endif

	if(PWMled2==0xff)
	{
		PWMled2=0;
	}
	
	if(PWMled1==0xff)
	{
		PWMled1=0;
	}
	
	if((PWMled1>100)||(PWMled2>100))
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}
	
	pwm_led_set(1,PWMled1);
	pwm_led_set(2,PWMled2);
	
	return EN_RESP_SUCCESS;
}

short BSP_VMOD1Calibration(uint32_t *databuf)
{
	float R_Shunt;
	uint8_t data_buf[2];
	short cali_value,tmp,tmp1 = 1 ,s16RegCurVal;
	

	VMOD1_EN();
	s16RegCurVal = 0;
#if(PLATFORM_TYPE == PLATFORM_6500)
	uint16_t u16CalRegVal;	
	uint8_t u8RegBuf[2];
	short  s16RegShuntVal;
	float fCurrentLSB, fRegCurrentVal, fCalCurrentVal, fShuntVol;

	fCurrentLSB = ((float)I_LIMIT)/MAX_VAL_INA230;
	cali_value = 5120000/(fCurrentLSB*R_SHUNT);
	printf("fCurrentLSB = %f, cali_vaule = %d \r\n",fCurrentLSB, cali_value);

	/* init INA230 */
	u8RegBuf[0] = 0x44;
	u8RegBuf[1] = 0xDF;
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CONFIG_ADDR,1,u8RegBuf,2,1000);
	SplitU16(u8RegBuf, cali_value);
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CAL_ADDR,1,u8RegBuf,2,1000);
	u8RegBuf[0] = 0x00;
	u8RegBuf[1] = 0x00;
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,MASK_ENABLE_ADDR,1,u8RegBuf,2,1000);
	u8RegBuf[0] = 0x00;
	u8RegBuf[1] = 0x00;
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,ALERT_ADDR,1,u8RegBuf,2,1000);
	HAL_Delay(50);
	*databuf = (uint32_t)cali_value;
	
	/* read current from INA230 current register */
	HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,CURRENT_ADDR,1,u8RegBuf,2,1000);
	s16RegCurVal = MergeU16(u8RegBuf);
	fRegCurrentVal = ((double)s16RegCurVal)/100; /* unit: mA */
	printf("s16RegCurVal=%d, tmp1=%d, tmp1 - s16RegCurVal=%d\r\n",s16RegCurVal,tmp1, (tmp1-s16RegCurVal));
	/*
	 * calculate current by use shunt voltage register;
	 * the Calculation formula: (ShuntVoltage * CalibrationRegister)*10/2048 --- unit: uA,because 
	 * the read shunt register valur unit is uV.
	 */
	HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VSHUNT_ADDR,1,u8RegBuf,2,1000);
	s16RegShuntVal = MergeU16(u8RegBuf);
	printf("s16RegShuntVal = %d\r\n",s16RegShuntVal);
	HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,CAL_ADDR,1,u8RegBuf,2,1000);
	u16CalRegVal = MergeU16(u8RegBuf);
	printf("read cali reg val:%d\r\n",u16CalRegVal);
	fCalCurrentVal = (((double)s16RegShuntVal * u16CalRegVal)/ 2048)*10;/*unit: uA*/
	fCalCurrentVal /=1000; /*unit mA*/

	printf("reg cur=%f mA, cal cur = %f mA\r\n",fRegCurrentVal, fCalCurrentVal);
	

	CALI_PIN_EN();
#endif

	HAL_Delay(50);
	HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,VSHUNT_ADDR,1,data_buf,2,1000);
	cali_value = ((uint16_t)data_buf[1])|((uint16_t)data_buf[0]<<8);
	R_Shunt = (double)cali_value* (2.5)/(33000);
#ifdef DEBUG_BSP
	printf("mod1 cali %d , %d \n",data_buf[0],data_buf[1]);
	printf("mod1 cali cac =  %d \n",cali_value);
	printf("r_shunt =  %.4f \n",R_Shunt);
#endif
	tmp = (uint16_t)(512000/(1000*R_Shunt));
	data_buf[0] = (uint8_t)(tmp>>8);
	data_buf[1] = (uint8_t)tmp;
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CAL_ADDR,1,data_buf,2,1000);
	HAL_Delay(50);
	HAL_I2C_Mem_Read(&hi2c1,PMOD1_DET_ADDR,CURRENT_ADDR,1,data_buf,2,1000);
	tmp1 = ((uint16_t)data_buf[1])|((uint16_t)data_buf[0]<<8);
	cali_value = tmp*3300/tmp1;
	#ifdef DEBUG_BSP
	//	printf("mod1 cali %d , %d \n",data_buf[0],data_buf[1]);
		printf("mod1 cali cac =  %d \n",cali_value);
	#endif
	*databuf = (uint32_t)cali_value;
	
	data_buf[0]= (uint8_t)(cali_value>>8);
	data_buf[1]= (uint8_t)cali_value;
	HAL_I2C_Mem_Write(&hi2c1,PMOD1_DET_ADDR,CAL_ADDR,1,data_buf,2,1000);

#if(PLATFORM_TYPE == PLATFORM_6500)
	CALI_PIN_DIS();
#endif	

	VMOD1_DISEN();

	return s16RegCurVal;

}

void BSP_VMOD2Calibration(uint32_t *databuf)
{
#if(PLATFORM_TYPE == PLATFORM_6500)
	DBG("there is no Vmod2 in board 6500\r\n");
	return;
#endif

	float R_Shunt;
	uint8_t data_buf[2],mod2;
	uint16_t cali_value,tmp,tmp1;
	mod2 = 0x1f;
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_ON,1,&mod2,1,1000);	
	HAL_Delay(50);
	HAL_I2C_Mem_Read(&hi2c1,PMOD2_DET_ADDR,VSHUNT_ADDR,1,data_buf,2,1000);
	cali_value = ((uint16_t)data_buf[1])|((uint16_t)data_buf[0]<<8);
	R_Shunt = (double)cali_value* 2.5/30000;
	#ifdef DEBUG_BSP
	printf("r_shunt2 =  %.4f \n",R_Shunt);
	#endif
	tmp = (uint16_t)(512000/(1000*R_Shunt));
	#ifdef DEBUG_BSP
	printf("tmp = %d \n",tmp);
	#endif
	data_buf[0] = (uint8_t)(tmp>>8);
	data_buf[1] = (uint8_t)tmp;
	HAL_I2C_Mem_Write(&hi2c1,PMOD2_DET_ADDR,CAL_ADDR,1,data_buf,2,1000);
	HAL_Delay(50);
	HAL_I2C_Mem_Read(&hi2c1,PMOD2_DET_ADDR,CURRENT_ADDR,1,data_buf,2,1000);
	tmp1 = ((uint16_t)data_buf[1])|((uint16_t)data_buf[0]<<8);

	cali_value = tmp*3000/tmp1;
	#ifdef DEBUG_BSP
	//	printf("mod2 cali %d , %d \n",data_buf[0],data_buf[1]);
		printf("mod2 cali cac =  %d \n",cali_value);
	#endif	

	*databuf = (uint32_t)cali_value;
	data_buf[0]= (uint8_t)(cali_value>>8);
	data_buf[1]= (uint8_t)cali_value;
	HAL_I2C_Mem_Write(&hi2c1,PMOD2_DET_ADDR,CAL_ADDR,1,data_buf,2,1000);
	
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_OFF,1,&mod2,1,1000);	
}

void BSP_MOD1MOD2Calibration()
{
	uint32_t u32dataBuf[2];
	uint8_t u8mod2=0x1f;
	uint16_t u16mod1 = 3300;
#if (PLATFORM_TYPE == PLATFORM_6500)
	uint8_t u8TmpBuf[2];
	short s16OrgCurRegVal;
#endif
	
	BSP_DacVolSetting(u16mod1);
	VMOD1_EN();
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_ON,1,&u8mod2,1,1000);	//ldo2 on  // buf on  voltage=3.3V
	HAL_Delay(120);
#endif
	IwdgFeed();
#if (PLATFORM_TYPE == PLATFORM_6500)
	s16OrgCurRegVal = BSP_VMOD1Calibration(&u32dataBuf[0]);
#else	
	BSP_VMOD1Calibration(&u32dataBuf[0]);
#endif
	BSP_VMOD2Calibration(&u32dataBuf[1]);
#ifdef DEBUG_BSP
	printf("cali data mod1 = %d , mod2 = %d \n",u32dataBuf[0],u32dataBuf[1]);
#endif
	Dev_STFlash_Sector8WriteWord(CURRENT_CALI_DATA_OFFSET,u32dataBuf,2,FLASH_SECTOR8_EFFECT_DATA_LENGTH);
#if (PLATFORM_TYPE == PLATFORM_6500)
	SplitU16(u8TmpBuf, s16OrgCurRegVal);
	Dev_STFlash_programData(u8TmpBuf, 2, ADDR_FLASH_SECTOR_8+64);
#endif


	BSP_DacVolSetting(g_u16Vmod1);
#if(PLATFORM_TYPE == PLATFORM_4940)
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_OFF,1,(u8* )&g_u8Vmod2,1,1000);	
	HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_OFF,1,(u8* )(&g_u8Buf),1,1000);
#endif
}

EN_RESP_TYPE BSP_PowerControl(uint8_t control,uint16_t delay)
{
	IwdgFeed();
	if(control == 0x80)   // POWER ON
	{
	#ifdef DEBUG_BSP
		printf("power on mod1 =%d buf = %d mod2 = %d spi = %d \n",g_u16Vmod1,g_u8Buf,g_u8Vmod2,g_u8SpiSped);
	#endif
		VMOD1_EN();
	#if(PLATFORM_TYPE == PLATFORM_4940)
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_ON,1,(u8* )(&g_u8Vmod2),1,1000);	//ldo2 on  // buf on  voltage=3.3V
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_ON,1,(u8* )(&g_u8Buf),1,1000);
	#endif
		HAL_Delay(delay);
	}
	else if(control == 0x08)   // POWER OFF
	{
	#if 0
		VMOD1_DISEN();
	#if(PLATFORM_TYPE == PLATFORM_4940)
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO1_OFF,1,(u8* )(&g_u8Vmod2),1,1000);	
		HAL_I2C_Mem_Write(&hi2c1,RT9367_ADDR,RT9367_LDO2_OFF,1,(u8* )(&g_u8Buf),1,1000);
	#endif
	#if(PLATFORM_TYPE == PLATFORM_6500)
		HAL_GPIO_WritePin(GPIOA, DIR_BUF1_Pin, GPIO_PIN_RESET);
	#endif
		HAL_Delay(delay);
	#endif
	}
	else
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}
	return EN_RESP_SUCCESS;
}

void BSP_Sector8Prasing(void)
{
	uint32_t u32sec8Buf[FLASH_SECTOR8_EFFECT_DATA_LENGTH];
	
	IwdgFeed();
	Dev_STFlash_Sector8ReadWord(0,u32sec8Buf,FLASH_SECTOR8_EFFECT_DATA_LENGTH,FLASH_SECTOR8_EFFECT_DATA_LENGTH);

	g_u16Mod1CaliVal = (uint16_t)u32sec8Buf[1];
	g_u16Mod2CaliVal = (uint16_t)u32sec8Buf[2];
	if(u32sec8Buf[3] == 0)
	{
		g_u16Vmod1 = (uint16_t)u32sec8Buf[4];
		g_u8Buf = (uint8_t)u32sec8Buf[5];
		g_u8Vmod2 = (uint8_t)u32sec8Buf[6];
		g_u8SpiSped = (uint8_t)u32sec8Buf[7];

	}

}

int BSP_CTRLResetPin(uint8_t staus)
{
	if(staus)
	{
		HAL_GPIO_WritePin(RSTN_GPIO_Port, RSTN_Pin, GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(RSTN_GPIO_Port, RSTN_Pin, GPIO_PIN_RESET);
	}

	return 0;
}

int Dev_SPI_SendData(SPI_HandleTypeDef *hspi , uint8_t * SendBuf , uint32_t size , uint32_t timeout)
{
	int retVal;
	GPIO_TypeDef * SPIx_CS_GPIOx;
	uint16_t SPIx_Pin;
	uint16_t i;
	if(hspi == &hspi1)
	{
		SPIx_CS_GPIOx = CS0_N_SPI_GPIO_Port;
		SPIx_Pin = CS0_N_SPI_Pin;
	}
	else if(hspi == &hspi3)
	{
		SPIx_CS_GPIOx = GPIOA;
		SPIx_Pin = GPIO_PIN_15;
	}

	__HAL_SPI_ENABLE(hspi);	// the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	HAL_GPIO_WritePin(SPIx_CS_GPIOx,SPIx_Pin,GPIO_PIN_RESET);
	for(i=0;i<size;i++)
	{
		IwdgFeed();
		retVal = HAL_SPI_Transmit(hspi,&SendBuf[i],1,timeout);
	}
	HAL_GPIO_WritePin(SPIx_CS_GPIOx,SPIx_Pin,GPIO_PIN_SET);
	return retVal;
	
}
int Dev_SPI_ReceiveData(SPI_HandleTypeDef *hspi , uint8_t * receBuf , uint32_t size , uint32_t timeout)
{
	int retVal;
	GPIO_TypeDef * SPIx_CS_GPIOx;
	uint16_t SPIx_Pin;
//	uint16_t i;
	if(hspi == &hspi1)
	{
		SPIx_CS_GPIOx = CS0_N_SPI_GPIO_Port;
		SPIx_Pin = CS0_N_SPI_Pin;
	}
	else if(hspi == &hspi3)
	{
		SPIx_CS_GPIOx = GPIOA;
		SPIx_Pin = GPIO_PIN_15;
	}
	
	IwdgFeed();
	memset(receBuf,0xff,size);
	__HAL_SPI_ENABLE(hspi); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	HAL_GPIO_WritePin(SPIx_CS_GPIOx,SPIx_Pin,GPIO_PIN_RESET);
	retVal = HAL_SPI_Receive(hspi,receBuf,size,timeout);
	HAL_GPIO_WritePin(SPIx_CS_GPIOx,SPIx_Pin,GPIO_PIN_SET);
	return retVal;
	
}

int Dev_SPI_ReceiveDataDMA(SPI_HandleTypeDef *hspi , uint8_t * receBuf , uint32_t size)
{
	int retVal;
	uint32_t i,packCnt,bufOfst;
	uint16_t lastPackSize;
	packCnt = size / (64*1024-1);
	lastPackSize = size % (64*1024-1);
	bufOfst = 0;
	for(i = 0;i <= packCnt;i++)
	{
		g_u8SpiRXCpltFlag = 0;
		if(i == packCnt)
		{
			memset(receBuf+bufOfst,0xff,lastPackSize);
			__HAL_SPI_ENABLE(&hspi1); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
			retVal = HAL_SPI_Receive_DMA(&hspi1,receBuf+bufOfst,lastPackSize);
			while(g_u8SpiRXCpltFlag != 1);

			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
		}
		else
		{
			memset(receBuf+bufOfst,0xff,(64*1024-1));
			__HAL_SPI_ENABLE(&hspi1); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
			retVal = HAL_SPI_Receive_DMA(&hspi1,receBuf+bufOfst,(64*1024-1));
			while(g_u8SpiRXCpltFlag != 1);
			bufOfst += 64*1024-1;
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
		}
	}
	return retVal;
}

int Dev_SPI_SendDataContinus(SPI_HandleTypeDef *hspi , uint8_t * sendBuf , uint32_t size)
{
	int retVal;
	__HAL_SPI_ENABLE(&hspi1); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_RESET);
	retVal = HAL_SPI_Transmit(hspi,sendBuf,size,1000);

	HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,GPIO_PIN_SET);
	return retVal;
	
}


void Dev_Flash_SPISendData(uint8_t * SendBuf , uint32_t size , uint32_t timeout)
{
	uint16_t i;
	__HAL_SPI_ENABLE(&hspi1);	// the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	//FLASH_SPI_CS_EN();
	for(i=0;i<1;i++)
	{
		HAL_SPI_Transmit(&hspi1,&SendBuf[i],size,timeout);
	}
	//FLASH_SPI_CS_DIS();

	
}
void Dev_Flash_SPIReceiveData(uint8_t * receBuf , uint32_t size , uint32_t timeout)
{

	memset(receBuf,0xff,size);
	__HAL_SPI_ENABLE(&hspi1); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	//FLASH_SPI_CS_EN();
	HAL_SPI_Receive(&hspi1,receBuf,size,timeout);
	//FLASH_SPI_CS_DIS();

	
}

void Dev_Flash_SPIReceiveDataDMA(uint8_t * receBuf , uint32_t size)
{

	g_u8SpiRXCpltFlag = 0;

	
	memset(receBuf,0xff,size);
	__HAL_SPI_ENABLE(&hspi1); // the spi_clk defalut level is hihg when set to AF_PP, need to enable spi make it change to low 

	//FLASH_SPI_CS_EN();
	HAL_SPI_Receive_DMA(&hspi1,receBuf,size);
	while(g_u8SpiRXCpltFlag != 1);

	//FLASH_SPI_CS_DIS();

	
}




