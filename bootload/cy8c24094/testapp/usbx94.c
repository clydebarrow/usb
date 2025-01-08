/*
 * 	USB interface routines for the Cypress CY8C24x94 devices.
 * 	Implements full speed.
 *
 * 	Copyright(C) 2005-2007 HI-TECH Software
 * 	www.htsoft.com
 *
 */

#define	xdata		// no such thing for PSoC

#include	<psoc.h>
#include	<cy8c24x94.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"bootload.h"
#include	"lcd.h"

#define	X_ON()			(PRT3DR |= 0x10)
#define	X_OFF()			(PRT3DR &= ~0x10)
#define	LED_OFF()		(PRT3DR |= 0x40)
#define	LED_ON()		(PRT3DR &= ~0x40)
#define	ACT_OFF()		(PRT3DR |= 0x20)
#define	ACT_ON()		(PRT3DR &= ~0x20)
#define	BOOT_SEL		(!(PRT3DR & 0x80))		// bootload if this bit pulled low

#define EP_IN_INDEX	2
#define EP_OUT_INDEX	1




static unsigned char const 		zeropacket[] = { 0, 0 };
static unsigned char const 		onepacket[] = { 1, 0 };

// max packet size for each endpoint - set up when configuration selected
// Enforce 64 bytes only

#define	PACKET_SIZE	64

// endpoint 0 packet size is smaller...

extern unsigned char	blkbuf[];
extern unsigned char g_toggle;

#define	PACKET_SIZE_0	8

#define	USB_INTON_0() (INT_MSK2 |= INT_MSK2_EP0)	// enable endpoint 0 interrupts
#define	USB_INTOFF_0() (INT_MSK2 &= ~INT_MSK2_EP0)	// disable endpoint 0 interrupts
#define	USB_INTON_1() (INT_MSK2 |= INT_MSK2_EP1)	// enable endpoint 0 interrupts
#define	USB_INTOFF_1() (INT_MSK2 &= ~INT_MSK2_EP1)	// disable endpoint 0 interrupts
#define	USB_INTON_2() (INT_MSK2 |= INT_MSK2_EP2)	// enable endpoint 0 interrupts
#define	USB_INTOFF_2() (INT_MSK2 &= ~INT_MSK2_EP2)	// disable endpoint 0 interrupts

// USB endpoint status
// These flags are set for each endpoint by interrupt routines.
// Mainline code need only test these flags to know what to do next.

// the x94 devices have 1K of RAM. We will use RAM at the very end for these things.
// This puts it out of the way of everything except the stack, which grows from
// somewhere in the last page of RAM to the end. We use 9 bytes in all.

/*unsigned char xdata	USB_status[NUM_ENDPOINTS] @ 0x3FA;
static USB_descriptor_table const * xdata	descriptor @ 0x3F8;
static unsigned char const *	xdata current_config @ 0x3F6;		// pointer to configuration
unsigned char xdata	fifo_cnt @ 0x3F5;
unsigned char xdata ack_flag @ 0x3F4;*/


/*volatile unsigned char xdata	USB_status[NUM_ENDPOINTS] @ 0x3F7;
static USB_descriptor_table const * xdata	descriptor @ 0x3ED;
static unsigned char const *	xdata current_config @ 0x3EB;		// pointer to configuration
unsigned char xdata	fifo_cnt @ 0x3EA;
unsigned char xdata ack_flag @ 0x3E9;
*/


// customizable serial number - this should be replaced with a real serial number when
// programming the device.

static const unsigned char SERIAL_NUMBER[26] =
{
	26, 3,	'0', 0, '6', 0, '6', 0, '6', 0, '6', 0, '6', 0,
			'N', 0, 'U', 0, 'M', 0, 'B', 0, 'E', 0, 'R', 0
};


//#define	USBMODE_CTRL_IDLE		0x1		// accept control transfers, NAK others
//#define	USBMODE_CTRL_ACK		0xB		// control transfer successful
#define	USBMODE_CTRL_DONE		0x2		// control transfer complete
//#define	USBMODE_CTRL_STALL		0x3		// stall in/out
//#define	USBMODE_CTRL_IN			0xF		// accept control transfers, ack IN, accept 0 for out
#define	USBMODE_CTRL_IN0		0x6		// accept control transfers, reply to in with 0 bytes
//#define	USBMODE_ACKIN			0xD		// data is ready to send to IN request
//#define	USBMODE_NAKIN			0xC		// in fifo empty
#define	USBMODE_ACKOUT			0x9		// Ready to receive data
#define	USBMODE_NAKOUT			0x8		// out fifo has data, nak further outs
#define USBMODE_ISOOUT			0x5
#define USBMODE_ISOIN			0x7

