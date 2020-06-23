#include "w25qxx.h" 


extern 	void IwdgFeed(void);

 
u16 W25QXX_TYPE=W25Q128;	//默认是W25Q128

//4Kbytes为一个Sector
//16个扇区为1个Block
//W25Q128
//容量为16M字节,共有128个Block,4096个Sector 
													 
//初始化SPI FLASH的IO口
//void W25QXX_Init(void)
//{ 
//	SPI1_SetSpeed(SPI_BaudRatePrescaler_4);		//设置为21M时钟,高速模式 
//	W25QXX_TYPE=W25QXX_ReadID();	//读取FLASH ID.
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
//读取W25QXX的状态寄存器
//BIT7  6   5   4   3   2   1   0
//SPR   RV  TB BP2 BP1 BP0 WEL BUSY
//SPR:默认0,状态寄存器保护位,配合WP使用
//TB,BP2,BP1,BP0:FLASH区域写保护设置
//WEL:写使能锁定
//BUSY:忙标记位(1,忙;0,空闲)
//默认:0x00

uint8_t W25QXX_ReadSR(void)   
{  
	uint8_t receData=0;   
	uint8_t sendData=W25X_ReadStatusReg;
	FLASH_SPI_CS_EN();                      		 //使能器件   
	Dev_Flash_SPISendData(&sendData,1,100);    		 //发送读取状态寄存器命令    
	Dev_Flash_SPIReceiveData(&receData,1,100);       //读取一个字节  
	FLASH_SPI_CS_DIS();                              //取消片选     
	return receData;   
} 
//写W25QXX状态寄存器
//只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写!!!
void W25QXX_Write_SR(uint8_t sr)   
{

	uint8_t sendData=W25X_WriteStatusReg;
	FLASH_SPI_CS_EN();                            //使能器件   
	Dev_Flash_SPISendData(&sendData,1,100);   //发送写取状态寄存器命令    
	sendData = sr;
	Dev_Flash_SPISendData(&sendData,1,100);               		//写入一个字节  
	FLASH_SPI_CS_DIS();                            //取消片选     	      
}   
//W25QXX写使能	
//将WEL置位   
void W25QXX_Write_Enable(void)   
{
	uint8_t sendData=W25X_WriteEnable;
	FLASH_SPI_CS_EN();                            //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);       //发送写使能  
	FLASH_SPI_CS_DIS();                           //取消片选     	      
} 
//W25QXX写禁止	
//将WEL清零  
void W25QXX_Write_Disable(void)   
{  
	uint8_t sendData=W25X_WriteDisable;
	FLASH_SPI_CS_EN();                           //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);     //发送写禁止指令    
	FLASH_SPI_CS_DIS();                            //取消片选     	      
} 		
//读取芯片ID
//返回值如下:				   
//0XEF13,表示芯片型号为W25Q80  
//0XEF14,表示芯片型号为W25Q16    
//0XEF15,表示芯片型号为W25Q32  
//0XEF16,表示芯片型号为W25Q64 
//0XEF17,表示芯片型号为W25Q128 	  
u16 W25QXX_ReadID(void)
{
	uint8_t sendData[4]={0x90,0,0,0};
	uint8_t receData=0;  
	u16 Temp = 0;	  
	FLASH_SPI_CS_EN(); 				    
	Dev_Flash_SPISendData(sendData,4,100); //发送读取ID命令	    
 			   
	Dev_Flash_SPIReceiveData(&receData,1,100);  
	Temp|=(uint16_t)receData<<8;
	Dev_Flash_SPIReceiveData(&receData,1,100);  
	Temp|=receData;	 
	FLASH_SPI_CS_DIS();				    
	return Temp;
}   		    
//读取SPI FLASH  
//在指定地址开始读取指定长度的数据
//pBuffer:数据存储区
//ReadAddr:开始读取的地址(24bit)
//NumByteToRead:要读取的字节数(最大65535)
void W25QXX_Read(uint8_t* pBuffer,u32 ReadAddr,u16 NumByteToRead)   
{ 
 	
	uint8_t sendData=W25X_ReadData;

	FLASH_SPI_CS_EN();                            //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);        //发送读取命令   
    sendData = ((uint8_t)((ReadAddr)>>16));  //发送24bit地址  
    Dev_Flash_SPISendData(&sendData,1,100);  
    sendData = ((uint8_t)((ReadAddr)>>8));  
	Dev_Flash_SPISendData(&sendData,1,100); 
    sendData = ((uint8_t)ReadAddr);   
	Dev_Flash_SPISendData(&sendData,1,100); 

    Dev_Flash_SPIReceiveData(pBuffer,NumByteToRead,1000);  ;   //读数  
    
	FLASH_SPI_CS_DIS(); 				    	      
}  
//SPI在一页(0~65535)内写入少于256个字节的数据
//在指定地址开始写入最大256字节的数据
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!	 
void W25QXX_Write_Page(uint8_t* pBuffer,u32 WriteAddr,u16 NumByteToWrite)
{
 
	uint8_t sendData;
    W25QXX_Write_Enable();                  //SET WEL 
	FLASH_SPI_CS_EN();                           //使能器件   
	sendData=W25X_PageProgram;
	Dev_Flash_SPISendData(&sendData,1,100);        //发送读取命令   
    sendData = ((uint8_t)((WriteAddr)>>16));  //发送24bit地址  
    Dev_Flash_SPISendData(&sendData,1,100);  
    sendData = ((uint8_t)((WriteAddr)>>8));  
	Dev_Flash_SPISendData(&sendData,1,100); 
    sendData = ((uint8_t)WriteAddr);   
	Dev_Flash_SPISendData(&sendData,1,100);   
	
	Dev_Flash_SPISendData(pBuffer,NumByteToWrite,1000);  

	FLASH_SPI_CS_DIS();                           //取消片选 
	W25QXX_Wait_Busy();					   //等待写入结束
} 
//无检验写SPI FLASH 
//必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
//具有自动换页功能 
//在指定地址开始写入指定长度的数据,但是要确保地址不越界!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)
//NumByteToWrite:要写入的字节数(最大65535)
//CHECK OK
void W25QXX_Write_NoCheck(uint8_t* pBuffer,u32 WriteAddr,u16 NumByteToWrite)   
{ 			 		 
	u16 pageremain;	   
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{	   
		W25QXX_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	};	    
} 
//写SPI FLASH  
//在指定地址开始写入指定长度的数据
//该函数带擦除操作!
//pBuffer:数据存储区
//WriteAddr:开始写入的地址(24bit)						
//NumByteToWrite:要写入的字节数(最大65535)   
uint8_t W25QXX_BUFFER[4096];		 
void W25QXX_Write(uint8_t* pBuffer,u32 WriteAddr,u32 NumByteToWrite)   
{ 
	u32 secpos;
	u16 secoff;
	u16 secremain;	   
 	u16 i;    
	uint8_t * W25QXX_BUF;	  
   	W25QXX_BUF=W25QXX_BUFFER;	     
 	secpos=WriteAddr/4096;//扇区地址  
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//不大于4096个字节
	while(1) 
	{	
		IwdgFeed();
		//CS0_High;
		W25QXX_Read(W25QXX_BUF,secpos*4096,4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(W25QXX_BUF[secoff+i]!=0xFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			//CS0_High;
			W25QXX_Erase_Sector(secpos);//擦除这个扇区
			for(i=0;i<secremain;i++)	   //复制
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];	  
			}
			//CS0_High;
			W25QXX_Write_NoCheck(W25QXX_BUF,secpos*4096,4096);//写入整个扇区  

		}
		else 
		{
			//CS0_High;
			W25QXX_Write_NoCheck(pBuffer,WriteAddr,secremain);
		}//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumByteToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuffer+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移	   
		   	NumByteToWrite-=secremain;				//字节数递减
			if(NumByteToWrite>4096)secremain=4096;	//下一个扇区还是写不完
			else secremain=NumByteToWrite;			//下一个扇区可以写完了
		}	 
	};	
	IwdgFeed();
}
//擦除整个芯片		  
//等待时间超长...
void W25QXX_Erase_Chip(void)   
{     
	uint8_t sendData=W25X_ChipErase;
	IwdgFeed();
    W25QXX_Write_Enable();                  //SET WEL 
    W25QXX_Wait_Busy();   
  	FLASH_SPI_CS_EN();                             //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);        //发送片擦除命令  
	FLASH_SPI_CS_DIS();                             //取消片选     	  
	IwdgFeed();
	W25QXX_Wait_Busy();   				   //等待芯片擦除结束
	IwdgFeed();
}   
//擦除一个扇区
//Dst_Addr:扇区地址 根据实际容量设置
//擦除一个山区的最少时间:150ms
void W25QXX_Erase_Sector(u32 Dst_Addr)   
{  
	//监视falsh擦除情况,测试用   
 	//printf("fe:%x\r\n",Dst_Addr);	  
 	uint8_t sendData=W25X_SectorErase;
    Dst_Addr*=4096;
    W25QXX_Write_Enable();                  //SET WEL 	 
    W25QXX_Wait_Busy();   
    FLASH_SPI_CS_EN();                            //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);       //发送扇区擦除指令 
    sendData = ((uint8_t)((Dst_Addr)>>16));  //发送24bit地址    
    Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)((Dst_Addr)>>8)); 
	Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)Dst_Addr);  
	Dev_Flash_SPISendData(&sendData,1,100);
	FLASH_SPI_CS_DIS();                             //取消片选     	      
    W25QXX_Wait_Busy();   				   //等待擦除完成
}  
//擦除一个block_64K
//Dst_Addr:block地址 根据实际容量设置
void W25QXX_Erase_Block(u32 Dst_Addr)   
{  
	//监视falsh擦除情况,测试用   
 	//printf("fe:%x\r\n",Dst_Addr);	  
 	uint8_t sendData=W25X_BlockErase;
    Dst_Addr*=64*1024;
    W25QXX_Write_Enable();                  //SET WEL 	 
    W25QXX_Wait_Busy();   
    FLASH_SPI_CS_EN();                            //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);       //发送扇区擦除指令 
    sendData = ((uint8_t)((Dst_Addr)>>16));  //发送24bit地址    
    Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)((Dst_Addr)>>8)); 
	Dev_Flash_SPISendData(&sendData,1,100);
    sendData = ((uint8_t)Dst_Addr);  
	Dev_Flash_SPISendData(&sendData,1,100);
	FLASH_SPI_CS_DIS();                             //取消片选     	      
    W25QXX_Wait_Busy();   				   //等待擦除完成
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
			sectorCnt = 16 - Dst_Addr % blockSize / sectorSize;	// top area, earse by sector,顶部不足一个块的区域
			secStrAddr = Dst_Addr / sectorSize;
			for(i=0;i<sectorCnt;i++)
			{
				IwdgFeed();
				W25QXX_Erase_Sector(secStrAddr+i);
				//printf("erase sector\n");
			}
		
			blockAddr = Dst_Addr / blockSize + 1;	// +1顶部有一部分不是块擦除
			blockCnt = (numByte -sectorCnt*sectorSize) / blockSize;
			for(i = 0; i<blockCnt;i++ )		// erase block,擦除整块节省时间
			{
				IwdgFeed();
				W25QXX_Erase_Block(blockAddr+i);
				IwdgFeed();
			}
			if(((numByte -sectorCnt*sectorSize) % blockSize) != 0)		// erase size != 64K * n,底部不足一个块的区域
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

