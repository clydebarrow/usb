#ifdef	HI_TECH_C
#include	<8051.h>
#else
#include	<c8051f320.h>
#include	"f320.h"
#endif

#include	<stdio.h>
#include	<htusb.h>
#include	"jtag.h"

// The USB descriptors.

extern USB_CONST USB_descriptor_table	jtag_table;

// interface table
static const dev_int * const	devices[] =
{
	&si_c2,
	&si_psoc,
};

static const dev_int *	cur_dev;

static unsigned char xdata	databuf[64];		// data packet buffer - must equal packet size

// request buffer is also in databuf

#define	JCMD	(*(xdata JCMD_t *)databuf)

// Response buffer
static JRES_t	JRES;

// choose which pins the activity and power leds are on, and
// their polarity

// Timer 2 reload period

#define	SYSCLK	24000000		// system clock freq
#define	T2PRE	12				// timer 2 prescaler
#define	T2RATE	(128-1)			// timer 2 desired interrupt rate - 128Hz

#define	T2LOAD	(SYSCLK/T2PRE/T2RATE)	// reload value for T2

unsigned char	error_code;		// output this error code on the activity led

static bit		active;			// busy doing something
bit				connected;
unsigned char	timeout;		// timeout value in ticks


static void interrupt
t2_isr(void) @ TIMER2
{
	static unsigned char		flashes, counter;
	unsigned char				i;

	TF2H = 0;					// clear interrupt flag
	if(timeout)
		--timeout;
	counter++;
	if(counter == 0) {
		if(!flashes && error_code) {
			flashes = error_code;
		}
		counter++;		// counter should never be 0
	}
	if(flashes) {
		if((counter & 0x1F) == 1)
			ACTIVE(1);		// turn led on
		else if((counter & 0x1F) == 0x10) {
			ACTIVE(0);		// turn led off
			flashes--;
		}
	}
	if(!error_code)
		ACTIVE(active);

	// make the power led flash (or not) based on certain things
	if(!connected)
		i = 0x40;
	else
		i = 0xFF;
	GREEN(counter & i);
}

static void
put_value(unsigned int val)
{
	((unsigned char *)&JRES.value)[0] = val;
	((unsigned char *)&JRES.value)[1] = val >> 8;
	((unsigned char *)&JRES.value)[2] = 0;
	((unsigned char *)&JRES.value)[3] = 0;
}


static void
send_res(void)
{
	unsigned char	i;
	unsigned char *	p;

	p = (unsigned char *)&JRES;
	i = sizeof JRES;
	do
		USB_send_byte(USB_TX, *p++);
	while(--i);
	USB_flushin(USB_TX, 0);
}

static unsigned int
get2(unsigned char xdata * p)
{
	return *p + (p[1] << 8);
}

static void
send_byte(unsigned char c)
{
	USB_send_byte(USB_TX, c);
}

static void
put2(unsigned char * p, unsigned int val)
{
	*p++ = val;
	*p = val >> 8;
}

