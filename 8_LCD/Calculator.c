#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define DELAY_LCD 1000
#define NOKEY 255
#define CLR 15

typedef unsigned char uchar;
// Last one Serves as Backspace
const char code numgram[]={'0','1','2','3','4','5','6','7','8','9','+','-','*','/','=',' '};
uchar row1[16];
uchar row2[16];
int cnt;

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
	EMI0CF = 0x1F;  // �Ǹ������ߣ���ʹ���ڲ� XRAM
	XBR2 = 0x42; 	// ʹ�� P0-P3 ��Ϊ���ߣ�����XBR
	P0MDOUT = 0xC0; // ����������ض˿�Ϊ������� P0.6 �� P0.7
	P1MDOUT = 0xFF; // ��λ��ַ
	P2MDOUT = 0xFF; // ��λ��ַ
	P3MDOUT = 0xFF; // ��������
}

void Delay(int num)
{
	int i;
	for(i=0;i<num;i++);
	return;
}

char isLcdBusy(void)
{
	P5 = 0xFF; 	// ���� P5 Ϊ����ģʽ
	P6 = 0x82; 	// RS=0, RW=1, EN=0
	Delay(DELAY_LCD); 
	P6 = 0x83; 	// RS=0, RW=1, EN=1
	Delay(DELAY_LCD);
	return (P5 & 0x80);   // ����æ״̬
}

// Write the control command
void Lcd1602_Write_Command(unsigned char Command)
{
	while(isLcdBusy());
	P5 = Command; 		// command to be written
	P6 = 0x80; 			// RS=0, RW=0, EN=0
	Delay(DELAY_LCD); 	// delay
	P6 = 0x81; 			// RS=0, RW=0, EN=1
	Delay(DELAY_LCD);
	P6 = 0x80; 			// RS=0, RW=0, EN=0
}

// Write the "Data" into the specific place (row,column)
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

void Lcd1602_init(void)
{
	Lcd1602_Write_Command(0x38);
	Lcd1602_Write_Command(0x08);
	Lcd1602_Write_Command(0x01);
	Lcd1602_Write_Command(0x06);
	Lcd1602_Write_Command(0x0C);
	Lcd1602_Write_Command(0x80);
	Lcd1602_Write_Command(0x02);
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
	if(i == 0) return NOKEY;
	key = dec[i] * 4;
	Delay(1000);
	P4 = 0xF0;
	Delay(100);
	i = ~P4;
	i >>= 4;
	if(i == 0) return NOKEY;
	key = key + dec[i];
	key = trans[key];
	return key;
}

void TIMER0_ISR (void) interrupt 1	  // ����ʱ���ж�ˢ����ʾ��
{
	int i;
	cnt++;
	if(cnt == 500){		// ˢ�µ�һ��Ϊrow1��ǰ����
		Lcd1602_Write_Command(0x80);
		for(i=0;i<16;i++)
			Lcd1602_Write_Data(numgram[row1[i]]);
	}
	else if(cnt == 1000){	  // ˢ�µڶ���Ϊrow2��ǰ����
	// ���ж�ˢ��Ҫע����������ˢ��ʱ�䲻�ܲ��̫С��������ˢ�����õ�ʱ�仹�������ģ��ܿ��ܵ�һ�п���ˢ�£����ڶ���Ҫˢ��ʱ��һ�л�ûˢ��������ˢ��ʧ��
		Lcd1602_Write_Command(0xC0);
		for(i=0;i<16;i++)
			Lcd1602_Write_Data(numgram[row2[15-i]]);
	}
	if(cnt > 1100){	// ˢ��ʱ����Ҫ����row2Ҫ��
		cnt = 0;	// ˢ�½�����0
	}
}

int change_to_int(char *c, int length){	//  change char* into int*
	int i, flag=0;
	int sum = 0;
	if(c[0] == 0xB) flag = 1;
	for(i=0;i<length;i++){
		if(flag && !i) continue;
		sum = sum*10 + (int)(c[i]);
	}
	if(flag) sum = -sum;
	return sum;
}

