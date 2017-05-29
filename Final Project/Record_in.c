#include <c8051f020.h>
#include <stdio.h>

#define SYSCLK 22118400
#define SPICLK 2000000
#define NOKEY 255
#define SAMPLERATE 8000		//录音采样频率为 8k
#define length 16384		//记录的长度
typedef unsigned char uchar;

long long int add;

//AD 寄存器设置
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA 寄存器设置
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//音频输出由 DAC1 驱动

unsigned sample;

unsigned int xdata record[length];
unsigned int itr;

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
	XBR0 = 0x06; // 允许 SPI 和 UART
	P0MDOUT = 0xC0; // 设置总线相关端口为推挽输出 P0.6 和 P0.7
	P0MDOUT |= 0x15; // TX, SCK, MOSI 设置为推挽输出
	P74OUT = 0x20; // p6.7 用作片选信号
	EMI0CF = 0x1F; // 非复用总线，不使用内部 XRAM
	XBR2 = 0x42; // 使用 P0-P3 作为总线，允许XBR
	P1MDOUT = 0xFF; // 高位地址
	P2MDOUT = 0xFF; // 低位地址
	P3MDOUT = 0xFF; // 数据总线
	P74OUT |= 0x80;	 //推挽输出
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
	for(i=0;i<10*num;i++);	//注意延时长度 vs 10000
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
void Delay(int num)
{
	int i;
	for(i=0;i<num;i++);
	return;
}
uchar getkey()
{
	uchar i;
	uchar key;
	const uchar code dec[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0};
	const uchar code trans[] = {0xC, 9, 5, 1, 0xD, 0, 6, 2, 0xE, 0xA, 7, 3, 0xF, 0xB, 8, 4};

	P4 = 0x0F;
	Delay(100);
	i = ~P4 & 0x0F;
	if (i == 0) return NOKEY;
	key = dec[i] * 4;
	Delay(1000);
	P4 = 0xF0;
	Delay(100);
	i = ~P4;
	i >>= 4;
	if (i == 0) return NOKEY;
	key = key + dec[i];
	key = trans[key];
	return key;
}

void main(void){
	uchar temp = NOKEY;		// 初始化NOKEY, 否则0与按键冲突
	uchar ctrl = NOKEY;
    int i, j;
	WDTCN = 0xde;
	WDTCN = 0xad;   // 禁止看门狗
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
	AD0EN = 0;

    itr = 0;
    add = 0;        // 每段数据长32KB，录制时注意改变起始地址位置！
	
	// REFRESH WHOLE FLASH
    P6 = 0x00; // 片选有效
	Timer0_us(1);
	SPI_Write(0x06); // 写入允许命令
	Timer0_us(1);
	P6 = 0x80; // 片选无效
	Timer0_us(1);
    P6 = 0x0; // 再次设置片选
	Timer0_us(1);
	SPI_Write(0x60); // 整片清除命令
	Timer0_us(1);
	P6 = 0x80; // 片选无效

    // 录音并存入FLASH
	for(i = 0; i < 6; i++){

		// 采集按键信息
		while(ctrl == temp){
			ctrl = getkey();
		}
		itr = 0;
        AD0EN = 1;		        // ADC打开
        while(itr < length);
        AD0EN = 0;			    // ADC关闭
        DAC1CN = 0x97;		    // DAC打开
        itr = 0;
		while(itr < length);
		DAC1CN = 0x17;		    // DAC关闭

        // WRITE FROM XDATA TO FLASH
        for (j=0; j<length; j++){
            P6 = 0x00; // 片选有效
			Timer0_us(1);
			SPI_Write(0x06); // 写入允许命令
			Timer0_us(1);
			P6 = 0x80; // 片选无效
			Timer0_us(1);
			P6 = 0x0; // 再次设置片选
			Timer0_us(1);
			SPI_Write(0x02); // 写命令
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 位地址
			SPI_Write((record[j] & 0x0000FF00)>> 8);     // 写入高位
			Timer0_us(1);
			P6 = 0x80; // 片选无效
			busywait(); // 读取状态等待写入完成
			add++;

			P6 = 0x00; // 片选有效
			Timer0_us(1);
			SPI_Write(0x06); // 写入允许命令
			Timer0_us(1);
			P6 = 0x80; // 片选无效
			Timer0_us(1);
			P6 = 0x0; // 再次设置片选
			Timer0_us(1);
			SPI_Write(0x02); // 写命令
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 位地址
			SPI_Write(record[j] & 0x00FF);     // 写入低位
			Timer0_us(1);
            P6 = 0x80; // 片选无效
			busywait(); // 读取状态等待写入完成
			add++;
        }

		ctrl = NOKEY;

	}
}
