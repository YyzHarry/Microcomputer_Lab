#include <C8051F020.h>
void PORT_Init (void)
{
	SYSCLK_Init(); // ʹ���ⲿ����
	EMI0CF = 0x1F; // �Ǹ������ߣ���ʹ���ڲ�XRAM
	XBR2 = 0x42; // ʹ��P0-P3��Ϊ����,����XBR
	P0MDOUT = 0xC0; // �������P0.6 �� P0.7
	P1MDOUT = 0xFF; // ��λ��ַ
	P2MDOUT = 0xFF; // ��λ��ַ
	P3MDOUT = 0xFF; // ��������
}

void Delay(int k) //��ʱ��ϵͳ
{
	int i;
	for (i = 0; i < k; ++i);
}

unsigned char xdata seg _at_ (0x8000); //����ܶ����ַ
unsigned char xdata cs _at_ (0x8001); //�����λ���ַ
const unsigned char code segs[] = //�����
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE}; //λ���

void main(void)
{
	unsigned int k; //ѭ������
	WDTCN = 0xde;
	WDTCN = 0xad; //��ֹ���Ź�
	PORT_Init();
	k = 0;
	while (1) { 
		unsigned char i;
		int j; // ѭ������/ 64
		j = k / 64;
		for (i = 0; i < 4; ++i) { //ѭ����ʾ��λ����
			seg = segs[j % 10]; 		//��i������
			cs = css[i]; 						//λ��
			j /= 10; 								//ʮ��������
			Delay(1000); 						//�ȴ�
			}
		k++;
	}
}