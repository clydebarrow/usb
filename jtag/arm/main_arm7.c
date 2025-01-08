#ifdef	HI_TECH_C
#include	<8051.h>
#else
#include	<c8051f320.h>
#include	"f320.h"
#endif

#include	<stdio.h>
#include	<htusb.h>
#include	"jtag_arm.h"
#include	"arm.h"

// The USB descriptors.

extern USB_CONST USB_descriptor_table	jtag_table;

// interface table
static const dev_int * const	devices[] =
{
	&arm7_interface,
};

static const dev_int *	cur_dev;


//	buffer for command requests

static xdata JCMD_t	JCMD;

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
//bit				connected;
unsigned char connected = FALSE;
unsigned char	timeout;		// timeout value in ticks

xdata unsigned char   is_stoped 	= 0;
xdata unsigned char pcadjustd 		= 0;

extern unsigned char arm_restore_pc(long numdecrement);
//extern xdata break_points_st g_break_points_t[20];
//extern xdata int g_no_breakpoints;
extern void arm_select_scanchain(unsigned char scan_no);



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
	unsigned char	idx, i, result;
	unsigned long word = 0;

	USB_read_packet(USB_RX, (xdata unsigned char *)&JCMD, sizeof JCMD);
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
	

	

	
	switch(JCMD.cmd) {

	case JT_INIT:
		//Drive TMS 0
		//Delay for 5 TCK clock cycles
		//
		break;
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

	case JT_RUN:
		//arm_save_dbgsts();
		//cur_dev->stop();
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
			
		} else if(cur_dev->go() == FALSE)
			{
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_STOPERR;
				JRES.status = JT_FAIL;
				connected = FALSE;
				pcadjustd = 1;		
			}
			//arm_restore_dbgsts();
			pcadjustd = 1;		
		
		
		send_res();
		break;

	/*case JT_STOP:
		arm_save_dbgsts();		
		
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
			
		} else if(cur_dev->stop() == FALSE)
			{
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_STOPERR;
				JRES.status = JT_FAIL;
				connected = FALSE;
			}
		
		
		send_res();
		break;*/

	case JT_READMEM:
		if(!connected || (is_stoped == 0)) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			while(len != 0) {
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
				len -= idx;
			}
		}
		while(len) {
			send_byte(0);
			len--;
		}
		USB_flushin(USB_TX, 0);
		send_res();
		break;

	case JT_READREG:
		if(!connected || (is_stoped == 0)) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			if(addr == ARM_CPSR){
				word = cur_dev->readcpsr();
			}else{
				word = cur_dev->readreg(addr);
				
			}
		
			for(i=0; i < len; i++)
			{
				send_byte((word >> i*8) & 0xFF );
			}
				

		}
	
		USB_flushin(USB_TX, 0);
		send_res();
		break;
	case JT_ISSTOP:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
		
			send_byte(cur_dev->isstopped() );		
			
			

		}
		
		USB_flushin(USB_TX, 0);
		send_res();
		break;
	case JT_WRITEDBG:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			if(len != 0) {
				if(len > sizeof databuf)
				idx = sizeof databuf;
				else
					idx = len;
				while(!(USB_status[USB_RX] & RX_READY))
				continue;
			
				arm_select_scanchain(SCAN_CHAIN_2);
				USB_read_packet(USB_RX, databuf, idx);
				for(i=0; i< idx ;)
				{
					word = databuf[i++];
					word |= ((unsigned long)databuf[i++] <<8);
					word |= ((unsigned long)databuf[i++]<<16);
					word |= ((unsigned long)databuf[i++]<<24);

					cur_dev->write_dbgreg(addr & 0xFF, word);
					addr++;
				}
			}
		}		
		send_res();
		break;
	case JT_READDBG:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {

			while(len != 0) {
				if(len > sizeof databuf){
					idx = sizeof databuf;
				}else{
					idx = len;
				}
				arm_select_scanchain(SCAN_CHAIN_2);
				for(i = 0; i< idx; i++){
					word = cur_dev->read_dbgreg(addr & 0xFF);
					databuf[i+0] = word     & 0xFF;
					databuf[i+1] = (word>>8 ) & 0xFF; 
					databuf[i+2] = (word>>16) & 0xFF; 
					databuf[i+3] = (word>>24) & 0xFF; 
				}
				
				
				i = 0;
				
				do
					send_byte(databuf[i]);
				while(++i != idx);
				addr += idx;
				len -= idx;
			}			
		}		
		send_res();
		break;

	/*case JT_SETBRK:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			if(cur_dev->set_break(addr, 0) == FALSE){
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_READFAIL;
				JRES.status = JT_FAIL;
				connected = FALSE;
			}
		}		
		send_res();
		break;

	case JT_CLRBRK:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			if(cur_dev->clear_break(addr) == FALSE){
				put2((unsigned char *)&JRES.resid, len);
				JRES.sense = JT_READFAIL;
				JRES.status = JT_FAIL;
				connected = FALSE;
			}
		}		
		send_res();
		break;*/



	case JT_WRITEREG:
	{
		if(!connected || (is_stoped == 0)) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			while(len != 0) {
				if(len > sizeof databuf)
					idx = sizeof databuf;
				else
					idx = len;
				while(!(USB_status[USB_RX] & RX_READY))
					continue;
				USB_read_packet(USB_RX, databuf, idx);
				word = databuf[0];
				word |= ((unsigned long)databuf[1]<<8);
				word |= ((unsigned long)databuf[2]<<16);
				word |= ((unsigned long)databuf[3]<<24);
					
				if(addr == ARM_CPSR){
					result = cur_dev->writecpsr( word);
				}else{
					result = cur_dev->writereg(addr, word);
				}
				if(result == FALSE){
			
					put2((unsigned char *)&JRES.resid, len);
					JRES.sense = JT_READFAIL;
					JRES.status = JT_FAIL;
					connected = FALSE;

				}
				

				len = 0;
			}
		}
		send_res();
		break;
	}
	case JT_WRITEMEM:
		if(!connected || (is_stoped == 0)) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else {
			
			//if(is_stoped == TRUE){
				while(len != 0) {
					if(len > sizeof databuf)
						idx = sizeof databuf;
					else
						idx = len;
				while(!(USB_status[USB_RX] & RX_READY))
					continue;
				USB_read_packet(USB_RX, databuf, idx);
				if(connected && !cur_dev->writemem(databuf, idx, addr, JCMD.mem)) {
					put2((unsigned char *)&JRES.resid, len);
					JRES.status = JT_FAIL;
					connected = FALSE;
				}
				addr += idx;
				len -= idx;
				}
			//}
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

	/*case JT_DEVERASE:
		if(!connected) {
			JRES.sense = JT_NOCHIP;
			JRES.status = JT_FAIL;
		} else if(!cur_dev->fullerase()) {
			connected = FALSE;
			JRES.sense = JT_PROGFAIL;
			JRES.status = JT_FAIL;
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
	P2MDIN  |= 0x10;//P2.4 as input
	P2MDOUT |= 0x07; // p2.1, p2.2 p2.4 to be pushpul
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
		
		if(USB_status[0] & USB_SUSPEND) {
			// suspended - turn the LEDs off lest we draw too much current.
			ACTIVE(FALSE);
			GREEN(FALSE);
		}
		if(USB_status[0] & (RX_READY|USB_SUSPEND)) {
			active = TRUE;
			USB_control();
		}
		if(USB_status[USB_RX] & RX_READY) {
			active = TRUE;
			Endpoint_1();
		}

		if(connected){
			is_stoped = cur_dev->isstopped();
			if((is_stoped == 1) && (pcadjustd == 1)){
				cur_dev->stop();
				arm_restore_pc(0xC);
				pcadjustd = 2;
			}
		}

		
	}
}

