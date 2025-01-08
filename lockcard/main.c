#include	<8051.h>
#include	<stdio.h>
#include	<htusb.h>
#include	"lockcard.h"

/*
 * 	SD card interface:
 *
 * 	P0.0		Activity LED (red)
 * 	P0.6		Power led (green or yellow)
 * 	P0.7		Analog VREF
 *
 * 	P2.0		Keypad led output (active low)
 * 	P2.1		Keypad D0 input (zero bits)
 * 	P2.2		Keypad D1 input (one bits)
 * 	P1.7		Latch drive output
 *
 */

#define	FALSE	0
#define	TRUE	1

#define	PINRSF	0x01			// external reset
#define	PORSF	0x02			// Power on reset
#define	MCDRSF	0x04			// missing clock detect
#define	WDTRSF	0x08			// watchdog timer reset
#define	SWRSF	0x10			// software reset
#define	C0RSEF	0x20			// comparator 0 reset enable
#define	FERROR	0x40			// flash error
#define	USBRSF	0x80			// USB reset



/***********	Application code ***************/


// choose which pins the activity and power leds are on, and
// their polarity

#define	POWER_LED		(P0_BITS.B6)
#define	ACTIVITY_LED	(P0_BITS.B0)
#define	LATCH_BIT		(P1_BITS.B7)
#define	LED_BIT			(P2_BITS.B0)
#define	ACTIVE(x)		ACTIVITY_LED = !(x)
#define	AMBER(x)		POWER_LED = !(x)
//#define	LED(x)			ACTIVE(x)	// for testing purposes
#define	LED(x)			LED_BIT = !(x)
#define	LATCH(x)		LATCH_BIT = !(x)

#define	SETLED(n)		(led_patt = (n))
#define	D0				P2_BITS.B2
#define	D1				P2_BITS.B1

#define	CRC				0x95		// all purpose CRC

// Timer 2 reload period

#define	SYSCLK	24000000		// system clock freq
#define	T2PRE	12				// timer 2 prescaler
#define	T2RATE	128				// timer 2 desired interrupt rate - 128Hz

#define	T2LOAD	(SYSCLK/T2PRE/(T2RATE-1))	// reload value for T2

#define	PATTERN_RATE	(T2RATE/8/2)	// entire led pattern (8 bits) shifts in 1/2 second

static bit				active;			// busy doing something
static bit				rx_flag;		// set when a valid keypress has been received
static bit				rx_timeout;		// detect receive timeout
static bit				ticked;			// clock has ticked
static unsigned long	rx_value;		// received value
static unsigned char	timeout;		// timeout value in ticks
static unsigned char	key_timeout;	// timeout for keypress
static unsigned char	rx_count;
static unsigned int		lock_timer;
static unsigned int		tempcode_valid;
static unsigned int		tempcode;
static unsigned long	current_time;
static unsigned int		subtimer;
static unsigned char	error_count;
static unsigned char	led_patt;
static unsigned char	amber_patt;
static unsigned char	patt_count, pattern;

// led patterns
//
#define	GREEN			0xFF
#define	RED				0x00
#define	FLASH			0xF0
#define	BLUR			0x55

#define	TIMEOUT_VAL(x)	((((x)*128UL))/1000+1)
#define	SET_TIMEOUT(x)	(timeout = (((x)*128UL))/1000+1)
#define	TIMEDOUT()		(timeout == 0)

#define	MAX_CODES	10			// max number of entry codes
#define	SIGNATURE	0x73AD0F11
#define	PROGMODE_TIMEOUT	(60*T2RATE)	// how long to stay in programming mode for.

typedef struct
{
	unsigned long           signature;              // ensure we know who we are
	unsigned short          open_time;              // how long in clock ticks to open the door
	unsigned short          lockout_time;   // how long to lock out after invalid code
	unsigned short          max_errors;             // max invalid entries before lockout
	unsigned short			tempcode_validity;		// how long in minutes should temp code last
	unsigned short          master;             // Master code
	unsigned short  		codes[MAX_CODES];
	unsigned char			checksum;
}   progdata;

