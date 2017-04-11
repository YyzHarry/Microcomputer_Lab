#include <C8051F020.h> 

void PORT_Init (void)
{
	P74OUT = 0x80;
}
void Delay()
{
	int i, j;
	for (i = 0; i < 500; ++i)
	for (j = 0; j < 100; ++j);
}
void SYSCLK_Init()
{
	int i;
	OSCXCN=0x67;
	for(i=0;i<256;i++);
	while(!(OSCXCN&0x80));
	OSCICN=0x88;
}
void main()
{
	WDTCN=0xDE;
	WDTCN=0xAD;
	SYSCLK_Init();
	PORT_Init();
	EA = 1;
	TMOD |= 0x02;
	TH0 = 0x00;
	TL0 = 0x00;
	TR0 = 1;
	ET0 = 1;
	P7 = 0x80;
	while (1);
}
void TIMER0_ISR (void) interrupt 1 
{
	static int count; 
	count++;
	if (count > 3600){ 
	count = 0;
	P7 = ~P7;
	}
}