static void
Endpoint_1(void)
{
	unsigned int	len;
	unsigned long	addr;
	unsigned char	idx, i, mem, offset;

	idx = USB_read_packet(USB_RX, databuf, sizeof databuf);
	if(idx < sizeof(JCMD_t) || JCMD.sig != JT_SIG) {
		USB_flushin(USB_TX, TRUE);
		return;
	}
	offset = sizeof(JCMD_t);
	idx -= offset;
	JRES.tag = JCMD.tag;
	JRES.sig = JT_RES;
	JRES.status = JT_OK;
	JRES.sense = 0;
	JRES.resid = 0;
	mem = JCMD.mem;
	len = get2((unsigned char xdata *)&JCMD.len);
	addr = get2((unsigned char xdata *)&JCMD.addr);
	addr += (long)get2((unsigned char xdata *)&JCMD.addr + 2) << 16;
	switch(JCMD.cmd) {

	case JT_IDENT:
		put_value(0x320);			// 8051F320 code
		send_res();
		break;

	case JT_LIST:
		len -= (sizeof devices/sizeof devices[0])*2;
		put2((unsigned char *)&JRES.resid, len);
		idx = 0;
		while(idx != sizeof devices/sizeof devices[0]) {
			send_byte(devices[idx]->dev);
			send_byte(devices[idx++]->dev >> 8);
		}
		while(len != 0) {
			send_byte(0);
			len--;
		}
		USB_flushin(USB_TX, 0);
		send_res();
		break;

	case JT_DISCONN:
		/*if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else */{
			cur_dev->disconnect();
			connected = FALSE;
		}
		send_res();
		break;

	case JT_READMEM:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			for(;;) {
				if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;
				if(idx == 0)
					break;
				if(connected && !cur_dev->readmem(databuf, idx, addr, mem & 0x7F)) {
					put2((unsigned char *)&JRES.resid, len);
					JRES.sense = JT_READFAIL;
					JRES.status = JT_FAIL;
					connected = FALSE;
					break;
				}
				i = 0;
				do
					send_byte(databuf[i]);
				while(++i != idx);
				addr += idx;
				len -= idx;
			}
		}
		while(len) {
			send_byte(0);
			len--;
		}
		//USB_flushin(USB_TX, 0);
		send_res();
		USB_flushin(USB_TX, 0);
		break;

	case JT_WRITEMEM:
		// there could be data in the balance of databuf here. idx has the
		// length, offset the position.
		while(connected) {
			if(idx != 0 && !cur_dev->writemem(databuf+offset, idx, addr, mem)) {
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_PROGFAIL;
				JRES.status = JT_FAIL;
				connected = FALSE;
				break;
			}
			addr += idx;
			len -= idx;
			if(len > sizeof databuf)
				idx = sizeof databuf;
			else
				idx = len;
			if(idx == 0)
				break;
			while(!(USB_status[USB_RX] & RX_READY))
				continue;
			idx = USB_read_packet(USB_RX, databuf, idx);
			offset = 0;
		}
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		}
		send_res();
		break;

	case JT_CONNECT:
		idx = 0;
		len = addr;
		JRES.status = JT_FAIL;
		JRES.sense = JT_BADREQ;
		do {
			if(len == devices[idx]->dev) {
				cur_dev = devices[idx];
				connected = cur_dev->connect();
				put_value(dev_id);
				if(!connected)
					JRES.sense = JT_NOCHIP;
				else
					JRES.status = JT_OK;
				break;
			}
		} while(++idx != sizeof devices/sizeof devices[0]);
		send_res();
		break;

	case JT_DEVERASE:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else if(!cur_dev->fullerase()) {
			connected = FALSE;
			JRES.sense = JT_PROGFAIL;
			JRES.status = JT_FAIL;
		}
		send_res();
		break;

	case JT_FIRMWARE:
		send_res();
		SET_TIMEOUT(500);
		while(!TIMEDOUT())
			continue;
		USB_detach();
		SET_TIMEOUT(500);
		while(!TIMEDOUT())
			continue;
		RSTSRC |= 0x10;			// software reset
		break;

	default:
		JRES.status = 1;
		JRES.sense = JT_BADREQ;
		if(mem & 0x80) {
			while(len != 0) {
				send_byte(0);
				len--;
			}
			USB_flushin(USB_TX, 0);
		} else {
			len -= idx;
			while(len != 0) {
				idx = sizeof databuf;
				if(len < sizeof databuf)
					idx = len;
				while(!(USB_status[USB_RX] & RX_READY))
					continue;
				USB_read_packet(USB_RX, databuf, idx);
				len -= idx;
			}
		}
		send_res();
		break;
	}
}

static void
CLK_init(void)
{
	// setup timer 2 as a periodic interrupt
	TMR2RLL = -T2LOAD & 0xFF;
	TMR2RLH = -T2LOAD/256;
	TR2 = 1;				// enable the timer - note first period will be extended.
	ET2 = 1;				// enable timer2 interrupts.
	EA = 1;					// enable global interrupts

}

static void
PORT_init(void)
{
	// init IO ports

	P0MDOUT = 0b01000001;	// bits 6 and 0 are push-pull output
	P1MDOUT = 0b00000000;	// All bits input
	P2MDOUT = 0b00000000;
	XBR1 =	  0x40;			// enable crossbar
	P0 = 	  0b101111110;	// turn on power led, others off
}

extern USB_CONST USB_descriptor_table	jtag_table;
unsigned long		dev_id;

void
main(void)
{
	PORT_init();
	CLK_init();
	USB_init(&jtag_table);
	for(;;) {
		active = FALSE;
		EA = 0;
		if(USB_status[0] & USB_SUSPEND) {
			// suspended - turn the LEDs off lest we draw too much current.
			ACTIVE(FALSE);
			GREEN(FALSE);
		}
		if(USB_status[0] & (RX_READY|USB_SUSPEND)) {
			EA = 1;
			active = TRUE;
			USB_control();
			continue;
		}
		if(USB_status[USB_RX] & RX_READY) {
			EA = 1;
			active = TRUE;
			Endpoint_1();
			continue;
		}
		EA = 1;
		PCON |= 1;		// sleep
	}
}

