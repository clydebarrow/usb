/*
 * 	USB interface routines for the Silicon Labs C8051F32x devices.
 * 	Includes support for the Bulk-Only Mass storage protocol
 * 	Implements full speed.
 *
 * 	this is the bug-fix version for the Zipcard.
 *
 * 	Copyright(C) 2005-2006 HI-TECH Software
 * 	www.htsoft.com
 *
 */

#include	<8051.h>
#include	<stdio.h>
#include	"bootload.h"


static unsigned char USB_CONST 		zeropacket[] = { 0, 0 };
static unsigned char USB_CONST 		onepacket[] = { 1, 0 };

static unsigned char USB_CONST *	SERIAL_NUMBER;

static USB_CONST unsigned char sno[0x100] @ 0x100;
static USB_CONST unsigned char dummy[26] =
{
	26, 3,	'1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0,
			'7', 0, '8', 0, '9', 0, '0', 0, '1', 0, '2', 0
};

// max packet size for each endpoint - set up when configuration selected
// Enforce 64 bytes only

#define	PACKET_SIZE	64

// USB endpoint status
// These flags are set for each endpoint by interrupt routines.
// Mainline code need only test these flags to know what to do next.

unsigned char xdata	USB_status[NUM_ENDPOINTS] @ 0x430;
unsigned char xdata	fifo_cnt @ 0x434;
static USB_descriptor_table USB_CONST * xdata	descriptor @ 0x43C;
static unsigned char USB_CONST *	xdata current_config @ 0x43E;		// pointer to configuration

// USB bit definitions

// E0CSR bits

#define	SSUEND		0x80		// Service setup end
#define	SOPRDY		0x40		// Service OPRDY
#define	SDSTL0		0x20		// Send stall
#define	SUEND		0x10		// Setup end (underrun)
#define	DATAEND		0x08		// Last data packet has been sent
#define	STSTL0		0x04		// Stall has been sent
#define	INPRDY0		0x02		// data has been loaded into FIFO to send
#define	OPRDY0		0x01		// data available in FIFO

// EINCSRL/H bits

#define	INPRDY		0x01
#define	FIFONE		0x02
#define	UNDRUN		0x04
#define	FLUSHI		0x08
#define	SDSTLI		0x10
#define	STSTLI		0x20
#define	CLRDTI		0x40

#define	SPLIT		0x04
#define	FCDT		0x08		// force data toggle
#define	DIRSEL		0x20		// 0 == out
#define	ISO			0x40
#define	DBIEN		0x80		// double buffer enable

// EOUTCSRL bits

#define	OPRDY		0x01
#define	FIFOFUL		0x02
#define	OVRUN		0x04
#define	DATERR		0x08
#define	FLUSHO		0x10
#define	SDSTLO		0x20
#define	STSTLO		0x40
#define	CLRDTO		0x80
#define	DBOEN		0x80

// CMINT bits

#define	SUSINT		0x01		// suspend interrupt
#define	RSUINT		0x02		// resume interrupt
#define	RSTINT		0x04		// reset interrupt
#define	SOFINT		0x08		// start of frame interrupt

#ifdef	USB_LOGGING

static xdata unsigned char	logbuf[128] @ 0x0;
unsigned xdata short logoffs @ 0x80;
unsigned xdata char logidx @ 0x82;

extern reentrant char progmem(unsigned int addr, unsigned char len);

static void
proglog(unsigned char b)
{
	logbuf[logidx++] = b;
	if(logidx == sizeof logbuf) {
		if(logoffs < 0x3E00) {
			progmem(logoffs, sizeof logbuf);
			logoffs += sizeof logbuf;
		}
		logidx = 0;
	}
}

reentrant void
addlog(unsigned char resp, unsigned char type, unsigned char subtype, unsigned short len)
{
	proglog(resp);
	proglog(type);
	proglog(subtype);
	proglog(len);
	proglog(len >> 8);
	proglog(0xF5);
	proglog(0xF6);
	proglog(0xF7);
}
#else
#define	addlog(a, b, c, d)
#define	proglog(x)
#endif

// write a byte to a USB register. Use for non-indexed locations

static reentrant void
USB_write(unsigned char addr, unsigned char databyte)
{
	while(USB0ADR & USB_BUSY)
		continue;
	USB0ADR = addr;
	USB0DAT = databyte;
}

