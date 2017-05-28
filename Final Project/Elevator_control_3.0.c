#include <c8051f020.h>
#include <stdio.h>
#include "font.h"

#define SYSCLK 22118400
#define NOKEY 255
#define DELAY_LCD 1000
#define CLR 15
#define SPICLK 2000000
#define SAMPLERATE 8000		//录音采样频率为 8k
#define length 16384 		//记录的长度
typedef unsigned char uchar;

// AD 寄存器设置
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

// DA 寄存器设置
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//音频输出由 DAC1 驱动

// 音频设置，从FLASH中取出已存好的数据（1~6楼提示音），DAC播放
unsigned int xdata record[length];

unsigned sample;
unsigned int itr;
int DAC_FLAG = 0;

// Lcd Display Definition
const char code first_up[]={'1','s','t',' ','F','l','o','o','r',' ','U','P',' ',' ',' ',' '};
const char code second_up[]={'2','n','d',' ','F','l','o','o','r',' ','U','P',' ',' ',' ',' '};
const char code third_up[]={'3','r','d',' ','F','l','o','o','r',' ','U','P',' ',' ',' ',' '};
const char code fourth_up[]={'4','t','h',' ','F','l','o','o','r',' ','U','P',' ',' ',' ',' '};
const char code fifth_up[]={'5','t','h',' ','F','l','o','o','r',' ','U','P',' ',' ',' ',' '};
const char code sixth_down[]={'6','t','h',' ','F','l','o','o','r',' ','D','O','W','N',' ',' '};
const char code fifth_down[]={'5','t','h',' ','F','l','o','o','r',' ','D','O','W','N',' ',' '};
const char code fourth_down[]={'4','t','h',' ','F','l','o','o','r',' ','D','O','W','N',' ',' '};
const char code third_down[]={'3','r','d',' ','F','l','o','o','r',' ','D','O','W','N',' ',' '};
const char code second_down[]={'2','n','d',' ','F','l','o','o','r',' ','D','O','W','N',' ',' '};
const char code first_go[]={'G','O',' ','T','O',' ','1','s','t',' ','F','l','o','o','r',' '};
const char code second_go[]={'G','O',' ','T','O',' ','2','n','d',' ','F','l','o','o','r',' '};
const char code third_go[]={'G','O',' ','T','O',' ','3','r','d',' ','F','l','o','o','r',' '};
const char code fourth_go[]={'G','O',' ','T','O',' ','4','t','h',' ','F','l','o','o','r',' '};
const char code fifth_go[]={'G','O',' ','T','O',' ','5','t','h',' ','F','l','o','o','r',' '};
const char code sixth_go[]={'G','O',' ','T','O',' ','6','t','h',' ','F','l','o','o','r',' '};
const char code error[]={'A','l','r','e','a','d','y',' ','H','e','r','e','!','!','!',' '};

// 电梯运行状态
#define UP 0
#define DOWN 1
#define STOP 2

#define STAIR_SUM 6     // 定义楼层数目

// Define for Display
#define UNIT_LENGTH	    12	// 显示最小单元的长和宽, 12*8
#define UNIT_WIDTH		8

#define ELEVATOR_LEFT 	144	// 电梯图形左边沿
#define ELEVATOR_RIGHT 	200	// 电梯图形右边沿
#define ELEVATOR_HEIGHT	80	// 每层楼高度，电梯位置改变量

#define BIAS			434 // 底层电梯的下边沿（即地面。。470-3*12）
#define INSIDE_COL		104	// 电梯内部对应楼层按键位置（显示数字）
#define OUTSIDE_COL		232	// 各楼层（电梯外部）上/下楼按键位置

#define DIREC_COL	    20	// 电梯运行方向（col）
#define DIREC_ROW		240 // 电梯运行方向（row）

// Display_change()传参设定
#define ELEVATOR		0
#define INSIDE			1
#define OUTSIDE_UP		2
#define OUTSIDE_DOWN	3
#define DIRECTION		4
#define OPEN_DOOR		5
#define CLOSE_DOOR		6


