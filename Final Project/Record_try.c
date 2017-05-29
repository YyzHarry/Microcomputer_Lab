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
#define SPICLK 2000000

unsigned sample;
long int add, add2;
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

void PORT_Init(void)
{
	EMI0CF = 0x1F;
	XBR0 = 0x06; // 允许SPI 和UART
	XBR2 = 0x42;
	P0MDOUT = 0xC0;
	P1MDOUT = 0xFF;
	P2MDOUT = 0xFF;
	P3MDOUT = 0xFF;
	P0MDOUT |= 0x15; // TX, SCK, MOSI 设置为推挽输出
	P74OUT = 0x20; // p6.7 用作片选信号
}
void SPI0_Init()
{
	SPI0CFG = 0x07; // 8 位帧大小
	SPI0CN = 0x03; // 主模式，允许SPI 设备
	SPI0CKR = SYSCLK/2/SPICLK;
}
unsigned char SPI_Write(unsigned char v)
{
	SPIF = 0; // 清除中断标志
	SPI0DAT = v; // 数据寄存器赋值
	while (SPIF == 0); // 等待发送完成
	return SPI0DAT; // 同时把接收到的结果返回
}
void Timer0_us(int num)
{
	int i;
	for(i=0;i<100*num;i++);	//注意延时长度 vs 10000
	return;
}
void busywait()	 //在芯片正处于擦除、写入等操作时等待其完成
{
    P6 = 0x00; // 片选有效
	Timer0_us(1);	// 写在里面？？
	while(1){
		unsigned char v;
		SPI_Write(0x05);	// 读状态字命令
		v = SPI_Write(0x00);
		v = v & 0x1;	// 读取S0
		if(v == 0)
			break;
	}
	P6 = 0x80;  // 片选无效
	Timer0_us(1);
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
	//record[itr] = ADC0;	 //	sample
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
	int i, j, k;
	unsigned char low, high;
	add = 0;
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
	
	P6 = 0x80; 				// 片选无效
	SPI0_Init(); 			// 初始化SPI
	//Timer0_us(1000); 		// 延时
	//P6 = 0x00; 		// 片选有效
	//P6 = 0x80; 		// 片选无效
	EA = 1;
	EIE2 |= 0x02;

	P74OUT = 0x30;      // P6_out
	
    itr = 0;

	for (i=0;i<6;i++){
		add = 0;        // 每段数据长32KB，录制时注意改变起始地址位置！

            P6 = 0x00; // 片选有效
			Timer0_us(1);
			SPI_Write(0x03); // 读数据命令
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 位地址
			Timer0_us(1);
			       
			for (k=0; k<length; k++){
				for (j=0; j<2; j++){
					if (!j){
						high = SPI_Write(0x00);         // 读高位
	                	record[k] = (high << 8);
					}
					else{
						low = SPI_Write(0x00);         // 读低位
						record[k] += low;
					}
					Timer0_us(1);
	            }
			}
			P6 = 0x80; 			   // 片选无效

            DAC1CN = 0x97;		   // DAC打开
			itr = 0;
			while(itr < length);
			DAC1CN = 0x17;		   // DAC关闭
	}
}
