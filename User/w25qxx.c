#include "w25qxx.h" 


extern 	void IwdgFeed(void);

 
u16 W25QXX_TYPE=W25Q128;	//Ĭ����W25Q128

//4KbytesΪһ��Sector
//16������Ϊ1��Block
//W25Q128
//����Ϊ16M�ֽ�,����128��Block,4096��Sector 
													 
//��ʼ��SPI FLASH��IO��
//void W25QXX_Init(void)
//{ 
//	SPI1_SetSpeed(SPI_BaudRatePrescaler_4);		//����Ϊ21Mʱ��,����ģʽ 
//	W25QXX_TYPE=W25QXX_ReadID();	//��ȡFLASH ID.
//}  
uint8_t Flash_Judgment(void)
{
	uint8_t FlashON;

	switch(W25QXX_ReadID())
	{
		case 0xE010:
			printf("Flash is BY25Q10A,memery size is 128KB.\r\n");
			FlashON = 1;
			break;
		case 0xEF11:
			printf("Flash is W25Q20EW,memery size is 256KB.\r\n");
			FlashON = 1;
			break;
		case 0xE012:
			printf("Flash is BY25Q40A,memery size is 1024KB.\r\n");
			FlashON = 1;
			break;
		case 0xC812:
			printf("Flash is GD25Q40C,memery size is 1024KB.\r\n");
			FlashON = 1;
			break;
		case 0x0b16:
			printf("Flash is XT25Q64BDSIGT,memery size is 8M.\r\n");
			FlashON = 1;
			break;
		default :
//			printf("0x%x\r\n",W25QXX_ReadID());
			printf("No Flash!\r\n");
			FlashON = 0;
			break;
	}

	return FlashON;
}
//��ȡW25QXX��״̬�Ĵ���
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:Ĭ��0,״̬�Ĵ�������λ,���WPʹ��
//TB,BP2,BP1,BP0:FLASH����д��������
//WEL:дʹ������
//BUSY:æ���λ(1,æ;0,����)
//Ĭ��:0x00

