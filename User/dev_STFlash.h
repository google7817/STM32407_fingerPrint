#ifndef __DEV_INTLFLASH_H
#define __DEV_INTLFLASH_H

#include "type.h"
#include "stm32f4xx_hal.h"

#include "protocol.h"

#define APP_INFO_ADDR						(ADDR_FLASH_SECTOR_3)
#define FW_LENGTH_ADDR						(ADDR_FLASH_SECTOR_3+4)
#define FW_CRC_ADDR							(ADDR_FLASH_SECTOR_3+8)
#define APP_START_ADDR						ADDR_FLASH_SECTOR_4
#define APP_BACKUP_ADDR						ADDR_FLASH_SECTOR_6



#define INFO_APP_NEW						0x01 //0x0600ABCD
#define INFO_APP_OLD						0x0600DCBA


/*MCU internal flash map*/
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbyte */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbyte */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbyte */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbyte */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbyte */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbyte */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbyte */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbyte */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbyte */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbyte */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbyte */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbyte */

#define FLASH_SECTOR8_EFFECT_DATA_LENGTH  		8
#define CURRENT_CALI_DATA_OFFSET				1
#define VOL_SPI_SET_VAL_OFFSET					3

u32 Dev_IntlFlash_getSectorSize(u32 flashSector);
bool Dev_STFlash_eraseSector(u32 flashAddress, u32 dataLength);
bool Dev_STFlas_programHalfWord(u32 flashAddr, u16 data);
bool Dev_STFlas_programWord(u32 flashAddr, u32 data);
void Dev_STFlash_programData(u8* dataBuf, u32 u32dataSize, u32 u32flashAddr);
bool Dev_STFlash_setAppInfo(u32 flashAddr, u32 appInfo);
void Dev_STFlash_Sector8WriteWord(uint32_t addrOffset,uint32_t *data,uint16_t size,uint16_t effDataLen);
void Dev_STFlash_Sector8ReadWord(uint32_t addrOffset,uint32_t *data,uint16_t size,uint16_t effDataLen);
#endif /* __DEV_INTLFLASH_H */

