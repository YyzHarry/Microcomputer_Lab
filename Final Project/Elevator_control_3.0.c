#include <c8051f020.h>
#include <stdio.h>
#include "font.h"

#define SYSCLK 22118400
#define NOKEY 255
#define DELAY_LCD 1000
#define CLR 15
#define SPICLK 2000000
#define SAMPLERATE 8000		//¼������Ƶ��Ϊ 8k
#define length 16384 		//��¼�ĳ���
typedef unsigned char uchar;

// AD �Ĵ�������
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

// DA �Ĵ�������
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//��Ƶ����� DAC1 ����

// ��Ƶ���ã���FLASH��ȡ���Ѵ�õ����ݣ�1~6¥��ʾ������DAC����
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

// ��������״̬
#define UP 0
#define DOWN 1
#define STOP 2

#define STAIR_SUM 6     // ����¥����Ŀ

// Define for Display
#define UNIT_LENGTH	    12	// ��ʾ��С��Ԫ�ĳ��Ϳ�, 12*8
#define UNIT_WIDTH		8

#define ELEVATOR_LEFT 	144	// ����ͼ�������
#define ELEVATOR_RIGHT 	200	// ����ͼ���ұ���
#define ELEVATOR_HEIGHT	80	// ÿ��¥�߶ȣ�����λ�øı���

#define BIAS			434 // �ײ���ݵ��±��أ������档��470-3*12��
#define INSIDE_COL		104	// �����ڲ���Ӧ¥�㰴��λ�ã���ʾ���֣�
#define OUTSIDE_COL		232	// ��¥�㣨�����ⲿ����/��¥����λ��

#define DIREC_COL	    20	// �������з���col��
#define DIREC_ROW		240 // �������з���row��

// Display_change()�����趨
#define ELEVATOR		0
#define INSIDE			1
#define OUTSIDE_UP		2
#define OUTSIDE_DOWN	3
#define DIRECTION		4
#define OPEN_DOOR		5
#define CLOSE_DOOR		6


// Define Finite-State Machine
#define F0 0	// ��ʼ��״̬
#define F1 1	// �ȴ�״̬�����б������ֲ���
#define F2 2	// ��������״̬������direction = UP
#define F3 3	// ��������������һ��ʱ��(2s)�����ü�����ֵ������¥��������ʾ����ʱ��
#define F4 4	// �ж�������Ϣ��Ŀ�ĵ���Ϣ������֮���ǵ����Ǽ������л���ͣ��
#define F5 5	// ͣ����ǰ¥�㣬����������������һ��ʱ��(2s)�����ü�����ֵ����ʾͣ��ʱ��
#define F6 6	// ��������״̬������direction = DOWN
#define F7 7	// ��������������һ��ʱ��(2s)�����ü�����ֵ����С¥��������ʾ�½�ʱ��
#define F8 8	// �ж�������Ϣ��Ŀ�ĵ���Ϣ������֮���ǵ����Ǽ������л���ͣ��


// COLOR DEFINE: 16bit, 5-6-5 represent R-G-B (e.g., RED: 0xF800)
#define YELLOW 0xFF00
#define WHITE  0xFFFF
#define BLACK  0x0000
#define RED	   0xF800


uchar direction = UP;           // ������ݵķ���
uchar state_now = F0;
uchar state_next = F1;
unsigned int stair_now = 1;		// Ŀǰ���ڵ�¥��

bit C1, C2, C3, C4;		        // State-change Condition

uchar up_request[STAIR_SUM + 1];
uchar down_request[STAIR_SUM + 1];

unsigned int count = 0;		// For Counter 0


unsigned char xdata reset _at_ (0x8003);
unsigned char xdata cmd _at_ (0x8002);
unsigned char xdata mydata _at_ (0x9002);

// ������趨
unsigned char xdata seg _at_ (0x8000);  //����ܶ����ַ
unsigned char xdata cs _at_ (0x8001);   //�����λ���ַ
const unsigned char code segs[] =       //�����
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE};  //λ���

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
	XBR0 = 0x06; // ���� SPI �� UART
	P0MDOUT = 0xC0; // ����������ض˿�Ϊ������� P0.6 �� P0.7
	P0MDOUT |= 0x15; // TX, SCK, MOSI ����Ϊ�������
	P74OUT = 0x20; // p6.7 ����Ƭѡ�ź�
	EMI0CF = 0x1F; // �Ǹ������ߣ���ʹ���ڲ� XRAM
	XBR2 = 0x42; // ʹ�� P0-P3 ��Ϊ���ߣ�����XBR
	P1MDOUT = 0xFF; // ��λ��ַ
	P2MDOUT = 0xFF; // ��λ��ַ
	P3MDOUT = 0xFF; // ��������
	P74OUT |= 0x80;	 //�������
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