// write a byte to an indexed USB register.
// Safe for use with interrupts on - USB ints are disabled around the access.

reentrant void
USB_index_write(unsigned char index, unsigned char addr, unsigned char databyte)
{
	while(USB0ADR & USB_BUSY)
		continue;
	USB0ADR = INDEX;
	USB0DAT = index;
	while(USB0ADR & USB_BUSY)
		continue;
	USB0ADR = addr;
	USB0DAT = databyte;
}

reentrant static unsigned char
USB_read(unsigned char addr)
{
	while(USB0ADR & USB_BUSY)
		continue;
	USB0ADR = addr | USB_BUSY;
	while(USB0ADR & USB_BUSY)
		continue;
	return USB0DAT;
}

reentrant static unsigned char
USB_index_read(unsigned char index, unsigned char addr)
{
	USB0ADR = INDEX;
	USB0DAT = index;
	return USB_read(addr);
}


// flush a packet to the input endpoint
// if the force flag is set, send a zero length packet if
// the fifo is empty

reentrant void
USB_flushin(unsigned char index, unsigned char force)
{
	if(!force && fifo_cnt == 0)
		return;
	USB_INTOFF();
	force = INPRDY;
	if(USB_status[index] & EP_HALT )
		force = INPRDY|SDSTLI;
	USB_index_write(index, EINCSRL, force);
	if(USB_read(EINCSRL) & INPRDY)
		USB_status[index] |= TX_BUSY;		// fifo full or halted
	USB_INTON();
	fifo_cnt = 0;
}

// write one byte to in input endpoint 
reentrant void
USB_send_byte(unsigned char index, unsigned char byte)
{
	// wait till transmitter buffer empty
	while(USB_status[index] & TX_BUSY)
		continue;
	USB_INTOFF();
	USB0ADR = FIFO0+index;
	while(USB0ADR & USB_BUSY)
		continue;
	USB0DAT = byte;
	if(++fifo_cnt == PACKET_SIZE)
		USB_flushin(index, 0);
	USB_INTON();
}

reentrant void
USB_flushout(unsigned char index)
{
	USB_INTOFF();
	USB_index_write(index, EOUTCSRL, 0);		// clear OPRDY
	USB_status[index] &= ~RX_READY;
	USB_INTON();
}

reentrant unsigned short
USB_read_packet(unsigned char index, unsigned char xdata * ptr, unsigned short cnt)
{
	unsigned short	pcnt;
	unsigned char	done;

	USB_INTOFF();
	USB_write(INDEX, index);
	pcnt = USB_read(EOUTCNTL) + (USB_read(EOUTCNTH) << 8);
	done = 0;
	if(cnt >= pcnt) {
		cnt = pcnt;
		done = 1;
	}
	pcnt = cnt;
	USB0ADR = (FIFO0+index) | USB_BUSY | USB_AUTORD;
	while(cnt-- != 0) {
		while(USB0ADR & USB_BUSY)
			continue;
		*ptr = USB0DAT;
		proglog(*ptr);
		ptr++;
	}
	if(done) {
		USB_write(EOUTCSRL, 0);		// clear OPRDY
		USB_status[index] &= ~RX_READY;
	}
	USB_INTON();
	return pcnt;
}

// send the given buffer to endpoint 0. Takes pointers to USB_CONST

reentrant void
USB_send_data_0(unsigned char USB_CONST * ptr, unsigned int cnt)
{
	unsigned char	status;
	unsigned int	tcnt;

	do {	// must send a packet even if zero length
		tcnt = PACKET_SIZE;
		if(tcnt > cnt)
			tcnt = cnt;
		cnt -= tcnt;
		status = INPRDY0;
		if(cnt == 0)
			status = INPRDY0|DATAEND;
		// wait till transmitter buffer empty
		while(USB_status[0] & TX_BUSY)
			USB_INTON();
		USB_INTOFF();
		USB0ADR = FIFO0;
		while(tcnt) {
			while(USB0ADR & USB_BUSY)
				continue;
			USB0DAT = *ptr++;
			tcnt--;
		}
		USB_status[0] |= TX_BUSY;
		USB_index_write(0, E0CSR, status);
	} while(cnt);
}


// stall endpoint 0 - protocol error

