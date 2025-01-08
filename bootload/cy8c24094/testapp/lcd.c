/*
 *	LCD interface example for Cypress PSocEval1
 *
 *	Uses routines from delay.c
 *	This code will interface to a standard LCD controller
 *	like the Hitachi HD44780. It uses it in 4 or 8 bit mode
 *
 */

#include	<psoc.h>
#include	<stdio.h>
#include	<cy8c24x94.h>
#include	"lcd.h"


#define	LCD_DATA	PRT4DR
#define	LCD_READ	0x40			// port 4 bit 6	- READ
#define	LCD_WRITE	0x00			// port 4 bit 6	- /WRITE
#define	LCD_RS		0x20			// port 4 bit 5 - register select
#define	LCD_ENABLE	0x10			// port 4 bit 4 - Enable

#define	LCD_IN()	((PRT4DM1=0xF),(PRT4DM0=0x70))
#define	LCD_OUT()	((PRT4DM1=0x0),(PRT4DM0=0x7F))
#define	LCD_INIT()	(PRT4DM2=0)



unsigned char
lcd_read_cmd_nowait(void)
{
	unsigned char c;

	LCD_IN();
	LCD_DATA = LCD_READ;
	LCD_DATA = LCD_READ|LCD_ENABLE;
	c = LCD_DATA << 4;
	LCD_DATA = LCD_READ;
	LCD_DATA = LCD_READ|LCD_ENABLE;
	c |= LCD_DATA & 0xF;
	LCD_DATA = 0;
	return c;
}

void
lcd_check_busy(void) // Return when the LCD is no longer busy, or we've waiting long enough!
{
	// To avoid hanging forever in event there's a bad or
	// missing LCD on hardware.  Will just run SLOW, but still run.
	unsigned int retry;
	unsigned char c;

	for (retry=1000; retry-- > 0; ) {
		c = lcd_read_cmd_nowait();
		if (0==(c&0x80)) break; // Check busy bit.  If zero, no longer busy
	}
}

/* send a command to the LCD */
void
lcd_cmd(unsigned char c)
{
	unsigned char	v;

	lcd_check_busy();
	LCD_OUT();
	v = (c >> 4) | LCD_WRITE;
	LCD_DATA = v;
	LCD_DATA = v|LCD_ENABLE;
	LCD_DATA = v;
	v = (c & 0xF) | LCD_WRITE;
	LCD_DATA = v;
	LCD_DATA = v|LCD_ENABLE;
	LCD_DATA = v;
}

/* send data to the LCD */
void
lcd_data(unsigned char c)
{
	unsigned char	v;

	lcd_check_busy();
	LCD_OUT();
	v = (c >> 4) | LCD_WRITE|LCD_RS;
	LCD_DATA = v;
	LCD_DATA = v|LCD_ENABLE;
	LCD_DATA = v;
	v = (c & 0xF) | LCD_WRITE|LCD_RS;
	LCD_DATA = v;
	LCD_DATA = v|LCD_ENABLE;
	LCD_DATA = v;
}

void
putch(char c)
{
	lcd_data(c);
}

delayMs(unsigned char c)
{
	unsigned char	b;
	c *= 2;
	LCD_DATA = 6;

	do {
		b = 0;
		do
			;
		while(--b != 0);
	} while(--c != 0);
}

/* write a string of chars to the LCD */

void
lcd_puts(const char * s)
{
	unsigned char	c;

	while(c = *s++)
		lcd_data(c);
}

/* initialize the LCD */
void
lcd_init(void)
{

	LCD_INIT();
	LCD_DATA = 5;
	LCD_OUT();
	// delay a little
	delayMs(15);
	LCD_DATA	 = 3;
	LCD_DATA	 = 3|LCD_ENABLE;
	LCD_DATA	 = 3;
	delayMs(5);
	LCD_DATA	 = 3|LCD_ENABLE;
	LCD_DATA	 = 3;
	delayMs(1);
	LCD_DATA	 = 3|LCD_ENABLE;
	LCD_DATA	 = 3;
	lcd_check_busy();

	LCD_DATA	= 2; // Set 4-bit mode
	LCD_DATA	= 2|LCD_ENABLE;
	LCD_DATA	= 2;
	lcd_cmd(0x28); // Function Set
	lcd_cmd(0xF); //Display On, Cursor On, Cursor Blink
	lcd_cmd(0x1); //Display Clear
	lcd_cmd(0x6); //Entry Mode
	lcd_cmd(0x80); //Initialize DDRAM address to zero
}