// Define Finite-State Machine
#define F0 0	// 初始化状态
#define F1 1	// 等待状态，所有变量保持不变
#define F2 2	// 向上运行状态，设置direction = UP
#define F3 3	// 开启计数，到达一定时间(2s)后重置计数初值并增加楼层数。表示上升时间
#define F4 4	// 判断请求信息和目的地信息，决定之后是电梯是继续运行还是停靠
#define F5 5	// 停靠当前楼层，开启计数器，到达一定时间(2s)后重置计数初值。表示停靠时间
#define F6 6	// 向下运行状态，设置direction = DOWN
#define F7 7	// 开启计数，到达一定时间(2s)后重置计数初值并减小楼层数。表示下降时间
#define F8 8	// 判断请求信息和目的地信息，决定之后是电梯是继续运行还是停靠


// COLOR DEFINE: 16bit, 5-6-5 represent R-G-B (e.g., RED: 0xF800)
#define YELLOW 0xFF00
#define WHITE  0xFFFF
#define BLACK  0x0000
#define RED	   0xF800


uchar direction = UP;           // 定义电梯的方向
uchar state_now = F0;
uchar state_next = F1;
unsigned int stair_now = 1;		// 目前所在的楼层

bit C1, C2, C3, C4;		        // State-change Condition

uchar up_request[STAIR_SUM + 1];
uchar down_request[STAIR_SUM + 1];

unsigned int count = 0;		// For Counter 0


unsigned char xdata reset _at_ (0x8003);
unsigned char xdata cmd _at_ (0x8002);
unsigned char xdata mydata _at_ (0x9002);

// 数码管设定
unsigned char xdata seg _at_ (0x8000);  //数码管段码地址
unsigned char xdata cs _at_ (0x8001);   //数码管位码地址
const unsigned char code segs[] =       //段码表
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE};  //位码表

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67;
	for (i=0; i < 256; i++);
	while (!(OSCXCN & 0x80));
	OSCICN = 0x88;
}

void PORT_Init (void)
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

void SHOW_LED(int elevator_now) 	// 显示目前电梯所在楼层
{
	uchar i;
	int j = elevator_now;
	for (i=0; i<4; ++i) {
		seg = segs[j % 10];
		cs = css[i];
		j /= 10;
		Delay(1000);
	}
}

