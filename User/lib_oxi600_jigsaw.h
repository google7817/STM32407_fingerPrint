#ifndef __LIB_OXI600_JIGSAW_H
#define __LIB_OXI600_JIGSAW_H
#include "type.h"
#include "lib_oxi600.h"

typedef struct _st_pic_size
{
	uint16_t		big_col;
	uint16_t		big_row;

	uint16_t		big_pic_per_lines;// _gain_col;
	uint16_t		gain_row;

	uint16_t		small_col;
	uint16_t		small_row;
} st_pic_size;

/*
* window image jigsaw
* 
*/
void Dev_Oxi600_WinJigsaw(EN_CHL600_PRJ_TYPE prjType ,uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen );

/*
*whole image jigsaw
*/
void Dev_Oxi600_WholeJigsaw(EN_CHL600_PRJ_TYPE prjType,uint8_t *picBuf,uint32_t *picLen);



#endif
