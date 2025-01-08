/*
 * 	USB interface routines for the Silicon Labs C8051F32x devices.
 * 	Implements full speed.
 *
 * 	Copyright(C) 2005 HI-TECH Software
 * 	www.htsoft.com
 *
 */

#ifdef	HI_TECH_C
#include	<8051.h>
#else
#include	<c8051f320.h>
#include	"f320.h"
#endif
#include	"usb.h"

static USB_descriptor_table USB_CONST *	descriptor;

// USB endpoint status
// These flags are set for each endpoint by interrupt routines.
// Mainline code need only test these flags to know what to do next.

unsigned char	USB_status[4];
static unsigned char USB_CONST *	current_config;		// pointer to current configuration
static unsigned char USB_CONST 		zeropacket[1] = { 0 };
// max packet size for each endpoint - set up when configuration selected
static unsigned short	usb_packet_size[4];

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

// USB0ADR bits

#define	BUSY		0x80		// data engine busy
#define	AUTORD		0x40		// automatically read next byte

// CMINT bits

#define	SUSINT		0x01		// suspend interrupt
#define	RSUINT		0x02		// resume interrupt
#define	RSTINT		0x04		// reset interrupt
#define	SOFINT		0x08		// start of frame interrupt


// write a byte to a USB register. Use for non-indexed locations

static void
USB_write(unsigned char addr, unsigned char databyte)
{
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = addr;
	USB0DAT = databyte;
}

// write a byte to an indexed USB register.
// Safe for use with interrupts on - USB ints are disabled around the access.

static void
USB_index_write(unsigned char index, unsigned char addr, unsigned char databyte)
{
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = INDEX;
	USB0DAT = index;
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = addr;
	USB0DAT = databyte;
}

static unsigned char
USB_read(unsigned char addr)
{
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = addr | BUSY;
	while(USB0ADR & BUSY)
		continue;
	return USB0DAT;
}

static unsigned char
USB_index_read(unsigned char index, unsigned char addr)
{
	USB0ADR = INDEX;
	USB0DAT = index;
	while(USB0ADR & BUSY)
		continue;
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = addr | BUSY;
	while(USB0ADR & BUSY)
		continue;
	return USB0DAT;
}


// send the given buffer to endpoints 1-3. Takes pointers to xdata.

void
USB_send_data(unsigned char index, unsigned char xdata * ptr, unsigned short cnt)
{
	unsigned short	tcnt;

	while(cnt) {
		tcnt = usb_packet_size[index];
		if(tcnt > cnt)
			tcnt = cnt;
		cnt -= tcnt;
		// wait till transmitter buffer empty
		while(USB_status[index] & TX_BUSY)
			USB_INTON();
		USB_INTOFF();
		USB0ADR = FIFO0+index;
		do {
			while(USB0ADR & BUSY)
				continue;
			USB0DAT = *ptr++;
		} while(--tcnt != 0);
		USB_index_write(index, EINCSRL, INPRDY);
		if(USB_read(EINCSRL) & INPRDY)
			USB_status[index] |= TX_BUSY;		// fifo full
	}
}

unsigned short
USB_read_packet(unsigned char index, unsigned char xdata * ptr, unsigned short cnt)
{
	unsigned short	pcnt;
	unsigned char	done;

	USB_write(INDEX, index);
	pcnt = USB_read(EOUTCNTL) + (USB_read(EOUTCNTH) << 8);
	done = 0;
	if(cnt >= pcnt) {
		cnt = pcnt;
		done = 1;
	}
	pcnt = cnt;
	USB0ADR = (FIFO0+index) | BUSY | AUTORD;
	while(cnt-- != 0) {
		while(USB0ADR & BUSY)
			continue;
		*ptr++ = USB0DAT;
	}
	if(done) {
		USB_write(EOUTCSRL, 0);		// clear OPRDY
		USB_status[index] &= ~RX_READY;
	}
	return pcnt;
}

// send the given buffer to endpoint 0. Takes pointers to USB_CONST

void
USB_send_data_0(unsigned char USB_CONST * ptr, unsigned int cnt)
{
	unsigned char	status;
	unsigned int	tcnt;

	do {	// must send a packet even if zero length
		tcnt = usb_packet_size[0];
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
			while(USB0ADR & BUSY)
				continue;
			USB0DAT = *ptr++;
			tcnt--;
		}
		USB_status[0] |= TX_BUSY;
		USB_index_write(0, E0CSR, status);
	} while(cnt);
}

