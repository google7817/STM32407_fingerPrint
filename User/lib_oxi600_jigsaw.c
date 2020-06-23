#include "lib_oxi600_jigsaw.h"
#include "lib_oxi600.h"

typedef enum _EN_CHL600_SENSOR_TYPE
{
	EN_OXI600_MK720_80UM= 0, 
	EN_OXI600_MK720_100UM, 
	EN_OXI600_MK810_80UM,
	EN_OXI600_MK320_100UM,

}EN_CHL600_SENSOR_TYPE;


st_pic_size  pic_size[] =
{
	/*whl_col row 				 sml_col	row	 */
	{ 439,	292,	0,		0,		148,	138, },		//MK720_80u(600c)
	{ 369,	228,	0,		0,		148,	128, },		//MK720_100u(600c)
	{ 510,	250,	0,		0,		148,	138, },		//MK810_80u(600c)
	{ 492,	250,	0,		0,		148,	128, },		//MK320_100u(600c)
};
st_pic_size  original_size[] =
{
	{ 439,	304,	74,		0,		184,	138, },		//MK720_80u(600c)
	{ 369,	304,	74,		0,		173,	128, },		//MK720_100u(600c)
	{ 510,	250,	64,		0,		148,	138, },		//MK810_80u(600c)
	{ 493,	250,	66,		0,		149,	128, },		//MK320_100u(600c)

};	


uint8_t tmpBuf[1200];		/*for backup*/

void Dev_Oxi600_MK720_80umWinJigsaw(uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,offset,widthWin,heightWin,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));
	if(gateBuf != NULL)
	{

		widthAA = pic_size[EN_OXI600_MK720_80UM].small_col - 10;
		widthOrign = original_size[EN_OXI600_MK720_80UM].small_col;
		if(startX <= 21)
		{
			for(i = 0;i < 10;i++)
			{
				memcpy(gateBuf+widthOrign*2*i,gateBuf+600*2*i+75*2,(9+5+22+widthAA)*2);	/*left float+dark+specif + aa*/
				memcpy(gateBuf+widthOrign*2*i+(9+5+22+widthAA)*2,gateBuf+600*2*i+504*2,10*2);  /*right dark dark piexl*/
			}
		}
		else
		{
			for(i = 0;i < 10;i++)
			{
				memcpy(gateBuf+widthOrign*2*i,gateBuf+600*2*i+75*2,(9+5+22)*2);/*left float+dark+specif */
				memcpy(gateBuf+widthOrign*2*i+(9+5+22)*2,gateBuf+600*2*i+(89+startX)*2,widthAA*2);	/*aa*/
				memcpy(gateBuf+widthOrign*2*i+(9+5+22+widthAA)*2,gateBuf+600*2*i+504*2,10*2);	/*right dark + dark pixel*/
			}
		}
		
		
	}
	
	if(picBuf != NULL)
	{	
		widthAA = pic_size[EN_OXI600_MK720_80UM].small_col - 10;
		widthWin =	pic_size[EN_OXI600_MK720_80UM].small_col;
		heightWin =  pic_size[EN_OXI600_MK720_80UM].small_row;
		widthOrign = original_size[EN_OXI600_MK720_80UM].small_col;
		heightOrign = original_size[EN_OXI600_MK720_80UM].small_row;
		if(gateBuf != NULL)
		{
			memcpy(picBuf+widthOrign*heightOrign*2,gateBuf,widthOrign*2*10);
			heightWin += 10;
		}
		if(startX <= 21)
		{
			for(i = 0; i < heightWin;i++)
			{
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+9*2,5*2);	/*left datk*/
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+(14+startX)*2,widthAA*2);/*aa*/
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+(widthOrign-10)*2,5*2);/*right dark pixel*/
			}

		}
		else
		{
			for(i = 0; i < heightWin;i++)
			{
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+9*2,5*2);	/*left dark*/
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+36*2,widthAA*2);/*aa*/
				memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i+(widthOrign-10)*2,5*2);/*right dark pixel*/
			}

		}

		*picLen = widthWin*heightWin*2;
	}

}



void Dev_Oxi600_MK720_80umWholeJigsaw(uint8_t *picBuf,uint32_t *picLen)
{
	/*not change yet*/
}