uint8_t W25QXX_ReadSR(void)   
{  
	uint8_t receData=0;   
	uint8_t sendData=W25X_ReadStatusReg;
	FLASH_SPI_CS_EN();                      		 //ʹ������   
	Dev_Flash_SPISendData(&sendData,1,100);    		 //���Ͷ�ȡ״̬�Ĵ�������    
	Dev_Flash_SPIReceiveData(&receData,1,100);       //��ȡһ���ֽ�  
	FLASH_SPI_CS_DIS();                              //ȡ��Ƭѡ     
	return receData;   
} 
//дW25QXX״̬�Ĵ���
//ֻ��SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)����д!!!
void W25QXX_Write_SR(uint8_t sr)   
{

	uint8_t sendData=W25X_WriteStatusReg;
	FLASH_SPI_CS_EN();                            //ʹ������   
	Dev_Flash_SPISendData(&sendData,1,100);   //����дȡ״̬�Ĵ�������    
	sendData = sr;
	Dev_Flash_SPISendData(&sendData,1,100);               		//д��һ���ֽ�  
	FLASH_SPI_CS_DIS();                            //ȡ��Ƭѡ     	      
}   
//W25QXXдʹ��	
//��WEL��λ   
void W25QXX_Write_Enable(void)   
{
	uint8_t sendData=W25X_WriteEnable;
	FLASH_SPI_CS_EN();                            //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);       //����дʹ��  
	FLASH_SPI_CS_DIS();                           //ȡ��Ƭѡ     	      
} 
//W25QXXд��ֹ	
//��WEL����  
void W25QXX_Write_Disable(void)   
{  
	uint8_t sendData=W25X_WriteDisable;
	FLASH_SPI_CS_EN();                           //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);     //����д��ָֹ��    
	FLASH_SPI_CS_DIS();                            //ȡ��Ƭѡ     	      
} 		
//��ȡоƬID
//����ֵ����:				   
//0XEF13,��ʾоƬ�ͺ�ΪW25Q80  
//0XEF14,��ʾоƬ�ͺ�ΪW25Q16    
//0XEF15,��ʾоƬ�ͺ�ΪW25Q32  
//0XEF16,��ʾоƬ�ͺ�ΪW25Q64 
//0XEF17,��ʾоƬ�ͺ�ΪW25Q128 	  
u16 W25QXX_ReadID(void)
{
	uint8_t sendData[4]={0x90,0,0,0};
	uint8_t receData=0;  
	u16 Temp = 0;	  
	FLASH_SPI_CS_EN(); 				    
	Dev_Flash_SPISendData(sendData,4,100); //���Ͷ�ȡID����	    
 			   
	Dev_Flash_SPIReceiveData(&receData,1,100);  
	Temp|=(uint16_t)receData<<8;
	Dev_Flash_SPIReceiveData(&receData,1,100);  
	Temp|=receData;	 
	FLASH_SPI_CS_DIS();				    
	return Temp;
}   		    
//��ȡSPI FLASH  
//��ָ����ַ��ʼ��ȡָ�����ȵ�����
//pBuffer:���ݴ洢��
//ReadAddr:��ʼ��ȡ�ĵ�ַ(24bit)
//NumByteToRead:Ҫ��ȡ���ֽ���(���65535)
void W25QXX_Read(uint8_t* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	
	uint8_t sendData=W25X_ReadData;

	FLASH_SPI_CS_EN();                            //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);        //���Ͷ�ȡ����   
    sendData = ((uint8_t)((ReadAddr)>>16));  //����24bit��ַ  
    Dev_Flash_SPISendData(&sendData,1,100);  
    sendData = ((uint8_t)((ReadAddr)>>8));  
	Dev_Flash_SPISendData(&sendData,1,100); 
    sendData = ((uint8_t)ReadAddr);   
	Dev_Flash_SPISendData(&sendData,1,100); 

    Dev_Flash_SPIReceiveData(pBuffer,NumByteToRead,1000);  ;   //����  
    
	FLASH_SPI_CS_DIS(); 				    	      
}  
//SPI��һҳ(0~65535)��д������256���ֽڵ�����
//��ָ����ַ��ʼд�����256�ֽڵ�����
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���256),������Ӧ�ó�����ҳ��ʣ���ֽ���!!!	 
void W25QXX_Write_Page(uint8_t* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
 
	uint8_t sendData;
    W25QXX_Write_Enable();                  //SET WEL 
	FLASH_SPI_CS_EN();                           //ʹ������   
	sendData=W25X_PageProgram;
	Dev_Flash_SPISendData(&sendData,1,100);        //���Ͷ�ȡ����   
    sendData = ((uint8_t)((WriteAddr)>>16));  //����24bit��ַ  
    Dev_Flash_SPISendData(&sendData,1,100);  
    sendData = ((uint8_t)((WriteAddr)>>8));  
	Dev_Flash_SPISendData(&sendData,1,100); 
    sendData = ((uint8_t)WriteAddr);   
	Dev_Flash_SPISendData(&sendData,1,100);   
	
	Dev_Flash_SPISendData(pBuffer,NumByteToWrite,1000);  

	FLASH_SPI_CS_DIS();                           //ȡ��Ƭѡ 
	W25QXX_Wait_Busy();					   //�ȴ�д�����
} 
//�޼���дSPI FLASH 
//����ȷ����д�ĵ�ַ��Χ�ڵ�����ȫ��Ϊ0XFF,�����ڷ�0XFF��д������ݽ�ʧ��!
//�����Զ���ҳ���� 
//��ָ����ַ��ʼд��ָ�����ȵ�����,����Ҫȷ����ַ��Խ��!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)
//NumByteToWrite:Ҫд����ֽ���(���65535)
//CHECK OK
void W25QXX_Write_NoCheck(uint8_t* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 			 		 
	u16 pageremain;	   
	pageremain=256-WriteAddr%256; //��ҳʣ����ֽ���		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//������256���ֽ�
	while(1)
	{	   
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//д�������
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //��ȥ�Ѿ�д���˵��ֽ���
			if(NumByteToWrite>256)pageremain=256; //һ�ο���д��256���ֽ�
			else pageremain=NumByteToWrite; 	  //����256���ֽ���
		}
	};	    
} 
//дSPI FLASH  
//��ָ����ַ��ʼд��ָ�����ȵ�����
//�ú�������������!
//pBuffer:���ݴ洢��
//WriteAddr:��ʼд��ĵ�ַ(24bit)						
//NumByteToWrite:Ҫд����ֽ���(���65535)   
uint8_t W25QXX_BUFFER[4096];		 
void W25QXX_Write(uint8_t* pBuffer,u32 WriteAddr,u32 NumByteToWrite)   
{ 
	u32 secpos;
	u16 secoff;
	u16 secremain;	   
 	u16 i;    
	uint8_t * W25QXX_BUF;	  
   	W25QXX_BUF=W25QXX_BUFFER;	     
 	secpos=WriteAddr/4096;//������ַ  
	secoff=WriteAddr%4096;//�������ڵ�ƫ��
	secremain=4096-secoff;//����ʣ��ռ��С   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//������
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//������4096���ֽ�
	while(1) 
	{	
		IwdgFeed();
		//CS0_High;
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);//������������������
		for(i=0;i<secremain;i++)//У������
		{
			if(W25QXX_BUF[secoff+i]!=0xFF)break;//��Ҫ����  	  
		}
		if(i<secremain)//��Ҫ����
		{
			//CS0_High;
			W25QXX_Erase_Sector(secpos);//�����������
			for(i=0;i<secremain;i++)	   //����
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			//CS0_High;
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);//д����������  

		}
		else 
		{
			//CS0_High;
			W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);
		}//д�Ѿ������˵�,ֱ��д������ʣ������. 				   
		if(NumByteToWrite==secremain)break;//д�������
		else//д��δ����
		{
			secpos++;//������ַ��1
			secoff=0;//ƫ��λ��Ϊ0 	 

		   	pBuffer+=secremain;  //ָ��ƫ��
			WriteAddr+=secremain;//д��ַƫ��	   
		   	NumByteToWrite-=secremain;				//�ֽ����ݼ�
			if(NumByteToWrite>4096)secremain=4096;	//��һ����������д����
			else secremain=NumByteToWrite;			//��һ����������д����
		}	 
	};	
	IwdgFeed();
}
//��������оƬ		  
//�ȴ�ʱ�䳬��...
void W25QXX_Erase_Chip(void)   
{     
	uint8_t sendData=W25X_ChipErase;
	IwdgFeed();
    W25QXX_Write_Enable();                  //SET WEL 
    W25QXX_Wait_Busy();   
  	FLASH_SPI_CS_EN();                             //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);        //����Ƭ��������  
	FLASH_SPI_CS_DIS();                             //ȡ��Ƭѡ     	  
	IwdgFeed();
	W25QXX_Wait_Busy();   				   //�ȴ�оƬ��������
	IwdgFeed();
}   
//����һ������
//Dst_Addr:������ַ ����ʵ����������
//����һ��ɽ��������ʱ��:150ms
void W25QXX_Erase_Sector(u32 Dst_Addr)   
{  
	//����falsh�������,������   
 	//printf("fe:%x\r\n",Dst_Addr);	  
 	uint8_t sendData=W25X_SectorErase;
    Dst_Addr*=4096;
    W25QXX_Write_Enable();                  //SET WEL 	 
    W25QXX_Wait_Busy();   
    FLASH_SPI_CS_EN();                            //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);       //������������ָ�� 
    sendData = ((uint8_t)((Dst_Addr)>>16));  //����24bit��ַ    
    Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)((Dst_Addr)>>8)); 
	Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)Dst_Addr);  
	Dev_Flash_SPISendData(&sendData,1,100);
	FLASH_SPI_CS_DIS();                             //ȡ��Ƭѡ     	      
    W25QXX_Wait_Busy();   				   //�ȴ��������
}  
//����һ��block_64K
//Dst_Addr:block��ַ ����ʵ����������
void W25QXX_Erase_Block(u32 Dst_Addr)   
{  
	//����falsh�������,������   
 	//printf("fe:%x\r\n",Dst_Addr);	  
 	uint8_t sendData=W25X_BlockErase;
    Dst_Addr*=64*1024;
    W25QXX_Write_Enable();                  //SET WEL 	 
    W25QXX_Wait_Busy();   
    FLASH_SPI_CS_EN();                            //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);       //������������ָ�� 
    sendData = ((uint8_t)((Dst_Addr)>>16));  //����24bit��ַ    
    Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)((Dst_Addr)>>8)); 
	Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)Dst_Addr);  
	Dev_Flash_SPISendData(&sendData,1,100);
	FLASH_SPI_CS_DIS();                             //ȡ��Ƭѡ     	      
    W25QXX_Wait_Busy();   				   //�ȴ��������
}  

