#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define BAUDRATE 1200
void PORT_Init()
{
	XBR0 = 0x04; // UART0 使能
	XBR2 = 0x40; // 使能交叉开关
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
int main(void)
{
	int a,b;
	char op;
	int ans;
	WDTCN=0XDE; // 禁止看门狗
	WDTCN=0XAD;
	SYSCLK_Init(); // 系统时钟初始化
	PORT_Init(); // 端口初始化
	UART0_Init(); // 串口初始化
	TI0 = 1; // 准备好发送数据
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