reentrant static void
USB_stall_0(void)
{
	addlog(0xFE, 0, 0, 0);
	USB_index_write(0, E0CSR, SDSTL0);		// protocol error - send stall
}

reentrant void
USB_halt(unsigned char index)
{
	USB_INTOFF();
	if(USB_status[index] & EP_IN)
		USB_index_write(index, EINCSRL, SDSTLI);		// Stall input request
	if(USB_status[index] & EP_OUT)
		USB_index_write(index, EOUTCSRL, SDSTLO);		// Stall output request
	USB_status[index] |= EP_HALT;
	USB_INTON();
}

reentrant static void
USB_ack_0(void)
{
	USB_index_write(0, E0CSR, SOPRDY);		// restores interrupts on the USB
}

// Setup an interface

static unsigned char USB_CONST * reentrant
USB_setup_interface(unsigned char USB_CONST * interface)
{
	unsigned char	ep, addr, mode;

	ep = interface[bNumEndpoints];
	interface += INTERFACE_SIZE;
	if(ep)
		do {
			mode = interface[bmAttributes] & 3;
			addr = interface[bEndpointAddress];
			if(addr & 0x80) {	// in endpoint
				addr &= 0x3;
				USB_status[addr] = EP_IN;
				USB_index_write(addr, EINCSRL, FLUSHI);		// flush fifo
				if(mode & 2)	// bulk or interrupt - control is not possible
					mode = DIRSEL;
				else
					mode = ISO|DIRSEL;
				USB_write(EINCSRH, mode);
			} else {
				USB_status[addr] = EP_OUT;
				USB_index_write(addr, EOUTCSRL, FLUSHO);		// flush fifo
				if(mode & 2)
					mode = 0;
				else
					mode = ISO;
				USB_write(EOUTCSRH, mode);
			}
			interface += ENDPOINT_SIZE;
		} while(--ep != 0);
	return interface;
}

// Setup SPLIT and double buffering bits

static void reentrant
USB_setup_buffer(unsigned char index)
{
	unsigned char	bufs;
	unsigned char	mode;

	mode = USB_index_read(index, EINCSRH);
	// calculate how many packets will fit into the FIFO
	bufs = 1 << index;				// number of 64 byte packets
	if((USB_status[index] & (EP_IN|EP_OUT)) == (EP_IN|EP_OUT)) {
		bufs >>= 1;
		mode |= SPLIT;
	}
	// bufs is now 1/2 the number of packets that will fit in the fifo
	if(bufs != 0) {
		if(USB_status[index] & EP_IN)
			mode |= DBIEN;
		if(USB_status[index] & EP_OUT)
			USB_index_write(index, EOUTCSRH, USB_index_read(index, EOUTCSRH) | DBOEN);
	}
	USB_index_write(index, EINCSRH,  mode);
}

// select a configuration and set up endpoints accordingly

static void reentrant
USB_set_config(unsigned char conf)
{
	unsigned char USB_CONST *	interface;
	unsigned char				inint, outint, mask;

	// reset everything in the endpoint registers, including the data toggle
	inint = 3;
	do {
		USB_index_write(inint, EINCSRL, CLRDTI|SDSTLI);
		USB_index_write(inint, EINCSRH, 0);
		USB_index_write(inint, EOUTCSRL, CLRDTO|SDSTLO);
		USB_index_write(inint, EOUTCSRH, 0);
		USB_status[inint] = EP_HALT;
	} while(--inint != 0);
	if(conf == 0) {			// unconfigure
		current_config = 0;
		USB_write(IN1IE, 1);		// disable all except endpoint 0
		USB_write(OUT1IE, 0);
		return;
	}
	current_config = descriptor->configurations[conf-1];

	// now scan through the interfaces of this configuration and
	// set up the endpoints accordingly.
	// This code does NOT handle alternate interfaces, though
	// multiple (simultaneously active) interfaces are permitted.

	if((conf = current_config[bNumInterfaces]) == 0)
		return;
	interface = current_config+CONFIGURATION_SIZE;
	do
		interface = USB_setup_interface(interface);
	while(--conf != 0);
	conf = 3;
	inint = 1;		// EP0 interrupts always on
	outint = 0;
	mask = 1 << 3;
	do {
		if(USB_status[conf] & EP_IN)
			inint |= mask;
		if(USB_status[conf] & EP_OUT)
			outint |= mask;
		mask >>= 1;
		USB_setup_buffer(conf);
	} while(--conf != 0);
	USB_write(IN1IE, inint);
	USB_write(OUT1IE, outint);
}