volatile __ioport unsigned char	EP0_CNT @	0x057;	// EP0 Count Register
#define EP0_CNT_DATA_TOGGLE (0x80)

static unsigned char g_toggle @ 0x3EA;

// flush a packet to the input endpoint
// if the force flag is set, send a zero length packet if
// the fifo is empty


void flush_buffer(unsigned char index){

	g_toggle ^= EP2_CNT1_DATA_TOGGLE;
	EP2_CNT1 |= g_toggle;
	PMA2_WA = 128;
	PMA2_RA = 128;
	USB_status[index] |= TX_BUSY;
	EP2_CR0 = USBMODE_ACKIN;
	USB_INTON_2();

}

// write one byte to in input endpoint
reentrant void
USB_send_bytes(unsigned char index, unsigned char *byte, unsigned char len,
			unsigned char offset, unsigned char flush)
{


	unsigned char count = offset;
	static unsigned char buffer_len = 0;
	while(USB_status[index] & TX_BUSY){

		USB_INTON_2();
	}

	USB_INTOFF_2();
	PMA2_WA = 128 + offset;



	while(len > 0){

		if((len+buffer_len) > 64 ){
			EP2_CNT	 = 64;
		}else{
			EP2_CNT = len + buffer_len;
		}
		while(1){
			PMA2_DR = *byte;
			byte++;
			count++;
			buffer_len++;
			if(buffer_len == 64){
				buffer_len = 0;
				EP2_CNT = 64;
				flush_buffer(index);
				count = 0;
			}
			len--;
			if(len == 0){
				break;
			}

		}
	}
	if(flush){
		flush_buffer(index);
		buffer_len = 0;

	}


}
#if 0



// write one byte to in input endpoint
reentrant void
USB_send_bytes(unsigned char index, unsigned char *byte, unsigned char len,
			unsigned char offset, unsigned char flush)
{


	unsigned char count = 0;
	while(USB_status[index] & TX_BUSY)
		USB_INTON_2();

	USB_INTOFF_2();
	PMA2_WA = 128+offset;
	EP2_CNT = len;

	/*while(len != 0){

				PMA2_DR = *byte;
				//printf("hi", 1);


				byte++;
				len--;

	}*/


	while(len > 0){


		if(len > 64 ){
			EP2_CNT	 = 64;
		}else{
			EP2_CNT = len;
		}
		while(1){
			PMA2_DR = *byte;
			byte++;
			count++;
			if(count == 64){
				flush_buffer(index);
				/*g_toggle ^= EP2_CNT1_DATA_TOGGLE;
				EP2_CNT1 |= g_toggle;
				PMA2_WA = 128;
				PMA2_RA = 128;
				USB_status[index] |= TX_BUSY;
				EP2_CR0 = USBMODE_ACKIN;
				USB_INTON_2();*/
				count = 0;
			}
			len--;
			if(len == 0){
				break;
			}

		}
	}
	if(flush){
		flush_buffer(index);

	}
	//putch(':');

}




reentrant void
USB_flushout(unsigned char index)
{
	/*
	USB_INTOFF();
	USB_index_write(index, EOUTCSRL, 0);		// clear OPRDY
	USB_status[index] &= ~RX_READY;
	USB_INTON();
	*/
}

reentrant unsigned short
USB_read_packet(unsigned char index, unsigned char xdata * ptr, unsigned short cnt)
{

	unsigned short	pcnt;
	unsigned char	done;
	unsigned char * tmpptr = ptr;

	// wait till transmitter buffer empty
	/*while(USB_status[index] & RX_READY)
		USB_INTON_1();*/


	USB_INTOFF_1();
	PMA1_RA = 64;
	pcnt = EP1_CNT;



	if(cnt >= pcnt) {
		cnt = pcnt;
		done = 1;
	}
	cnt = pcnt;
	while(cnt-- != 0) {
		/*while(USB0ADR & USB_BUSY)
			continue;*/
		*ptr = PMA1_DR;

		ptr++;
	}

	USB_status[EP_OUT_INDEX] &= ~RX_READY;


	EP1_CNT1 = 0; //clear all the bits in EP1 Count Reg
	EP1_CNT	 = 64;
	EP1_CR0  |= USBMODE_ACKOUT; // setting EP1 as IN end point;
	PMA1_WA = 64;

	USB_INTON_1();
	ptr = tmpptr ;

	X_OFF();

	return pcnt;


}