static const progdata	primary_list @ 0x3A00 = {
	SIGNATURE,
	4*T2RATE,
	15*T2RATE,
	3,
	1440,		// temp code lasts 24 hours
	41345,
	{
		0,
	},
	0,
};

static const unsigned char	flash[] @0;
extern xdata unsigned char	flash_write[];
#asm
global _flash_write
_flash_write equ 0
#endasm
static const progdata	secondary_list @ 0x3800 = {0};
static progdata	codelist;


static enum {
	READY, OPEN, PROGMODE, ENTERCODE, LOCKOUT
}	state;
unsigned char		codenum;

#define	KICK_DOG()		(PCA0CPH4 = 0)			// kick watchdog
#define	DELAY_MS(n)		do { SET_TIMEOUT(n); while(!TIMEDOUT()) KICK_DOG();} while(0)
#define	DELAY_SEC(n)	DELAY_MS(n*1000)

static void
writemem(unsigned int addr, unsigned int len, unsigned char  * p)
{
	char	c;
	const unsigned char *	p1;
	unsigned int			i;

	// first check that the memory is changed

	p1 = p;
	i = 0;
	for(;;) {
		if(i == len)
			return;
		if(flash[addr+i] != *p1++)
			break;
		i++;
	}
	EA = 0;
	if((addr & 0x1FF) == 0) {	// must erase the block
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 3;
		flash_write[addr] = 0xFF;
		PSCTL = 0;
	}
	while(len-- != 0) {
		KICK_DOG();
		c = *p++;
		EA = 0;
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 1;
		flash_write[addr] = c;
		EA = 1;
		addr++;
		PSCTL = 0;
	}
	EA = 1;
}

static void
update(void)
{
	unsigned char *	p;
	unsigned char	i, c;


	codelist.checksum = 0;
	p = (char *)&codelist;
	c = 0;
	i = sizeof codelist; 
	do 
		c += *p++;
	while(--i != 0);
	codelist.checksum = -c;
	writemem((unsigned int)&secondary_list, sizeof codelist, (unsigned char *)&codelist);
	writemem((unsigned int)&primary_list, sizeof codelist, (unsigned char *)&codelist);
}

static void interrupt
pca_isr(void) @ PCA
{
	if(PCA0CN & 1) {
		PCA0CN &= ~1;
		if(D1 == 0 && D0 == 1) {
			rx_timeout = 1;
			if(!rx_flag) {
				rx_count++;
				rx_value <<= 1;
			}
		}
	}
	if(PCA0CN & 2) {
		PCA0CN &= ~2;
		if(D0 == 0 && D1 == 1) {
			rx_timeout = 1;
			if(!rx_flag) {
				rx_count++;
				rx_value <<= 1;
				rx_value |= 1;
			}
		}
	}
}


static void interrupt
t2_isr(void) @ TIMER2
{
	unsigned char				i;

	TF2H = 0;					// clear interrupt flag
	ticked = 1;					// interrupt has fired
	if(--subtimer == 0) {
		subtimer = T2RATE*60;	// count minutes
		if(tempcode_valid != 0)	// decrement temp code validity
			tempcode_valid--;
	}
	if(lock_timer) {
		if(--lock_timer == 0) {
			LATCH(0);
			SETLED(RED);
			rx_count = 0;
			error_count = 0;
			amber_patt = 0x7F;
			state = READY;
		}
	}
	switch(state) {
		//case OPEN:
		//case LOCKOUT:
		//	break;		// ignore entries during these states

		case READY:
		case PROGMODE:
		case ENTERCODE:
			if(rx_count && !rx_flag) {
				if(!rx_timeout)
					rx_flag = 1;		// entry completed
				else
					rx_timeout = 0;
			}
			break;
	}
	if(timeout)
		--timeout;
	if(--patt_count == 0) {
		patt_count = PATTERN_RATE;
		pattern = (pattern << 1) | (pattern >> 7);
	}
	if(pattern & led_patt)
		LED(1);
	else
		LED(0);
	if(pattern & amber_patt)
		AMBER(1);
	else
		AMBER(0);
}