uint8_t W25QXX_Erase_multi_Sector(u32 Dst_Addr,uint32_t numByte)
{
	uint32_t sectorCnt,secStrAddr,blockAddr,blockCnt,i,sectorSize,blockSize;
	sectorSize = 4*1024;
	blockSize = 64 * 1024;
	sectorCnt = numByte / sectorSize;
	secStrAddr = Dst_Addr / sectorSize;
	if(numByte % sectorSize || Dst_Addr % sectorSize)
	{
		printf("erase multiple sector ,para in err,not 4*1024*n, addr = %d,numbyte = %d\n",Dst_Addr,numByte);
		return 1;
	}

	if(numByte >= blockSize)	// byte > 64K  ,might be use earse block
	{
		if((Dst_Addr % blockSize) == 0)		// start addr == block addr 
		{
			blockAddr = Dst_Addr / blockSize;	//block start addr_no
			blockCnt = numByte / blockSize;		// block count
			for(i = 0; i<blockCnt;i++ )
			{
				IwdgFeed();
				W25QXX_Erase_Block(blockAddr+i);
				IwdgFeed();
			}
			if((numByte % blockSize) != 0)		// erase size != 64K * n;
			{
				sectorCnt = numByte % blockSize / sectorSize;	//not enough 64K earse by sector
				secStrAddr = (Dst_Addr + blockSize*blockCnt)/sectorSize;
				for(i = 0;i<sectorCnt;i++)
				{
					IwdgFeed();
					W25QXX_Erase_Sector(secStrAddr+i);
					//printf("erase sector\n");
				}
			}
				
		}
		else		// start add != block addr 
		{
			sectorCnt = 16 - Dst_Addr % blockSize / sectorSize;	// top area, earse by sector,��������һ���������
			secStrAddr = Dst_Addr / sectorSize;
			for(i=0;i<sectorCnt;i++)
			{
				IwdgFeed();
				W25QXX_Erase_Sector(secStrAddr+i);
				//printf("erase sector\n");
			}
		
			blockAddr = Dst_Addr / blockSize + 1;	// +1������һ���ֲ��ǿ����
			blockCnt = (numByte -sectorCnt*sectorSize) / blockSize;
			for(i = 0; i<blockCnt;i++ )		// erase block,���������ʡʱ��
			{
				IwdgFeed();
				W25QXX_Erase_Block(blockAddr+i);
				IwdgFeed();
			}
			if(((numByte -sectorCnt*sectorSize) % blockSize) != 0)		// erase size != 64K * n,�ײ�����һ���������
			{
				sectorCnt = ((numByte -sectorCnt*sectorSize) % blockSize)/ sectorSize;
				secStrAddr = (blockSize*(blockAddr+blockCnt))/sectorSize;
				for(i = 0;i<sectorCnt;i++)
				{
					IwdgFeed();
					W25QXX_Erase_Sector(secStrAddr+i);
					//printf("erase sector\n");
				}
			}

		}
	}
	else	//erase area not enough 64K, erase sector
	{
		secStrAddr = Dst_Addr / sectorSize;
		sectorCnt = numByte / sectorSize;
		for(i = 0;i < sectorCnt;i++)
		{
			IwdgFeed();
			W25QXX_Erase_Sector(secStrAddr+i);
		}
	}	
	return 0;
	
}