//等待空闲
void W25QXX_Wait_Busy(void)   
{   
	//while((W25QXX_ReadSR()&0x01)==0x01);   // 等待BUSY位清空
	while(1)
	{
		if(!(W25QXX_ReadSR()&0x01)==0x01)
			break;
	}
}  
//进入掉电模式
void W25QXX_PowerDown(void)   
{ 
	uint8_t sendData=W25X_PowerDown;

  	FLASH_SPI_CS_EN();                           //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);         //发送掉电命令  
	FLASH_SPI_CS_DIS();                            //取消片选     	      
    HAL_Delay(3);                               //等待TPD  
}   
//唤醒
void W25QXX_WAKEUP(void)   
{  
	uint8_t sendData=W25X_ReleasePowerDown;

  	FLASH_SPI_CS_EN();                           //使能器件   
    Dev_Flash_SPISendData(&sendData,1,100);    //  send W25X_PowerDown command 0xAB    
	FLASH_SPI_CS_DIS();                             //取消片选     	      
    HAL_Delay(3);                               //等待TRES1
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
	
//	W25QXX_Read(Sdata+2,0x2000,2);//大端模式
	
	W25QXX_Read(Sdata+2,0x2001,1);//小端模式
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
	
//	W25QXX_Read(Sdata+2,0x3000,12);//10----->12   小端模式
	
	//由原来的大端模式改成小端模式
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
//	length = ((Sdata[0]<<24)|(Sdata[1]<<16)|(Sdata[2]<<8)|(Sdata[3]))+1;//小端模式
	length = ((Sdata[3]<<24)|(Sdata[2]<<16)|(Sdata[1]<<8)|(Sdata[0]))+1;//大端模式
	
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
	
	W25QXX_Erase_Sector(0);//sector0内部会乘以4096
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
	W25QXX_Write_Page(Sdata,0,256);//写入寄存器参数
	
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
	W25QXX_Write_Page(Sdata,0x100,128);//写入重配寄存器参数
	
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
	
	W25QXX_Erase_Sector(0);//sector0内部会乘以4096
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
	W25QXX_Write_Page(Sdata,0,256);//写入寄存器参数
	
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
	W25QXX_Erase_Sector(1);//sector1内部会乘以4096
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
	W25QXX_Erase_Sector(1);//sector1内部会乘以4096
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
	W25QXX_Erase_Sector(1);//sector1内部会乘以4096
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
	W25QXX_Erase_Sector(2);//sector2内部会乘以4096
//	for(i=0;i<2;i++)//大端模式
//	{
//		Sdata[i]=USB_Rx_Buffer[i+2];
//	}
	Sdata[0]=USB_Rx_Buffer[3];//由原来的大端模式改成小端模式
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
	W25QXX_Erase_Sector(3);//sector2内部会乘以4096
//	for(i=0;i<12;i++)//10---->12//大端模式
//	{
//		Sdata[i]=USB_Rx_Buffer[i+2];
//	}
	//由原来的大端模式改成小端模式
	Sdata[0]=USB_Rx_Buffer[5];		//length
	Sdata[1]=USB_Rx_Buffer[4];
	Sdata[2]=USB_Rx_Buffer[3];
	Sdata[3]=USB_Rx_Buffer[2];
	
	Sdata[4]=USB_Rx_Buffer[9];		//CRC16
	Sdata[5]=USB_Rx_Buffer[8];
	Sdata[6]=USB_Rx_Buffer[7];
	Sdata[7]=USB_Rx_Buffer[6];
	
	Sdata[8]=USB_Rx_Buffer[10];	//GAIN图张数+排列组合		
	Sdata[9]=USB_Rx_Buffer[11];
	
	Sdata[10]=USB_Rx_Buffer[12];	//darkline标志
	Sdata[11]=USB_Rx_Buffer[13];
	
	W25QXX_Write_Page(Sdata,0x3000,12);//10----------->12
	
	IIC_WriteByte(Open_I2Cx,RT9367_addr,0x24,BUF,1);//Power off buffer
	PWROFF_1117;
//	SendOK();
}









#endif














