#include <c8051f020.h>
#include<stdio.h>
#define SYSCLK 22118400
#define SPICLK 2000000
#define BAUDRATE 1200 //波特率 9600?

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
	for(i=0;i<1000*num;i++);	//注意延时长度 vs 10000
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

void main()
{
	int j;
	unsigned char c, v, v1;
	long int add;
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init(); 			// 系统时钟设置
	PORT_Init(); 			// IO 管脚设置
	P6 = 0x80; 				// 片选无效
	UART0_Init(); 			// 初始化串口
	SPI0_Init(); 			// 初始化SPI
	Timer0_us(1000); 		// 延时
	P6 = 0x00; 		// 片选有效
	Timer0_us(1);	// 延时
	
	TI0 = 1;		   //初始化代码只需要在最外部运行即可!!
	SPI_Write(0x9F); 	 // 写入读 JEDEC ID 命令
	v = SPI_Write(0x00); // 从SPI设备读出厂商标识
	printf("Manufacturer ID: %bx\r\n", v);
	v = SPI_Write(0x00); // 从SPI设备读出存储器类型
	printf("Memory Type ID: %bx\r\n", v);
	v = SPI_Write(0x00); // 从SPI设备读出容量：理解为写入1个无关数据，MOSI和MISO的同步性
	printf("Capacity ID: %bx\r\n", v);
	Timer0_us(1); 	// 延时
	P6 = 0x80; 		// 片选无效
	
	while(1) {
		do { // 过滤掉回车和空格字符，读取命令字节
			c = getchar();
		} while ((c == ' ') || (c == '\r') || (c == '\n'));
		scanf("%lx", &add); // 读入地址值
		switch (c) {
			case 'd': // Display
				P6 = 0x00; // 片选有效
				Timer0_us(1);
				SPI_Write(0x03); // 读数据命令
				SPI_Write((add & 0x00FF0000) >> 16);
				SPI_Write((add & 0x0000FF00) >> 8);
				SPI_Write(add & 0x00FF); // 24 位地址
				Timer0_us(1);
					 
				//TI0 = 1;		//不需要在运行中加入初始化代码!				
				for(j = 0; j < 16; j++){
					v = SPI_Write(0x00); // 读数据
					printf("%02bx", v);
					if(j != 15)
						printf(" ");
					if(j == 7)
						printf("- ");
					if(j == 15)	
						printf("\r\nDisplay %lx OK\r\n", add);
				}
				Timer0_us(1);
				busywait();
				P6 = 0x80; // 片选无效
				break;
			case 'w': // Write
				scanf("%bx", &v1); // 继续读入要写入的字节内容
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
				SPI_Write(v1); // 数据
				Timer0_us(1);
				P6 = 0x80; // 片选无效
				Timer0_us(1);
				 
				//TI0 = 1;	// 不需要在运行中加入初始化代码
				busywait(); // 读取状态等待写入完成
				printf("\r\nWrite %lx %bx OK\r\n", add, v1);
				break;
			case 'c': // Clear
				P6 = 0x00; // 片选有效				Timer0_us(1);
				SPI_Write(0x06); // 写入允许命令
				Timer0_us(1);
				P6 = 0x80; // 片选无效
				Timer0_us(1);

				P6 = 0x0; // 再次设置片选
				Timer0_us(1);
				SPI_Write(0x20); // 4k清除命令
				SPI_Write((add & 0x00FF0000) >> 16);
				SPI_Write((add & 0x0000FF00) >> 8);
				SPI_Write(add & 0x00FF); // 24 位地址
				Timer0_us(1);
				P6 = 0x80; // 片选无效

				Timer0_us(1);
				//TI0 = 1;		 // Here？这里需要启动传输？
				busywait(); // 读取状态等待写入完成
				printf("\r\nClear 4KB memory starting from %lx has done.\r\n", add);
				break;
			default:
				printf("\r\rWrong command!\r\n");
		}
	}
}