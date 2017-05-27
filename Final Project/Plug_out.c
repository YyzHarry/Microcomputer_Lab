#include <c8051f020.h>
#include <stdio.h>

#define SYSCLK 22118400
#define SPICLK 2000000
#define SAMPLERATE 8000		//录音采样频率为 8k
#define length 16384		//记录的长度

//AD 寄存器设置
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA 寄存器设置
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//音频输出由 DAC1 驱动

// 音频设置，从FLASH中取出已存好的数据（1~6楼提示音），DAC播放
unsigned int audio_first[length];
unsigned int audio_second[length];
unsigned int audio_third[length];
unsigned int audio_fourth[length];
unsigned int audio_fifth[length];
unsigned int audio_sixth[length];

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

void main(void){
    int i, j;
	unsigned char c, v, v1;
	long int add;
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init();
	Timer3_Init(SYSCLK/SAMPLERATE);
	ADC0_Init();
	PORT_Init();

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

    itr = 0;
    add = 0;        // 每段数据长4KB，录制时注意改变起始地址位置！


	for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_first[i] = SPI_Write(0x00);            // 读数据
        add++;
    }
    for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_second[i] = SPI_Write(0x00);            // 读数据
        add++;
    }
    for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_third[i] = SPI_Write(0x00);            // 读数据
        add++;
    }
    for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_fourth[i] = SPI_Write(0x00);            // 读数据
        add++;
    }
    for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_fifth[i] = SPI_Write(0x00);            // 读数据
        add++;
    }
    for (i=0; i<length; i++){
        P6 = 0x00;                      // 片选有效
        Timer0_us(1);
		SPI_Write(0x03);                // 读数据命令
		SPI_Write((add & 0x00FF0000) >> 16);
		SPI_Write((add & 0x0000FF00) >> 8);
		SPI_Write(add & 0x00FF);        // 24 位地址
		Timer0_us(1);

        audio_sixth[i] = SPI_Write(0x00);            // 读数据
        add++;
    }

}
