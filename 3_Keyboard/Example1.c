#include <C8051F020.h>
void PORT_Init (void)
{
	SYSCLK_Init(); // 使用外部晶振
	EMI0CF = 0x1F; // 非复用总线，不使用内部XRAM
	XBR2 = 0x42; // 使用P0-P3作为总线,允许XBR
	P0MDOUT = 0xC0; // 推挽输出P0.6 和 P0.7
	P1MDOUT = 0xFF; // 高位地址
	P2MDOUT = 0xFF; // 地位地址
	P3MDOUT = 0xFF; // 数据总线
}

void Delay(int k) //延时子系统
{
	int i;
	for (i = 0; i < k; ++i);
}

unsigned char xdata seg _at_ (0x8000); //数码管段码地址
unsigned char xdata cs _at_ (0x8001); //数码管位码地址
const unsigned char code segs[] = //段码表
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE}; //位码表

void main(void)
{
	unsigned int k; //循环计数
	WDTCN = 0xde;
	WDTCN = 0xad; //禁止看门狗
	PORT_Init();
	k = 0;
	while (1) { 
		unsigned char i;
		int j; // 循环计数/ 64
		j = k / 64;
		for (i = 0; i < 4; ++i) { //循环显示四位数码
			seg = segs[j % 10]; 		//第i个段码
			cs = css[i]; 						//位码
			j /= 10; 								//十进制右移
			Delay(1000); 						//等待
			}
		k++;
	}
}