#include <stdio.h>
#include <string.h>
#include "dev_stflash.h"

#include "data.h"
#include "heap_5.h"

//#define DEBUG_FLAHS

#define BACKUP_SECTOR			FLASH_SECTOR_11	
#define BACKUP_SECTOR_ADDR		ADDR_FLASH_SECTOR_11

static void sDev_STFlash_Unlock(void)
{
	 HAL_FLASH_Unlock();    
	 __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |  \
	 						FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

static void sDev_IntlFlash_Lock(void)
{
	HAL_FLASH_Lock();
}

static uint32_t sDev_STFlash_getSector(uint32_t flashAddress)
{
	uint32_t u32sector = 0;
	
	if((flashAddress < ADDR_FLASH_SECTOR_1) && (flashAddress >= ADDR_FLASH_SECTOR_0))  
	{    
		u32sector = FLASH_SECTOR_0;    
	}  
	else if((flashAddress < ADDR_FLASH_SECTOR_2) && (flashAddress >= ADDR_FLASH_SECTOR_1))
	{    
		u32sector = FLASH_SECTOR_1;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_3) && (flashAddress >= ADDR_FLASH_SECTOR_2))
	{    
		u32sector = FLASH_SECTOR_2;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_4) && (flashAddress >= ADDR_FLASH_SECTOR_3))
	{    
		u32sector = FLASH_SECTOR_3;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_5) && (flashAddress >= ADDR_FLASH_SECTOR_4))
	{    
		u32sector = FLASH_SECTOR_4;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_6) && (flashAddress >= ADDR_FLASH_SECTOR_5))
	{    
		u32sector = FLASH_SECTOR_5;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_7) && (flashAddress >= ADDR_FLASH_SECTOR_6))
	{    
		u32sector = FLASH_SECTOR_6;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_8) && (flashAddress >= ADDR_FLASH_SECTOR_7))
	{    
		u32sector = FLASH_SECTOR_7;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_9) && (flashAddress >= ADDR_FLASH_SECTOR_8))
	{    
		u32sector = FLASH_SECTOR_8;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_10) && (flashAddress >= ADDR_FLASH_SECTOR_9))
	{    
		u32sector = FLASH_SECTOR_9;    
	}
	else if((flashAddress < ADDR_FLASH_SECTOR_11) && (flashAddress >= ADDR_FLASH_SECTOR_10))
	{    
		u32sector = FLASH_SECTOR_10;    
	}
	else
	{
		DBG("invalide flash address\r\n");
		u32sector = 0xFFFFFFFF;
	}
		
	return u32sector;
}

bool Dev_STFlash_eraseSector(uint32_t flashAddress, uint32_t dataLength)
{
	FLASH_EraseInitTypeDef stEraseInit;
	HAL_StatusTypeDef enRslt;
	uint32_t u32startSector,u32endSector, u32sectorError;

	u32startSector = sDev_STFlash_getSector(flashAddress);
	u32endSector = sDev_STFlash_getSector(flashAddress+dataLength-1);
	if(u32startSector == 0xFFFFFFFF)
	{
		DBG("flash address:%08X invalide\r\n",flashAddress);
		return FALSE;
	}
	sDev_STFlash_Unlock();
	stEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
	stEraseInit.Sector = u32startSector;
	stEraseInit.NbSectors = u32endSector-u32startSector+1;
	stEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	enRslt  = HAL_FLASHEx_Erase(&stEraseInit, &u32sectorError);	
	FLASH_WaitForLastOperation(5000);
	sDev_IntlFlash_Lock();
	if(enRslt != HAL_OK)
	{
		DBG("internal flash error, enRslt:%02X,u32sectorError:%08X\r\n",enRslt,u32sectorError);
		return FALSE;
	}
	return TRUE;
}


bool Dev_STFlas_programHalfWord(uint32_t flashAddr, uint16_t data)
{
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, flashAddr, data);
	HAL_FLASH_Lock();
	
	return TRUE;
}

