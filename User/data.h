#ifndef __DATA_H_
#define __DATA_H_
#include <stdint.h>

#include "type.h"

extern uint8_t* g_pu8DataBuf;

uint16_t CRC16(uint8_t* Buf, uint32_t BufLen, uint16_t u16CRC);
uint16_t Checksum16(uint8_t* Buf, uint32_t Length);
uint16_t MergeU16(uint8_t* SrcBuf);
uint32_t MergeU32(uint8_t* SrcBuf);
void SplitU16(uint8_t* DstBuf, uint16_t Value);
void SplitU32(uint8_t* DstBuf, uint32_t Value);
uint32_t getAlignTimes(uint32_t dividend, uint32_t Divisor);
extern bool mallocBuf(uint32_t size);

#endif