void change_ans_to_char(long int a){		// ��Ϊ�ַ�����ֱ�Ӵ浽��ʾ��row2��
	int i = 0;
	long int tmp = a;
	bit minus = 0;   // �жϸ���
	if(a < 0){
		minus = 1;
		tmp = -a;
	}
	if(tmp == 0){
		row2[0] = 0;
		return;
	}
	while(tmp != 0){
		row2[i] = tmp%10;
		i++;
		tmp /= 10;
	}
	if(minus)
		row2[i] = 0xB;	// 0xB == '-'
}

void init()		// initiate two operater numbers
{
	int i;
	for(i=0;i<16;i++){
		row1[i] = CLR;
		row2[i] = CLR;
	}
}

void main(void)
{	  
	//int i;
	int j = 0; 
	bit operator_input = 0;
	uchar temp;
	uchar ctrl;
	int curplace = 0;
	uchar input_num1[10];	// ������1
	uchar input_num2[10];	// ������2
	uchar op;				// ��������
	long int num[3] = {0};
	int len1 = 0;
	int len2 = 0;
	long int answer = 0;
	bit complete = 0;	// ����֮ǰ�����Ƿ���ɣ��仯��־�ֱ���'='���²�����������	
	ctrl = NOKEY;
	temp = NOKEY;
	//bit judgeneg = 0;	// �ж��Ƿ����븺��

	WDTCN = 0xde;
	WDTCN = 0xad; // Stop Watch-dog
	SYSCLK_Init();
	PORT_Init();
	P74OUT = 0x30; // P6 out
	EA = 1;
	TMOD |= 0x02;
	TH0 = 0x00;
	TL0 = 0x00;
	TR0 = 1;
	ET0 = 1;
	Lcd1602_init();
	Lcd1602_Write_Command(0x01);
	
	init();		// initiate
	while(1){
		while(ctrl == temp)    // ȥ���ظ��İ���
			ctrl = getkey();
		temp = ctrl;
		
		if(complete == 1 && ctrl != NOKEY){ // ��������������ʱ�����³�ʼ����ز�������ʾ��
			complete = 0;
			curplace = 0;

			init();	   // clear the data
			len1 = 0;
			len2 = 0;
			j = 0;
			operator_input = 0;
		}
	
		if(ctrl < 0xE){  // ������ǵȺŻ��˸��ڵ�һ��ֱ�������ʾ������Ӧ���ַ�
			row1[curplace++] = ctrl;
			if(ctrl > 0x9){
				if(ctrl == 0xB && !len1){ // �����һ����Ϊ����
					len1++;
					input_num1[j++] = ctrl;
					continue;
				}
				if(ctrl == 0xB && !len2 && operator_input){ // ����ڶ�����Ϊ����
					len2++;
					input_num2[j++] = ctrl;
					continue;
				}
				op = ctrl;
				operator_input = 1;
				j = 0;
			}
			else{
				if(operator_input){
					len2++;
					input_num2[j++] = ctrl;
				}
				else{
					len1++;
					input_num1[j++] = ctrl;
				}
			}
		}
		else if(ctrl == 0xF){	// Backspace
			if(curplace > 0){
				row1[--curplace] = ctrl;
				if(operator_input){
					if(j != 0){
						if(len2 > 0){	 
							input_num2[--j] = ' ';
							len2--;	
						}
					}
					else{   // Delete operator
						operator_input = 0;
						j = len1;
					}
				}
				else{
					if(len1 > 0){
						input_num1[--j] = ' ';
						len1--;
					}
				}
			}
		}
		// ע�⵽��ֻ����һ�������µȺţ���ʾ��Ӧ�������������
		else if(ctrl == 0xE){	// operator '='
			j = 0;
			if(operator_input == 0) op = 0;  // only num[1]
			num[1] = change_to_int(input_num1,len1);	// change to int*
			num[2] = change_to_int(input_num2,len2);

			switch(op){
				case 0xA:
					answer = num[1] + num[2];
					break;
				case 0xB:
					answer = num[1] - num[2];
					break;
				case 0xC:
					answer = num[1] * num[2];
					break;
				case 0xD:
					answer = num[1] / num[2];
					break;
				default:
					answer = num[1];
					break;
			}
			change_ans_to_char(answer);
			complete = 1;	// ��һ�ΰ���ʱ�����ʾ������
		}
	}
}