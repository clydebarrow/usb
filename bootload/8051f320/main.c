#ifdef	HI_TECH_C
#include	<8051.h>
#else
#include	<c8051f320.h>
#include	"f320.h"
#endif

#include	<stdio.h>
#include	"bootload.h"

/*
 * 	USB Bootloader for C8051F320.
 *
 * 	Port P3.0 (C2D) is used as a bootload flag - when set, or if no
 * 	user program is loaded in flash, the program goes into bootload mode,
 * 	otherwise it runs the user program.
 *
 */

#define	FALSE	0
#define	TRUE	1

// reset source bits

#define	PINRSF	0x01			// external reset
#define	PORSF	0x02			// Power on reset
#define	MCDRSF	0x04			// missing clock detect
#define	WDTRSF	0x08			// watchdog timer reset
#define	SWRSF	0x10			// software reset
#define	C0RSEF	0x20			// comparator 0 reset enable
#define	FERROR	0x40			// flash error
#define	USBRSF	0x80			// USB reset

// choose which pins the activity and power leds are on, and
// their polarity

#define	POWER_LED		(P0_BITS.B6)
#define	ACTIVITY_LED	(P0_BITS.B0)
#define	ACTIVE(x)		ACTIVITY_LED = !(x)
#define	GREEN(x)		POWER_LED = !(x)
#define	BOOT_SEL		(!P3_BITS.B0)		// bootload if this bit pulled low

static xdata unsigned char	blkbuf[64] @ 0x0;

#define	LOCK_BYTE		(*(unsigned char code *)0x3DFF)		// flash lock byte

static void
PORT_init(void)
{
	// init IO ports

	P0MDOUT = 0b01000001;	// bits 6 and 0 are push-pull output for the LEDs
	XBR1 =	  0x40;			// enable crossbar
}

extern USB_CONST USB_descriptor_table bootload_table;
static BCMD	xdata			CMD;
static BRES xdata			RES;

static void
put_value(unsigned int val)
{
	((unsigned char xdata *)&RES.value)[0] = val;
	((unsigned char xdata *)&RES.value)[1] = val >> 8;
	((unsigned char xdata *)&RES.value)[2] = 0;
	((unsigned char xdata *)&RES.value)[3] = 0;
}


static void
send_res(void)
{
	unsigned char	i;
	unsigned char xdata *	p;

	p = (unsigned char xdata *)&RES;
	i = sizeof RES;
	do
		USB_send_byte(2, *p++);
	while(--i);
	USB_flushin(2, 0);
}

reentrant char
progmem(unsigned int addr, unsigned char len)
{
	unsigned char	i;

	EA = 0;
	if((addr & 0x1FF) == 0) {	// must erase the block
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 3;
		*(unsigned char xdata *)addr = 0xFF;
		PSCTL = 0;
	}
	for(i = 0 ; i != len ; i++) {
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 1;
		((unsigned char xdata *)addr)[i] = blkbuf[i];
		PSCTL = 0;
	}
	EA = 1;
	for(i = 0 ; i != len ; i++)
		if(((unsigned char code *)addr)[i] != blkbuf[i])
			return 0;
	return 1;
}

static void
Endpoint_1(void)
{
	unsigned int	addr;
	unsigned char	idx, len;

	USB_read_packet(1, (xdata unsigned char *)&CMD, sizeof CMD);
	if(CMD.sig != BL_SIG) {
		USB_halt(2);
		return;
	}

	RES.tag = CMD.tag;		// copy the tag ready for response
	RES.sig = BL_RES;
	RES.status = 0;
	RES.sense = 0;
	len = CMD.len;
	switch(CMD.cmd) {

	case BL_IDENT:
		put_value(0x320);			// 8051F320 code
		break;

	case BL_QBASE:
		put_value(USER_START);			// 8051F320 code
		break;

	case BL_DONE:
		send_res();
		for(idx = 0 ; ++idx ; )
			for(len = 0 ; ++len != 0 ; )
				continue;
		USB_detach();
		for(idx = 0 ; ++idx ; )
			for(len = 0 ; ++len != 0 ; )
				continue;
		PCA0MD |= 0x40;			// enable watchdog, allow it to reset us
		return;

	case BL_DNLD:
		addr =  ((unsigned char xdata *)&CMD.addr)[0] +
				(((unsigned char xdata *)&CMD.addr)[1] << 8);
		while((USB_status[1] & RX_READY) == 0)
			continue;
		USB_read_packet(1, blkbuf, len);
		if(addr < USER_START || addr +len > 0x3E00 || len > sizeof blkbuf) {
			RES.status = 1;
			RES.sense = BL_BADREQ;
			break;
		}
		if(!progmem(addr, len)) {
			RES.status = 1;
			RES.sense = BL_PROGFAIL;
		}
		break;


	default:
		RES.status = 1;
		RES.sense = BL_BADREQ;
		break;
	}
	send_res();
}
void 
main(void)
{
	unsigned int	i;

	if(LOCK_BYTE == 0xFF) {		// ensure lock byte set
		blkbuf[0] = 0b11100000;
		progmem((unsigned int)&LOCK_BYTE, 1);
	}
	// Enter bootload mode if:
	// 		* There is no user program (i.e. no LJMP at the base address), or
	// 		* If P3.0 is pulled low at reset, or
	// 		* If the reset was a software reset - this occurs when the
	// 		  application program requests bootload mode.
	// 	Otherwise just jump to the user program

	if(*(unsigned char code *)USER_START == 0x02 && !BOOT_SEL
			&& (RSTSRC & (PORSF|SWRSF)) != SWRSF)
		(*(void (*)(void))USER_START)();

	PORT_init();
	EA = 1;

	USB_init(&bootload_table);
	for(;;) {
		i++;
		if(USB_status[0] & USB_SUSPEND) {
			GREEN(FALSE);
		} else
			GREEN(i & 0x8000);
		ACTIVE(FALSE);
		if(USB_status[0] & (RX_READY|USB_SUSPEND))
			USB_control();
		if(USB_status[1] & RX_READY) {
			ACTIVE(TRUE);
			Endpoint_1();
		}
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

