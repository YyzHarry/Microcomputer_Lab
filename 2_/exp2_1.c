#include <C8051F020.h> 

void PORT_Init (void){
	P74OUT = 0x80;
}

void Delay(){
	int i, j;
	for (i = 0; i < 500; ++i)
	for (j = 0; j < 100; ++j);
}

void main(void){
	WDTCN = 0xde;
	WDTCN = 0xad;
	PORT_Init();
	while (1) {
		P7 = 0x80;
		Delay();
		P7 = 0;
		Delay();
	}
}