void Dev_Oxi600_MK720_100umWinJigsaw(uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,offset,widthWin,heightWin,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));
	if(gateBuf != NULL)
	{
		widthAA = pic_size[EN_OXI600_MK720_100UM].small_col - 20;
		widthOrign = original_size[EN_OXI600_MK720_100UM].small_col;
		for(i = 0;i < 10;i++)
		{
			memcpy(gateBuf+widthOrign*2*i,gateBuf+600*2*i+34*2,(10+25)*2);		/*left dark*/
			memcpy(gateBuf+widthOrign*2*i+(10+25)*2,gateBuf+600*2*i+(34+99+startX)*2,widthAA*2);	/*AA */
			memcpy(gateBuf+widthOrign*2*i+(10+25+widthAA)*2,gateBuf+600*2*i+457*2,10*2);	/*right dark pixel*/
		}
	}
	
	if(picBuf != NULL)
	{	
		widthAA = pic_size[EN_OXI600_MK720_100UM].small_col - 20;
		widthWin =	pic_size[EN_OXI600_MK720_100UM].small_col;
		heightWin =  pic_size[EN_OXI600_MK720_100UM].small_row;
		widthOrign = original_size[EN_OXI600_MK720_100UM].small_col;
		heightOrign = original_size[EN_OXI600_MK720_100UM].small_row;
		if(gateBuf != NULL)
		{
			memcpy(picBuf+widthOrign*heightOrign*2,gateBuf,widthOrign*2*10);
			heightWin += 10;
		}

		for(i = 0; i < heightWin;i++)
		{
			memcpy(picBuf+widthWin*2*i,picBuf+widthOrign*2*i,10*2);			/*left dark*/
			memcpy(picBuf+widthWin*2*i+10*2,picBuf+widthOrign*2*i+(10+25)*2,(widthAA+10)*2);	/*AA + dark piexl*/
		}
		*picLen = widthWin*heightWin*2;
	}
#if 0	
	if(gateBuf != NULL)
	{
		
		for(i = 0;i < 10;i++)
		{
			offset = widthWin * 2 * i;
			memcpy(tmpBuf,gateBuf + 600*2*i + 35*2, 5*2);	/*back dark pixel*/
			memcpy(gateBuf + offset,gateBuf + 600*2*i + 31*2,4*2); /*copy dark line*/
			memcpy(gateBuf + offset + 5*2, gateBuf + 600*2*i + (69+ startX)*2, widthAA*2);
			memcpy(gateBuf + offset + (widthAA+5)*2,tmpBuf,5*2);
		}
		memcpy(picBuf+*picLen,gateBuf,widthWin*2*10);
		*picLen += widthWin*2*10;
	}
#endif	
}


void Dev_Oxi600_MK720_100umWholeJigsaw(uint8_t *picBuf,uint32_t *picLen)
{
	/*not change yet*/
}

void Dev_Oxi600_MK810DoubleFingerJigsaw(uint8_t *destBuf,uint8_t *srcBuf,uint16_t startY1,uint16_t startY2,uint32_t dataLen)
{
	int i;
	if (startY1 < startY2)
	{
		memcpy(destBuf,srcBuf,(startY2-startY1)*(10+138)*2);	/*finger1 未重合区域*/
		for (i = 0; i < 138 - (startY2 - startY1); i++)
		{
			memcpy(destBuf + (startY2 - startY1)*(10 + 138) * 2 + i*(138+10)*2, srcBuf+ (startY2 - startY1)*(10 + 138) * 2 + i*(138+138+10)*2,(138+10)*2);/*finger1 重合区域遮黑+aa*/
			memcpy(destBuf + (138 + 10) * 148 * 2 + i*(138 + 10)*2, srcBuf + (startY2 - startY1)*(10 + 138) * 2 + i*(138 + 138 + 10) * 2, 10 * 2);		/*finger2 遮黑*/
			memcpy(destBuf + (138 + 10) * 148 * 2 + i*(138 + 10)*2+10*2, srcBuf + (startY2 - startY1)*(10 + 138) * 2 + i*(138 + 138 + 10) * 2 + (10+138)*2,138*2);/*finger2 a*/
		}
		memcpy(destBuf + (138 + 10) * 148 * 2+(138-(startY2-startY1))*(10+138)*2, srcBuf + (startY2 - startY1)*(10 + 138) * 2 + (138 - (startY2 - startY1))*(138+138+10)*2, (startY2 - startY1)*(10 + 138) * 2);/*finger2 未重合区域*/

	}
	else	 /* y1 > y2 */
	{
		memcpy(destBuf+(138+10)*148*2,srcBuf,(startY1 - startY2)*(10+138)*2 );	/*finger2 未重合区域*/
		for (i = 0; i < 138 - (startY1 - startY2); i++)
		{
			memcpy(destBuf + (138+10)*148*2 + (startY1 - startY2)*(10 + 138) * 2 + i *(138+10)*2,srcBuf + (startY1 - startY2)*(10 + 138) * 2 + i*(138+138+10)*2,10*2);/*finger2 遮黑*/
			memcpy(destBuf + (138+10)*148*2 + (startY1 - startY2)*(10 + 138) * 2 + i *(138+10)*2+10*2,srcBuf + (startY1 - startY2)*(10 + 138) * 2 + i*(138+138+10)*2+(10+138)*2,138*2);/*finger2 aa*/
			memcpy(destBuf + (138+10)*2*i,srcBuf + (startY1 - startY2)*(10 + 138) * 2 + i*(138 + 138 + 10) * 2,(138+10)*2); /*finger1 遮黑+aa*/
		}
		memcpy(destBuf+(138+10)*2 *(138-(startY1-startY2)),srcBuf + (startY1 - startY2)*(10 + 138) * 2+ (138 - (startY1 - startY2))*(138+138+10)*2,(startY1-startY2)*(10+138)*2);	/*finger1 未重合区域*/
	}

	return	;
}


