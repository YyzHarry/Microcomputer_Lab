#include <c8051f020.h>
#define RX_LEN 20
#define SYSCLK 22118400
#define BAUDRATE 1200
//-----------------------------------与键盘输入有关的定义
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
	for(m = 0; m < 100; m++)  //注意此处加上延时，使得每次按键只有一个值在电脑端显示，克服了之前说的两个问题
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
	if (RI0 == 1) { // 接收中断
		RI0 = 0; // 清中断标记
		c = SBUF0; // 读取接收到的数
		if ((c>='a' && c<='f')||(c>='0' && c<='9')){	// 在缓存中存储多个数
			for(i=0;i<RX_LEN-1;i++)
				RxBuf[i] = RxBuf[i+1];
			RxBuf[RX_LEN-1] = c;
		}
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
	uchar tmp;
	WDTCN = 0xde; // 禁止看门狗
	WDTCN = 0xad;
	SYSCLK_Init (); // 初始化时钟
	PORT_Init (); // 初始化交叉开关和端口
	UART0_Init (); // 初始化 UART0
	ES0 = 1; // 允许串口中断
	EA = 1; // 允许全局中断
	tmp = NOKEY;
	ch = tmp;
	TI0 = 1; // 启动发送
	
	while (1) {    //显示屏上从左到右依次显示最近4个读入的数
		char c;
		uchar i;
		tmp = getkey();	
		TI0 = 0;
		if(tmp != NOKEY){
			ch = tmp;
			TI0 = 1;  // 启动发送
		}
		for(i = 16; i < 20; ++i) {	// 因为RX_LEN定义为20，故取四个数为16~19
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