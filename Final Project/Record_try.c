#include <c8051f020.h>
//AD �Ĵ�������
sfr16 TMR3RL = 0x92;
sfr16 TMR3 = 0x94;
sfr16 ADC0 = 0xbe;

//DA �Ĵ�������
sfr16 RCAP4 = 0xe4;
sfr16 T4 = 0xf4;
sfr16 DAC1 = 0xd5;			//��Ƶ����� DAC1 ����

#define SYSCLK 22118400
#define SAMPLERATE 8000		//¼������Ƶ��Ϊ 8k
#define length 16384		//��¼�ĳ���
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
	XBR0 = 0x06; // ����SPI ��UART
	XBR2 = 0x42;
	P0MDOUT = 0xC0;
	P1MDOUT = 0xFF;
	P2MDOUT = 0xFF;
	P3MDOUT = 0xFF;
	P0MDOUT |= 0x15; // TX, SCK, MOSI ����Ϊ�������
	P74OUT = 0x20; // p6.7 ����Ƭѡ�ź�
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
	for(i=0;i<100*num;i++);	//ע����ʱ���� vs 10000
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

//AD
void ADC0_Init (void)
{
	ADC0CN = 0x05;
	REF0CN = 0x03;
	AMX0SL = 0x01;	 // ѡ��AIN1��Ϊ����
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

void ADC0_ISR (void) interrupt 15	//ADCת������ж�
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
	
	P6 = 0x80; 				// Ƭѡ��Ч
	SPI0_Init(); 			// ��ʼ��SPI
	//Timer0_us(1000); 		// ��ʱ
	//P6 = 0x00; 		// Ƭѡ��Ч
	//P6 = 0x80; 		// Ƭѡ��Ч
	EA = 1;
	EIE2 |= 0x02;

	P74OUT = 0x30;      // P6_out
	
    itr = 0;

	for (i=0;i<6;i++){
		add = 0;        // ÿ�����ݳ�32KB��¼��ʱע��ı���ʼ��ַλ�ã�

            P6 = 0x00; // Ƭѡ��Ч
			Timer0_us(1);
			SPI_Write(0x03); // ����������
			SPI_Write((add & 0x00FF0000) >> 16);
			SPI_Write((add & 0x0000FF00) >> 8);
			SPI_Write(add & 0x00FF); // 24 λ��ַ
			Timer0_us(1);
			       
			for (k=0; k<length; k++){
				for (j=0; j<2; j++){
					if (!j){
						high = SPI_Write(0x00);         // ����λ
	                	record[k] = (high << 8);
					}
					else{
						low = SPI_Write(0x00);         // ����λ
						record[k] += low;
					}
					Timer0_us(1);
	            }
			}
			P6 = 0x80; 			   // Ƭѡ��Ч

            DAC1CN = 0x97;		   // DAC��
			itr = 0;
			while(itr < length);
			DAC1CN = 0x17;		   // DAC�ر�
	}
}
