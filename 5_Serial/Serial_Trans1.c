#include <c8051f020.h>
#define RX_LEN 20
#define SYSCLK 22118400
#define BAUDRATE 1200
//-----------------------------------与键盘输入有关的定义
#define NOKEY 0xFF
#define uchar unsigned char
unsigned char ch;

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
	XBR0 = 0x04; // UART0 使能
	XBR2 = 0x42; // 使能交叉开关
	P0MDOUT=0XC0;
	P1MDOUT=0XFF;
	P2MDOUT=0XFF;
	P3MDOUT=0XFF;
	P0MDOUT |= 0x01; // TX0 设置为推挽输出

}
void UART0_Init (void)
{
	SCON0 = 0x50; // SCON0: 串口方式1 使能RX
	TMOD = 0x20; // 定时器 1 采用自装载模式
	TH1 = -(SYSCLK/BAUDRATE/16/12); // Timer1 载入值
	TR1 = 1; // 启动 Timer1
	PCON |= 0x80; // SMOD0 = 1
}
void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; // 启动外部晶振
	for (i=0; i < 256; i++); // 延时一段时间
	while (!(OSCXCN & 0x80)); // 等待振荡稳定
	OSCICN = 0x88; // 使用外部振荡器
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
	for(m = 0; m < 100; m++)
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
	if (RI0 == 1) { // 接收中断
		RI0 = 0; // 清中断标记
		c = SBUF0; // 读取接收到的数
		RxBuf[RxIdx] = c; // 存入缓冲区	
	}
	else if (TI0 == 1) { // 发送中断
		TI0 = 0; // 清中断标记
		if(ch != NOKEY) {
			SBUF0 = ch;
			ch = NOKEY;
		}
	}
}
void main (void) {
	unsigned char tmp;
	int i, j;
	
	WDTCN = 0xde; // 禁止看门狗
	WDTCN = 0xad;
	SYSCLK_Init (); // 初始化时钟
	PORT_Init (); // 初始化交叉开关和端口
	UART0_Init (); // 初始化 UART0
	
	ES0 = 1; // 允许串口中断
	EA = 1; // 允许全局中断
	RxIdx = 0;
	tmp = NOKEY;
	ch = tmp;
	while(1){
		tmp = getkey();	
		TI0 = 0;
		if(tmp != NOKEY){
			ch = tmp;
			TI0 = 1;  // 启动发送
		}
		// 数码管显示：将一个数据转换成二进制显示
		j = 0;
		if(RxBuf[RxIdx] >= '0' && RxBuf[RxIdx] <= '9')
			j = RxBuf[RxIdx]-'0';
		else if(RxBuf[RxIdx] >= 'a' && RxBuf[RxIdx] <= 'f')
			j = RxBuf[RxIdx]-'a'+10;
		for (i = 0; i < 4; ++i) { // 循环显示四位数码
			seg = segs[j % 10]; // 第i个段码
			cs = css[i]; // 位码
			j /= 10; // 十进制右移
			Delay(1000); // 等待
		}
	}
}