static reentrant void
CLK_start(void)
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
}

/* process endpoint 0 (setup) packets */

reentrant void
USB_control(void)
{
	unsigned char			packet[REQ_SIZE];
	unsigned char			idx;
	unsigned int			cnt;
USB_CONST unsigned char *	sp;	// string pointer

	// clear the status bit ready for next interrupt

	USB_INTOFF();
	if(USB_status[0] & USB_SUSPEND) {
		USB0XCN &= ~0x40;		// disable transceiver
		CLKSEL = 0x10;			// don't use multiplier
		CLKMUL = 0;				// disable it
		OSCICN |= 0x20;			// stop the clock.
		CLK_start();
		USB0XCN |= 0x40;		// re-enable transceiver
		USB_status[0] &= ~USB_SUSPEND;
	}
	if(USB_status[0] & RX_READY) {
		USB_status[0] &= ~RX_READY;
		while(USB0ADR & USB_BUSY)
			continue;
		USB0ADR = FIFO0 | USB_BUSY | USB_AUTORD;
		idx = 0;
		do {
			while(USB0ADR & USB_BUSY)
				continue;
			packet[idx] = USB0DAT;
			proglog(packet[idx]);
		} while(++idx != sizeof packet);
		idx = packet[REQ_INDEX] & 15;
		switch(packet[REQ_REQ]) {

		case SET_ADDRESS:
			USB_write(FADDR, packet[REQ_VALUE]);
			addlog(0xFF, SET_ADDRESS, 0, 0);
			USB_ack_0();
			break;

		case GET_DESCRIPTOR:
			sp = 0;
			switch(packet[REQ_VALUE+1]) {	// check requested type

			case DESC_STRING:
				if(packet[REQ_VALUE] < descriptor->string_cnt) {
					sp = descriptor->strings[packet[REQ_VALUE]];
					if(sp == 0)		// serial number
						sp = SERIAL_NUMBER;
					cnt = *sp;
				}
				break;

			case DESC_DEVICE:
				sp = descriptor->device;
				cnt = DEVICE_SIZE;
				break;

			case DESC_CONFIG:
				if(packet[REQ_VALUE] < descriptor->conf_cnt) {
					sp = descriptor->configurations[packet[REQ_VALUE]];
					cnt = descriptor->conf_sizes[packet[REQ_VALUE]];
				}
				break;
			}
			addlog(0xFF, GET_DESCRIPTOR, packet[REQ_REQ], cnt);
			if(sp) {
				if(cnt > packet[REQ_LENGTH] + (packet[REQ_LENGTH+1]<< 8))
					cnt = packet[REQ_LENGTH] + (packet[REQ_LENGTH+1]<< 8);
				USB_ack_0();
				USB_send_data_0(sp, cnt);
			} else
				USB_stall_0();
			break;

		case BOMS_RESET:
			addlog(0xFF, BOMS_RESET, idx, 0);
			USB_status[0] |= PROT_RESET;
			USB_ack_0();
			break;

		case BOMS_GETMAXLUN:
			addlog(0xFF, BOMS_GETMAXLUN, idx, 0);
			USB_ack_0();
			USB_send_data_0(zeropacket, 1);
			break;

		case GET_CONFIG:
			USB_ack_0();
			if(current_config) {
				addlog(0xFF, GET_CONFIG, current_config[5], 1);
				USB_send_data_0(current_config+5, 1);
			} else {
				addlog(0xFF, GET_CONFIG, 0, 1);
				USB_send_data_0(zeropacket, 1);
			}
			break;

		case SET_CONFIG:
			// assume configurations are numbered monotonically from 1
			// usbconfig generated descriptors guarantee this.
			if(packet[REQ_VALUE] <= descriptor->conf_cnt) {
				USB_set_config(packet[REQ_VALUE]);
				addlog(0xFF, SET_CONFIG, packet[REQ_VALUE], 0);
				USB_ack_0();
			} else
				USB_stall_0();
			break;

		case CLEAR_FEATURE:
			if(packet[REQ_VALUE] == 0 && idx < NUM_ENDPOINTS) {	// clear endpoint halt
				addlog(0xFF, CLEAR_FEATURE, packet[REQ_INDEX], 0);
				USB_status[idx] &= ~(EP_HALT|RX_READY|TX_BUSY);
				if(USB_status[idx] & EP_IN)
					USB_index_write(idx, EINCSRL, CLRDTI);					// clear stall
				else
					USB_index_write(idx, EOUTCSRL, CLRDTO);					// clear stall
				USB_ack_0();
			} else
				USB_stall_0();
			break;

		case SET_FEATURE:
			if(packet[REQ_VALUE] == 0 && idx < NUM_ENDPOINTS) {	// halt endpoint
				addlog(0xFF, SET_FEATURE, packet[REQ_INDEX], 0);
				USB_status[idx] |= EP_HALT;
				if(packet[REQ_INDEX] & 0x80)
					USB_index_write(idx, EINCSRL, SDSTLI);					// stall endpoint
				else
					USB_index_write(idx, EOUTCSRL, SDSTLO);
				USB_ack_0();
			} else
				USB_stall_0();
			break;

		case GET_INTERFACE:			// always reply with zer0
			addlog(0xFF, GET_INTERFACE, 0, 1);
			USB_ack_0();
			USB_send_data_0(zeropacket, 1);
			break;

		case SET_INTERFACE:			// alternate interface settings are not supported
			addlog(0xFF, SET_INTERFACE, 0, 0);
			if(idx == 0)
				USB_ack_0();
			else
				USB_stall_0();
			break;

		case GET_STATUS:
			addlog(0xFF, GET_STATUS, packet[REQ_INDEX], 2);
			if(packet[REQ_TYPE] & 2 && idx < NUM_ENDPOINTS) {	// endpoint status request?
				USB_ack_0();
				if(USB_status[idx] & EP_HALT)					// bit zer0 is the halt status
					USB_send_data_0(onepacket, 2);
				else
					USB_send_data_0(zeropacket, 2);
			} else if(packet[REQ_TYPE] == 0x80) {				// device status request
				USB_ack_0();
				USB_send_data_0(zeropacket, 2);
			} else
				USB_stall_0();
			break;
					
		default:
			USB_stall_0();
			break;
		}
	}
	USB_INTON();
#ifdef	DEBUG
	printf("USB Packet: ");
	for(idx = 0 ; idx != sizeof packet ; idx++)
		printf("%2.2X ", packet[idx]);
	putch('\n');
#endif
}

