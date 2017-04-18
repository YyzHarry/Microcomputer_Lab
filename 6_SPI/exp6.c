#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define SPICLK 1000000	// vs 100000
#define BAUDRATE 1200 // 波特率

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; // 启动外部晶振
	for (i=0; i < 256; i++); // 延时一段时间
	while (!(OSCXCN & 0x80)); // 等待振荡稳定
	OSCICN = 0x88; // 使用外部振荡器
}

void PORT_Init()
{
	XBR0 = 0x06; // 允许SPI 和UART
	XBR2 = 0x40; // 允许XBR
	P0MDOUT |= 0x15; // TX, SCK, MOSI 设置为推挽输出
	P74OUT = 0x20; // p6.7 用作片选信号
}

void UART0_Init (void)
{
	SCON0 = 0x50; // SCON0: 串口方式1 使能RX
	TMOD = 0x20; // 定时器 1 采用自装载模式
	TH1 = -(SYSCLK/BAUDRATE/16/12); // Timer1 载入值
	TR1 = 1; // 启动 Timer1
	PCON |= 0x80; // SMOD0 = 1
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
	for(i=0;i<1000*num;i++);
	return;
}

void main()
{
	unsigned char v;
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init(); 			// 系统时钟设置
	PORT_Init(); 			// IO 管脚设置
	P6 = 0x80; 				// 片选无效
	UART0_Init(); 			// 初始化串口
	SPI0_Init(); 			// 初始化SPI
	Timer0_us(1000); 		// 延时
	P6 = 0x00; 				// 片选有效
	Timer0_us(1); 			// 延时
	TI0 = 1;				// 开始发送数据。但是为什么呢？？中断？？
	SPI_Write(0x9F); 		// 写入读JEDEC ID 命令
	v = SPI_Write(0x00); 	// 从SPI 设备读出厂商标识
	printf("Manufacturer ID: %bx\r\n", v);
	v = SPI_Write(0x00); 	// 从SPI 设备读出存储器类型
	printf("Memory Type ID: %bx\r\n", v);
	v = SPI_Write(0x00); 	// 从SPI 设备读出容量
	printf("Capacity ID: %bx\r\n", v);
	Timer0_us(1); 			// 延时
	P6 = 0x80; 				// 片选无效
	while(1);
}