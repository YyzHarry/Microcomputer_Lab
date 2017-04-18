#include <c8051f020.h>
#include<stdio.h>
#define SYSCLK 22118400
#define SPICLK 2000000
#define BAUDRATE 1200 //������ 9600?

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

void main()
{
	int j;
	unsigned char c, v, v1;
	long int add;
	WDTCN = 0xde;
	WDTCN = 0xad;
	SYSCLK_Init(); 			// ϵͳʱ������
	PORT_Init(); 			// IO �ܽ�����
	P6 = 0x80; 				// Ƭѡ��Ч
	UART0_Init(); 			// ��ʼ������
	SPI0_Init(); 			// ��ʼ��SPI
	Timer0_us(1000); 		// ��ʱ
	P6 = 0x00; 		// Ƭѡ��Ч
	Timer0_us(1);	// ��ʱ
	
	TI0 = 1;		   //��ʼ������ֻ��Ҫ�����ⲿ���м���!!
	SPI_Write(0x9F); 	 // д��� JEDEC ID ����
	v = SPI_Write(0x00); // ��SPI�豸�������̱�ʶ
	printf("Manufacturer ID: %bx\r\n", v);
	v = SPI_Write(0x00); // ��SPI�豸�����洢������
	printf("Memory Type ID: %bx\r\n", v);
	v = SPI_Write(0x00); // ��SPI�豸�������������Ϊд��1���޹����ݣ�MOSI��MISO��ͬ����
	printf("Capacity ID: %bx\r\n", v);
	Timer0_us(1); 	// ��ʱ
	P6 = 0x80; 		// Ƭѡ��Ч
	
	while(1) {
		do { // ���˵��س��Ϳո��ַ�����ȡ�����ֽ�
			c = getchar();
		} while ((c == ' ') || (c == '\r') || (c == '\n'));
		scanf("%lx", &add); // �����ֵַ
		switch (c) {
			case 'd': // Display
				P6 = 0x00; // Ƭѡ��Ч
				Timer0_us(1);
				SPI_Write(0x03); // ����������
				SPI_Write((add & 0x00FF0000) >> 16);
				SPI_Write((add & 0x0000FF00) >> 8);
				SPI_Write(add & 0x00FF); // 24 λ��ַ
				Timer0_us(1);
					 
				//TI0 = 1;		//����Ҫ�������м����ʼ������!				
				for(j = 0; j < 16; j++){
					v = SPI_Write(0x00); // ������
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
				P6 = 0x80; // Ƭѡ��Ч
				break;
			case 'w': // Write
				scanf("%bx", &v1); // ��������Ҫд����ֽ�����
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
				SPI_Write(v1); // ����
				Timer0_us(1);
				P6 = 0x80; // Ƭѡ��Ч
				Timer0_us(1);
				 
				//TI0 = 1;	// ����Ҫ�������м����ʼ������
				busywait(); // ��ȡ״̬�ȴ�д�����
				printf("\r\nWrite %lx %bx OK\r\n", add, v1);
				break;
			case 'c': // Clear
				P6 = 0x00; // Ƭѡ��Ч				Timer0_us(1);
				SPI_Write(0x06); // д����������
				Timer0_us(1);
				P6 = 0x80; // Ƭѡ��Ч
				Timer0_us(1);

				P6 = 0x0; // �ٴ�����Ƭѡ
				Timer0_us(1);
				SPI_Write(0x20); // 4k�������
				SPI_Write((add & 0x00FF0000) >> 16);
				SPI_Write((add & 0x0000FF00) >> 8);
				SPI_Write(add & 0x00FF); // 24 λ��ַ
				Timer0_us(1);
				P6 = 0x80; // Ƭѡ��Ч

				Timer0_us(1);
				//TI0 = 1;		 // Here��������Ҫ�������䣿
				busywait(); // ��ȡ״̬�ȴ�д�����
				printf("\r\nClear 4KB memory starting from %lx has done.\r\n", add);
				break;
			default:
				printf("\r\rWrong command!\r\n");
		}
	}
}