reentrant void
USB_halt(unsigned char index)
{
	/*
	USB_INTOFF();
	if(USB_status[index] & EP_IN)
		USB_index_write(index, EINCSRL, SDSTLI);		// Stall input request
	if(USB_status[index] & EP_OUT)
		USB_index_write(index, EOUTCSRL, SDSTLO);		// Stall output request
	USB_status[index] |= EP_HALT;
	USB_INTON();
	*/
}
// send the given buffer to endpoint 0. Takes pointers to const
// EP0 interrupts are disabled on entry and exit
// On completion of transmission, wait for an OUT packet or another setup packet
// to signal acceptance by the host.
// IF short is set, then the last packet must be a short one (since the
// data being sent is less than that expected by the host)


reentrant void
USB_send_data_0(unsigned char shrt, unsigned char const * ptr, unsigned int cnt)
{
	unsigned char	toggle = 0;
	unsigned char	tcnt, idx;

	do {	// must send a packet even if zero length

		tcnt = PACKET_SIZE_0;
		if(tcnt > cnt)
			tcnt = cnt;
		if(tcnt == 0 && !shrt)		// don't send zero-length packet unless reqd.
			break;
		cnt -= tcnt;
		idx = 0;
		// wait till transmitter buffer empty
		while(USB_status[0] & TX_BUSY)
			USB_INTON_0();
		USB_INTOFF_0();
		if(USB_status[0] & (RX_READY|USB_RESET))	// fall out if bus reset or setup received
			break;
		while(idx != tcnt)
			EP0_DR[idx++] = *ptr++;
		USB_status[0] = TX_BUSY;
		toggle ^= EP0_CNT_DATA_TOGGLE;
		EP0_CNT = tcnt|toggle;
		EP0_CR = USBMODE_CTRL_IN;

	} while(tcnt == PACKET_SIZE_0);
	USB_status[0] &= ~TX_BUSY;		// clear flag since we're done here.
}



// this function must be called with EP0 interrupts disabled

reentrant static void
USB_ack_0(void)
{
	// we have completed handling a setup packet which requires a 0-length
	// IN packet as status. Set the controller to do just that.
	USB_status[0] |= TX_BUSY;
	EP0_CR = USBMODE_CTRL_IN0;
}

// stall endpoint 0 - protocol error. The stall will remain in effect
// until another status packet is received and acked.

reentrant static void
USB_stall_0(void)
{
	EP0_CR = USBMODE_CTRL_STALL;
}

// Setup an interface

static unsigned char const * reentrant
USB_setup_interface(unsigned char const * interface)
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
				USB_status[EP_IN_INDEX] = EP_IN;

				EP2_CNT1 = 0; //clear all the bits in EP1 Count Reg
				EP2_CNT	 = 0;
				EP2_CR0  = USBMODE_ACKIN; // setting EP1 as IN end point;
				PMA2_WA = 128;
				interface += ENDPOINT_SIZE;


			} else {
				USB_status[EP_OUT_INDEX] = EP_OUT ;

				EP1_CNT1 = 0; //clear all the bits in EP1 Count Reg
				EP1_CNT	 = 64;
				EP1_CR0  = USBMODE_ACKOUT; // setting EP1 as IN end point;
				PMA1_WA = 64;

				interface += ENDPOINT_SIZE;
			}

		} while(--ep != 0);


	return interface;
}

// Setup SPLIT and double buffering bits

static void reentrant
USB_setup_buffer(unsigned char index)
{
	unsigned char	bufs;
	unsigned char	mode;
/*
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
	*/
}

// select a configuration and set up endpoints accordingly

static void reentrant
USB_set_config(unsigned char conf)
{
	unsigned char const *	interface;
	unsigned char			inint, outint, mask;

	// reset everything in the endpoint registers, including the data toggle
	/*
	inint = 3;
	do {
		USB_index_write(inint, EINCSRL, CLRDTI|SDSTLI);
		USB_index_write(inint, EINCSRH, 0);
		USB_index_write(inint, EOUTCSRL, CLRDTO|SDSTLO);
		USB_index_write(inint, EOUTCSRH, 0);
		USB_status[inint] = EP_HALT;
	} while(--inint != 0); */
	if(conf == 0) {			// unconfigure
		current_config = 0;
		INT_MSK2 &= ~(INT_MSK2_EP1|INT_MSK2_EP2|INT_MSK2_EP3);	// disable all non-0 endpoints

		return;
	}
	current_config = descriptor->configurations[conf-1];

	// now scan through the interfaces of this configuration and
	// set up the endpoints accordingly.
	// This code does NOT handle alternate interfaces, though
	// multiple (simultaneously active) interfaces are permitted.

	if((conf = current_config[bNumInterfaces]) == 0)
	{
		//printf("No Interfaces" );
		return;
	}
	interface = current_config+CONFIGURATION_SIZE;
	do
		interface = USB_setup_interface(interface);
	while(--conf != 0);

	USB_INTON_1();

	USB_INTON_2();

	/*conf = 3;
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
	} while(--conf != 0);*/
}