void timer_ms(int num)
{
	int i;
	for(i = 0; i < num; i++){
		SHOW_LED(stair_now);
		Delay(1000);
	}
	return;
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
	// AD0EN = 1;
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

/* LCD1602 Definition, for real-time display Key information */
char isLcdBusy(void)
{
	P5 = 0xFF; 			// 设置 P5 为输入模式
	P6 = 0x82; 			// RS=0, RW=1, EN=0
	Delay(DELAY_LCD);
	P6 = 0x83; 			// RS=0, RW=1, EN=1
	Delay(DELAY_LCD);
	return (P5 & 0x80); // 返回忙状态
}

// write the control command
void Lcd1602_Write_Command(unsigned char Command)
{
	while(isLcdBusy());
	P5 = Command; 			// command to be written
	P6 = 0x80; 				// RS=0, RW=0, EN=0
	Delay(DELAY_LCD);   	// delay
	P6 = 0x81; 				// RS=0, RW=0, EN=1
	Delay(DELAY_LCD);
	P6 = 0x80; 				// RS=0, RW=0, EN=0
}

// write the "Data" into the specific place (row,column)
void Lcd1602_Write_Data(Data)
{
	P5 = Data; 		// data to be writen
	P6 = 0x84; 		// RS=1, RW=0, EN=0
	Delay(DELAY_LCD);
	P6 = 0x85; 		// RS=1, RW=0, EN=1
	Delay(DELAY_LCD);
	P6 = 0x84; 		// RS=1, RW=0, EN=0
}

void Lcd1602_WriteXY_Data(uchar row, uchar column, uchar Data)
{
	while(isLcdBusy());
	if (row == 1)
		column |= 0xC0; 			// D7=1, offset address is 0x40
	else
		column |= 0x80; 			// D7=1
	Lcd1602_Write_Command(column); 	// set the address
	Lcd1602_Write_Data(Data); 		// write the "Data"
}

Lcd1602_init(void)
{
	Lcd1602_Write_Command(0x38);
	Lcd1602_Write_Command(0x08);
	Lcd1602_Write_Command(0x01);
	Lcd1602_Write_Command(0x06);
	Lcd1602_Write_Command(0x0C);
	Lcd1602_Write_Command(0x80);
	Lcd1602_Write_Command(0x02);
}

/* init the emif using the lower port  */
void EMIF_Low(void)
{
    SYSCLK_Init();
    EMI0CF = 0x1F;  // non-multiplexed mode, external only
    XBR2 = 0x42;    // Enable xbr
    P0MDOUT = 0xC0;
    P1MDOUT = 0xFF;
    P2MDOUT = 0xFF;
    P3MDOUT = 0xFF;
}

// 关于显示屏的函数
void lcd_init9481(void)
{
    reset = 1;
    Delay(200);
    cmd = 0x11; // Exit from sleeping
    Delay(3000);
    cmd = 0xD0; // Power Setting
    mydata = 0x07;
    mydata = 0x41;
    mydata = 0x1D;
    mydata = 0x0D;

    cmd = 0xD1; // VCOM Control
    mydata = 0x00;
    mydata = 0x2B;
    mydata = 0x1F;

    cmd = 0x0C; // get pixel format (why get?)
    mydata = 0x55;
    cmd = 0x3A; // set pixel format
    mydata = 0x55; // 16bit/pixel
    cmd = 0xB4; // Display mode;
    mydata = 0;

    cmd= 0xC0; // Panel Driving Setting
    mydata = 0;
    mydata = 0x3B;
    mydata = 0x0;
    mydata = 0x2;
    mydata = 0x11;
    mydata = 0;

    cmd = 0xC5; // Frame rate and Inversion Control
    mydata = 0x03;

    cmd = 0xC8;  // Gamma Setting
    mydata = 0;
    mydata = 14;
    mydata = 0x33;
    mydata = 0x10;
    mydata = 0x00;
    mydata = 0x16;
    mydata = 0x44;
    mydata = 0x36;
    mydata = 0x77;
    mydata = 0x00;
    mydata = 0x0F;
    mydata = 0x00;

    cmd = 0xF3;
    mydata = 0x40;
    mydata = 0x0A;

    cmd = 0x36; // Address Mode
    mydata = 0x0A;

    cmd = 0xF0;
    mydata = 0x08;

    cmd = 0xF6;
    mydata = 0x84;
    cmd = 0xF7;
    mydata = 0x80;
    cmd = 0x36;  // Address Mode;
    mydata = 0x0A;

    Delay(3000);
    cmd = 0x29;  // Set display on
}

void lcd_init9486(void) /* ZHA */
{
    reset = 1;
    Delay(200);
    cmd = 0xF2;
    mydata = 0x18;
    mydata = 0xA3;
    mydata = 0x12;
    mydata = 0x02;
    mydata = 0xB2;
    mydata = 0x12;
    mydata = 0xFF;
    mydata = 0x10;
    mydata = 0x00;
    cmd = 0xF8;
    mydata = 0x21;
    mydata = 0x04;
    cmd = 0xF9;
    mydata = 0x00;
    mydata = 0x08;
    cmd = 0x36;
    mydata = 0x08;
    cmd = 0x3A;
    mydata = 0x05;
    cmd = 0xB4;
    mydata = 0x01;//0x00
    cmd = 0xB6;
    mydata = 0x02;
    mydata = 0x22;
    cmd = 0xC1;
    mydata = 0x41;
    cmd = 0xC5;
    mydata = 0x00;
    mydata = 0x07;//0x18
    cmd = 0xE0;
    mydata = 0x0F;
    mydata = 0x1F;
    mydata = 0x1C;
    mydata = 0x0C;
    mydata = 0x0F;
    mydata = 0x08;
    mydata = 0x48;
    mydata = 0x98;
    mydata = 0x37;
    mydata = 0x0A;
    mydata = 0x13;
    mydata = 0x04;
    mydata = 0x11;
    mydata = 0x0D;
    mydata = 0x00;
    cmd = 0xE1;
    mydata = 0x0F;
    mydata = 0x32;
    mydata = 0x2E;
    mydata = 0x0B;
    mydata = 0x0D;
    mydata = 0x05;
    mydata = 0x47;
    mydata = 0x75;
    mydata = 0x37;
    mydata = 0x06;
    mydata = 0x10;
    mydata = 0x03;
    mydata = 0x24;
    mydata = 0x20;
    mydata = 0x00;
    cmd = 0x11;
    Delay(200);
    cmd = 0x29;
}

void dis_color(unsigned int c)
{
    int i, j;
    cmd = 0x2C;    // start write
    for (i = 0; i < 480; ++i)
        for (j = 0; j < 320; ++j) {
            mydata = c >> 8;
            mydata = c & 0xFF;
        }
}

void show_char(unsigned int x, unsigned int y, unsigned int color, unsigned char f)
{
    unsigned char i, j, c;
    cmd = 0x2A; // set column address
    mydata = (x >> 8) & 1;
    mydata = (x & 0xFF);
    mydata = ((x + 7) >> 8) & 1;
    mydata = (x + 7) & 0xFF;
    cmd = 0x2B; // set page address
    mydata = (y >> 8) & 1;
    mydata = (y & 0xFF);
    mydata = ((y + 11) >> 8) & 1;
    mydata = (y + 11) & 0xFF;
    cmd = 0x2C;
    for (i = 0; i < 12; ++i) {
        c = font_8x12[f][i];
        for (j = 0; j < 8; ++j) {
            if (c & 1) {
                mydata = color >> 8;
                mydata = color & 0xFF;
            } else {
                mydata = 0xff;
                mydata = 0;
            }
            c = c >> 1;
        }
    }
}


void refresh_scrn()		// 刷新屏幕
{
	int i, j;
	for (i=0; i<359; i+=8){
		for (j=0; j<479; j+=8){
			show_char(i, j, YELLOW, 0xDB);
		}
	}
}

// 更新屏幕的函数，包括更新电梯所在位置、电梯内部按键状态、各层是否有人要乘电梯上/下楼、电梯运行方向
void Display_change(unsigned int stair, uchar object, unsigned int color)
{
	unsigned int i, j;
	switch(object){
		case ELEVATOR:
			for(i = ELEVATOR_LEFT; i <= ELEVATOR_RIGHT; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			break;
		case INSIDE:	//更新电梯内部按键状态，如有楼层被按下，那么相应的数字将会被标红
			if(stair == 1){
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS+UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-2*UNIT_LENGTH, color, 0xDB);
			}
			else if(stair == 2){
				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);

				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
			}
			else if(stair == 3){
				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
			}
			else if(stair == 4){
				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
			}
			else if(stair == 5){
				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
			}
			else if(stair == 6){
                show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1), color, 0xDB);

				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0xDB);

				show_char(INSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL+UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
				show_char(INSIDE_COL-UNIT_WIDTH, BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH, color, 0xDB);
			}
			break;
		case OUTSIDE_UP:	//如果有楼层的“向上”按键被按下，在屏幕上相应向上的三角被标红
			show_char(OUTSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0x1E);
			break;
		case OUTSIDE_DOWN:	//如果有楼层的“向下”按键被按下，在屏幕上相应向下的三角被标红
			show_char(OUTSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0x1F);
			break;
		case DIRECTION:		//更新电梯运行方向
			if(direction == UP){
				show_char(DIREC_COL, DIREC_ROW-2*UNIT_LENGTH, color, 0x1E);
				show_char(DIREC_COL, DIREC_ROW+UNIT_LENGTH, (RED-color), 0x1F);
			}
			else if(direction == DOWN){
				show_char(DIREC_COL, DIREC_ROW-2*UNIT_LENGTH, (RED-color), 0x1E);
				show_char(DIREC_COL, DIREC_ROW+UNIT_LENGTH, color, 0x1F);
			}
			break;
		case OPEN_DOOR:
			for(i = ELEVATOR_LEFT+3*UNIT_WIDTH; i <= ELEVATOR_RIGHT-3*UNIT_WIDTH; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			timer_ms(100);
			for(i = ELEVATOR_LEFT+2*UNIT_WIDTH; i <= ELEVATOR_RIGHT-2*UNIT_WIDTH; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			timer_ms(100);
			for(i = ELEVATOR_LEFT+UNIT_WIDTH; i <= ELEVATOR_RIGHT-UNIT_WIDTH; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			timer_ms(100);
			for(i = ELEVATOR_LEFT; i <= ELEVATOR_RIGHT; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			break;
		case CLOSE_DOOR:
			for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH){
				show_char(ELEVATOR_LEFT,j,RED,0xDB);
				show_char(ELEVATOR_RIGHT,j,RED,0xDB);
			}
			timer_ms(100);
			for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH){
				show_char(ELEVATOR_LEFT+UNIT_WIDTH,j,RED,0xDB);
				show_char(ELEVATOR_RIGHT-UNIT_WIDTH,j,RED,0xDB);
			}
			timer_ms(100);
			for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH){
				show_char(ELEVATOR_LEFT+2*UNIT_WIDTH,j,RED,0xDB);
				show_char(ELEVATOR_RIGHT-2*UNIT_WIDTH,j,RED,0xDB);
			}
			timer_ms(100);
			for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH){
				show_char(ELEVATOR_LEFT+3*UNIT_WIDTH,j,RED,0xDB);
				show_char(ELEVATOR_RIGHT-3*UNIT_WIDTH,j,RED,0xDB);
			}
			break;
		default:
			break;
	}
}

