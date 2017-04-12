#include <c8051f020.h>
//AD 寄存器设置
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA 寄存器设置
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//音频输出由 DAC1 驱动

#define SYSCLK 22118400
#define SAMPLERATE 8000		//录音采样频率为 8k
#define length 16384		//记录的长度

unsigned sample;

unsigned int xdata record[length];
unsigned itr;

void SYSCLK_Init()
{
	int i;
	OSCXCN=0x67;
	for(i=0;i<256;i++);
	while(!(OSCXCN&0x80));
	OSCICN=0x88;
}

void PORT_Init (void)
{	
	EMI0CF = 0x1F; 
	XBR2 = 0x42;
	P0MDOUT = 0xC0; 
	P1MDOUT = 0xFF; 
	P2MDOUT = 0xFF; 
	P3MDOUT = 0xFF;	
}

//AD
void ADC0_Init (void)
{
	ADC0CN = 0x05;
	REF0CN = 0x03;
	AMX0SL = 0x01;	 // 选择AIN1作为输入
	ADC0CF = (SYSCLK/2500000) << 3; 
	ADC0CF &= ~0x07;
	EIE2 &= ~0x02;
	AD0EN = 1;
}

void Timer3_Init (int counts)
{
	TMR3CN = 0x02;
	TMR3RL = -counts;
	TMR3 = 0xffff;
	TMR3CN |= 0x04;
}

void Timer3_ISR (void) interrupt 14
{
	//record[itr] = sample;
	itr++;
	TMR3CN &= 0x7F;
}

void ADC0_ISR (void) interrupt 15	//ADC转换完毕中断
{
	record[itr] = ADC0;	 //	sample
	AD0INT = 0;
}

//DA
void Timer4_Init (int counts)
{
	T4CON = 0;
	CKCON |= 0x40;
	RCAP4 = -counts;
	T4 = RCAP4;
	EIE2 |= 0x04;
	T4CON |= 0x04;				
}

void Timer4_ISR (void) interrupt 16
{
	DAC1 = record[itr];	
	itr++;
	T4CON &= ~0x80;
}

void main (void) {
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init ();
	Timer3_Init(SYSCLK/SAMPLERATE); 
	ADC0_Init ();
	PORT_Init ();
	
	REF0CN = 0x03;
	DAC1CN = 0x97;
	DAC1CN = 0x17;
	Timer4_Init(SYSCLK/SAMPLERATE); 
	
	EA = 1;
	EIE2 |= 0x02;
	
    itr = 0;
	//目前reset重置
	while (1) {
		if(itr >= length)
		{
			AD0EN = 0;			   //ADC关闭
			DAC1CN = 0x97;		   //DAC打开
			itr = 0;
			while(itr < length);
			DAC1CN = 0x17;		   //DAC关闭
			//while(1);

			itr = 0;
			AD0EN = 1;		//ADC打开
		}
		
	}
}
