#include	<psoc.h>
#include	"cy8c24x94.h"
#include	<string.h>
#include	<stdio.h>
#include	"bootload.h"
#include	"lcd.h"

/*
 * 	USB Bootloader for Cypress CY8C24x94
 *
 * 	Port P3.7  is used as a bootload flag - when set, or if no
 * 	user program is loaded in flash, the program goes into bootload mode,
 * 	otherwise it runs the user program.
 *
 */

#define	FALSE	0
#define	TRUE	1

// reset source bits

// pins used: one led, one sense pin




static unsigned char	flashcontent[64];
//static unsigned char	usb_packet[64];

volatile unsigned char KEY1 		@ 0xF8;
volatile unsigned char KEY2 		@ 0xF9;
volatile unsigned char BLOCKID 		@ 0xFA;
volatile unsigned char *POINTER 	@ 0xFB;
volatile unsigned char CLOCK 		@ 0xFC;
volatile unsigned char DELAY 		@ 0xFE;
//volatile unsigned char FLS_PR1 		@ 0x1FA;
volatile __ioport unsigned char	FLS_PR1 		@ 0x1FA;
static unsigned char M_VAL			= 0xFF;
static unsigned char B_VAL			= 0xFF;
static unsigned char MULT_VAL		= 0xFF;
static unsigned int CLOCK_E			= -1;
static unsigned int CLOCK_W			= -1;

static unsigned char g_usbqoffset	= 0;

extern unsigned char g_toggle;
const unsigned char userstart @ USER_START;
unsigned char* rompointer = &userstart;

//#define ei()	asm("or F, 01h")

#define COLD	0


#define  DELAY_VAL	0xB2 //DELAY = (100*10^-6 * 24 * 10^6 - 80) / 13 = 178.4 Rounded 178 (0xB2)
/*CLOCK:
 *
 */

#define	LOCK_BYTE		(*(unsigned char code *)0x3DFF)		// flash lock byte

static void
PORT_init(void)
{
	// init IO ports
	// P3.7 is an input, P3.6 and p3.5 are output (LED drive)
	PRT3DM2 = 0x0F;
	PRT3DM1 = 0x8F;
	PRT3DM0 = 0x70;
	//enable port1[0] for input
	PRT1DM2 &= ~0x1;
	PRT1DM1 |= 0x01;
	PRT1DM0 &= ~0x01;
}

extern USB_CONST USB_descriptor_table psoctest_table;
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
q_usbdata(char *data, int len, unsigned char force){
	int offset = 0;

	while(len > MAX_USBDATA){
		USB_send_bytes(EP_IN_INDEX, data+(offset*MAX_USBDATA), MAX_USBDATA, g_usbqoffset, 1);
		len -= MAX_USBDATA;
		offset++;
	}
	if(force){

		/*memcpy((void*)(usb_packet+g_usbqoffset),
				(void*)(data +(offset*MAX_USBDATA)) , len);

		USB_send_bytes(EP_IN_INDEX, usb_packet, g_usbqoffset+len);
		printf("%X,%X,%X ", data[0], data[1], g_usbqoffset);
		printf("%X,%X ", usb_packet[0], usb_packet[1]);

		g_usbqoffset = 0;*/

		USB_send_bytes(EP_IN_INDEX, data, len, g_usbqoffset, 1);


	}else{

		//dont send just queue it
		/*memcpy((void*)(usb_packet+g_usbqoffset),
				(void*)(data +(offset*MAX_USBDATA)) , len);
		g_usbqoffset += len;
		//if more than MAX_USBDATA is been copied then we need to send
		if(g_usbqoffset >= MAX_USBDATA){
			printf("NF%X,%X", offset,g_usbqoffset );
			USB_send_bytes(EP_IN_INDEX, usb_packet, MAX_USBDATA);
			g_usbqoffset =0;*/

		if(g_usbqoffset + len  >= MAX_USBDATA){
			USB_send_bytes(EP_IN_INDEX, data, MAX_USBDATA , g_usbqoffset, 1);
			len -= MAX_USBDATA;
		}

		USB_send_bytes(EP_IN_INDEX, data, len, g_usbqoffset, 0);
		g_usbqoffset+= len;
	}

}





static void
send_res(void)
{

	unsigned char xdata *p;
	p = (unsigned char xdata *)&RES;
	q_usbdata(p, sizeof RES, 1);
}
volatile unsigned  char temp;


reentrant unsigned char
table_read(unsigned char blockid){
	asm("mov X, SP");
	asm("mov [_temp], X");
	KEY2 = temp+3;
	/*setting up parameters*/

	KEY1 	= 0x3A;
	BLOCKID = blockid;
	asm("mov A, 0x06");
	asm("ssc");



}


reentrant void
swbootreset(void){
	asm("mov A, 0x00");
	asm("ssc");
}




reentrant unsigned char
write_block(unsigned char blockid, unsigned char bank){
	asm("mov X, SP");
	asm("mov [_temp], X");
	KEY2 = temp+3;
	/*setting up parameters*/
	FLS_PR1 = bank & 0x03;
	KEY1 	= 0x3A;
	BLOCKID = blockid;
	CLOCK 	= CLOCK_W;
	DELAY 	= DELAY_VAL;
	MVR_PP = 1;
	asm("mov A, 0x02");
	asm("ssc");
}





