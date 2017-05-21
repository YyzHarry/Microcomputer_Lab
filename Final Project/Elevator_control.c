#include <c8051f020.h>
#include <stdio.h>

#define SYSCLK 22118400
#define NOKEY 255
#define OPERATION_TIME 1500
#define STOP_TIME 2000
#define STAIR_SUM 7		// 定义楼层数目

// Define Finite-State Machine
#define F0 0
#define F1 1
#define F2 2
#define F3 3
#define F4 4
#define F5 5
#define F6 6
#define F7 7
#define F8 8

typedef unsigned char uchar;

// 电梯运行状态
#define UP 0
#define DOWN 1
#define STOP 2

// 数码管设定				  
unsigned char xdata seg _at_ (0x8000);		//数码管段码地址
unsigned char xdata cs _at_ (0x8001);		//数码管位码地址
const unsigned char code segs[] =			//段码表
{0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8,
0x80, 0x90, 0x88, 0x83, 0xC6, 0xA1, 0x86, 0x8E};
const unsigned char code css[] = {0x7, 0xB, 0xD, 0xE}; 		//位码表


uchar direction = STOP;		// 定义电梯的方向
uchar state_now = F0;
uchar state_next =	F0;
unsigned int stair_now = 1;		// 目前所在的楼层

bit K1, K2, K3, K4;		// ??

uchar up_request[STAIR_SUM + 1];
uchar down_request[STAIR_SUM + 1];


void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; 
	for (i=0; i < 256; i++); 
	while (!(OSCXCN & 0x80)); 
	OSCICN = 0x88; 
}		  

void PORT_Init (void)
{
	SYSCLK_Init(); 		// 使用外部晶振
	EMI0CF = 0x1F; 		// 非复用总线，不使用内部XRAM
	XBR2 = 0x42; 		// 使用P0-P3作为总线,允许XBR
	P0MDOUT = 0xC0; 	// 推挽输出P0.6 和 P0.7
	P1MDOUT = 0xFF; 	// 高位地址
	P2MDOUT = 0xFF; 	// 地位地址
	P3MDOUT = 0xFF; 	// 数据总线
}

void Delay(int num)
{
	int i;
	for(i=0;i<num;i++);
	return;
}

uchar getkey()
{
	uchar i;
	uchar key;
	const uchar code dec[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0};
	const uchar code trans[] = {0xC, 9, 5, 1, 0xD, 0, 6, 2, 0xE, 0xA, 7, 3, 0xF, 0xB, 8, 4};

	P4 = 0x0F; 								
	Delay(100); 							
	i = ~P4 & 0x0F; 					
	if (i == 0) return NOKEY; 
	key = dec[i] * 4; 				
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

void SHOW_LED(int elevator_now) 	// 显示目前电梯所在楼层
{
	uchar i;
	int j = elevator_now;
	for (i=0; i<4; ++i) {	
		seg = segs[j % 10]; 
		cs = css[i];
		j /= 10;
		Delay(1000);
	}
}

void timer_ms(int num)
{
	int i;
	for(i=0;i<num;i++){
		SHOW_LED(stair_now);
		Delay(1000);
	}
	return;
}


void state_convert(void)
{
	switch(state_now){
		case F0: 
			stair_now = 1;
			direction = STOP;
			state_next = F1;
			break;
		case F1: 
			if(K1)
				state_next = F2;
			else if(K2)
				state_next = F6;
			else
				state_next = F1;
			break;
		case F2: 
			direction = UP;
			state_next = F3;
			break;
		case F3: 
			timer_ms(OPERATION_TIME);
			state_next = F4;
			break;
		case F4: 
			stair_now++;
			if( K3 || ((!K1)&&(!K3)) )	
				state_next = F5;
			else	
				state_next = F3;
			break;
		case F5:	// 清空已到楼层的请求标记
			if(stair_now == 1 || stair_now == 5){
				up_request[stair_now] = 0;
				down_request[stair_now] = 0;
			}
			else{
				if(direction == UP)
					up_request[stair_now] = 0;	
				else if(direction == DOWN)
					down_request[stair_now] = 0;
			}
			// direction = STOP; 
			timer_ms(STOP_TIME);
			state_next = F1;
			break;
		case F6: 
			direction = DOWN;
			state_next = F7;
			break;
		case F7: 
			timer_ms(OPERATION_TIME);
			state_next = F8;
			break;
		case F8: 
			stair_now--;
			if( K4 || ((!K2)&&(!K4)) )	
				state_next = F5;
			else	
				state_next = F7;
			break;
		default:
			state_next = F0;
			break;
	}
}


void TIMER0_ISR (void) interrupt 1 
{
	uchar i;
	
	// update K1,K2,K3,K4
	K1 = 0;
	K2 = 0;
	K3 = 0;
	K4 = 0;
	if(direction == UP){
		if(up_request[stair_now] == 1)
			K3 = 1;		// 上升过程中的停靠标志
		for(i=stair_now+1; i<=STAIR_SUM; i++){
			if(up_request[i] == 1 || down_request[i] == 1)
				K1 = 1;		// stair之上还需要停靠的标志
		}
	}
	else if(direction == DOWN){
		if(down_request[stair_now] == 1)	// 下降过程中的停靠标志
			K4 = 1;		
		for(i=stair_now-1; i>=0; i--){
			if(down_request[i]==1 || up_request[i]==1)	 // stair之下还需要停靠的标志
				K2 = 1;		
		}
	}
	
	//change the state
	state_convert();
	state_now = state_next;
}

void main(void){
	uchar temp;
	uchar ctrl;
	uchar i;

	WDTCN = 0xde;
	WDTCN = 0xad; //禁止看门狗
	SYSCLK_Init();
	PORT_Init();

	//计数器0的设置
	EA = 1;
	TMOD |= 0x02;
	TH0 = 0x00;
	TL0 = 0x00;
	TR0 = 1;
	ET0 = 1;
	P7 = 0x80;

	for(i=0; i<=STAIR_SUM; i++){
		up_request[i] = 0;
		down_request[i] = 0;
	}
	
	while(1){
		SHOW_LED(stair_now);

		while(ctrl == temp){
			SHOW_LED(stair_now);
			ctrl = getkey();
		}
		temp = ctrl;
		
		switch(ctrl){
			case 0x1:
				up_request[1] = 1;
				down_request[1] = 1;
				break;
			case 0x2:
				up_request[2] = 1;
				break;
			case 0x3:
				up_request[3] = 1;
				break;
			case 0x4:
				up_request[4] = 1;
				break;
			case 0x5:
				up_request[5] = 0;
				down_request[5] = 1;
				break;
			case 0x6:
				down_request[2] = 1;
				break;
			case 0x7:
				down_request[3] = 1;
				break;
			case 0x8:
				down_request[4] = 1;
				break;
			case 0xA:
				down_request[1] = 1;
				break;
			case 0xB:
				if(stair_now < 2)
					up_request[2] = 1;
				else if(stair_now > 2)
					down_request[2] = 1;
				break;
			case 0xC:
				if(stair_now < 3)
					up_request[3] = 1;
				else if(stair_now > 3)
					down_request[3] = 1;
				break;
			case 0xD:
				if(stair_now < 4)
					up_request[4] = 1;
				else if(stair_now > 4)
					down_request[4] = 1;
				break;
			case 0xE:
				up_request[5] = 1;
				break;
			default:
				break;
		}

	}
}