// stall endpoint 0 - protocol error

static void
USB_stall_0(void)
{
	USB_index_write(0, E0CSR, SDSTL0);		// protocol error - send stall
}

static void
USB_ack_0(void)
{
	USB_index_write(0, E0CSR, SOPRDY);		// restores interrupts on the USB
}

// Setup an interface

static unsigned char USB_CONST *
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
				USB_status[addr] |= EP_IN;
				USB_index_write(addr, EINCSRL, FLUSHI);		// flush fifo
				if(mode & 2)	// bulk or interrupt - control is not possible
					mode = DIRSEL;
				else
					mode = ISO|DIRSEL;
				USB_index_write(addr, EINCSRH, mode);
			} else {
				USB_status[addr] |= EP_OUT;
				USB_index_write(addr, EOUTCSRL, FLUSHO);		// flush fifo
				if(mode & 2)
					mode = 0;
				else
					mode = ISO;
				USB_index_write(addr, EOUTCSRH, mode);
			}
			usb_packet_size[addr] = interface[wMaxPacketSize] +
				(interface[wMaxPacketSize+1] << 8);
			interface += ENDPOINT_SIZE;
		} while(--ep != 0);
	return interface;
}

// Setup SPLIT and double buffering bits

static void
USB_setup_buffer(unsigned char index)
{
	unsigned char	bufs;
	unsigned char	mode;

	mode = 0;
	// calculate how many packets will fit into the FIFO
	bufs = (32 << index) / usb_packet_size[index];
	if((USB_status[index] & (EP_IN|EP_OUT)) == (EP_IN|EP_OUT)) {
		bufs >>= 1;
		mode = SPLIT;
	}
	// bufs is now 1/2 the number of packets that will fit in the fifo
	if(bufs != 0) {
		mode |= DBIEN;
		USB_index_write(index, EOUTCSRH, USB_index_read(index, EOUTCSRH) | DBOEN);
	}
	USB_index_write(index, EINCSRH, USB_index_read(index, EINCSRH) | mode);
}

// select a configuration and set up endpoints accordingly