//�ȴ�����
void W25QXX_Wait_Busy(void)   
{   
	//while((W25QXX_ReadSR()&0x01)==0x01);   // �ȴ�BUSYλ���
	while(1)
	{
		if(!(W25QXX_ReadSR()&0x01)==0x01)
			break;
	}
}  
//�������ģʽ
void W25QXX_PowerDown(void)   
{ 
	uint8_t sendData=W25X_PowerDown;

  	FLASH_SPI_CS_EN();                           //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);         //���͵�������  
	FLASH_SPI_CS_DIS();                            //ȡ��Ƭѡ     	      
    HAL_Delay(3);                               //�ȴ�TPD  
}   
//����
void W25QXX_WAKEUP(void)   
{  
	uint8_t sendData=W25X_ReleasePowerDown;

  	FLASH_SPI_CS_EN();                           //ʹ������   
    Dev_Flash_SPISendData(&sendData,1,100);    //  send W25X_PowerDown command 0xAB    
	FLASH_SPI_CS_DIS();                             //ȡ��Ƭѡ     	      
    HAL_Delay(3);                               //�ȴ�TRES1
} 

EN_RESP_TYPE W25QXX_writeGainImg(uint8_t *dataBuf,uint32_t size, uint32_t addr )
{
	if((addr+ size)> 8*1024*1024)
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}
	W25QXX_Write(dataBuf,addr ,size);
	return EN_RESP_SUCCESS;
}

