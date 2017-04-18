#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define SPICLK 1000000	// vs 100000
#define BAUDRATE 1200 // ������

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; // �����ⲿ����
	for (i=0; i < 256; i++); // ��ʱһ��ʱ��
	while (!(OSCXCN & 0x80)); // �ȴ����ȶ�
	OSCICN = 0x88; // ʹ���ⲿ����
}

void PORT_Init()
{
	XBR0 = 0x06; // ����SPI ��UART
	XBR2 = 0x40; // ����XBR
	P0MDOUT |= 0x15; // TX, SCK, MOSI ����Ϊ�������
	P74OUT = 0x20; // p6.7 ����Ƭѡ�ź�
}

void UART0_Init (void)
{
	SCON0 = 0x50; // SCON0: ���ڷ�ʽ1 ʹ��RX
	TMOD = 0x20; // ��ʱ�� 1 ������װ��ģʽ
	TH1 = -(SYSCLK/BAUDRATE/16/12); // Timer1 ����ֵ
	TR1 = 1; // ���� Timer1
	PCON |= 0x80; // SMOD0 = 1
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
	for(i=0;i<1000*num;i++);
	return;
}

void main()
{
	unsigned char v;
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init(); 			// ϵͳʱ������
	PORT_Init(); 			// IO �ܽ�����
	P6 = 0x80; 				// Ƭѡ��Ч
	UART0_Init(); 			// ��ʼ������
	SPI0_Init(); 			// ��ʼ��SPI
	Timer0_us(1000); 		// ��ʱ
	P6 = 0x00; 				// Ƭѡ��Ч
	Timer0_us(1); 			// ��ʱ
	TI0 = 1;				// ��ʼ�������ݡ�����Ϊʲô�أ����жϣ���
	SPI_Write(0x9F); 		// д���JEDEC ID ����
	v = SPI_Write(0x00); 	// ��SPI �豸�������̱�ʶ
	printf("Manufacturer ID: %bx\r\n", v);
	v = SPI_Write(0x00); 	// ��SPI �豸�����洢������
	printf("Memory Type ID: %bx\r\n", v);
	v = SPI_Write(0x00); 	// ��SPI �豸��������
	printf("Capacity ID: %bx\r\n", v);
	Timer0_us(1); 			// ��ʱ
	P6 = 0x80; 				// Ƭѡ��Ч
	while(1);
}