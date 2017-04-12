#include <c8051f020.h>
#define RX_LEN 20
#define SYSCLK 22118400
#define BAUDRATE 1200
//-----------------------------------����������йصĶ���
#define NOKEY 0xFF
#define uchar unsigned char
uchar ch;
unsigned char xdata seg _at_ (0x8000); 
unsigned char xdata cs _at_ (0x8001); 
const unsigned char code segs[] = 
	{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
	0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE};
//-------------------------------------------------------
char RxBuf[RX_LEN];
unsigned char RxIdx;
void PORT_Init()
{
	EMI0CF=0x1F;
	XBR0 = 0x04; // UART0 ʹ��
	XBR2 = 0x42; // ʹ�ܽ��濪��
	P0MDOUT=0XC0;
	P1MDOUT=0XFF;
	P2MDOUT=0XFF;
	P3MDOUT=0XFF;
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
void Delay(int k)
{
	int i;
	for (i = 0; i < k; ++i);
}
char getkey()
{
	int m;
	uchar i;
	uchar key;
	const uchar code dec[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0};
	const char code trans[] = {'c', '9', '5', '1', 'd', '0', '6', '2', 'e', 'a', '7', '3', 'f', 'b', '8', '4'};
	P4 = 0x0F;
	Delay(100);
	i = ~P4 & 0x0F;
	if (i == 0) return NOKEY;			  
	key = dec[i] * 4; 
	for(m = 0; m < 100; m++)  //ע��˴�������ʱ��ʹ��ÿ�ΰ���ֻ��һ��ֵ�ڵ��Զ���ʾ���˷���֮ǰ˵����������
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
void UART0_ISR (void) interrupt 4
{
	char c;
	uchar i;
	if (RI0 == 1) { // �����ж�
		RI0 = 0; // ���жϱ��
		c = SBUF0; // ��ȡ���յ�����
		if ((c>='a' && c<='f')||(c>='0' && c<='9')){	// �ڻ����д洢�����
			for(i=0;i<RX_LEN-1;i++)
				RxBuf[i] = RxBuf[i+1];
			RxBuf[RX_LEN-1] = c;
		}
	} 
	else if (TI0 == 1) { // �����ж�
		TI0 = 0; // ���жϱ��
		if(ch != NOKEY) {
			SBUF0 = ch;
			ch = NOKEY;
		}
	}
}
void main (void) {
	uchar tmp;
	WDTCN = 0xde; // ��ֹ���Ź�
	WDTCN = 0xad;
	SYSCLK_Init (); // ��ʼ��ʱ��
	PORT_Init (); // ��ʼ�����濪�غͶ˿�
	UART0_Init (); // ��ʼ�� UART0
	ES0 = 1; // �������ж�
	EA = 1; // ����ȫ���ж�
	tmp = NOKEY;
	ch = tmp;
	TI0 = 1; // ��������
	
	while (1) {    //��ʾ���ϴ�����������ʾ���4���������
		char c;
		uchar i;
		tmp = getkey();	
		TI0 = 0;
		if(tmp != NOKEY){
			ch = tmp;
			TI0 = 1;  // ��������
		}
		for(i = 16; i < 20; ++i) {	// ��ΪRX_LEN����Ϊ20����ȡ�ĸ���Ϊ16~19
			c = RxBuf[i];
			if(c>='0' && c<='9')
				seg = segs[c-'0']; 
			else
				seg = segs[c-'a'+10]; 
			cs = css[19-i];
			Delay(1000);
		}
	}
}