static void
USB_set_config(unsigned char conf)
{
	unsigned char USB_CONST *	interface;
	unsigned char				inint, outint, mask;

	if(conf == 0) {			// unconfigure
		current_config = 0;
		for(conf = 1 ; conf != 3 ; conf++) {
			USB_index_write(conf, EINCSRL, 0);
			USB_index_write(conf, EINCSRH, 0);
			USB_index_write(conf, EOUTCSRL, 0);
			USB_index_write(conf, EOUTCSRH, 0);
		}
		USB_write(IN1IE, 1);		// disable all except endpoint 0
		USB_write(OUT1IE, 0);
		return;
	}
	current_config = descriptor->configurations[conf-1];

	// reset the endpoint status bits

	conf = 3;
	do
		USB_status[conf] = 0;
	while(--conf != 0);

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

/* process endpoint 0 (setup) packets */

void
USB_control(void)
{
	unsigned char	packet[REQ_SIZE];
	unsigned char	idx;
	unsigned int	cnt;

	// clear the status bit ready for next interrupt

	USB_INTOFF();
	USB_status[0] &= ~RX_READY;
	while(USB0ADR & BUSY)
		continue;
	USB0ADR = FIFO0 | BUSY | AUTORD;
	idx = 0;
	do {
		while(USB0ADR & BUSY)
			continue;
		packet[idx++] = USB0DAT;
	} while(idx != sizeof packet);
	switch(packet[REQ_REQ]) {

	case SET_ADDRESS:
		USB_write(FADDR, packet[REQ_VALUE]);
		USB_ack_0();
		break;

	case GET_DESCRIPTOR:
		switch(packet[REQ_VALUE+1]) {	// check requested type

		case DESC_STRING:
			if(packet[REQ_LENGTH+1])		// strings can't exceed 255 bytes
				packet[REQ_LENGTH] = 255;
			if(packet[REQ_VALUE] < descriptor->string_cnt) {
				if(packet[REQ_LENGTH] > descriptor->strings[packet[REQ_VALUE]][0])
					packet[REQ_LENGTH] = descriptor->strings[packet[REQ_VALUE]][0];
				USB_ack_0();
				USB_send_data_0(descriptor->strings[packet[REQ_VALUE]], packet[REQ_LENGTH]);
			} else
				USB_stall_0();
			break;

		case DESC_DEVICE:
			USB_ack_0();
			USB_send_data_0(descriptor->device, DEVICE_SIZE);
			break;

		case DESC_CONFIG:
			if(packet[REQ_VALUE] < descriptor->conf_cnt) {
				USB_ack_0();
				cnt = packet[REQ_LENGTH] + (packet[REQ_LENGTH+1] << 8);
				if(cnt > descriptor->conf_sizes[packet[REQ_VALUE]])
					cnt = descriptor->conf_sizes[packet[REQ_VALUE]];
				USB_send_data_0(descriptor->configurations[packet[REQ_VALUE]], cnt);
			} else
				USB_stall_0();
			break;
		default:
			USB_stall_0();
			break;
		}
		break;

	case GET_CONFIG:
		USB_ack_0();
		if(current_config)
			USB_send_data_0(current_config+5, 1);
		else
			USB_send_data_0(zeropacket, 1);
		break;

	case SET_CONFIG:
		// assume configurations are numbered monotonically from 1
		// usbconfig generated descriptors guarantee this.
		if(packet[REQ_VALUE] <= descriptor->conf_cnt) {
			USB_ack_0();
			USB_set_config(packet[REQ_VALUE]);
		} else
			USB_stall_0();
		break;

	case GET_STATUS:
	case CLEAR_FEATURE:
	case SET_FEATURE:
	case GET_INTERFACE:
	case SET_INTERFACE:

	default:
		USB_stall_0();
		break;
	}
	USB_INTON();
}

#ifdef	HI_TECH_C
static void interrupt
USB_isr(void) @ USB0
#else
static void
USB_isr(void) interrupt 8
#endif
{
	unsigned char	channels, csr, index;

	csr = USB_read(CMINT);
	if(csr & SUSINT)
		USB_status[0] |= USB_SUSPEND;
	channels = USB_read(IN1INT);
	if(channels & 1) {	// Endpoint 0
		csr = USB_index_read(0, E0CSR);
		if(csr & STSTL0)			// stall has been sent - clear the bit
			USB0DAT = 0;		// clear E0CSR - addr already set
		if(csr & SUEND)			// setup end event - just clear it
			USB0DAT = SSUEND;
		if(csr & OPRDY)
			USB_status[0] |= RX_READY;
		if(csr & INPRDY0)
			USB_status[0] |= TX_BUSY;
		else
			USB_status[0] &= ~TX_BUSY;
	}
	index = 3;
	do {
		if(channels & 8) {
			csr = USB_index_read(index, EINCSRL);
			if(csr & STSTLI)
				USB0DAT = 0;
			if(csr & INPRDY)
				USB_status[index] |= TX_BUSY;
			else
				USB_status[index] &= ~TX_BUSY;
		}
		channels <<= 1;
	} while(--index != 0);
	channels = USB_read(OUT1INT);
	index = 3;
	do {
		if(channels & 8) {
			csr = USB_index_read(index, EOUTCSRL);
			if(csr & STSTLO)
				USB0DAT = 0;
			if(csr & OPRDY)
				USB_status[index] |= RX_READY;
			else
				USB_status[index] &= ~RX_READY;
		}
		channels <<= 1;
	} while(--index != 0);
}


void
USB_init(USB_descriptor_table USB_CONST * table)
{
	descriptor = table;

	// init the USB

	usb_packet_size[0] = 64;
	USB_write(POWER, 0x8);		// Reset the USB hardware
	USB_write(IN1IE, 0x01);		//	Enable Endpoint 0 interrupts
	USB_write(CMIE, 0x07);		//	Enable common interrupts

	USB0XCN = 0xE0;				// 0B1110 0000;		// enable USB transceiver, pullup on
	USB_write(CLKREC, 0x89);	// enable clock recovery
	USB_write(POWER, 0x01);		// Enable the USB hardware
	USB_INTON();				// enable USB interrupts generally
}

void
USB_detach(void)
{
	USB0XCN = 0;
}
