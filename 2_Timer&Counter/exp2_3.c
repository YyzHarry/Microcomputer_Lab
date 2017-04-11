#include <C8051F020.h>

void PORT_Init (void){
	P74OUT = 0x80;
}

void Delay(){
	int i, j;
	for (i = 0; i < 500; ++i)
		for (j = 0; j < 100; ++j);
}

void SYSCLK_Init(){
	int i;
	OSCXCN = 0x67;
	for(i=0;i<256;i++);
	while(!(OSCXCN&0x80));
	OSCICN = 0x88;
}

void main(){
	WDTCN = 0xDE;
	WDTCN = 0xAD;
	SYSCLK_Init();
	PORT_Init();
	EA = 1;
	TMOD |= 0x02;
	TH0 = 0x00;
	TL0 = 0x00;
	TR0 = 1;
	ET0 = 1;
	P7 = 0x80;
	while(1);
}

//counter 0 interrupt
void TIMER0_ISR (void) interrupt 1{
	static int count; 			//control dutycycle's change freq.
	static int count2;			//control LED's freq.
	static int glow;		//control dutycycle to change brightness
	static bit direction;	//a bit controls direction of glow, 0 for +, 1 for -
	count++;
	count2++;

	if(count2 <= glow){
		P7 = 0x80;					//LED glows
	}
	else if(count2 > 48){   //150Hz is counted as 48
		count2 = 0;
	}
	else{
		P7 = 0x00;					//LED extinguishes
	}

	//count decides changing freq. of dutycycle 3600~0.5s
	//every 0.5s change dutycycle once
	if (count > 3600){
		count = 0;
		//initial value between 1~24
		if(glow>24 || glow<1)
			glow = 1;

		//dutycycle increase
		if(direction == 0){
			glow++;
			//glow=24 == dutycycle=50%, then decrease to 0
			if(glow > 24){
				direction =~ direction;
				glow--;
			}
		}
		//dutycycle decrease
		else{
			glow--;
			if(glow < 1){
				direction =~ direction;
				glow++;
			}
		}
	}
}
