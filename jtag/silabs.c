#include	<8051.h>
#include	"jtag.h"

/*
 * 	JTAG/C2 interface for Silicon Labs 8051 devices
 *
 * 	JTAG connector ports
 *	P2.5		JTAG pin 11	C2CK	(aka /RST)
 *	P2.3		JTAG pin 15	C2DAT
 *
 */

#define	C2CK	(P2_BITS.B5)
#define	C2D		(P2_BITS.B3)

#define	ClkStrobe()	((C2CK = 0), (C2CK = 1))// pulse clock low
#define	ClkEnable()	(P2MDOUT |= (1<<5))		// set clock driver as push/pull
#define	ClkDisable()(P2MDOUT &= ~(1<<5))	// set clock driver as open drain
#define	DatEnable()	(P2MDOUT |= (1<<3))		// set data driver as push/pull
#define	DatDisable() (P2MDOUT &= ~(1<<3))	// set data driver as open drain

#define	usDelay(n)	_delay(n*24)			// busy delay in microseconds


// C2 Registers
#define	FPDAT		0xB4
#define	FPCTL		0x02
#define	DEVICEID	0x00
#define	REVID		0x01

// C2 status return codes

#define	INVALID_COMMAND	0x00
#define	COMMAND_FAILED	0x02
#define	COMMAND_OK		0x0D

//	C2	interface	commands

#define	GET_VERSION		0x01
#define	BLOCK_READ		0x06
#define	BLOCK_WRITE		0x07
#define	PAGE_ERASE		0x08
#define	DEVICE_ERASE	0x03
#define	REG_READ		0x09
#define	REG_WRITE		0x0A
#define	RAM_READ		0x0B
#define	RAM_WRITE		0x0C

// FPCTL codes
#define INIT_KEY1   0x02        // first key
#define INIT_KEY2   0x01        // second key
#define RESUME_EXEC 0x08        // resume code execution

static bit flash_inited;		// if we have initialized flash programming

static void
c2_reset(void)
{
	DatDisable();
	ClkEnable();
	C2CK = 0;		// drive clock low for
	usDelay(500);	// exceeds min spec of 20us
	C2CK = 1;		// set clock high
	flash_inited = FALSE;
	ClkDisable();
}

static void
c2_send(unsigned char v)
{
	unsigned char	i;

	i = 8;
	do {
		C2D = v & 1;
		ClkStrobe();
		v >>= 1;
	} while(--i != 0);
	DatDisable();
	C2D = 1;
	ClkStrobe();
}

// write the address register

static void
c2_writeAR(unsigned char v)
{
	EA = 0;
	ClkStrobe();
	C2D = 1;
	DatEnable();
	ClkStrobe();
	ClkStrobe();
	c2_send(v);
	EA = 1;
}

static unsigned char
c2_read(void)
{
	unsigned char	i, data;

	data = 0;
	i = 8;
	do {
		data >>= 1;
		_delay(2);
		ClkStrobe();
		if(C2D)
			data |= 0x80;
	} while(--i != 0);
	ClkStrobe();			// stop field
	EA = 1;
	return data;
}

static unsigned char
c2_readAR(void)
{
	EA = 0;
	ClkStrobe();
	C2D = 0;
	DatEnable();
	ClkStrobe();
	C2D = 1;
	ClkStrobe();
	DatDisable();
	return c2_read();
}

// read the data register 

static unsigned char
c2_readDR(void)
{
	unsigned char	i;

	EA = 0;
	ClkStrobe();
	C2D = 0;
	DatEnable();
	ClkStrobe();
	ClkStrobe();
	ClkStrobe();	// strobe length field (0 => 1 byte)
	ClkStrobe();
	DatDisable();
	C2D = 1;
	i = 0;
	do				// wait for device ready
		ClkStrobe();
	while(C2D == 0 && --i != 0);
	return c2_read();
}

// write the data register
static void
c2_writeDR(unsigned char v)
{
	EA = 0;
	ClkStrobe();
	C2D = 1;
	DatEnable();
	ClkStrobe();
	C2D = 0;
	ClkStrobe();
	ClkStrobe();
	ClkStrobe();
	c2_send(v);
	v = 0;
	while(C2D == 0 && --v != 0)		// wait for done
		ClkStrobe();
	ClkStrobe();		// stop field
	EA = 1;
}


static BOOL
c2_outReady(void)
{
	while((c2_readAR() & 0x01) == 0)
		if(TIMEDOUT())
			return FALSE;
	return TRUE;
}

static BOOL
c2_inBusy(void)
{
	while(c2_readAR() & 0x02)
		if(TIMEDOUT())
			return FALSE;
	return TRUE;
}