void State_transition(void)
{
    uchar i, j;
	switch(state_now){
		case F0:
			stair_now = 1;	 //direction = STOP;
			state_next = F1;
			break;
		case F1:        // BUG HERE! Fixed. 注意理清电梯运行方向和下一状态的关系
			if(C1 && C2){
                if(direction == UP)
                    state_next = F2;
                else
                    state_next = F6;
			}
			else if(C1 && !C2)
                state_next = F2;
			else if(C2 && !C1)
                state_next = F6;
			else
                state_next = F1;
			break;
		case F2:
			direction = UP;
			Display_change(stair_now,DIRECTION,RED);
			state_next = F3;
			break;
		case F3:
			//打开计数器开始计数，运行2s后更新楼层
			if(TR0 == 0)
		 		TR0 = 1;
			break;
		case F4:
			if( C3 || ((!C1)&&(!C3)) )
				state_next = F5;
			else
				state_next = F3;
			break;
		case F5:
			// 清空已到楼层的请求标记
			if(stair_now == 1 || stair_now == 6){
				up_request[stair_now] = 0;
				down_request[stair_now] = 0;
			}
			else{
                // Bug HERE! -- Fixed, needs Check
				if(direction == UP){
                    if(up_request[stair_now])
                        up_request[stair_now] = 0;
                    else{
                        down_request[stair_now] = 0;    // 说明电梯方向反转
                        direction = DOWN;
                    }
                    /*
                    direction = DOWN;
                    for(i = stair_now+1; i <= STAIR_SUM; i++){
                        if(up_request[i] || down_request[i]){
                            direction = UP;
                            break;
                        }
                    }*/
				}
				else if(direction == DOWN){
                    if(down_request[stair_now])
                        down_request[stair_now] = 0;
                    else{
                        up_request[stair_now] = 0;    // 说明电梯方向反转
                        direction = UP;
                    }
                    /*
                    direction = UP;
                    for(i = stair_now-1; i >= 1; i--){
                        if(up_request[i] || down_request[i]){
                            direction = DOWN;
                            break;
                        }
                    }*/
				}
			}
			// 打开计数器开始计数，停留2s
			if(TR0 == 0)
		 		TR0 = 1;
			break;
		case F6:
			direction = DOWN;
			Display_change(stair_now,DIRECTION,RED);
			state_next = F7;
			break;
		case F7:
			// 打开计数器开始计数，运行2s后更新楼层
			if(TR0 == 0)
		 		TR0 = 1;
			break;
		case F8:
			if( C4 || ((!C2)&&(!C4)) )
				state_next = F5;
			else
				state_next = F7;
			break;
		default:
			state_next = F0;
			break;
	}
}