void Dev_Oxi600_MK810_80umWinJigsaw(uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,widthWin,heightWin,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));
	if(gateBuf != NULL)
	{
		widthAA = pic_size[EN_OXI600_MK810_80UM].small_col - 10;
		widthOrign = original_size[EN_OXI600_MK810_80UM].small_col;
		for(i = 0;i < 10;i++)
		{
			memcpy(gateBuf+widthOrign*2*i,gateBuf+600*2*i+31*2,10*2);	/*left dark+dark pixel*/
			memcpy(gateBuf+widthOrign*2*i+10*2,gateBuf+600*2*i+(31+38+startX)*2,widthAA*2);	/*AA*/
			
		}
	}
	
	if(picBuf != NULL)
	{	
		widthAA = pic_size[EN_OXI600_MK810_80UM].small_col - 10;
		widthWin =  pic_size[EN_OXI600_MK810_80UM].small_col;
		heightWin =  pic_size[EN_OXI600_MK810_80UM].small_row;
		widthOrign = original_size[EN_OXI600_MK810_80UM].small_col;
		heightOrign = original_size[EN_OXI600_MK810_80UM].small_row;
		
		if(gateBuf != NULL)
		{
			memcpy(picBuf+widthOrign*heightOrign*2,gateBuf,widthOrign*2*10);
			heightWin += 10;
		}

		for(i = 0; i < heightWin;i++)
		{
			memcpy(tmpBuf,picBuf + widthOrign*i*2 + 4*2 , 5*2);		/*backup dark pixel*/
			picBuf[widthWin*i*2 + 4*2] = 0;
			picBuf[widthWin*i*2 + 4*2 + 1] = 0;        		   /*dark not enough 5 , add 0*/	

			memcpy(picBuf + widthWin*i*2 + 5*2, picBuf + widthOrign*i*2 + 10*2, widthAA*2); /*copy AA area*/ 
			memcpy(picBuf + widthWin*i*2 + (widthAA+5) * 2 , tmpBuf,5*2);	/*dark pixel*/
		}
		*picLen = widthWin*heightWin*2;
	}
#if 0	
	if(gateBuf != NULL)
	{
		
		for(i = 0;i < 10;i++)
		{
			offset = widthWin * 2 * i;
			memcpy(tmpBuf,gateBuf + 600*2*i + 35*2, 5*2);	/*back dark pixel*/
			memcpy(gateBuf + offset,gateBuf + 600*2*i + 31*2,4*2); /*copy dark line*/
			memcpy(gateBuf + offset + 5*2, gateBuf + 600*2*i + (69+ startX)*2, widthAA*2);
			memcpy(gateBuf + offset + (widthAA+5)*2,tmpBuf,5*2);
		}
		memcpy(picBuf+*picLen,gateBuf,widthWin*2*10);
		*picLen += widthWin*2*10;
	}