static void
CLK_init(void)
{
	unsigned char	i;
	// Clock setup

	CLKMUL = 0x80;			// enable multipler
	i = 40;
	while(--i != 0)
		continue;
	CLKMUL = 0xC0;			// initialize it (whatever that does!)
	while(!(CLKMUL & 0x20))	// wait for multiplier to stabilize
		continue;
	CLKSEL = 2;				// 24MHz SYSCLK, 48MHz USBCLK
	OSCICN = 0x83;			// enable clock
	// setup timer 2 as a periodic interrupt
	TMR2RLL = -T2LOAD & 0xFF;
	TMR2RLH = -T2LOAD/256;
	TR2 = 1;				// enable the timer - note first period will be extended.
	ET2 = 1;				// enable timer2 interrupts.
	PCA0CPL4 = 0xFF;			// watchdog timer period
	PCA0MD = 0b01100000;	// enable and lock watchdog timer
	KICK_DOG();

}

static void
PORT_init(void)
{
	// init IO ports

	P1 =	   0b11111111;
	P2 =	   0b11111111;
	P0SKIP =   0b11111111;	// skip all
	P1SKIP =   0b11111111;	// skip all
	P2SKIP =   0b001;		// skip P2.0
	P0MDOUT =  0b01010101;	// bits 6,1 and 0 are push-pull output
	//P1MDOUT =  0b10000000;	// bit 7 is push-pull output
	P2MDOUT =  0b00000001;	// bit 0 is push-pull output
	XBR0 = 	   0x0;			// disable SPI pins
	PCA0CN =   0b01000000;	// enable PCA
	PCA0CPM0 = 0b00010001;	// enable interrupt on negative edge - input 0
	PCA0CPM1 = 0b00010001;	// enable interrupt on negative edge - input 1
	EIE1  	|= 0b00010000;	// enable PCA interrupts
	XBR1 =	   0xC2;		// enable crossbar and CEX 0,1, disable weak pullups
	P0 = 	   0b01111111;	// turn on power led, others off
}

static void
enter_progmode()
{
		state = PROGMODE;
		lock_timer = PROGMODE_TIMEOUT;
		SETLED(FLASH);
}

static void
open_door(void)
{
	LATCH(1);
	state = OPEN;
	SETLED(GREEN);
	lock_timer = codelist.open_time;
	error_count = 0;
}