bool Dev_STFlas_programWord(uint32_t flashAddr, uint32_t data)
{
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flashAddr, data);
	HAL_FLASH_Lock();
	
	return TRUE;
}

void Dev_STFlash_programData(uint8_t* dataBuf, uint32_t u32dataSize, uint32_t u32flashAddr)
{	
	uint32_t i;

	//HAL_FLASH_Unlock();	// first byte cannot write to flash,sunhua find out
	sDev_STFlash_Unlock();

	for(i=0;i < u32dataSize; i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, u32flashAddr+i, dataBuf[i]);
	}
	
	HAL_FLASH_Lock();
}

bool Dev_STFlash_setAppInfo(u32 flashAddr, u32 appInfo)
{
	uint8_t* pu8tmpBuf = NULL;
	uint32_t u32remainSize;

	pu8tmpBuf = malloc(16*1024);
	if(pu8tmpBuf == NULL)
	{
		DBG("Dev_STFlash_setAppInfo malloc 16K bytes failed, current free ram:%d bytes\r\n", \
																		xPortGetFreeHeapSize());
		return FALSE;
	}

	memcpy(pu8tmpBuf, (uint8_t* )(flashAddr&0xFFFFF000), 16*1024);
	Dev_STFlash_eraseSector(APP_INFO_ADDR, 16*1024);
	if((flashAddr - APP_INFO_ADDR) >0)
	{
		Dev_STFlash_programData(pu8tmpBuf,(flashAddr - APP_INFO_ADDR), APP_INFO_ADDR);
	}

	Dev_STFlas_programWord(flashAddr, appInfo);
	u32remainSize = 16*1024 - (flashAddr - APP_INFO_ADDR) - 4;
	Dev_STFlash_programData(&pu8tmpBuf[16*1024-u32remainSize], u32remainSize, flashAddr+4);
	
	

	return TRUE;
}

uint32_t Dev_STFlash_readWord(uint32_t addr)
{
	return *(uint32_t *)addr;
}

uint8_t Dev_STFlash_readByte(uint32_t addr)
{
	return *(uint8_t*)addr;
}

void Dev_STFlash_Sector8WriteWord(uint32_t addrOffset,uint32_t *data,uint16_t size,uint16_t effDataLen)
{
	uint32_t *preadBuf = NULL;
	uint16_t i;
	preadBuf = malloc(effDataLen);

	for(i = 0;i < effDataLen; i++)
	{
		preadBuf[i] = Dev_STFlash_readWord(ADDR_FLASH_SECTOR_8+i*4);
	}

	memcpy(&preadBuf[addrOffset],data,size*sizeof(int));
	Dev_STFlash_eraseSector(ADDR_FLASH_SECTOR_8,128*1024);


	for(i = 0;i < effDataLen ;i++)
	{
		Dev_STFlas_programWord(ADDR_FLASH_SECTOR_8+i*4,preadBuf[i]);
	}
#ifdef DEBUG_FLAHS
	printf("after write \n");
	for(i = 0;i < effDataLen; i++)
	{
		tmp[i] = Dev_STFlash_readWord(ADDR_FLASH_SECTOR_8+i*4);
		printf("%u ",tmp[i]);
	}
	printf("\n");
#endif	
	free(preadBuf);
	
}
void Dev_STFlash_Sector8ReadWord(uint32_t addrOffset,uint32_t *data,uint16_t size,uint16_t effDataLen)
{
	uint16_t i;
	for(i = 0;i < size; i++)
	{
		data[i] = Dev_STFlash_readWord(ADDR_FLASH_SECTOR_8+addrOffset+i*4);
	}
#ifdef DEBUG_FLAHS
	uint32_t tmp[8];
	printf("read sector 8 \n");
	for(i = 0;i < effDataLen; i++)
	{
		tmp[i] = Dev_STFlash_readWord(ADDR_FLASH_SECTOR_8+i*4);
		printf("%u ",tmp[i]);
	}
	printf("\n");
#endif		
}