static bit
isdigit(unsigned char c)
{
	return c >= '0' && c <= '9';
}

reentrant void
USB_init(USB_descriptor_table USB_CONST * table)
{
	USB_INTOFF();				// enable USB interrupts generally
	CLK_start();
	descriptor = table;
	current_config = 0;
#ifdef	USB_LOGGING
	logoffs = 0x3000;
	logidx = 0;
#endif

	// init the USB

	USB_status[0] = 0;
	USB_status[1] = 0;
	USB_status[2] = 0;
	USB_status[3] = 0;
	USB_write(POWER, 0x8);		// Reset the USB hardware
	USB_write(IN1IE, 0x01);		//	Enable Endpoint 0 interrupts
	USB_read(CMINT);			// clear interrupt bits
	USB_write(CMIE, 0x07);		//	Enable common interrupts

	USB0XCN = 0xE0;				// 0B1110 0000;		// enable USB transceiver, pullup on
	USB_write(CLKREC, 0x89);	// enable clock recovery
	USB_write(POWER, 0x01);		// Enable the USB hardware
	USB_INTON();				// enable USB interrupts generally
	fifo_cnt = 0;				// doesn't get zeroed automatically
	SERIAL_NUMBER = sno;
	do {
		if(*SERIAL_NUMBER >= 20 && *SERIAL_NUMBER <= 26 &&
				SERIAL_NUMBER[1] == 3 &&
				isdigit(SERIAL_NUMBER[2]))
			break;
	} while(++SERIAL_NUMBER != sno+sizeof sno);
	if(SERIAL_NUMBER == sno+sizeof sno)
		SERIAL_NUMBER = dummy;
}

reentrant void
USB_detach(void)
{
	USB_INTOFF();
	USB0XCN = 0;
	USB_write(POWER, 0x8);		// Reset the USB hardware
}