static void
process_code(unsigned int codeval)
{
	unsigned char	i;

	switch(state) {
		case PROGMODE:
			switch(codeval) {

				case 29:
					for(i = 0 ; i != MAX_CODES ; i++)
						codelist.codes[i] = 0;
					tempcode = 0;
					update();
					SETLED(GREEN);
					DELAY_SEC(1);
					enter_progmode();
					return;

				case 0:
					state = READY;
					SETLED(RED);
					return;

				default:
					if(codeval >= 10 && codeval < 10+MAX_CODES) {
						codenum = codeval-10;
						state = ENTERCODE;
						SETLED(BLUR);
						return;
					}
					if(codeval >= 20 && codeval < 29) {
						codenum = codeval;
						state = ENTERCODE;
						SETLED(BLUR);
						return;
					}
					SETLED(RED);
					DELAY_SEC(1);
					enter_progmode();
					return;
			}

		case ENTERCODE:
			SETLED(RED);
			switch(codenum) {
				case 20:
					if(codeval <= 9999) {
						tempcode = codeval;
						tempcode_valid = codelist.tempcode_validity;
						SETLED(GREEN);
					}
					break;

				case 21:	// open time
					codelist.open_time = codeval * T2RATE;
					SETLED(GREEN);
					break;

				case 22:	// lockout time
					codelist.lockout_time = codeval * T2RATE;
					SETLED(GREEN);
					break;

				case 23:	// errors before lockout
					codelist.max_errors = codeval;
					SETLED(GREEN);
					break;

				case 24:	// temp code validity
					tempcode_valid = codeval * 60;		// convert hours to minutes
					codelist.tempcode_validity = tempcode_valid;
					SETLED(GREEN);
					break;

				case 28:	// program master code
					if(codeval >= 10000 && codeval <= 60000) {
						codelist.master = codeval;
						SETLED(GREEN);
					}
					break;

				default:
					if(codenum < MAX_CODES && codeval <= 9999) {
						SETLED(GREEN);
						codelist.codes[codenum] = codeval;
					}
					break;
			}
			update();
			DELAY_SEC(1);
			enter_progmode();
			return;

		case OPEN:
		case LOCKOUT:
			return;
	}

	// state is READY

	if(codeval == codelist.master) {
		enter_progmode();
		return;
	}
	if(codeval == tempcode && tempcode_valid != 0) {
		open_door();
		return;
	}

	if(codeval != 0 && codeval != 0xFFFF) {
		for(i = 0 ; i != MAX_CODES ; i++) {
			if(codelist.codes[i] == codeval) {
				open_door();
				return;
			}
		}
	}
	error_count++;
	if(error_count == codelist.max_errors) {
		state = LOCKOUT;
		lock_timer = codelist.lockout_time;
		amber_patt = FLASH;
	}
}

void
press(int n)
{
	process_code(n);
	while(state == OPEN)
		KICK_DOG();
	DELAY_SEC(3);
}

// see if the nominated code list is valid
bit
verify(const progdata * list)
{
	const unsigned char *	p;
	int						check;
	unsigned char			cnt;

	if(list->signature != SIGNATURE)
		return FALSE;
	p = (const unsigned char *)list;
	check = 0;
	cnt = sizeof(*list);
	do
		check += *p++;
	while(--cnt != 0);
	return check == 0;
}


void
main(void)
{
	PORT_init();
	CLK_init();

	pattern = 1;
	amber_patt = 0xFF;
	if((RSTSRC & (PORSF|WDTRSF)) == WDTRSF)
		amber_patt = BLUR;
	patt_count = PATTERN_RATE;
	error_count = 0;
	lock_timer = 0;
	tempcode_valid = 0;
	subtimer = T2RATE*60;		// counts minutes
	rx_flag = 0;
	rx_count = 0;
	rx_value = 0;
	SETLED(RED);
	state = READY;
	EA = 1;
	if(verify(&secondary_list))
		codelist = secondary_list;
	else 
		codelist = primary_list;
	KICK_DOG();
	update();
	state = READY;
	// test code
	/*
	press(41345);
	press(10);
	press(2677);	// program a code in slot 0
	press(12);		// program a code in slot 2
	press(0x1255);
	press(21);		// program open time 
	press(5);
	press(22);		// lockout time
	press(30);
	press(23);		// errors before lockout
	press(2);
	press(24);		// temp code validity
	press(48);
	press(28);		// change master code
	press(41346);
	press(20);		// program temp code
	press(8765);
	press(0);
	//press(0x1255);		// test code
	press(8765);		// test temp code */

	for(;;) {
		KICK_DOG();
		if(rx_flag) {
			ACTIVE(1);
			process_code(rx_value);
			rx_flag = 0;
			rx_count = 0;
			rx_value = 0;
		}
		ACTIVE(0);
	}
}
#ifdef	HI_TECH_C
#asm	
	psect   text,class=CODE
	global  powerup,start1

powerup:
	; The powerup routine is needed to disable the Watchdog timer
	; before the user code, or else it may time out before main is
	; reached.
	anl     217,#-65
	jmp     start1
#endasm
#endif
