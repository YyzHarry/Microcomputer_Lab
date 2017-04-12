#include <c8051f020.h>
#define SYSCLK 22118400   // ϵͳʱ��
#define BAUDRATE 1200	// ������1200bps
void PORT_Init()
{
	XBR0 = 0x04; // UART0 ʹ��
	XBR2 = 0x40; // ʹ�ܽ��濪��
	P0MDOUT |= 0x01; // TX0 ����Ϊ�������
}
void UART0_Init (void)
{
	SCON0 = 0x50; // SCON0: ���ڷ�ʽ1 ʹ��
	TMOD = 0x20; // ��ʱ�� 1 ������װ��ģ
	TH1 = -(SYSCLK/BAUDRATE/16/12); // Timer1 ����ֵ��T1Mȱʡ0��12���������ڡ�
	TR1 = 1; // ���� Timer1
	PCON |= 0x80; // SMOD0 = 1,�����ʼӱ�
}

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; // �����ⲿ����
	for (i=0; i < 256; i++); // ��ʱһ��ʱ��
	while (!(OSCXCN & 0x80)); // �ȴ����ȶ�
	OSCICN = 0x88; // ʹ���ⲿ����
}

void main()
{
	WDTCN=0XDE; // ��ֹ���Ź�
	WDTCN=0XAD;
	SYSCLK_Init(); // ϵͳʱ�ӳ�ʼ��
	PORT_Init(); // �˿ڳ�ʼ��
	UART0_Init(); // ���ڳ�ʼ��
	while(1)
	{
		SBUF0 = 0x8A;  // ͨ�����ڷ�������
		while(!TI0);  //�����ж�
		TI0=0;  //������ɣ�������������TI				   
	}
}