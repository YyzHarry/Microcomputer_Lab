#include <c8051f020.h>
#include <stdio.h>
#define SYSCLK 22118400
#define BAUDRATE 1200
#define I2CCLK 80000	//80kHz

#define SMB_START 0x08
#define SMB_RP_START 0x10
#define SMB_MTADDACK 0x18
#define SMB_MTDBACK 0x28
#define SMB_MRADDACK 0x40
#define SMB_MRDBACK 0x50
#define SMB_MRDBNACK 0x58
#define SMB_MTADDNACK 0x20
#define SMB_MTDBNACK 0x30
#define SMB_MTARBLOST 0x38
#define SMB_MRADDNACK 0x48

#define S_OVER 0x01    // 随意写一个不为0的数
#define R_OVER 0x01    // 随意写一个不为0的数
#define SMB_FAIL 0x01    // 随意写一个不为0的数


unsigned char smb_buf[10];
int smb_len;
unsigned char result;

void SYSCLK_Init (void)
{
	int i;
	OSCXCN = 0x67; 
	for (i=0; i < 256; i++); 
	while (!(OSCXCN & 0x80)); 
	OSCICN = 0x88; 
}

void PORT_Init()
{
	XBR0 = 0x05; 
	XBR2 = 0x40; 
	P0MDOUT |= 0x01;
}

void UART0_Init (void)
{
	SCON0 = 0x50;
	TMOD = 0x20; 
	TH1 = -(SYSCLK/BAUDRATE/16/12); 
	TR1 = 1; 
	PCON |= 0x80; 
}

void I2C_Init()
{
	SMB0CN = 0x07; 
	SMB0CR = 257 - (SYSCLK / (2 * I2CCLK)); 
	SMB0CN |= 0x40; 
	STO = 0;
}

void Timer0_ms(long num)
{
	long i;
	for(i=0;i<200*num;i++);
	return;
}

void Delay(int num)
{
	int i;
	for(i=0;i<num;i++);
	return;
}

void SMB_Transmit(unsigned char addr, unsigned char len)
{
	result = 0; 			
	smb_buf[0] = 0xD0;
	smb_buf[1] = addr;
	smb_len = len; 		
	STO = 0; 					
	STA = 1;
	while (result == 0);
	Delay(100);
}

void SMB_Receive(unsigned char len)
{
	result = 0; 					
	smb_buf[0] = 0xD1; 		
	smb_len = len;				
	STO = 0; 							
	STA = 1;
	while (result == 0);	
	Delay(100);
}

void SMBUS_ISR (void) interrupt 7
{
	bit FAIL = 0; 
	static unsigned char i; 
	switch (SMB0STA){
		case SMB_START: 				
		case SMB_RP_START: 			
			SMB0DAT = smb_buf[0]; 
			STA = 0; 							
			i = 0; 								
			break;
		case SMB_MTADDACK: 						
			SMB0DAT = smb_buf[1]; 			
			break;
		case SMB_MTDBACK: 						
			if (i < smb_len){
				SMB0DAT = smb_buf[2 + i]; 
				i++; 
			} 
			else {
				result = S_OVER; 	
				STO = 1; 					
			}
			break;
		case SMB_MRADDACK: 		
			if (smb_len == 1) 	
				AA = 0; 					
			else
				AA = 1; 					
			break;
		case SMB_MRDBACK: 		
			if (i < smb_len){
				smb_buf[i + 1] = SMB0DAT; 
				i++; 
				AA = 1; 
			}
			if (i >= smb_len) 
				AA = 0; 
			break;
		case SMB_MRDBNACK: 
			smb_buf[i + 1] = SMB0DAT; 
			STO = 1; 
			AA = 1; 	
			result = R_OVER; 
			break;
		case SMB_MTADDNACK:
		case SMB_MTDBNACK: 
		case SMB_MTARBLOST:
		// Master Receiver: Slave address + READ transmitted. NACK received.
		case SMB_MRADDNACK: 
			FAIL = 1; 
			break;
		default:
			FAIL = 1;
			break;
	}
	if (FAIL){
		SMB0CN &= ~0x40; 
		SMB0CN |= 0x40;
		STA = 0;
		STO = 0;
		AA = 0;
		result = SMB_FAIL; 
		FAIL = 0;
	}
	SI = 0; 
}


void main()
{
	int i;
	char c;
	WDTCN=0xDE;
	WDTCN=0xAD;
	SYSCLK_Init();
	PORT_Init();
	UART0_Init();
	TI0 = 1;
	I2C_Init();
	SI = 0; 						
	EIE1 |= 0x02; 			
	EA = 1; 						
	SMB_Transmit(0, 0); 
	SMB_Receive(1); 
	
	// CH=1(CLK is stopped), start the clock
	if (smb_buf[1] & 0x80) { 
		unsigned char b = smb_buf[1];
		smb_buf[2] = b & 0x7F; 
		SMB_Transmit(0, 1); 
	}
	
	//scanf("%02bx %02bx %02bx %02bx %02bx %02bx",&smb_buf[8],&smb_buf[7],&smb_buf[6],&smb_buf[4],&smb_buf[3],&smb_buf[2]);
	//SMB_Transmit(0, 7);
	
	while(1) {
		c = getchar();	// 当输入是c时，设置串口的值
		if(c == 'c'){
			scanf("%02bx %02bx %02bx %02bx %02bx %02bx",&smb_buf[8],&smb_buf[7],&smb_buf[6],&smb_buf[4],&smb_buf[3],&smb_buf[2]);
			SMB_Transmit(0, 7);	
		}
		SMB_Transmit(0, 0);
		SMB_Receive(8); 
		Timer0_ms(500);
		// 否则显示目前时钟值
		// year-month-day hour:minute:second
		printf("\r\n%02bx-%02bx-%02bx %02bx:%02bx:%02bx\r\n",
			   smb_buf[7],smb_buf[6],smb_buf[5],smb_buf[3],smb_buf[2],smb_buf[1]);
	}
}