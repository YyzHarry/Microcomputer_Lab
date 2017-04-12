#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define BAUDRATE 1200
void PORT_Init()
{
	XBR0 = 0x04; // UART0 ʹ��
	XBR2 = 0x40; // ʹ�ܽ��濪��
	P0MDOUT |= 0x01; // TX0 ����Ϊ�������
}
void UART0_Init (void)
{
	SCON0 = 0x50; // SCON0: ���ڷ�ʽ1 ʹ��RX
	TMOD = 0x20; // ��ʱ�� 1 ������װ��ģʽ
	TH1 = -(SYSCLK/BAUDRATE/16/12); // Timer1 ����ֵ
	TR1 = 1; // ���� Timer1
	PCON |= 0x80; // SMOD0 = 1
}
void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; // �����ⲿ����
	for (i=0; i < 256; i++); // ��ʱһ��ʱ��
	while (!(OSCXCN & 0x80)); // �ȴ����ȶ�
	OSCICN = 0x88; // ʹ���ⲿ����
}
int main(void)
{
	int a,b;
	char op;
	int ans;
	WDTCN=0XDE; // ��ֹ���Ź�
	WDTCN=0XAD;
	SYSCLK_Init(); // ϵͳʱ�ӳ�ʼ��
	PORT_Init(); // �˿ڳ�ʼ��
	UART0_Init(); // ���ڳ�ʼ��
	TI0 = 1; // ׼���÷�������
	while(1){
		printf("Enter a equation:\r\n");
		scanf("%d %c %d",&a,&op,&b);
		switch (op){
			case '+': {
				ans=a+b;
				printf("Answer is %d\n", ans);
				break;
			}
			case '-': {
				ans=a-b;
				printf("Answer is %d\n", ans);
				break;
			}
			case '*': {
				ans=a*b;
				printf("Answer is %d\n", ans);
				break;
			}
			case '/': {
				ans=a/b;
				printf("Answer is %d\n", ans);
				break;
			}
			default: {
				printf("peration is unrecognized.\n");
			}
		}
	}
}