#endif	
}


void Dev_Oxi600_MK810_80umWholeJigsaw(uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,widthWhl,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));
	if(picBuf != NULL)
	{	
		widthAA = 500;
		widthWhl =	pic_size[EN_OXI600_MK810_80UM].big_col;
		heightAA =	pic_size[EN_OXI600_MK810_80UM].big_row;
		widthOrign = original_size[EN_OXI600_MK810_80UM].big_col;
		heightOrign = original_size[EN_OXI600_MK810_80UM].big_row;

		for(i = 0; i < heightAA;i++)
		{
			memcpy(tmpBuf,picBuf + widthOrign*i*2 + 4*2 , 5*2);		/*backup dark pixel*/
			picBuf[widthWhl*i*2 + 4*2] = 0;
			picBuf[widthWhl*i*2 + 4*2 + 1] = 0;        		   /*dark not enough 5 , add 0*/	

			memcpy(picBuf + widthWhl*i*2 + 5*2, picBuf + widthOrign*i*2 + 10*2, widthAA*2); /*copy AA area*/ 
			memcpy(picBuf + widthWhl*i*2 + (widthAA+5) * 2 , tmpBuf,5*2);	/*dark pixel*/
		}
		*picLen = widthWhl*heightAA*2;
	}
	
}



void Dev_Oxi600_MK320_100umWinJigsaw(uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,widthWin,heightWin,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));

	if(gateBuf != NULL)
	{
		widthAA = pic_size[EN_OXI600_MK320_100UM].small_col - 20;
		widthOrign = original_size[EN_OXI600_MK320_100UM].small_col;
		for(i = 0;i < 10;i++)
		{
			memcpy(gateBuf + i*widthOrign*2,gateBuf + i*600*2 + 34*2,21*2);	/*lfet dark+dark pixel*/
			memcpy(gateBuf + i*widthOrign*2 + 21*2,gateBuf + i*600*2 + (34+45+startX)*2,widthAA*2);/*aa*/
		}
	}
	
	if(picBuf != NULL)
	{	
		widthAA = pic_size[EN_OXI600_MK320_100UM].small_col - 20;
		widthWin =	pic_size[EN_OXI600_MK320_100UM].small_col;
		heightWin =	pic_size[EN_OXI600_MK320_100UM].small_row;
		widthOrign = original_size[EN_OXI600_MK320_100UM].small_col;
		heightOrign = original_size[EN_OXI600_MK320_100UM].small_row;
		if(gateBuf != NULL)
		{
			memcpy(picBuf + widthOrign*heightOrign*2,gateBuf,widthOrign*10*2);
			heightWin += 10;
		}
		for(i = 0; i < heightWin;i++)
		{			
			memcpy(tmpBuf,picBuf + widthOrign*i*2 + 15*2 , 6*2); 	/*backup dark pixel*/

			memcpy(picBuf + widthWin*i*2 + 0*2, picBuf + widthOrign*i*2 + 0*2, 3*2); /*copy 3drak */ 
			memcpy(picBuf + widthWin*i*2 + 3*2, picBuf + widthOrign*i*2 + 14*2, 1*2); /*copy 1drak */ 
			memset(picBuf + widthWin*i*2 + 4*2, 0 , 6*2);							/*spare dark write 0*/
			memcpy(picBuf + widthWin*i*2 + 10*2, picBuf + widthOrign*i*2 + 21*2,widthAA*2);  /*copy AA*/
			memcpy(picBuf + widthWin*i*2 + (widthAA+10)*2,tmpBuf,6*2);					/*copy dark pixel*/
			memset(picBuf + widthWin*i*2 + (widthAA+10+6)*2,0,4*2);						/*spare write 0*/
		}
		*picLen = widthWin*heightWin*2;
	}