EN_RESP_TYPE W25QXX_readGainImg(uint8_t *dataBuf,uint32_t size,uint32_t addr)
{
	if((addr+ size)> 8*1024*1024)
	{
		return EN_RESP_CMO_ERR_INVALID_PARA;
	}

	W25QXX_Read(dataBuf,addr,size);
	return EN_RESP_SUCCESS;
}

uint32_t W25QXX_getFlashID(void)
{
	uint16_t flashid;
	flashid = W25QXX_ReadID();
	return (uint32_t)flashid;
}
#if 0
void ReadMCUPMUDataToPC(void)
{
	Sdata[0]=0xf0;
	Sdata[1]=0x00;
	
	STMFLASH_ReadforByte(GAIN_PARAMETER_SAVE_ADDR,Sdata+2,126);
		
	Sdata[128] = 0xff;
	Sdata[129] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
//void ReadMCUGAIN_InforToPC(void)
//{
//	
//}

void WritePMUDataToMCU(void)
{
	uint16_t i;
	STMFLASH_ReadforByte(FLASH_SAVE_ADDR,Sdata,372);
	for(i=0;i<126;i++)
	{
		Sdata[i+372]=USB_Rx_Buffer[i+2];
	}
	STMFLASH_Write(FLASH_SAVE_ADDR,(uint32_t*)Sdata,500);
	
	SendOK();
}

void ReadW25QXXCMOSDataToPC(void)
{
	uint8_t temp=0;
	uint16_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata,0,256);
	
	for(i=0;i<256;i=i+2)
	{
		if((Sdata[i]==0xff)&&(Sdata[i+1]==0xff))
		{
			break;
		}
		else
		{
			temp += Sdata[i];
			temp += Sdata[i+1];
		}
	}
	if(temp == Sdata[255])
	{
		Sdata[0]=0xf0;
		Sdata[1]=0x00;
		
		W25QXX_Read(Sdata+2,0,i);
	
		Sdata[i+2] = 0xff;
		Sdata[i+3] = 0xff;
		
		while(USB_StatusDataSended==0);
		USB_StatusDataSended = 0;
		DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
	}
	else{SendError();}
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
}
void ReadW25QXXPMUDataToPC(void)
{
	uint8_t i,temp=0;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata,0,256);
	
	for(i=0;i<122;i++)
	{
		temp += Sdata[i];
	}
	if(temp == Sdata[255])
	{
		W25QXX_Read(Sdata,0x100,128);
		temp = 0;
		for(i=0;i<4;i++)
		{
			temp += Sdata[i];
		}
		
		if(temp == Sdata[127])
		{
			Sdata[0]=0xf0;
			Sdata[1]=0x00;
			W25QXX_Read(Sdata+2,0,122);
			W25QXX_Read(Sdata+124,0x100,4);
		
			Sdata[128] = 0xff;
			Sdata[129] = 0xff;
			while(USB_StatusDataSended==0);
			USB_StatusDataSended = 0;
			DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
		}
		else{SendError();}
	}
	else{SendError();}
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
}
void ReadW25QXXPCBANumToPC(void)
{
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata+2,0x1000,64);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);	
	Sdata[0] = 0xf0;
	Sdata[1] = 0x00;
	Sdata[66] = 0xff;
	Sdata[67] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
