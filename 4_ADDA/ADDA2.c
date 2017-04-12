#include <c8051f020.h>

sfr16 TMR3RL = 0x92;	//Timer3重载值
sfr16 TMR3 = 0x94;		//Timer3计数值
sfr16 ADC0 = 0xbe;		//ADC0数据寄存器

#define SYSCLK 22118400
#define SAMPLERATE 10000

//数码管显示部分
unsigned char xdata seg _at_ (0x8000);
unsigned char xdata cs _at_ (0x8001); 
const unsigned char code segs[] = 
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE}; 

void SYSCLK_Init (void);
void ADC0_Init (void);
void Timer3_Init (int counts);
void ADC0_ISR (void);
void Delay (int k);

//外部时钟初始化
void SYSCLK_Init()
{
	int i;
	OSCXCN=0x67;
	for(i=0;i<256;i++);
	while(!(OSCXCN&0x80));
	OSCICN=0x88;
}

//端口初始化，驱动外存扩展
void PORT_Init(void)
{	
	EMI0CF = 0x1F; 	//使用复用总线，不使用内部XRAM
	XBR2 = 0x42;	//使用P0-P3作为总线，允许XBR
	P0MDOUT = 0xC0; //设置总线相关端口为推挽输出 P0.6 和 P0.7
	P1MDOUT = 0xFF; //高位地址
	P2MDOUT = 0xFF; //低位地址
	P3MDOUT = 0xFF;	//数据总线
}

void Delay(int k)
{
	int i;
	for (i = 0; i < k; ++i);
}

unsigned sample;	//ADC0转换结果

void main (void) {
	WDTCN = 0xde;	//禁止看门狗
	WDTCN = 0xad;
	SYSCLK_Init ();
	Timer3_Init (SYSCLK/SAMPLERATE); 
	ADC0_Init ();	// 初始化 ADC0
	PORT_Init();	// 端口初始化，驱动外存扩展
	EA = 1;			// 允许全局中断
	EIE2 |= 0x02;	// 允许 ADC0 中断
	while (1) {
		unsigned char i;
		unsigned j;
		j = sample;					//采样结果
		
		//数码管显示
		for (i = 0; i < 4; ++i){
			seg = segs[j & 0xF]; 
			Delay(1);
			cs = css[i];
			j = j >> 4;;
			Delay(1000);
		}
	}
}
void ADC0_Init (void)
{
	ADC0CN = 0x05;
	REF0CN = 0x03;
	AMX0SL = 0x00;
	ADC0CF = (SYSCLK/2500000) << 3; //ADC 始终为2.5MHz
	ADC0CF &= ~0x07;				//PGA 增益设置为 1
	EIE2 &= ~0x02;					//禁止 ADC0 中断
	AD0EN = 1;						//允许 ADC0
}

void Timer3_Init (int counts)
{
	TMR3CN = 0x02;		//Timer3停止并清零
	TMR3RL = -counts;	//初始化重载值
	TMR3 = 0xffff;		//初始化计数值，下个周期重载
	EIE2 &= ~0x01;		//禁止 Timer3 中断
	TMR3CN |= 0x04;		//启动 Timer3
}

void ADC0_ISR (void) interrupt 15
{
	static unsigned int count = 0;
	count++;
	AD0INT = 0;			//清除 ADC 转换结束标志
	
	if (count >= SAMPLERATE / 2){  //降低sample改变的频率
		count = 0;
		sample = ADC0;	//保存转换结果
	}
}