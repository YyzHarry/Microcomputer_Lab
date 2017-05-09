#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define DELAY_LCD 1000
typedef unsigned char uchar;

uchar code bitmap[]={0x04,0x1F,0x04,0x0E,0x00,0x1F,0x11,0x1F};  // 汉字'吉'

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; 
	for (i=0; i < 256; i++); 
	while (!(OSCXCN & 0x80)); 
	OSCICN = 0x88; 
}

void Delay(int num)
{
	int i;
	for(i=0;i<num;i++);
	return;
}

char isLcdBusy(void)
{
	P5 = 0xFF; // 设置 P5 为输入模式
	P6 = 0x82; // RS=0, RW=1, EN=0
	Delay(DELAY_LCD);
	P6 = 0x83; // RS=0, RW=1, EN=1
	Delay(DELAY_LCD);
	return (P5 & 0x80); // 返回忙状态
}

// write the control command
void Lcd1602_Write_Command(unsigned char Command)
{
	while(isLcdBusy());
	P5 = Command; 		// command to be written
	P6 = 0x80; 				// RS=0, RW=0, EN=0
	Delay(DELAY_LCD); // delay
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
		column |= 0xC0; 	// D7=1, offset address is 0x40
	else
		column |= 0x80; 	// D7=1
	Lcd1602_Write_Command(column); 	// set the address
	Lcd1602_Write_Data(Data); 			// write the "Data"
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

void main(void)
{
	uchar i, j;
	WDTCN = 0xde;
	WDTCN = 0xad;  // Stop Watch-Dog
	SYSCLK_Init();
	P74OUT = 0x30; // P6_out
	Lcd1602_init();
	
	Lcd1602_Write_Command(0x40); 	// No.7 command, set the address as 0, the first character
	for (i = 0; i < 8; i++) Lcd1602_Write_Data(bitmap[i]); // write the "bitmap"
	Lcd1602_Write_Command(0x80); 	// display the address
	Lcd1602_Write_Data(0x00); 		// display the first custom character
	
	while(1);  // stop
}