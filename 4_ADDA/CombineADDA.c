#include <c8051f020.h>

//AD 寄存器设置
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA 寄存器设置
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC0 = 0xd2;

#define SYSCLK 22118400
#define SAMPLERATE 10000

//数码管显示
unsigned char xdata seg _at_ (0x8000);
unsigned char xdata cs _at_ (0x8001); 
const unsigned char code segs[] = 
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE}; 

unsigned sample;	//采样值

void Delay(int k)
{
	int i;
	for (i = 0; i < k; ++i);
}

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

//AD 过程中的函数
void ADC0_Init (void)
{
	ADC0CN = 0x05;
	REF0CN = 0x03;
	AMX0SL = 0x00;
	ADC0CF = (SYSCLK/2500000) << 3; 
	ADC0CF &= ~0x07;
	EIE2 &= ~0x02;
	AD0EN = 1;
}
void Timer3_Init (int counts)
{
	TMR3CN = 0x02;
	TMR3RL = -counts;	//补码表示下的数值
	TMR3 = 0xffff;
	EIE2 &= ~0x01;		//禁止Timer3中断
	TMR3CN |= 0x04;
}
void ADC0_ISR (void) interrupt 15
{
	static unsigned int count = 0;
	count++;
	AD0INT = 0;			//清除 ADC 转换结束标志
	
	if (count >= SAMPLERATE / 2){  //降低sample改变的频率
		count = 0;
		sample = ADC0;	//保存转换结果
	}
}

//DA 过程中的函数
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
	DAC0 = sample;
	T4CON &= ~0x80;
}

void main (void) {
	WDTCN = 0xde;	//禁止看门狗
	WDTCN = 0xad;
	SYSCLK_Init ();
	Timer3_Init (SYSCLK/SAMPLERATE); 
	ADC0_Init ();
	PORT_Init();
	
	REF0CN = 0x03;	//允许内部参考电压并输入
	DAC0CN = 0x97;	//允许DACO，数据左对齐
					//使用Timer4作为启动采样触发
	Timer4_Init(SYSCLK/SAMPLERATE); 
	
	EA = 1;			//允许全局中断
	EIE2 |= 0x02;	//允许 ADC0 中断
	while (1) {
		unsigned char i;
		unsigned j;
		j = sample;
		for (i = 0; i < 4; ++i) {
			seg = segs[j & 0xF]; 
			Delay(1);
			cs = css[i];
			j = j >> 4;;
			Delay(1000);
		}
	}
}