void SHOW_LED(int elevator_now) 	// ��ʾĿǰ��������¥��
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
	AMX0SL = 0x01;	 // ѡ��AIN1��Ϊ����
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

void ADC0_ISR (void) interrupt 15	//ADCת������ж�
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
	SPI0CFG = 0x07; // 8 λ֡��С
	SPI0CN = 0x03; // ��ģʽ������SPI �豸
	SPI0CKR = SYSCLK/2/SPICLK;
}

unsigned char SPI_Write(unsigned char v)
{
	SPIF = 0; // ����жϱ�־
	SPI0DAT = v; // ���ݼĴ�����ֵ
	while (SPIF == 0); // �ȴ��������
	return SPI0DAT; // ͬʱ�ѽ��յ��Ľ������
}

void Timer0_us(int num)
{
	int i;
	for(i=0;i<1000*num;i++);	//ע����ʱ���� vs 10000
	return;
}

void busywait()	 //��оƬ�����ڲ�����д��Ȳ���ʱ�ȴ������
{
    P6 = 0x00; // Ƭѡ��Ч
	Timer0_us(1);	// д�����棿��
	while(1){
		unsigned char v;
		SPI_Write(0x05);	// ��״̬������
		v = SPI_Write(0x00);
		v = v & 0x1;	// ��ȡS0
		if(v == 0)
			break;
	}
	P6 = 0x80;  // Ƭѡ��Ч
	Timer0_us(1);
}

