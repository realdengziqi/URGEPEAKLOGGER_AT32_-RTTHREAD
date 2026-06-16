#include "lcd_init.h"
#include "delay.h"

void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4,GPIO_PIN_SET);

}


/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{	
	u8 i;
	for(i=0;i<8;i++)
	{			  
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
	LCD_Writ_Bus(dat>>8);
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}


/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	if(USE_HORIZONTAL==0)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==1)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1+80);
		LCD_WR_DATA(y2+80);
		LCD_WR_REG(0x2c);//储存器写
	}
	else if(USE_HORIZONTAL==2)
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1);
		LCD_WR_DATA(x2);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
	else
	{
		LCD_WR_REG(0x2a);//列地址设置
		LCD_WR_DATA(x1+80);
		LCD_WR_DATA(x2+80);
		LCD_WR_REG(0x2b);//行地址设置
		LCD_WR_DATA(y1);
		LCD_WR_DATA(y2);
		LCD_WR_REG(0x2c);//储存器写
	}
}

void LCD_Init(void)
{
LCD_GPIO_Init();//初始化GPIO
	LCD_RES_Set();
	delay_ms(50);
	LCD_RES_Clr();//复位
	delay_ms(50);
	LCD_RES_Set();
	delay_ms(120);
	
	LCD_BLK_Set();
	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x11); //Sleep out 
	delay_ms(120);              //Delay 120ms 
	//************* Start Initial Sequence **********// 
	LCD_WR_REG(0x36);
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0);
      
	LCD_WR_REG(0x3A);     
	LCD_WR_DATA8(0x05);   

	LCD_WR_REG(0xB2);     
	LCD_WR_DATA8(0x1F);   
	LCD_WR_DATA8(0x1F);   
	LCD_WR_DATA8(0x00);   
	LCD_WR_DATA8(0x33);   
	LCD_WR_DATA8(0x33);  

	LCD_WR_REG(0xB7);    
	LCD_WR_DATA8(0x00);  //VGH=12.2V,VGL=-7.16V 

	LCD_WR_REG(0xBB);     
	LCD_WR_DATA8(0x3F);  

	LCD_WR_REG(0xC0);               
	LCD_WR_DATA8(0x2C); 

	LCD_WR_REG(0xC2);     
	LCD_WR_DATA8(0x01);   

	LCD_WR_REG(0xC3);     
	LCD_WR_DATA8(0x0F); //4.3V 

	LCD_WR_REG(0xC4);     
	LCD_WR_DATA8(0x20);   //VDV, 0x20:0v

	LCD_WR_REG(0xC6);     
	LCD_WR_DATA8(0x13);     

	LCD_WR_REG(0xD0);     
	LCD_WR_DATA8(0xA4);   
	LCD_WR_DATA8(0xA1);   

	LCD_WR_REG(0xD6);     
	LCD_WR_DATA8(0xA1);   //sleep in后，gate输出为GND

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x06);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x2E);
	LCD_WR_DATA8(0x43);
	LCD_WR_DATA8(0x45);
	LCD_WR_DATA8(0x38);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x25);
	LCD_WR_DATA8(0x29);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x0A);
	LCD_WR_DATA8(0x08);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x23);
	LCD_WR_DATA8(0x2E);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x44);
	LCD_WR_DATA8(0x3A);
	LCD_WR_DATA8(0x16);
	LCD_WR_DATA8(0x17);
	LCD_WR_DATA8(0x26);
	LCD_WR_DATA8(0x2C);  

	LCD_WR_REG(0xE4);    
	LCD_WR_DATA8(0x1D); //使用240根gate  (N+1)*8
	LCD_WR_DATA8(0x00); //设定gate起点位置
	LCD_WR_DATA8(0x00); //当gate没有用完时，bit4(TMG)设为0

	LCD_WR_REG(0x21);     
	LCD_WR_REG(0x11); //Sleep out 
	delay_ms(120);              //Delay 120ms 
	LCD_WR_REG(0x29);     
}