reentrant char
erase_block(unsigned char blockid , unsigned char bank){
	KEY1 = 0x3a;
	asm("mov X, SP");
	asm("mov [_temp], X");
	KEY2 	= temp+3;
	BLOCKID = blockid;
	CLOCK 	= CLOCK_E;
	DELAY 	= DELAY_VAL;
	FLS_PR1 = bank & 0x03;
	asm("mov A, 0x03");
	asm("ssc");
}

reentrant char
progmem(unsigned int addr, unsigned char len)
{
	/*
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
			*/
	return 1;
}


static void
Endpoint_1(void)
{
	unsigned long	addr;
	char some[8];
	unsigned int len;
	unsigned char	idx;
	unsigned char bank = 0, block= 0;




	USB_read_packet(EP_OUT_INDEX, (xdata unsigned char *)&CMD, sizeof CMD);




	if(CMD.sig != BL_SIG) {
		return;
	}








	RES.tag = CMD.tag;		// copy the tag ready for response
	RES.sig = BL_RES;
	RES.status = 0;
	RES.sense = 0;
	len = CMD.len;
	addr = CMD.addr;





	switch(CMD.cmd) {


	case BL_IDENT:
		printf("Ident");
		RES.value = 0xabcd;
		break;

	case BL_QBASE:
		RES.value = 0x1000;
		break;
	case BL_REQBL:
		send_res();
		while(USB_status[EP_IN_INDEX] & TX_BUSY);
		USB_detach();
		bl_main();
		break;



#if 0
	case BL_DONE:
		USB_detach();
		send_res();
		asm("ljmp _userstart");

	case BL_INIT:
		/*read table 3 and initialize M and B values*/
		table_read(3);
		if(COLD){
			M_VAL 		= KEY1;
			B_VAL 		= KEY2;
			MULT_VAL 	= *((unsigned char*)0xFA);
		}else{
			M_VAL 		= *((unsigned char*)0xFB);
			B_VAL 		= *((unsigned char*)0xFC);
			MULT_VAL 	= *((unsigned char*)0xFD);
		}
		CLOCK_E = B_VAL - ((2*M_VAL*25)/256);
		CLOCK_W = (CLOCK_E * MULT_VAL )/64;
	break;

	case BL_VERIFY:

		while(!(USB_status[EP_OUT_INDEX] & RX_READY))
				continue;

		USB_read_packet(EP_OUT_INDEX, flashcontent, len);
		rompointer = addr;
		idx = 0;

		while(*((unsigned char*)(rompointer+idx)) == *((unsigned char*)(flashcontent+idx))){


			idx++;
			if(idx == len){

				break;
			}
		}
		if(idx != len){
			RES.status = 1;
			RES.sense = BL_VERIFYERR;
		}
		break;



	default:
		RES.status = 1;
		RES.sense = BL_BADREQ;
		break;
#endif

	}
	/*flashcontent[0] = 0x0;
	flashcontent[1] = 0x1;
	flashcontent[2] = 0x2;
	flashcontent[3] = 0x3;
	flashcontent[9] = 0x9;
	flashcontent[63] = 0x63;

	USB_send_bytes(EP_IN_INDEX, flashcontent, 64, 0, 0);
	flashcontent[0] = 0x10;
	flashcontent[1] = 0x11;
	flashcontent[2] = 0x12;
	flashcontent[3] = 0x13;
	flashcontent[9] = 0x19;
	USB_send_bytes(EP_IN_INDEX, flashcontent, 10, 0, 0);*/

	flashcontent[0] = 0x20;
	flashcontent[1] = 0x21;
	flashcontent[2] = 0x22;
	flashcontent[3] = 0x23;
	flashcontent[9] = 0x29;
	USB_send_bytes(EP_IN_INDEX, flashcontent, 16, 0, 1);
		printf("*Here");

	//send_res();
}
extern delayMs(unsigned char c);
extern volatile char g_test;

void main(void){
	unsigned int	i;

	/*	if(LOCK_BYTE == 0xFF) {		// ensure lock byte set
			blkbuf[0] = 0b11100000;
			progmem((unsigned int)&LOCK_BYTE, 1);
		} */
		// Enter bootload mode if:
		// 		* There is no user program (i.e. no LJMP at the base address), or
		// 		* If P3.0 is pulled low at reset, or
		// 		* If the reset was a software reset - this occurs when the
		// 		  application program requests bootload mode.
		// 	Otherwise just jump to the user program

		/*
		if(*(unsigned char code *)USER_START == 0x02 && !BOOT_SEL
				&& (RSTSRC & (PORSF|SWRSF)) != SWRSF)
			(*(void (*)(void))USER_START)();
	*/
		OSC_CR0 = 0x3;		// fastest clock available
		USB_CR1 = USB_CR1_ENABLE_LOCK | USB_CR1_REG_ENABLE; // 5V operation, lock clock
		PORT_init();

#if	DEBUG
		lcd_init();

#endif


		ei();

		ACT_OFF();
		X_OFF();


		USB_init(&psoctest_table);





		for(;;) {
			RES_WDT = 0x38;
			i++;
			if(i == 0x8000) {
				ACT_OFF();
				LED_OFF();
			} else if(i == 0)
				LED_ON();
			if(USB_status[0] & (RX_READY|USB_SUSPEND)) {
				ACT_ON();
				USB_control();


			}


			if(USB_status[EP_OUT_INDEX] & RX_READY) {
				LED_ON();
				Endpoint_1();
			}
	}
	return;
}



#if	0
void
log(const char * s)
{
	static char	where;
	lcd_goto(where);
	lcd_puts("               ");
	lcd_goto(where);
	lcd_puts(s);
	where ^= 0x40;
}
#endif