void TIMER0_ISR (void) interrupt 1
{
	count++;
	if(count >= 14400){
		if(state_now == F3){
			Display_change(stair_now,ELEVATOR,BLACK);
			stair_now++;
			Display_change(stair_now,ELEVATOR,RED);
			state_next = F4;
		}
		else if(state_now == F7){
			Display_change(stair_now,ELEVATOR,BLACK);
			stair_now--;
			Display_change(stair_now,ELEVATOR,RED);
			state_next = F8;
		}
		else if(state_now == F5){
			state_next = F1;    // 处理结束后回到F1状态

			// Displaying Bug here!
			if((direction == UP && stair_now != 6)||(stair_now == 1))
				Display_change(stair_now,OUTSIDE_UP,BLACK);
			if((direction == DOWN && stair_now != 1)||(stair_now == 6))
				Display_change(stair_now,OUTSIDE_DOWN,BLACK);

			// 注意这里不能用DAC中断，因为优先级问题!
			// 之后实现开门+关门显示
			Display_change(stair_now,OPEN_DOOR,BLACK);
			timer_ms(500);
			Display_change(stair_now,CLOSE_DOOR,RED);
			timer_ms(500);
			Display_change(stair_now,INSIDE,BLACK);
		}
		count = 0;
		TR0 = 0;
	}
}


