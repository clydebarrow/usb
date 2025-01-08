#ifndef _LCD_H_
#define _LCD_H_

/*
 *	LCD interface header file
 */

/* 	Defining CHECKBUSY will check if the LCD is busy. The RW bit of the
 * 	LCD must connected to a port of the processor for the check busy
 * 	process to work.
 *
 * 	If CHECKBUSY is not defined it will instead use a delay loop.
 * 	The RW bit of the LCD does not need to connected in this case.
 */

#define MESSAGE_LINE		0x0

#define	lcd_cursor(x)			lcd_cmd(((x)&0x7F)|0x80)
#define lcd_clear()			lcd_cmd(0x1)
#define lcd_putch(x)			lcd_data(x)
#define lcd_goto(x)			lcd_cmd(0x80|(x));
#define lcd_cursor_right()		lcd_cmd(0x14)
#define lcd_cursor_left()		lcd_cmd(0x10)
#define lcd_display_shift()		lcd_cmd(0x1C)
#define lcd_home()			lcd_cmd(0x2)

extern void lcd_cmd(unsigned char);
extern void lcd_data(unsigned char);
extern void lcd_puts(const char * s);
/*#if	DEBUG
extern void log(const char * s);
#else
#define	log(x)
#endif*/
extern void lcd_init(void);

#endif

