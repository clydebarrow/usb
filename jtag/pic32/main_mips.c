#ifdef	HI_TECH_C
#include	<8051.h>
#else
#include	<c8051f320.h>
#include	"f320.h"
#endif

#include	<stdio.h>
#include	<htusb.h>
#include	"jtag_mips.h"
#include	"mips.h"

// The USB descriptors.

extern USB_CONST USB_descriptor_table	jtag_table;

// interface table
static const dev_int * const	devices[] =
{
	&mips_interface,
};

static const dev_int *	cur_dev;


//	buffer for command requests

//static xdata JCMD_t	JCMD;
//
#define	JCMD	(*(xdata JCMD_t *)databuf)
//

// Response buffer
static JRES_t	JRES;
static unsigned char xdata	databuf[64];		// data packet buffer

// choose which pins the activity and power leds are on, and
// their polarity

// Timer 2 reload period

#define	SYSCLK	24000000		// system clock freq
#define	T2PRE	12				// timer 2 prescaler
#define	T2RATE	(128-1)			// timer 2 desired interrupt rate - 128Hz

#define	T2LOAD	(SYSCLK/T2PRE/T2RATE)	// reload value for T2

unsigned char	error_code;		// output this error code on the activity led

static bit		active;			// busy doing something
unsigned char connected = FALSE;
unsigned char	timeout;		// timeout value in ticks


extern unsigned long test;
xdata static unsigned char FW_VERSION;



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
put_long_value(unsigned long val)
{
	((unsigned char *)&JRES.value)[0] = val;
	((unsigned char *)&JRES.value)[1] = val >> 8;
	((unsigned char *)&JRES.value)[2] = val >> 16;
	((unsigned char *)&JRES.value)[3] = val >> 24;
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
	USB_flushin(USB_TX, 1);
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
	unsigned char	idx, i, result,mem, offset;
	unsigned long word = 0, ret_val;
	/*USB_read_packet(USB_RX, (xdata unsigned char *)&JCMD, sizeof JCMD);
	if(JCMD.sig != JT_SIG) {
		USB_flushin(USB_TX, TRUE);
		return;
	}
	JRES.tag = JCMD.tag;
	JRES.sig = JT_RES;
	JRES.status = JT_OK;
	JRES.sense = 0;
	JRES.resid = 0;
	len = get2((unsigned char xdata *)&JCMD.len);
	addr = get2((unsigned char xdata *)&JCMD.addr);
	addr += (long)get2((unsigned char xdata *)&JCMD.addr + 2) << 16;
*/

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

	case JT_INIT:
		//Drive TMS 0
		//Delay for 5 TCK clock cycles
		//
		break;
	/*case JT_IDENT:
		put_value(0x320);			// 8051F320 code
		send_res();
		break;*/

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
		cur_dev->disconnect();
		connected = FALSE;
		send_res();
		break;
	case JT_RUN:

		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;

		} else if(cur_dev->go() == FALSE)
			{
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_STOPERR;
				JRES.status = JT_FAIL;
				connected = FALSE;
			}



		send_res();
		break;
	case JT_STEP:

		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;

		} else if(cur_dev->step() == FALSE)
			{
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_STOPERR;
				JRES.status = JT_FAIL;
				connected = FALSE;
			}



		send_res();
		break;

	case JT_STOP:
		//mips_save_dbgsts();

		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;

		} else if(cur_dev->stop() == FALSE)
			{
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_STOPERR;
				JRES.status = JT_FAIL;
				//connected = FALSE;
			}


		send_res();
		break;

	case JT_CONNECT:
		len = get2((unsigned char xdata *)&JCMD.addr);
		idx = 0;
		JRES.status = JT_FAIL;
		JRES.sense = JT_BADREQ;
		while(idx != sizeof devices/sizeof devices[0]) {
			if(len == devices[idx]->dev) {
				cur_dev = devices[idx];
				connected = cur_dev->connect();

				put_long_value(dev_id);
				if(!connected)
					JRES.sense = JT_NOCHIP;
				else
					JRES.status = JT_OK;
				break;
			}
			idx++;
		}
		send_res();
		break;
	case JT_READREG:
		if(!connected /*|| (is_stoped == 0)*/) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			ret_val = cur_dev->readreg(addr);
		}
		test = ret_val;

		put_long_value(ret_val);
	/*	for(i=0; i < len; i++)
		{
			send_byte((ret_val >> i*8) & 0xFF );
		}*/
		send_res();
		//USB_flushin(USB_TX, 1);
		break;
#if 0
	case JT_READPC:
		if(!connected /*|| (is_stoped == 0)*/) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			ret_val = cur_dev->readpc();
		}
		for(i=0; i < len; i++)
		{
			send_byte((ret_val >> i*8) & 0xFF );
		}
		USB_flushin(USB_TX, 0);
		send_res();
		break;
