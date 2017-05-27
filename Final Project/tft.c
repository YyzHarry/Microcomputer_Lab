#include <C8051F020.h>
#include "font.h"

#define YELLOW 0xFF00

unsigned char xdata reset _at_ (0x8003);
unsigned char xdata cmd _at_ (0x8002);
unsigned char xdata mydata _at_ (0x9002);

void Delay(int k)
{
    int i;
    for (i = 0; i < k; ++i);
}

void SYSCLK_Init()
{
    OSCXCN = 0x67;  // enabled external crystal
    while (!(OSCXCN & 0x80));  // wait for stable
    OSCICN = 0x88;  // use external crystal
}

/* init the emif using the lower port  */
void EMIF_Low(void)
{
    SYSCLK_Init();
    EMI0CF = 0x1F; // non-multiplexed mode, external only
    XBR2 = 0x42;    // Enable xbr
    P0MDOUT = 0xC0;
    P1MDOUT = 0xFF;
    P2MDOUT = 0xFF;
    P3MDOUT = 0xFF;
}

void lcd_init9481(void)
{
    reset = 1;
    Delay(200);
    cmd = 0x11; // Exit from sleeping
    Delay(3000);
    cmd = 0xD0; // Power Setting
    mydata = 0x07;
    mydata = 0x41;
    mydata = 0x1D;
    mydata = 0x0D;

    cmd = 0xD1; // VCOM Control
    mydata = 0x00;
    mydata = 0x2B;
    mydata = 0x1F;

    cmd = 0x0C; // get pixel format (why get?)
    mydata = 0x55;
    cmd = 0x3A; // set pixel format
    mydata = 0x55; // 16bit/pixel
    cmd = 0xB4; // Display mode;
    mydata = 0;

    cmd= 0xC0; // Panel Driving Setting
    mydata = 0;
    mydata = 0x3B;
    mydata = 0x0;
    mydata = 0x2;
    mydata = 0x11;
    mydata = 0;

    cmd = 0xC5; // Frame rate and Inversion Control
    mydata = 0x03;

    cmd = 0xC8;  // Gamma Setting
    mydata = 0;
    mydata = 14;
    mydata = 0x33;
    mydata = 0x10;
    mydata = 0x00;
    mydata = 0x16;
    mydata = 0x44;
    mydata = 0x36;
    mydata = 0x77;
    mydata = 0x00;
    mydata = 0x0F;
    mydata = 0x00;

    cmd = 0xF3;
    mydata = 0x40;
    mydata = 0x0A;

    cmd = 0x36; // Address Mode
    mydata = 0x0A;

    cmd = 0xF0;
    mydata = 0x08;

    cmd = 0xF6;
    mydata = 0x84;
    cmd = 0xF7;
    mydata = 0x80;
    cmd = 0x36;  // Address Mode;
    mydata = 0x0A;

    Delay(3000);
    cmd = 0x29;  // Set display on
}

void lcd_init9486(void) /* ZHA */
{
    reset = 1;
    Delay(200);
    cmd = 0xF2;
    mydata = 0x18;
    mydata = 0xA3;
    mydata = 0x12;
    mydata = 0x02;
    mydata = 0xB2;
    mydata = 0x12;
    mydata = 0xFF;
    mydata = 0x10;
    mydata = 0x00;
    cmd = 0xF8;
    mydata = 0x21;
    mydata = 0x04;
    cmd = 0xF9;
    mydata = 0x00;
    mydata = 0x08;
    cmd = 0x36;
    mydata = 0x08;
    cmd = 0x3A;
    mydata = 0x05;
    cmd = 0xB4;
    mydata = 0x01;//0x00
    cmd = 0xB6;
    mydata = 0x02;
    mydata = 0x22;
    cmd = 0xC1;
    mydata = 0x41;
    cmd = 0xC5;
    mydata = 0x00;
    mydata = 0x07;//0x18
    cmd = 0xE0;
    mydata = 0x0F;
    mydata = 0x1F;
    mydata = 0x1C;
    mydata = 0x0C;
    mydata = 0x0F;
    mydata = 0x08;
    mydata = 0x48;
    mydata = 0x98;
    mydata = 0x37;
    mydata = 0x0A;
    mydata = 0x13;
    mydata = 0x04;
    mydata = 0x11;
    mydata = 0x0D;
    mydata = 0x00;
    cmd = 0xE1;
    mydata = 0x0F;
    mydata = 0x32;
    mydata = 0x2E;
    mydata = 0x0B;
    mydata = 0x0D;
    mydata = 0x05;
    mydata = 0x47;
    mydata = 0x75;
    mydata = 0x37;
    mydata = 0x06;
    mydata = 0x10;
    mydata = 0x03;
    mydata = 0x24;
    mydata = 0x20;
    mydata = 0x00;
    cmd = 0x11;
    Delay(200);
    cmd = 0x29;
}

void dis_color(unsigned int c)
{
    int i, j;
     cmd = 0x2C; // start write
    for (i = 0; i < 480; ++i)
        for (j = 0; j < 320; ++j) {
            mydata = c >> 8;
            mydata = c & 0xFF;
        }
}

void show_char(unsigned int x, unsigned int y, unsigned int color, unsigned char f)
{
    unsigned char i, j, c;
    cmd = 0x2A; // set column address
    mydata = (x >> 8) & 1;
    mydata = (x & 0xFF);
    mydata = ((x + 7) >> 8) & 1;
    mydata = (x + 7) & 0xFF;
    cmd = 0x2B; // set page address
    mydata = (y >> 8) & 1;
    mydata = (y & 0xFF);
    mydata = ((y + 11) >> 8) & 1;
    mydata = (y + 11) & 0xFF;
    cmd = 0x2C;
    for (i = 0; i < 12; ++i) {
        c = font_8x12[f][i];
        for (j = 0; j < 8; ++j) {
            if (c & 1) {
                mydata = color >> 8;
                mydata = color & 0xFF;
            } else {
                mydata = 0xff;
                mydata = 0;
            }
            c = c >> 1;
        }
    }
}

void refresh_scrn()		// Ë¢ÐÂÆÁÄ»
{
	int i, j;
	for (i=0; i<359; i+=8){
		for (j=0; j<479; j+=8){
			show_char(i, j, YELLOW, 0xDB);
		}
	}
}

void main(void)
{
    WDTCN = 0xde;
    WDTCN = 0xad; // Disable watchdog
    EMIF_Low();
    EMI0TC = 0x41;
    lcd_init9486();
    dis_color(0xFF00);
	refresh_scrn();
    while (1) {
    }
}