// process endpoint 0 (setup) packets
// This routine always ends with one of:
// 	USB_ack_0()			for successful processing, with no IN data to send
// 						This results in a zero-length IN packet.
// 	USB_send_data_0()	returning data to the host
// 	USB_stall_0			any IN or OUT is stalled, indicating protocol error
//

reentrant void
USB_control(void)
{
	unsigned char			idx;
	unsigned int			cnt;
	const unsigned char *	sp;	// string pointer

	USB_INTOFF_0();
	if(USB_status[0] & RX_READY) {
		USB_status[0] &= ~(RX_READY|USB_RESET);
		idx = EP0_DR[REQ_INDEX] & 15;
		cnt = EP0_DR[REQ_LENGTH] + (EP0_DR[REQ_LENGTH+1]<< 8);
		//log("\n%X/%X/%X/%X", EP0_DR[REQ_REQ], EP0_DR[REQ_VALUE+1], EP0_DR[REQ_VALUE], cnt);
		switch(EP0_DR[REQ_REQ]) {

		case SET_ADDRESS:
			// ack the request before changing our address
			idx = USB_CR0_ENABLE | EP0_DR[REQ_VALUE];
			USB_ack_0();
			USB_INTON_0();
			while(USB_status[0] & TX_BUSY)
				continue;						// wait till status packet sent
			USB_CR0 = idx;						// now change address

			break;

			// getting a descriptor. cnt is already set to the requested
			// length, idx will be set to the actual length (will be < 256)
		case GET_DESCRIPTOR:
			sp = 0;
			switch(EP0_DR[REQ_VALUE+1]) {	// check requested type


			case DESC_STRING:
				if(EP0_DR[REQ_VALUE] < descriptor->string_cnt) {
					sp = descriptor->strings[EP0_DR[REQ_VALUE]];
					if(sp == 0)		// serial number
						sp = SERIAL_NUMBER;
					idx = *sp;
				}
				break;

			case DESC_DEVICE:
				sp = descriptor->device;
				idx = DEVICE_SIZE;
				break;

			case DESC_CONFIG:

				if(EP0_DR[REQ_VALUE] < descriptor->conf_cnt) {
					sp = descriptor->configurations[EP0_DR[REQ_VALUE]];
					idx = descriptor->conf_sizes[EP0_DR[REQ_VALUE]];

				}
				break;
			}
			if(sp) {
				if(idx > cnt)
					idx = cnt;
				USB_send_data_0(idx < cnt, sp, cnt);
			} else
			{

				USB_stall_0();
			}
			break;

		case BOMS_RESET:
			USB_status[0] |= PROT_RESET;
			USB_ack_0();
			break;

		case BOMS_GETMAXLUN:
			USB_send_data_0(0, zeropacket, 1);
			break;

		case GET_CONFIG:
			if(current_config)
				USB_send_data_0(0, current_config+5, 1);
			else
				USB_send_data_0(0, zeropacket, 1);
			break;

		case SET_CONFIG:
			// assume configurations are numbered monotonically from 1
			// usbconfig generated descriptors guarantee this.
			if(EP0_DR[REQ_VALUE] <= descriptor->conf_cnt) {

				USB_set_config(EP0_DR[REQ_VALUE]);
				USB_ack_0();

			} else{
				USB_stall_0();
			}
			break;

		case CLEAR_FEATURE:
			if(EP0_DR[REQ_VALUE] == 0 && idx < NUM_ENDPOINTS) {	// clear endpoint halt
				USB_status[idx] &= ~(EP_HALT|RX_READY|TX_BUSY);
				/*
				if(USB_status[idx] & EP_IN)
					USB_index_write(idx, EINCSRL, CLRDTI);					// clear stall
				else
					USB_index_write(idx, EOUTCSRL, CLRDTO);					// clear stall
					*/
				USB_ack_0();
			} else
				USB_stall_0();
			break;

			/*
		case SET_FEATURE:
			if(EP0_DR[REQ_VALUE] == 0 && idx < NUM_ENDPOINTS) {	// halt endpoint
				USB_status[idx] |= EP_HALT;
				if(EP0_DR[REQ_INDEX] & 0x80)
					USB_index_write(idx, EINCSRL, SDSTLI);					// stall endpoint
				else
					USB_index_write(idx, EOUTCSRL, SDSTLO);
				USB_ack_0();
			} else
				USB_stall_0();
			break;
			*/

		case GET_INTERFACE:			// always reply with zer0
			USB_send_data_0(0, zeropacket, 1);
			break;

		case SET_INTERFACE:			// alternate interface settings are not supported
			if(idx == 0)
				USB_ack_0();
			else
				USB_stall_0();
			break;

		case GET_STATUS:
			if(EP0_DR[REQ_TYPE] & 2 && idx < NUM_ENDPOINTS) {	// endpoint status request?
				if(USB_status[idx] & EP_HALT)					// bit zer0 is the halt status
					USB_send_data_0(0, onepacket, 2);
				else
					USB_send_data_0(0, zeropacket, 2);
			} else if(EP0_DR[REQ_TYPE] == 0x80) {				// device status request
				USB_send_data_0(0, zeropacket, 2);
			} else
				USB_stall_0();
			break;

		default:
			USB_stall_0();
			break;
		}
	}
	USB_INTON_0();
}