#endif
	case JT_DEBUG:
		for(i=0; i < len; i++)
		{
			send_byte((test >> i*8) & 0xFF );
		}


		send_res();
		//USB_flushin(USB_TX, 1);
		break;
	case JT_CHIPERASE:
		cur_dev->chiperase();
		send_res();
		break;
	case JT_VERSION:

		//for(i=0; i < len; i++)
		//{
			send_byte(FW_VERSION);// >> i*8) & 0xFF );
		//}
		send_res();
		break;
	case JT_WRITEREG:
	{
		if(!connected /*|| (is_stoped == 0)*/) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			while(len != 0) {

				word = databuf[offset+0];
				word |= ((unsigned long)databuf[offset+1]<<8);
				word |= ((unsigned long)databuf[offset+2]<<16);
				word |= ((unsigned long)databuf[offset+3]<<24);

				result = cur_dev->writereg(addr, word);

				if(result == FALSE){

					put2((unsigned char *)&JRES.resid, len);
					JRES.sense = JT_READFAIL;
					JRES.status = JT_FAIL;
					//connected = FALSE;

				}
				len -= idx;
				addr += 4;
				if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;
				if (idx == 0)
					break;

				while(!(USB_status[USB_RX] & RX_READY))
					continue;

				idx = 	USB_read_packet(USB_RX, databuf, idx);
				len = 0;


			}
		}
		send_res();
		break;
	}
	case JT_STARTFROM:
	{
		if(!connected /*|| (is_stoped == 0)*/) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {


			result = cur_dev->startfrom(addr);

			if(result == FALSE){
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_READFAIL;
				JRES.status = JT_FAIL;
				//connected = FALSE;
			}
			len = 0;
		}
		send_res();
		break;
	}


	case JT_READMEM:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			while(1) {
				if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;


				if(cur_dev->readmem(databuf, idx, addr) == FALSE) {
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
				len  -= idx;
				if(len == 0){
					break;
				}
			}
		}
		/*while(len) {
			send_byte(0);
			len--;
		}*/
		send_res();
		//USB_flushin(USB_TX, 1);
		break;
	case JT_WRITEMEM:
		if(!connected  ) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			while(len != 0) {
				/*if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;
			while(!(USB_status[USB_RX] & RX_READY))
				continue;*/


			if(connected && !cur_dev->writemem(databuf+offset, idx, addr, JCMD.mem)) {
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
		}
		send_res();
		break;

	case JT_EXECINSTR:
		if(!connected /*|| (is_stoped == 0)*/) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			//while(len != 0) {
				/*if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;
				while(!(USB_status[USB_RX] & RX_READY))
					continue;
				USB_read_packet(USB_RX, databuf, idx);*/
				/*for(i=0; i < idx;i+=4){
					word = databuf[0];
					word |= ((unsigned long)databuf[1]<<8);
					word |= ((unsigned long)databuf[2]<<16);
					word |= ((unsigned long)databuf[3]<<24);*/
					cur_dev->execinstr(0);
					ret_val = cur_dev->execinstr(addr);
					cur_dev->execinstr(0);
				//}


				for(i=0; i < len; i++)
				{
					send_byte((ret_val >> i*8) & 0xFF );
				}

				//USB_flushin(USB_TX, 0);
				len = 0;
			//}
		}
		send_res();
		//USB_flushin(USB_TX, 1);
		break;

	case JT_FDATA:
		if(!connected  ) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {

			while(len != 0) {
				len -= idx;
				i = offset;
				while(idx > 0 ){

					//take work by word and execute fast data
					//just use existing addr variable
					addr = databuf[i];
					addr = addr | ((unsigned long)databuf[i+1])<<8;
					addr = addr | ((unsigned long)databuf[i+2])<<16;
					addr = addr | ((unsigned long)databuf[i+3])<<24;

					idx-=4;
					i+= 4;

					test = addr;
					ret_val = cur_dev->fastdata(addr, JCMD.mem);

				}

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
		}
		for(i=0; i < 4; i++)
		{
			send_byte((ret_val >> i*8) & 0xFF );
		}

		send_res();
		//USB_flushin(USB_TX, 1);
		break;

	/*case JT_FDATA:
		if(!connected ) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			test = JCMD.mem;

			//ret_val = cur_dev->execinstr(0);
			ret_val = cur_dev->fastdata(addr, JCMD.mem);

			for(i=0; i < 4; i++)
			{
				send_byte((ret_val >> i*8) & 0xFF );
			}
			USB_flushin(USB_TX, 0);
		}
		send_res();
		break;*/





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
	case JT_ISRUNNING:

		put_value(cur_dev->isrunning());
		send_res();
		break;

	default:
		JRES.status = 1;
		JRES.sense = JT_BADREQ;
		if(JCMD.mem & 0x80) {
			while(len != 0) {
				send_byte(0);
				len--;
			}
			USB_flushin(USB_TX, 0);
		} else {
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
	//P2MDOUT = 0b00000000;
	XBR1 =	  0x40;			// enable crossbar
	P0 = 	  0b101111110;	// turn on power led, others off
	//Configure P2
	//P2.2 P2.1 P2.4 to output and P2.0 for input
	//P2 Input config register 0xF3
	P2MDIN  |= 0x1;//P2.0 as input
	P2MDOUT |= 0x0E; // p2.1, p2.2 to be pushpul
	P1MDOUT |= 0xC0; // p1.7 to be pushpul

}

extern USB_CONST USB_descriptor_table	jtag_table;
unsigned long		dev_id;

void
main(void)
{
	PORT_init();
	CLK_init();
	USB_init(&jtag_table);
	FW_VERSION = 0x13;
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

		/*if(connected){
			is_stoped = cur_dev->isstopped();
			if((is_stoped == 1) && (pcadjustd == 1)){
				cur_dev->stop();
				mips_restore_pc(0xC);
				pcadjustd = 2;
			}
		}*/
		EA = 1;
		PCON |= 1;		// sleep



	}
}