void ReadW25QXXModuleNumToPC(void)
{
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata+2,0x1000+64,64);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
	Sdata[0] = 0xf0;
	Sdata[1] = 0x00;
	Sdata[66] = 0xff;
	Sdata[67] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
void ReadW25QXXColorNumToPC(void)
{
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata+2,0x1000+128,2);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
	Sdata[0] = 0xf0;
	Sdata[1] = 0x00;
	Sdata[4] = 0xff;
	Sdata[5] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
void ReadW25QXXGAIN_AVRToPC(void)
{
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
//	W25QXX_Read(Sdata+2,0x2000,2);//���ģʽ
	
	W25QXX_Read(Sdata+2,0x2001,1);//С��ģʽ
	W25QXX_Read(Sdata+3,0x2000,1);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
	Sdata[0] = 0xf0;
	Sdata[1] = 0x00;
	Sdata[4] = 0xff;
	Sdata[5] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
void ReadW25QXXGAIN_InforToPC(void)
{
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
//	W25QXX_Read(Sdata+2,0x3000,12);//10----->12   С��ģʽ
	
	//��ԭ���Ĵ��ģʽ�ĳ�С��ģʽ
	W25QXX_Read(Sdata+2,0x3003,1);		//length
	W25QXX_Read(Sdata+3,0x3002,1);
	W25QXX_Read(Sdata+4,0x3001,1);
	W25QXX_Read(Sdata+5,0x3000,1);
	
	W25QXX_Read(Sdata+6,0x3007,1);		//CRC16
	W25QXX_Read(Sdata+7,0x3006,1);
	W25QXX_Read(Sdata+8,0x3005,1);
	W25QXX_Read(Sdata+9,0x3004,1);
	
	W25QXX_Read(Sdata+10,0x3008,4);		//others
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
	
	Sdata[0] = 0xf0;
	Sdata[1] = 0x00;
	Sdata[14] = 0xff;
	Sdata[15] = 0xff;
	while(USB_StatusDataSended==0);
	USB_StatusDataSended = 0;
	DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
}
void ReadW25QXXGAINDataToPC(void)
{
	uint32_t i,length;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	W25QXX_Read(Sdata,0x3000,4);
//	length = ((Sdata[0]<<24)|(Sdata[1]<<16)|(Sdata[2]<<8)|(Sdata[3]))+1;//С��ģʽ
	length = ((Sdata[3]<<24)|(Sdata[2]<<16)|(Sdata[1]<<8)|(Sdata[0]))+1;//���ģʽ
	
//	while(USB_StatusDataSended==0);
//	USB_StatusDataSended = 0;
	for(i=0;i<length;i=i+512)
	{
		W25QXX_Read(Sdata,0x4000+i,512);
		while(USB_StatusDataSended==0);
		USB_StatusDataSended = 0;
		DCD_EP_Tx(&USB_OTG_dev,CDC_IN_EP,Sdata,512);
	}
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(50);
}
void Read_mk110_sn()
{
//	PWRON_1117;
////	delay_ms(2);
//	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
//	delay_ms(60);
	HANDSHAKE();
	
	CS0_Low;
	SPI1_ReadWriteByte(0xef);
	SPI1_ReadWriteByte(0x01);
	SPI1_ReadWriteByte(0x01);
	SPI1_ReadWriteByte(0x00);
	SPI1_ReadWriteByte(0x04);
	SPI1_ReadWriteByte(0x09);
	SPI1_ReadWriteByte(USB_Rx_Buffer[2]);
	SPI1_ReadWriteByte((uint8_t)((0x0e+USB_Rx_Buffer[2])>>8));
	SPI1_ReadWriteByte((uint8_t)(0x0e+USB_Rx_Buffer[2]));
	CS0_High;
	delay_ms(10);    //2018.12.27
	if(!ReadyFor_module_sn_cmd())
		{
			SendErrorCode(0,0x0000);
			IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
			PWROFF_1117;
			delay_ms(100);
			return ;
		}
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	delay_ms(100);		
}
void WriteMK110_SN()
{	
	uint8_t i=0;
	uint16_t checksum=0;
//	if(USB_Rx_Buffer[67]==0xff)
///	{
	Sdata[0]=0xef;
	Sdata[1]=0x01;
	Sdata[2]=0x01;
	Sdata[3]=0x00;
	Sdata[4]=0x44;
	Sdata[5]=0x08;
	Sdata[6]=USB_Rx_Buffer[2];
	for(i=0;i<64;i++)
	{
		Sdata[7+i]=USB_Rx_Buffer[3+i];
		checksum=checksum+USB_Rx_Buffer[3+i];
	}
	checksum=checksum+0x01+0x44+0x08+Sdata[6];
	Sdata[71]=(uint8_t)(checksum>>8);
	Sdata[72]=(uint8_t)(checksum);
//	PWRON_1117;
//	//delay_ms(2);
//	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
//	delay_ms(60);
	HANDSHAKE();
	
	CS0_Low;
	for(i=0;i<73;i++)
		{
			SPI1_ReadWriteByte(Sdata[i]);
		}
			CS0_High;
			delay_ms(10);    //2018.12.27		
//	delay_ms(20);	
	if(!ReadyForCMD())
		{
			SendErrorCode(0,0x0000);
			IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
			PWROFF_1117;
			delay_ms(100);				
			return ;
		}
			SendOK();
			IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
			PWROFF_1117;
			delay_ms(100);	
//	}
	
}
void WritePMUDataToW25QXX(void)
{
	uint8_t temp=0;
	uint16_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Erase_Sector(0);//sector0�ڲ������4096
	for(i=0;i<256;i++)
	{
		Sdata[i] = 0xFF;
	}
	for(i=0;i<122;i++)
	{
		Sdata[i]=USB_Rx_Buffer[i+2];

		temp += USB_Rx_Buffer[i+2];
	}
	Sdata[255] = temp;
	W25QXX_Write_Page(Sdata,0,256);//д��Ĵ�������
	
	temp = 0;
	temp += USB_Rx_Buffer[124];temp += USB_Rx_Buffer[125];
	temp += USB_Rx_Buffer[126];temp += USB_Rx_Buffer[127];
	for(i=0;i<256;i++)
	{
		Sdata[i] = 0xFF;
	}
	Sdata[0] = USB_Rx_Buffer[124];
	Sdata[1] = USB_Rx_Buffer[125];
	Sdata[2] = USB_Rx_Buffer[126];
	Sdata[3] = USB_Rx_Buffer[127];
	Sdata[127] = temp;
	W25QXX_Write_Page(Sdata,0x100,128);//д������Ĵ�������
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WriteCMOSDataToW25QXX(void)
{
	uint8_t temp=0;
	uint16_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Erase_Sector(0);//sector0�ڲ������4096
	for(i=0;i<256;i++)
	{
		Sdata[i] = 0xFF;
	}
	for(i=0;i<256;i=i+2)
	{
		if((USB_Rx_Buffer[i+2] == 0xff)&&(USB_Rx_Buffer[i+3] == 0xff))
		{
			break;
		}
		else
		{
			Sdata[i]=USB_Rx_Buffer[i+2];
			Sdata[i+1]=USB_Rx_Buffer[i+3];

			temp += USB_Rx_Buffer[i+2];
			temp += USB_Rx_Buffer[i+3];
		}
		
	}
	Sdata[255] = temp;
	W25QXX_Write_Page(Sdata,0,256);//д��Ĵ�������
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WritePCBANumToW25QXX(void)
{
	uint8_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata,0x1000,130);
	W25QXX_Erase_Sector(1);//sector1�ڲ������4096
	for(i=0;i<64;i++)
	{
		Sdata[i]=USB_Rx_Buffer[i+2];
	}
	
	W25QXX_Write_Page(Sdata,0x1000,130);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WriteModuleNumToW25QXX(void)
{
	uint8_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata,0x1000,130);
	W25QXX_Erase_Sector(1);//sector1�ڲ������4096
	for(i=0;i<64;i++)
	{
		Sdata[64+i]=USB_Rx_Buffer[i+2];
	}
	
	W25QXX_Write_Page(Sdata,0x1000,130);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WriteColorNumToW25QXX(void)
{
	uint8_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	
	W25QXX_Read(Sdata,0x1000,130);
	W25QXX_Erase_Sector(1);//sector1�ڲ������4096
	for(i=0;i<2;i++)
	{
		Sdata[128+i]=USB_Rx_Buffer[i+2];
	}
	
	W25QXX_Write_Page(Sdata,0x1000,130);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WriteGAIN_AVRToW25QXX(void)
{
//	uint8_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	W25QXX_Erase_Sector(2);//sector2�ڲ������4096
//	for(i=0;i<2;i++)//���ģʽ
//	{
//		Sdata[i]=USB_Rx_Buffer[i+2];
//	}
	Sdata[0]=USB_Rx_Buffer[3];//��ԭ���Ĵ��ģʽ�ĳ�С��ģʽ
	Sdata[1]=USB_Rx_Buffer[2];
	
	W25QXX_Write_Page(Sdata,0x2000,2);
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
	SendOK();
}
void WriteGAIN_InforToW25QXX(void)
{
//	uint8_t i;
	
	PWRON_1117;
	delay_ms(2);
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x25,BUF,1);//Power on buffer
	delay_ms(1);
	W25QXX_Erase_Sector(3);//sector2�ڲ������4096
//	for(i=0;i<12;i++)//10---->12//���ģʽ
//	{
//		Sdata[i]=USB_Rx_Buffer[i+2];
//	}
	//��ԭ���Ĵ��ģʽ�ĳ�С��ģʽ
	Sdata[0]=USB_Rx_Buffer[5];		//length
	Sdata[1]=USB_Rx_Buffer[4];
	Sdata[2]=USB_Rx_Buffer[3];
	Sdata[3]=USB_Rx_Buffer[2];
	
	Sdata[4]=USB_Rx_Buffer[9];		//CRC16
	Sdata[5]=USB_Rx_Buffer[8];
	Sdata[6]=USB_Rx_Buffer[7];
	Sdata[7]=USB_Rx_Buffer[6];
	
	Sdata[8]=USB_Rx_Buffer[10];	//GAINͼ����+�������		
	Sdata[9]=USB_Rx_Buffer[11];
	
	Sdata[10]=USB_Rx_Buffer[12];	//darkline��־
	Sdata[11]=USB_Rx_Buffer[13];
	
	W25QXX_Write_Page(Sdata,0x3000,12);//10----------->12
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
//	SendOK();
}









#endif














