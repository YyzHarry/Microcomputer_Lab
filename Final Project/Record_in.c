#include <c8051f020.h>
#include <stdio.h>

#define SYSCLK 22118400
#define SPICLK 2000000
#define SAMPLERATE 8000		//¼������Ƶ��Ϊ 8k
#define length 16384		//��¼�ĳ���

//AD �Ĵ�������
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA �Ĵ�������
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//��Ƶ����� DAC1 ����

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

void PORT_Init(void)
{
	SYSCLK_Init();
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

void main(void){
    int i, j, flag=0;
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


	P6 = 0x80; 				// Ƭѡ��Ч
	SPI0_Init(); 			// ��ʼ��SPI
	//Timer0_us(1000); 		// ��ʱ
	//P6 = 0x00; 		// Ƭѡ��Ч
	//P6 = 0x80; 		// Ƭѡ��Ч

	EA = 1;
	EIE2 |= 0x02;

    itr = 0;
    add = 0;        // ÿ�����ݳ�32KB��¼��ʱע��ı���ʼ��ַλ�ã�

    // REFRESH WHOLE FLASH
    P6 = 0x00; // Ƭѡ��Ч
	Timer0_us(1);
	SPI_Write(0x06); // д����������
	Timer0_us(1);
	P6 = 0x80; // Ƭѡ��Ч
	Timer0_us(1);
    P6 = 0x0; // �ٴ�����Ƭѡ
	Timer0_us(1);
	SPI_Write(0x60); // ��Ƭ�������
	Timer0_us(1);
	P6 = 0x80; // Ƭѡ��Ч

    // ¼��������FLASH
	for (i=0; i<6; i++) {

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

        // WRITE FROM XDATA TO FLASH
        for (j=0; j<length; j++){
            P6 = 0x00; // Ƭѡ��Ч
			Timer0_us(1);
			SPI_Write(0x06); // д����������
			Timer0_us(1);
			P6 = 0x80; // Ƭѡ��Ч
			Timer0_us(1);
			P6 = 0x0; // �ٴ�����Ƭѡ
			Timer0_us(1);
			SPI_Write(0x02); // д����
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 λ��ַ
			SPI_Write((record[j + i*length] & 0x0000FF00)>> 8);     // д���λ
			Timer0_us(1);
			P6 = 0x80; // Ƭѡ��Ч
			busywait(); // ��ȡ״̬�ȴ�д�����
			add++;

			P6 = 0x00; // Ƭѡ��Ч
			Timer0_us(1);
			SPI_Write(0x06); // д����������
			Timer0_us(1);
			P6 = 0x80; // Ƭѡ��Ч
			Timer0_us(1);
			P6 = 0x0; // �ٴ�����Ƭѡ
			Timer0_us(1);
			SPI_Write(0x02); // д����
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 λ��ַ
			SPI_Write(record[j + i*length] & 0x00FF);     // д���λ
			Timer0_us(1);
            P6 = 0x80; // Ƭѡ��Ч
			busywait(); // ��ȡ״̬�ȴ�д�����
			add++;
        }

		ctrl = NOKEY;

	}
}
