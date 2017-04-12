#include <c8051f020.h>

sfr16 RCAP4 = 0xe4;	//Timer4捕获/重载
sfr16 T4 = 0xf4;	//Timer4 
sfr16 DAC0 = 0xd2;	//DACO数据

#define SYSCLK 22118400		//系统时钟频率
#define SAMPLERATE 10000

void main (void);
void SYSCLK_Init();
void Timer4_Init (int counts);
void Timer4_ISR (void);

void SYSCLK_Init()
{
	int i;
	OSCXCN=0x67;
	for(i=0;i<256;i++);
	while(!(OSCXCN&0x80));
	OSCICN=0x88;
}

void main (void) 
{
	WDTCN = 0xde;	//禁止看门狗
	WDTCN = 0xad;
	SYSCLK_Init();
	REF0CN = 0x03;	//允许内部参考电压并输入
	DAC0CN = 0x97;	//允许DACO，数据左对齐
					//使用Timer4作为启动采样触发
	Timer4_Init(SYSCLK/SAMPLERATE); 
	EA = 1;			//允许全局中断
 	while (1);
}

void Timer4_Init (int counts)
{
	T4CON = 0;
	CKCON |= 0x40;		//使用SYSCLK作为时钟源
	RCAP4 = -counts;	//重载值设置
	T4 = RCAP4;
	EIE2 |= 0x04;		//允许Timer4中断
	T4CON |= 0x04;		//启动Timer4
}

void Timer4_ISR (void) interrupt 16
{
	static unsigned phase = 0;
	DAC0 = phase;
	phase += 0x10;		//更新相位计数
	T4CON &= ~0x80;		//清除计数溢出标志
}