#if 0	
	if(gateBuf != NULL)
	{
		
		for(i = 0;i < 10;i++)
		{
			offset = widthWin * 2 * i;
			memcpy(tmpBuf,gateBuf + 600*2*i + 49*2, 6*2);			/*back dark pixel*/
			memcpy(gateBuf + offset + 0*2,gateBuf + 600*2*i + 34*2,4*2); /*copy 3dark line*/
			memcpy(gateBuf + offset + 3*2,gateBuf + 600*2*i + 48*2,1*2); /*copy 3dark line*/
			memset(gateBuf + offset + 4*2,0,6*2); 					/*spar dark wirte 0*/
			memcpy(gateBuf + offset + 10*2, gateBuf + 600*2*i + (78+ startX)*2, widthAA*2);	/*aa*/
			memcpy(gateBuf + offset + (widthAA+10)*2,tmpBuf,6*2); 		/*dark piexl*/
			memset(gateBuf + offset + (widthAA+10+6)*2,0,4*2);		/*spare wirte 0*/
		}
		memcpy(picBuf+*picLen,gateBuf,widthWin*2*10);
		*picLen += widthWin*2*10;
	}
#endif	
}

void Dev_Oxi600_MK320_100umWholeJigsaw(uint8_t *picBuf,uint32_t *picLen)
{
	uint32_t i;
	uint16_t widthAA,heightAA,widthWhl,originOfset,widthOrign,heightOrign;

	memset(tmpBuf,0x00,sizeof(tmpBuf));
	if(picBuf != NULL)
	{	
		widthAA = 472;
		widthWhl =	pic_size[EN_OXI600_MK320_100UM].big_col;
		heightAA =	pic_size[EN_OXI600_MK320_100UM].big_row;
		widthOrign = original_size[EN_OXI600_MK320_100UM].big_col;
		heightOrign = original_size[EN_OXI600_MK320_100UM].big_row;

		for(i = 0; i < heightAA;i++)
		{			
			memcpy(tmpBuf,picBuf + widthOrign*i*2 + 15*2 , 6*2); 	/*backup dark pixel*/

			memcpy(picBuf + widthWhl*i*2 + 0*2, picBuf + widthOrign*i*2 + 0*2, 3*2); /*copy 3drak */ 
			memcpy(picBuf + widthWhl*i*2 + 3*2, picBuf + widthOrign*i*2 + 14*2, 1*2); /*copy 1drak */ 
			memset(picBuf + widthWhl*i*2 + 4*2, 0 , 6*2);							/*spare dark write 0*/
			memcpy(picBuf + widthWhl*i*2 + 10*2, picBuf + widthOrign*i*2 + 21*2,widthAA*2);  /*copy AA*/
			memcpy(picBuf + widthWhl*i*2 + (widthAA+10)*2,tmpBuf,6*2);					/*copy dark pixel*/
			memset(picBuf + widthWhl*i*2 + (widthAA+10+6)*2,0,4*2);						/*spare write 0*/
		}
		
		*picLen = widthWhl*heightAA*2;
	}
	
}



void Dev_Oxi600_WholeJigsaw(EN_CHL600_PRJ_TYPE prjType,uint8_t *picBuf,uint32_t *picLen)
{
	switch(prjType)
	{
		case EN_PRJ_OXI600_MK720_80UM:
			Dev_Oxi600_MK720_80umWholeJigsaw(picBuf,picLen);
			
			break;
			
		case EN_PRJ_OXI600_MK720_100UM:
			Dev_Oxi600_MK720_100umWholeJigsaw(picBuf,picLen);
			break;
			
		case EN_PRJ_OXI600_MK810_80UM:
			Dev_Oxi600_MK810_80umWholeJigsaw(picBuf,picLen);
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			Dev_Oxi600_MK320_100umWholeJigsaw(picBuf,picLen);
			break;

		default:
			break;
	}

}

void Dev_Oxi600_WinJigsaw(EN_CHL600_PRJ_TYPE prjType ,uint8_t *gateBuf,uint16_t startX,uint8_t *picBuf,uint32_t *picLen )
{
	switch(prjType)
	{
		case EN_PRJ_OXI600_MK720_80UM:
			Dev_Oxi600_MK720_80umWinJigsaw(gateBuf,startX,picBuf,picLen);
			break;
			
		case EN_PRJ_OXI600_MK720_100UM:
			Dev_Oxi600_MK720_100umWinJigsaw(gateBuf,startX,picBuf,picLen);
			break;
			
		case EN_PRJ_OXI600_MK810_80UM:
			Dev_Oxi600_MK810_80umWinJigsaw(gateBuf,startX,picBuf,picLen);
			break;
			
		case EN_PRJ_OXI600_MK320_100UM:
			Dev_Oxi600_MK320_100umWinJigsaw(gateBuf,startX,picBuf,picLen);
			break;

		default:
			break;
	}

}






