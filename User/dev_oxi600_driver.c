
#include "dev_oxi600_driver.h"
#include "stm32f4xx_hal.h"
#include "lib_oxi600.h"
#include "main.h"
#if 1

void delay_ms(uint32_t n)
{
	if( n!= 0)
	HAL_Delay(n-1);
}

uint32_t Dev_Oxi600_GetLocalTime(void)
{
	HAL_GetTick();
}


/**
 * @brief Write values to specified registers
 * @param RegAddr which register address will be wirte
 * @param regData register value
 * @retval none
 */
void Dev_Oxi600_WriteReg(uint8_t regAddr,uint8_t regData)
{
	uint8_t u8regBuf[2]={0xff,0xff};
	u8regBuf[0] = regAddr;
	u8regBuf[1] = regData;
	
	Dev_SPI_SendData(&hspi1, u8regBuf,2,100);
}



/**
 * @brief Read values in specified registers
 * @param RegAddr which register address will be read
 * @retval Register value
 */
uint16_t Dev_Oxi600_ReadReg(uint8_t RegAddr)
{
	uint16_t u16RetVal ;	
	uint8_t u8RegReceBUf[2];
	RegAddr &= 0x7F;
	Dev_Oxi600_WriteReg(RegAddr,0xff);
	Dev_SPI_ReceiveData(&hspi1, u8RegReceBUf,2,100);
	u16RetVal = ((uint16_t)u8RegReceBUf[0])<<8 | u8RegReceBUf[1];
	return u16RetVal;
}


void Dev_Oxi600_SPI_ReceiveContinuous(uint8_t *dataBuf,uint32_t dataLen)
{
 	Dev_SPI_ReceiveDataDMA(&hspi1,dataBuf,dataLen);
}

void Dev_Oxi600_SPI_SendContinuous(uint8_t *dataBuf,uint32_t dataLen)
{
 	Dev_SPI_SendDataContinus(&hspi1,dataBuf,dataLen);
}

void Dev_Cmos_Reset(u16 KpLowTim)
{
	HAL_GPIO_WritePin(RSTN_GPIO_Port, RSTN_Pin, GPIO_PIN_RESET);
///	delay_ms(KpLowTim);
	delay_ms(10);
	HAL_GPIO_WritePin(RSTN_GPIO_Port, RSTN_Pin, GPIO_PIN_SET);
}
#endif