/* LCD1602 Definition, for real-time display Key information */
char isLcdBusy(void)
{
	P5 = 0xFF; 			// ���� P5 Ϊ����ģʽ
	P6 = 0x82; 			// RS=0, RW=1, EN=0
	Delay(DELAY_LCD);
	P6 = 0x83; 			// RS=0, RW=1, EN=1
	Delay(DELAY_LCD);
	return (P5 & 0x80); // ����æ״̬
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

// ������ʾ���ĺ���
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


void refresh_scrn()		// ˢ����Ļ
{
	int i, j;
	for (i=0; i<359; i+=8){
		for (j=0; j<479; j+=8){
			show_char(i, j, YELLOW, 0xDB);
		}
	}
}

// ������Ļ�ĺ������������µ�������λ�á������ڲ�����״̬�������Ƿ�����Ҫ�˵�����/��¥���������з���
void Display_change(unsigned int stair, uchar object, unsigned int color)
{
	unsigned int i, j;
	switch(object){
		case ELEVATOR:
			for(i = ELEVATOR_LEFT; i <= ELEVATOR_RIGHT; i += UNIT_WIDTH)
				for(j = BIAS-ELEVATOR_HEIGHT*(stair-1)-2*UNIT_LENGTH; j <= BIAS-ELEVATOR_HEIGHT*(stair-1)+2*UNIT_LENGTH; j += UNIT_LENGTH)
					show_char(i,j,color,0xDB);
			break;
		case INSIDE:	//���µ����ڲ�����״̬������¥�㱻���£���ô��Ӧ�����ֽ��ᱻ���
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
		case OUTSIDE_UP:	//�����¥��ġ����ϡ����������£�����Ļ����Ӧ���ϵ����Ǳ����
			show_char(OUTSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)-UNIT_LENGTH, color, 0x1E);
			break;
		case OUTSIDE_DOWN:	//�����¥��ġ����¡����������£�����Ļ����Ӧ���µ����Ǳ����
			show_char(OUTSIDE_COL, BIAS-ELEVATOR_HEIGHT*(stair-1)+UNIT_LENGTH, color, 0x1F);
			break;
		case DIRECTION:		//���µ������з���
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
		case F1:        // BUG HERE! Fixed. ע������������з������һ״̬�Ĺ�ϵ
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
			//�򿪼�������ʼ����������2s�����¥��
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
			// ����ѵ�¥���������
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
                        down_request[stair_now] = 0;    // ˵�����ݷ���ת
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
                        up_request[stair_now] = 0;    // ˵�����ݷ���ת
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
			// �򿪼�������ʼ������ͣ��2s
			if(TR0 == 0)
		 		TR0 = 1;
			break;
		case F6:
			direction = DOWN;
			Display_change(stair_now,DIRECTION,RED);
			state_next = F7;
			break;
		case F7:
			// �򿪼�������ʼ����������2s�����¥��
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
			state_next = F1;    // ���������ص�F1״̬

			// Displaying Bug here!
			if((direction == UP && stair_now != 6)||(stair_now == 1))
				Display_change(stair_now,OUTSIDE_UP,BLACK);
			if((direction == DOWN && stair_now != 1)||(stair_now == 6))
				Display_change(stair_now,OUTSIDE_DOWN,BLACK);

			// ע�����ﲻ����DAC�жϣ���Ϊ���ȼ�����!
			// ֮��ʵ�ֿ���+������ʾ
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
	uchar temp = NOKEY;		// ��ʼ��NOKEY, ����0�밴����ͻ
	uchar ctrl = NOKEY;
	uchar i;
	unsigned int j, k, flag=0;
	unsigned char c, v, v1;
	long int add;

	WDTCN = 0xde;
	WDTCN = 0xad;   // ��ֹ���Ź�
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
    // ÿ�����ݳ�8KB��¼��ʱע��ı���ʼ��ַλ�ã�
    AD0EN = 0;			   // ADC�ر�

    // �ɼ�������Ϣ
	while(ctrl == temp){
		ctrl = getkey();
	}
	itr = 0;
    AD0EN = 1;		        // ADC��
    while(itr < length);
    AD0EN = 0;			    // ADC�ر�
    DAC1CN = 0x97;		    // DAC��
    itr = 0;
	while(itr < length);
	DAC1CN = 0x17;		    // DAC�ر�

    ctrl = NOKEY;
	temp = NOKEY;

    DAC_FLAG = 1;	    // ¼������
	P74OUT = 0x30;      // P6_out

	// ������0������
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
		SHOW_LED(stair_now);	// �������ʾ¥��

		// ���ݰ�������״̬������������FSM����Ļ����ʾ
		switch(temp){
			case 0x1:
				Lcd1602_Write_Command(0x80); 	    // display the first line
                for (j = 0; j < 16; j++)
                    Lcd1602_Write_Data(first_up[j]); 	    // DATA

				if(stair_now == 1){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 2){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 3){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 4){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 5){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 6){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 2){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 3){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 4){		  // ���ڱ��㰴�£�ֱ�ӿ���
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

				if(stair_now == 5){		  // ���ڱ��㰴�£�ֱ�ӿ���
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
				if(stair_now == 1){		  // ��ʾ����!
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
				if(stair_now == 2){		  // ��ʾ����!
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
				if(stair_now == 3){		  // ��ʾ����!
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
				if(stair_now == 4){		  // ��ʾ����!
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
				if(stair_now == 5){		  // ��ʾ����!
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
				if(stair_now == 6){		  // ��ʾ����!
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
		C1 = 0;	// C1=1: �ڵ�ǰ¥��(stair_now)����������(��/��)��������ڲ����µ�Ŀ�ĵظ��ڵ�ǰ¥��
		C2 = 0; // C2=1: �ڵ�ǰ¥��(stair_now)����������(��/��)��������ڲ����µ�Ŀ�ĵص��ڵ�ǰ¥��
		C3 = 0;	// C3=1: ��ǰ¥��ΪĿ�ĵ�֮һ����˲�����Ҫ�˵�����¥����Ҫͣ���˲�
		C4 = 0; // C4=1: ��ǰ¥��ΪĿ�ĵ�֮һ����˲�����Ҫ�˵�����¥����Ҫͣ���˲�

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
			// F5 ״̬˵�����ݵ���ĳһ�㣬��Ҫ����������ʾ
            DAC1CN = 0x97;		   // DAC��
			itr = 0;
			while(itr < length);
			DAC1CN = 0x17;		   // DAC�ر�
		}


		// Change State
		State_transition();
		state_now = state_next;

		// �ɼ�������Ϣ
		if(ctrl == temp){
			ctrl = getkey();
			if(ctrl == temp)
				continue;
		}
		temp = ctrl;
	}
}
