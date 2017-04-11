#include <C8051F020.h>
#define NOKEY 255
#define uchar unsigned char
void SYSCLK_Init(void)
{
    int i;
    OSCXCN = 0x67;
    for(i=0; i<256; ++i);
    while( !(OSCXCN & 0x80) );
    OSCICN = 0x88;
}

void PORT_Init(void)
{
    SYSCLK_Init();
    EMI0CF = 0x1f;
    XBR2 = 0x42;
    P0MDOUT = 0xc0;
    P1MDOUT = 0xff;
    P2MDOUT = 0xff;
    P3MDOUT = 0xff;
}

void Delay(int k)
{
    int i;
    for(i=0; i<k; ++i);
}

unsigned char xdata seg _at_ (0x8000);
unsigned char xdata cs _at_ (0x8001);

// increase length of segs
const unsigned char code segs[] =
{0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8,
 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e, 0xff};

const unsigned char code css[] = {0x7, 0xb, 0xd, 0xe};

const uchar code dec[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0};
const uchar code trans[] = {0xc, 9, 5, 1, 0xd, 0, 6, 2, 0xe,
                            0xa, 7, 3,0xf,0xb,8, 4};

uchar getkey()	//get key number
{
    uchar i;
    uchar key;

    P4 = 0x0f;
    Delay(100);
    i = ~P4 & 0x0f;
    if(i == 0)
        return NOKEY;

    key = dec[i] * 4;
    Delay(1000);
    P4 = 0xf0;
    Delay(100);
    i = ~P4;
    i >>= 4;
    if(i == 0)
        return NOKEY;

    key += dec[i];
    key = trans[key];
    return key;
}


void main(void)
{
	unsigned int num = 0;
	unsigned char i;
	unsigned char pos[4] = {16,16,16,16};  // pos array
    WDTCN = 0xde;
    WDTCN = 0xad;

    PORT_Init();

    while(1){
        num = getkey();
		
        if(num != NOKEY){
			uchar temp0 = num, temp = num;
			while(temp0 == temp){	// verify input
				temp = getkey();
			}
			temp0 = temp;
			if (temp != NOKEY)
				continue;
			pos[3] = pos[2];
			pos[2] = pos[1];
			pos[1] = pos[0];
			pos[0] = num;	
        }

        for(i=0; i<4; ++i){
			seg = segs[pos[i]];
			//else seg = segs[16];
			cs = css[i];
			num /= 16;
			Delay(1000);
        }
    }
}



#include <C8051F020.h>
#define NOKEY 255
#define uchar unsigned char
void SYSCLK_Init(void)
{
    int i;
    OSCXCN = 0x67;
    for(i=0; i<256; ++i);
    while( !(OSCXCN & 0x80) );
    OSCICN = 0x88;
}

void PORT_Init(void)
{
    SYSCLK_Init();
    EMI0CF = 0x1f;
    XBR2 = 0x42;
    P0MDOUT = 0xc0;
    P1MDOUT = 0xff;
    P2MDOUT = 0xff;
    P3MDOUT = 0xff;
}

void Delay(int k)
{
    int i;
    for(i=0; i<k; ++i);
}

unsigned char xdata seg _at_ (0x8000);
unsigned char xdata cs _at_ (0x8001);

// increase length of segs
const unsigned char code segs[] =
{0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8,
 0x80, 0x90, 0x88, 0x83, 0xc6, 0xa1, 0x86, 0x8e, 0xff};

const unsigned char code css[] = {0x7, 0xb, 0xd, 0xe};

const uchar code dec[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0};
const uchar code trans[] = {0xc, 9, 5, 1, 0xd, 0, 6, 2, 0xe,
                            0xa, 7, 3,0xf,0xb,8, 4};

uchar getkey()	//get key number
{
    uchar i;
    uchar key;

    P4 = 0x0f;
    Delay(100);
    i = ~P4 & 0x0f;
    if(i == 0)
        return NOKEY;

    key = dec[i] * 4;
    Delay(1000);
    P4 = 0xf0;
    Delay(100);
    i = ~P4;
    i >>= 4;
    if(i == 0)
        return NOKEY;

    key += dec[i];
    key = trans[key];
    return key;
}


void main(void)
{
	unsigned int num = 0;
	unsigned int flag = 1;
	unsigned char i;
	unsigned char pos[4] = {16,16,16,16};  // pos array
    WDTCN = 0xde;
    WDTCN = 0xad;

    PORT_Init();

    while(1){
        num = getkey();
		if(num == NOKEY)
			flag = 1; 
        if(num != NOKEY && flag == 1){
			//uchar temp0 = num, temp = num;
			//while(temp0 == temp){	// verify input
			//	temp = getkey();
			//}
			
			pos[3] = pos[2];
			pos[2] = pos[1];
			pos[1] = pos[0];
			pos[0] = num;
			flag = 0;	
        }

        for(i=0; i<4; ++i){
			seg = segs[pos[i]];
			//else seg = segs[16];
			cs = css[i];
			num /= 16;
			Delay(1000);
        }
    }
}