// connect the target device, get device ID

static reentrant BOOL
c2_connect(void)
{
	unsigned char i;

	c2_reset();		// reset chip
	usDelay(2);
	ClkEnable();
	c2_writeAR(DEVICEID);
	i = c2_readDR();
	if(i == 0xFF)
		return FALSE;
	dev_id = i << 8;
	c2_writeAR(REVID);
	dev_id |= c2_readDR();
	return TRUE;
}
// disconnect the target - just reset it

static reentrant BOOL
c2_disconnect(void)
{
	c2_reset();		// reset chip
//	c2_writeAR(FPCTL);		// address the FPCTL register
//	c2_writeDR(RESUME_EXEC);	// run away....
	return TRUE;
}

// enable flash programming

static void
c2_flashInit(void)
{
	if(flash_inited)
		return;
	c2_writeAR(FPCTL);		// address the FPCTL register
	c2_writeDR(INIT_KEY1);
	c2_writeDR(INIT_KEY2);			// unlock commands
	usDelay(20000);			// wait 20ms
	flash_inited = TRUE;
}

static BOOL
c2_writeDRwait(unsigned char v)
{
	c2_writeDR(v);
	return c2_inBusy();
}

static BOOL
c2_writeCmd(unsigned char v)
{
	c2_writeDR(v);
	c2_inBusy();
	c2_outReady();
	return c2_readDR() == COMMAND_OK;
}

// erase the entire device memory

static reentrant BOOL
c2_deviceErase(void)
{
	unsigned char status;

	c2_flashInit();
	SET_TIMEOUT(1500);
	c2_writeAR(FPDAT);
	if(!c2_writeCmd(DEVICE_ERASE))
		return FALSE;
	// confirm the command with a three byte sequence 0xDE, 0xAD, 0xA5.
	c2_writeDRwait(0xDE);
	c2_writeDRwait(0xAD);
	c2_writeDRwait(0xA5);
	return c2_outReady();
}

// Read memory

static reentrant BOOL
c2_readmem(unsigned char xdata * p, unsigned char len, unsigned long addr, unsigned char mem)
{
	unsigned char status;

	c2_flashInit();
	SET_TIMEOUT(500);
	c2_writeAR(FPDAT);
	switch(mem) {

	case JT_PROGMEM:
		if(!c2_writeCmd(BLOCK_READ))
			return FALSE;
		c2_writeDRwait((addr >> 8) & 0xFF);
		c2_writeDRwait(addr & 0xFF);
		if(!c2_writeCmd(len))
			return FALSE;
		do {
			if(!c2_outReady())
				return FALSE;
			*p++ = c2_readDR();
		} while(--len != 0);
		return TRUE;

	case JT_SFR:
		if(!c2_writeCmd(REG_READ))
			return FALSE;
		c2_writeDRwait(addr & 0xFF);
		error_code = 2;
		if(!c2_writeDRwait(len))
			return FALSE;
		error_code = 3;
		do {
			if(!c2_outReady())
				return FALSE;
			*p++ = c2_readDR();
		} while(--len != 0);
		return TRUE;

	case JT_DATAMEM:
		if(addr >= 0x100)
			return TRUE;		// don't try to read any
		if(!c2_writeCmd(RAM_READ)) {
			error_code = 1;
			return FALSE;
		}
		c2_writeDRwait(addr & 0xFF);
		if(!c2_writeDRwait(len)) {
			error_code = 2;
			return FALSE;
		}
		error_code = 3;
		do {
			if(!c2_outReady())
				return FALSE;
			*p++ = c2_readDR();
		} while(--len != 0);
		return TRUE;
	}
	return FALSE;
}

// Program flash memory

static reentrant BOOL
c2_writemem(unsigned char xdata * p, unsigned char len, unsigned long addr, unsigned char mem)
{
	unsigned char status;

	if(mem == JT_PROGMEM) {
		c2_flashInit();
		SET_TIMEOUT(500);
		c2_writeAR(FPDAT);
		if(!c2_writeCmd(BLOCK_WRITE))
			return FALSE;
		c2_writeDRwait((addr >> 8) & 0xFF);
		c2_writeDRwait(addr & 0xFF);
		if(!c2_writeCmd(len))
			return FALSE;
		do {
			c2_writeDRwait(*p++);
		} while(--len != 0);
		return c2_outReady();
	}
	return FALSE;
}

const dev_int 	si_c2 =
{
	JT_SILABSC2,
	c2_connect,
	c2_deviceErase,
	c2_disconnect,
	c2_writemem,
	c2_readmem,
};