void main(void){
	uchar temp = NOKEY;		// 初始化NOKEY, 否则0与按键冲突
	uchar ctrl = NOKEY;
	uchar i;
	unsigned int j, k, flag=0;
	unsigned char c, v, v1;
	long int add;

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

	EA = 1;
	EIE2 |= 0x02;


    // RECORD FIRST
    // 每段数据长8KB，录制时注意改变起始地址位置！
    AD0EN = 0;			   // ADC关闭

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

    ctrl = NOKEY;
	temp = NOKEY;

    DAC_FLAG = 1;	    // 录音结束
	P74OUT = 0x30;      // P6_out

	// 计数器0的设置
	EA = 1;
	TMOD |= 0x02;
	TH0 = 0x00;
	TL0 = 0x00;
	ET0 = 1;
	P7 = 0x80;

	for(i = 0; i <= STAIR_SUM; i++){
		up_request[i] = 0;
		down_request[i] = 0;
	}

	Lcd1602_init();     // Initialize Lcd1602
	Lcd1602_Write_Command(0x01);

    EMIF_Low();
    EMI0TC = 0x41;
    lcd_init9486();

	refresh_scrn();

	// initial the board
	show_char(DIREC_COL+2*UNIT_WIDTH, DIREC_ROW-2*UNIT_LENGTH, BLACK, 'U');
	show_char(DIREC_COL+3*UNIT_WIDTH, DIREC_ROW-2*UNIT_LENGTH, BLACK, 'P');
	show_char(DIREC_COL+2*UNIT_WIDTH, DIREC_ROW+UNIT_LENGTH, BLACK, 'D');
	show_char(DIREC_COL+3*UNIT_WIDTH, DIREC_ROW+UNIT_LENGTH, BLACK, 'O');
	show_char(DIREC_COL+4*UNIT_WIDTH, DIREC_ROW+UNIT_LENGTH, BLACK, 'W');
	show_char(DIREC_COL+5*UNIT_WIDTH, DIREC_ROW+UNIT_LENGTH, BLACK, 'N');

	Display_change(i,DIRECTION,RED);
	for(i = 1; i <= 6; i++){
		if(i > 1)
			Display_change(i,ELEVATOR,BLACK);
		else
			Display_change(i,ELEVATOR,RED);
		Display_change(i,INSIDE,BLACK);
		if(i < 6)
			Display_change(i,OUTSIDE_UP,BLACK);
		if(i > 1)
			Display_change(i,OUTSIDE_DOWN,BLACK);
	}

	while(1){
		SHOW_LED(stair_now);	// 数码管显示楼层

		// 根据按键更新状态，包括：电梯FSM，屏幕的显示
		switch(temp){
			case 0x1:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(first_up[j]); 	    // DATA

				if(stair_now == 1){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(1,OUTSIDE_UP,RED);
					up_request[1] = 1;
					down_request[1] = 1;
				}
				break;
			case 0x2:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(second_up[j]); 	    // DATA

				if(stair_now == 2){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(2,OUTSIDE_UP,RED);
					up_request[2] = 1;
				}
				break;
			case 0x3:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(third_up[j]); 	    // DATA

				if(stair_now == 3){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(3,OUTSIDE_UP,RED);
					up_request[3] = 1;
				}
				break;
			case 0x4:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fourth_up[j]); 	    // DATA

				if(stair_now == 4){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(4,OUTSIDE_UP,RED);
					up_request[4] = 1;
				}
				break;
			case 0x5:
				for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fifth_up[j]); 	    // DATA

				if(stair_now == 5){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(5,OUTSIDE_UP,RED);
					up_request[5] = 1;
				}
				break;
            case 0x6:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(sixth_down[j]); 	    // DATA

				if(stair_now == 6){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(6,OUTSIDE_DOWN,RED);
					up_request[6] = 0;
					down_request[6] = 1;
				}
				break;
			case 0x7:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(second_down[j]); 	    // DATA

				if(stair_now == 2){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(2,OUTSIDE_DOWN,RED);
					down_request[2] = 1;
				}
				break;
			case 0x8:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(third_down[j]); 	    // DATA

				if(stair_now == 3){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(3,OUTSIDE_DOWN,RED);
					down_request[3] = 1;
				}
				break;
			case 0x9:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fourth_down[j]); 	    // DATA

				if(stair_now == 4){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(4,OUTSIDE_DOWN,RED);
					down_request[4] = 1;
				}
				break;
            case 0x0:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fifth_down[j]); 	    // DATA

				if(stair_now == 5){		  // 若在本层按下，直接开门
					Display_change(stair_now,OPEN_DOOR,BLACK);
					timer_ms(500);
					Display_change(stair_now,CLOSE_DOOR,RED);
					timer_ms(500);
					Display_change(stair_now,INSIDE,BLACK);
				}
				else{
					Display_change(5,OUTSIDE_DOWN,RED);
					down_request[5] = 1;
				}
				break;
			case 0xA:
				if(stair_now == 1){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				down_request[1] = 1;
				Display_change(1,INSIDE,RED);
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                   	Lcd1602_Write_Data(first_go[j]); 	    // DATA
				break;
			case 0xB:
				if(stair_now == 2){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				Display_change(2,INSIDE,RED);
				if(stair_now < 2)
					up_request[2] = 1;
				else if(stair_now > 2)
					down_request[2] = 1;
                Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(second_go[j]); 	    // DATA
				break;
			case 0xC:
				if(stair_now == 3){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				Display_change(3,INSIDE,RED);
				if(stair_now < 3)
					up_request[3] = 1;
				else if(stair_now > 3)
					down_request[3] = 1;
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(third_go[j]); 	    // DATA
				break;
			case 0xD:
				if(stair_now == 4){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				Display_change(4,INSIDE,RED);
				if(stair_now < 4)
					up_request[4] = 1;
				else if(stair_now > 4)
					down_request[4] = 1;
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fourth_go[j]); 	    // DATA
				break;
            case 0xE:
				if(stair_now == 5){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				Display_change(5,INSIDE,RED);
				if(stair_now < 5)
					up_request[5] = 1;
				else if(stair_now > 5)
					down_request[5] = 1;
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(fifth_go[j]); 	    // DATA
				break;
			case 0xF:
				if(stair_now == 6){		  // 提示错误!
					Lcd1602_Write_Command(0x80); 	    // display the first line
                	for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(error[j]); 	    // DATA
					break;
				}
				Display_change(6,INSIDE,RED);
				up_request[6] = 1;
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(sixth_go[j]); 	    // DATA
				break;
			default:
				break;
		}

		// Update C1, C2, C3, C4
		C1 = 0;	// C1=1: 在当前楼层(stair_now)以上有请求(上/下)，或电梯内部按下的目的地高于当前楼层
		C2 = 0; // C2=1: 在当前楼层(stair_now)以下有请求(上/下)，或电梯内部按下的目的地低于当前楼层
		C3 = 0;	// C3=1: 当前楼层为目的地之一，或此层有人要乘电梯上楼，需要停靠此层
		C4 = 0; // C4=1: 当前楼层为目的地之一，或此层有人要乘电梯下楼，需要停靠此层

		if((up_request[stair_now] == 1) && (direction == UP))
			C3 = 1;
		for(i = stair_now+1; i <= STAIR_SUM; i++){
			if(up_request[i] == 1 || down_request[i] == 1)
				C1 = 1;
		}

		if((down_request[stair_now] == 1) && (direction == DOWN))
			C4 = 1;
		for(i = stair_now-1; i > 0; i--){
			if(down_request[i] == 1 || up_request[i] == 1)
				C2 = 1;
		}

		// PLAY VOICE DATA, ONLY ONCE!
		if(state_now == F5){
			// F5 状态说明电梯到达某一层，需要进行语音提示
            DAC1CN = 0x97;		   // DAC打开
			itr = 0;
			while(itr < length);
			DAC1CN = 0x17;		   // DAC关闭
		}


		// Change State
		State_transition();
		state_now = state_next;

		// 采集按键信息
		if(ctrl == temp){
			ctrl = getkey();
			if(ctrl == temp)
				continue;
		}
		temp = ctrl;
	}
}
