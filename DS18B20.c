#include "ds18b20.h"
#include "stm32f10x.h"
#include "stdio.h"
#include "utils.h"

#define EnableINT()  
#define DisableINT()

#define DS_PORT   GPIOA
#define DS_DQIO   GPIO_Pin_1

#define DS_RCC_PORT  RCC_APB2Periph_GPIOA

#define DS_PRECISION 0x7f   //精度配置寄存器 1f=9位; 3f=10位; 5f=11位; 7f=12位;
#define DS_AlarmTH  0x64
#define DS_AlarmTL  0x8a
#define DS_CONVERT_TICK 1000

#define ResetDQ() GPIO_ResetBits(DS_PORT,DS_DQIO)
#define SetDQ()  GPIO_SetBits(DS_PORT,DS_DQIO)
#define GetDQ()  GPIO_ReadInputDataBit(DS_PORT,DS_DQIO)
 
 

void Delay_us(u32 Nus) 
{  
 SysTick->LOAD=Nus*9;          //时间加载       
 SysTick->CTRL|=0x01;             //开始倒数     
 while(!(SysTick->CTRL&(1<<16))); //等待时间到达  
 SysTick->CTRL=0X00000000;        //关闭计数器 
 SysTick->VAL=0X00000000;         //清空计数器     	
} 

void Delay_us_bak(u32 i)
{
    for(; i > 0; i--)
    {
        __nop();
        __nop();
        __nop();
        __nop();
    }
}

unsigned char ResetDS18B20(void)
{
 unsigned char resport;
 SetDQ();
 Delay_us(50);
 
 ResetDQ();
 Delay_us(500);  //500us （该时间的时间范围可以从480到960微秒）
 SetDQ();
 Delay_us(40);  //40us
 //resport = GetDQ();
 while(GetDQ());
 Delay_us(500);  //500us
 SetDQ();
 return resport;
}

void DS18B20WriteByte(unsigned char Dat)
{
 unsigned char i;
 for(i=8;i>0;i--)
 {
   ResetDQ();     //在15u内送数到数据线上，DS18B20在15-60u读数
  Delay_us(5);    //5us
  if(Dat & 0x01)
   SetDQ();
  else
   ResetDQ();
  Delay_us(65);    //65us
  SetDQ();
  Delay_us(2);    //连续两位间应大于1us
  Dat >>= 1; 
 } 
}


unsigned char DS18B20ReadByte(void)
{
 unsigned char i,Dat;
 SetDQ();
 Delay_us(5);
 for(i=8;i>0;i--)
 {
   Dat >>= 1;
    ResetDQ();     //从读时序开始到采样信号线必须在15u内，且采样尽量安排在15u的最后
  Delay_us(5);   //5us
  SetDQ();
  Delay_us(5);   //5us
  if(GetDQ())
    Dat|=0x80;
  else
   Dat&=0x7f;  
  Delay_us(65);   //65us
  SetDQ();
 }
 return Dat;
}


void ReadRom(unsigned char *Read_Addr)
{
 unsigned char i;

 DS18B20WriteByte(ReadROM);
  
 for(i=8;i>0;i--)
 {
  *Read_Addr=DS18B20ReadByte();
  Read_Addr++;
 }
}


void DS18B20Init(unsigned char Precision,unsigned char AlarmTH,unsigned char AlarmTL)
{
	printf("ds18b20 dis int\n");
 DisableINT();
 printf("ds18b20 reset\n");
 ResetDS18B20();
 printf("ds18b20 write\n");
 DS18B20WriteByte(SkipROM); 
 DS18B20WriteByte(WriteScratchpad);
 DS18B20WriteByte(AlarmTL);
 DS18B20WriteByte(AlarmTH);
 DS18B20WriteByte(Precision);

 ResetDS18B20();
 DS18B20WriteByte(SkipROM); 
 DS18B20WriteByte(CopyScratchpad);
 printf("ds18b20 inting\n");
 EnableINT();

 while(!GetDQ());  //等待复制完成 ///////////
}


void DS18B20StartConvert(void)
{
 DisableINT();
 ResetDS18B20();
 DS18B20WriteByte(SkipROM); 
 DS18B20WriteByte(StartConvert); 
 EnableINT();
}

void DS18B20_Configuration(void)
{
 GPIO_InitTypeDef GPIO_InitStructure;
 
 RCC_APB2PeriphClockCmd(DS_RCC_PORT, ENABLE);

 GPIO_InitStructure.GPIO_Pin = DS_DQIO;
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; //开漏输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //2M时钟速度
 GPIO_Init(DS_PORT, &GPIO_InitStructure);
}


void ds18b20_init(void)
{
 DS18B20_Configuration();
 DS18B20Init(DS_PRECISION, DS_AlarmTH, DS_AlarmTL);
 DS18B20StartConvert();
}


float ds18b20_read(void)
{
	unsigned char DL, DH;
	unsigned short TemperatureData;
	float Temperature;

	DisableINT();
	DS18B20StartConvert();
	ResetDS18B20();
	DS18B20WriteByte(SkipROM); 
	DS18B20WriteByte(ReadScratchpad);
	DL = DS18B20ReadByte();
	DH = DS18B20ReadByte(); 
	EnableINT();

	TemperatureData = DH;
	TemperatureData <<= 8;
	TemperatureData |= DL;

	Temperature = (float)((float)TemperatureData * 0.0625);

	return  Temperature;
}