static reentrant void
USB_reset(void)
{
	INT_MSK2 = 0;				// disable all usb interrupts
	USB_status[0] = 0;
	USB_status[1] = 0;
	USB_status[2] = 0;
	USB_status[3] = 0;
	USB_CR0 = USB_CR0_ENABLE;	// enable the USB controller
	EP0_CR = USBMODE_CTRL_IDLE;	// endpoint 0 for control requests only
	USBIO_CR1 = USBIO_CR1_USBPUEN;
	INT_MSK2 |= INT_MSK2_EP0|INT_MSK2_BUS_RESET;
	ack_flag = 0;
	fifo_cnt = 0;				// doesn't get zeroed automatically
}

static void interrupt reentrant
USB_busreset(void) @ USB_BUS_RESET_VECTOR
{
	if(descriptor != 0)
		USB_reset();
	USB_status[0] = USB_RESET;
}

volatile char g_test;

// make this reentrant to avoid using any ram that the application might want
// Simply set status flags to indicate successful transmission or reception, and
// revert the channel to its idle mode, where SETUP packets are accepted, but
// others are NAKed.

static void interrupt reentrant
USB_isr0(void) @ USB_EP0_VECTOR
{
	unsigned char status;

	status = EP0_CR;


	if(status & EP0_CR_SETUP_RCVD) {
		USB_status[0] |= RX_READY;	// flag data ready
		USB_status[0] &= ~TX_BUSY;	// reset busy flag
	} else if(status & EP0_CR_IN_RCVD) { 	// Data has been sent
		if(USB_status[0] & TX_BUSY)
			USB_status[0] &= ~TX_BUSY;	// reset busy flag
		else
			EP0_CR = USBMODE_CTRL_DONE;	// respond to OUT packet
	}
	if((status & EP0_CR_OUT_RCVD) && (EP1_CNT1 & EP1_CNT1_DATA_VALID) ){		// out has been received, end of transaction

		EP0_CR = USBMODE_ISOOUT;
		USB_status[0] |= RX_READY;	// flag data ready
	}


}


static void interrupt reentrant
USB_isr2(void) @ USB_EP2_VECTOR
{

	unsigned char status = EP2_CR0;
	X_OFF();

	if(status & USBMODE_NAKIN) { 	// OUT recieved and SIE changed mode to NAK OUT
			EP2_CNT1 ^= EP1_CNT1_DATA_TOGGLE;

			USB_status[EP_IN_INDEX] &= ~TX_BUSY;
			PMA2_WA = 0;


	}


}


static void interrupt reentrant
USB_isr1(void) @ USB_EP1_VECTOR
{
	unsigned char status;

	status = EP1_CR0;


	if(status & USBMODE_NAKOUT) { 	// OUT recieved and SIE changed mode to NAK OUT

		USB_status[EP_OUT_INDEX] |= RX_READY;
		/*g_out_count ^= EP1_CNT1_DATA_TOGGLE;
		g_toggle = (EP1_CNT1 & EP1_CNT1_DATA_TOGGLE) & g_out_count;
		EP2_CNT1 = g_toggle;*/

	}
}


reentrant void
USB_init(USB_descriptor_table const * table)
{
	descriptor = table;
	current_config = 0;
	USB_reset();
}

	// init the USB


reentrant void
USB_detach(void)
{
	INT_MSK2 = 0;				// disable all usb interrupts
	EP0_CR = 0;
	USB_CR0 = 0;				// disable the usb hardware
}
#endif
