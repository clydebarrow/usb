#include	<8051.h>
#include	<stdio.h>
#include	<htusb.h>
#include	"zipcard.h"

/*
 * 	SD card interface:
 *
 * 	P0.0		Activity LED (red)
 * 	P0.1		Card present input
 * 	P0.2		SPI Clock
 * 	P0.3		Unused
 * 	P0.4		Unused
 * 	P0.5		Unused
 * 	P0.6		Power led (green or yellow)
 * 	P0.7		Analog VREF
 * 	
 * 	P1.0		Data0/ MISO (Master in,slave out)
 * 	P1.1		Data1 - not used
 * 	P1.2		Data2 - not used
 * 	P1.3		Data3 / Slave select
 * 	P1.4		CMD / MOSI
 * 	P1.5		Write protect input
 *
 */

#define	FALSE	0
#define	TRUE	1

// Error codes for the SD

enum {
	RESET_FAIL=1, READ_FAIL, CSD_FAIL, WRITE_FAIL, STATUS_FAIL, ACMD_FAIL, ERASE_FAIL
};

// mass storage engine states
enum {
	BM_IDLE = 0,		// waiting for command
   	BM_CMD,			// command received
	BM_CSW,			// response created and ready to send
	BM_DATAIN,		// sending card data to the host
	BM_DATAOUT,		// receiving card data from the host
	BM_CDATA,		// sending const data
	BM_XDATA,		// sending xdata data
	BM_READ,		// receiving a packet from the host
	BM_STALL,		// waiting for stall to end
}	mse_state;

// card present states

enum {
	CARD_MISSING = 0,
	CARD_PRESENT,
	CARD_READY,
	CARD_FAIL,
	CARD_EJECTED
}	card_state;

static const unsigned char *	cptr;		// const data pointer
static xdata unsigned char *	xptr;		// data pointer
static unsigned short			data_cnt;	// bytes remaining
static bit						pad_bytes;	// Data transfer will be short
static unsigned long			card_size;	// card size in blocks
static unsigned long			part_offset;// offset to start of first partition
static unsigned char			file_format;	// partition layout
static unsigned long			current_block;	// current block being transferred
static unsigned short			block_count;	// blocks remaining to be sent/received
static unsigned char			sense_key;
static unsigned char			additional_sense;

#define	V33			(1<< 6)		// bit identifying 3.3V
#define	SD_BLKLEN	512			// block size
#define	CSD_SIZE	16			// CSD size in bytes
#define	CID_SIZE	16			// CID size in bytes

// The USB descriptors.

extern USB_CONST USB_descriptor_table	zipcard_table;

unsigned char xdata	blkbuf[SD_BLKLEN];

// logging

#ifdef	DEBUG
static unsigned char xdata	logbuf[SD_BLKLEN] @ 0x400;
static unsigned char		inptr, optr;		// buffer pointers
#endif


//	buffer for CBW requests

static xdata CBW_t	CBW;

// CSW buffer
static xdata CSW_t	CSW;

// handle CBWs from the host

// inquiry data - pretend to be a Zip 100 drive
static const unsigned char	inquiry_data[36] =
{
	0x00, 0x80, 0x00, 0x01, 0x75, 0x00, 0x00, 0x00,
	'H',  'I',  '-',  'T',  'E',  'C',  'H',  0x20,
	'Z',  'i',  'p',  'C',  'a',  'r',  'd',  0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	'0' + VERS_MAJOR,  '.',  '0'+VERS_MINOR,  0x20,
};
static const unsigned char	iomega_data[36] =
{
	0x00, 0x80, 0x00, 0x01, 0x75, 0x00, 0x00, 0x00,
	'I',  'O',  'M',  'E',  'G',  'A',  ' ',  0x20,
	'Z',  'I',  'P',  ' ',  '1',  '0',  '0',  0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	'7',  '9',  '.',  'E',
};


// max formatted capacity request - return similar to a zip
// drive, but report up to 1GB
// 
static const unsigned char	max_format_data[20] =
{
 0x00, 0x00, 0x00, 0x10,
 0x00, 0x20, 0x00, 0x00,
 0x03, 0x00, 0x02, 0x00,
 0x00, 0x20, 0x00, 0x00,
 0x00, 0x00, 0x02, 0x00,
};


/***********	Application code ***************/

//	SD card bits

#define	CARD_PRESENT	(P0_BITS.B1==0)
#define	WRITE_PROTECT	(P1_BITS.B5)

#define	ACMD	55			// command following is application specific

// choose which pins the activity and power leds are on, and
// their polarity

#define	POWER_LED		(P0_BITS.B6)
#define	ACTIVITY_LED	(P0_BITS.B0)
#define	ACTIVE(x)		ACTIVITY_LED = !(x)
#define	GREEN(x)		POWER_LED = !(x)
#define	CARD_SELECT		(P1_BITS.B3)

#define	SPIBUSY	0x80	// spi busy bit

#define	CRC				0x95		// all purpose CRC

// Timer 2 reload period

#define	SYSCLK	24000000		// system clock freq
#define	T2PRE	12				// timer 2 prescaler
#define	T2RATE	(128-1)			// timer 2 desired interrupt rate - 128Hz

#define	T2LOAD	(SYSCLK/T2PRE/T2RATE)	// reload value for T2

static unsigned char	error_code;		// output this error code on the activity led

static bit				card_present;	// card is present (debounced version)
static bit				card_debounce;	// debouncing bit
static bit				write_protect;	// write not enabled
static bit				write_debounce;	// debouncing bit
static bit				active;			// busy doing something
static bit				fat32;			// set if we have a fat32 card
static bit				fat_check;		// we need to check the fat again
static bit				avidyne;		// set if we are talking to an MFD
static unsigned char	timeout;		// timeout value in ticks

#define	SET_TIMEOUT(x)	(timeout = (((x)*128UL))/1000+1)
#define	TIMEDOUT()		(timeout == 0)

#ifdef	LOGGING
#define	BUFSIZE	(sizeof cmdlog/sizeof cmdlog[0])
static unsigned char code	cmdlog[128][32] @ 0x2C00;	// make sure this doesn't overlap
static code struct
{
		unsigned char	csd[2][16];
		unsigned char	part[4][16];
		unsigned char	serial[2];
}							cardinfo @ 0x2A00;

static unsigned char		logoffset;					// offset into log buffer
static unsigned int			logidx;

static void reentrant
writemem(unsigned int addr, unsigned char i, unsigned char xdata * p)
{
	char	c;

	EA = 0;
	if((addr & 0x1FF) == 0) {	// must erase the block
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 3;
		*(unsigned char xdata *)addr = 0xFF;
		PSCTL = 0;
	}
	while(i-- != 0) {
		c = *p++;
		FLKEY = 0xA5;
		FLKEY = 0xF1;
		PSCTL = 1;
		*((unsigned char xdata *)addr) = c;
		addr++;
		PSCTL = 0;
	}
	EA = 1;
}

static void
writelog(void)
{
	if(!avidyne && logidx >= 2)
		return;
	if(logoffset == BUFSIZE)
		logoffset = 0;
	writemem((unsigned int)(cmdlog+logoffset), (sizeof CBW)-2, (unsigned char xdata *)&CBW);
	blkbuf[0] = logidx >> 8;
	if(avidyne)
		blkbuf[0] |= 0x80;
	blkbuf[1] = logidx;
	blkbuf[2] = sense_key;
	writemem((unsigned int)(cmdlog+logoffset)+sizeof CBW-2, 3, blkbuf);
	logoffset++;
	logidx++;
}

#else	// LOGGING
#define	writelog()		// do nothing
#endif	// LOGGING

static void interrupt
t2_isr(void) @ TIMER2
{
	static unsigned char		flashes, counter;
	unsigned char				i;

	TF2H = 0;					// clear interrupt flag
	if(CARD_PRESENT == card_debounce)	// debounce card present switch
		card_present = card_debounce;
	card_debounce = CARD_PRESENT;
	if(WRITE_PROTECT == write_debounce)	// debounce write enable
		write_protect = write_debounce;
	write_debounce = WRITE_PROTECT;

	if(timeout)
		--timeout;
	counter++;
	if(counter == 0) {
		if(!flashes && error_code) {
			flashes = error_code;
			ACTIVE(1);
		}
		counter++;		// counter should never be 0
	}
	if(flashes) {
		if((counter & 0x1F) == 0)
			ACTIVE(1);		// turn led on
		else if((counter & 0x1F) == 0x10) {
			ACTIVE(0);		// turn led off
			flashes--;
		}
	}
	if(!error_code)
		ACTIVE(active);

	// make the power led flash (or not) based on certain things
	i = 0xFF;
	if(fat32)
		i = 0x78;
	if(card_state != CARD_READY)
		i = 0x40;
	if(USB_status[0] & USB_SUSPEND)
		i = 0;
	GREEN(counter & i);
}

static void
SPI_init(void)
{
	// setup the SPI for the SD card. 
	// Data latched on rising edge, input valid in middle of clock,
	// idle state is high.

	SPI0CKR = 0x30;				// start SPI real slow
	SPI0CFG = 0B01000000;
	SPI0CN = 1;

}

// Send one byte to the SPI

static void
SPI_send(unsigned char databyte)
{
	while(TXBMT == 0)
		continue;
	SPI0DAT = databyte;
}

// read one byte from the SPI

static unsigned char
SPI_read(void)
{
	SPIF = 0;
	SPI0DAT = 0xFF;
	//while(SPI0CFG & SPIBUSY)
	while(!SPIF)
		continue;
	return SPI0DAT;
}

// Send idle (0xFF) a specified number of bytes

static void
SD_idle(unsigned char cnt)
{
	while(cnt--)
		SPI_send(0xFF);
}

// Send a command and wait for a response

static unsigned char
SD_send(unsigned char cmd, unsigned long arg)
{
	unsigned char	tok;

	CARD_SELECT = 0;			// select the card
	SD_idle(3);				// lead-in
	SPI_send(cmd|0x40);			// send the command
	SPI_send(arg >> 24);
	SPI_send(arg >> 16);
	SPI_send(arg >> 8);
	SPI_send(arg);
	SPI_send(CRC);
	cmd = 16;
	do
		tok = SPI_read();	// wait for response - up to 16 times
	while(tok & 0x80 && --cmd != 0);
	return tok;
}

static bit
SD_reset(void)
{
	SPI_init();
	SD_idle(100);
	CARD_SELECT = 0;		// select the card
	SD_idle(100);			// send numerous clocks to initialize
	CARD_SELECT = 1;
	SD_idle(100);
	SD_send(0, 0);			// send initialization command
	SET_TIMEOUT(1000);		// initialization should take no more than 1 second
	do {
		if(TIMEDOUT()) {
			error_code = RESET_FAIL;
			return FALSE;
		}
		SD_send(55, 0);			// app-specific command follows
	} while(SD_send(41, 0) & 1);
	SPI0CKR = 0;				// maximum speed - 12 MHz
	return TRUE;
}


static unsigned char
SD_cmd(unsigned char cmd, unsigned long arg)
{
	if(SD_send(cmd, arg) == 0)
		return 0;
	GREEN(0);
	_delay(24000000/4);		// wait 250 ms to ensure buffers flushed
	SD_reset();
	return SD_send(cmd, arg);
}

#ifdef	LOGGING
// read the status register

static unsigned
SD_readstatus(void)
{
	unsigned char	i;

	i = SD_cmd(13, 0);
	if(i & 0x80) {
		error_code = STATUS_FAIL;
		return FALSE;
	}
	return (i << 8) + SPI_read();
}
#endif

// read the CSD - 16 bytes 

static bit
SD_readcsd(void)
{
	unsigned char	token;
	unsigned char	i;
	unsigned int	len;

	if(SD_cmd(9, 0)) {
		error_code = CSD_FAIL;
		return FALSE;
	}
	SET_TIMEOUT(200);
	do
		token = SPI_read();
	while(token != 0xFE && !TIMEDOUT());
	if(token & 1) {
		error_code = CSD_FAIL;
		return FALSE;
	}
	i = 0;
	do
		blkbuf[i] = SPI_read();
	while(++i != CSD_SIZE);
	SPI_read();
	SPI_read();
	i = (blkbuf[15-6] & 3) << 1;		// bits 48-49
	i += blkbuf[15-5] >> 7;			// bit 47
	i += 2;
	len = blkbuf[15-8] << 2;
	len += blkbuf[15-7] >> 6;
	len += (blkbuf[15-9] & 3) << 10;
	len++;
	card_size = (1L << i) * len - 1;	// set to number of last block
	file_format = (blkbuf[15-1] >> 2) & 3;
#ifdef	LOGGING
	if(SD_cmd(10, 0)) {
		error_code = CSD_FAIL;
		return FALSE;
	}
	SET_TIMEOUT(200);
	do
		token = SPI_read();
	while(token != 0xFE && !TIMEDOUT());
	if(token & 1) {
		error_code = CSD_FAIL;
		return FALSE;
	}
	i = 0;
	do
		blkbuf[i+CSD_SIZE] = SPI_read();
	while(++i != CID_SIZE);
	writemem((unsigned)cardinfo.csd, CSD_SIZE+CID_SIZE, blkbuf);
	blkbuf[0] = VERS_MAJOR;
	blkbuf[1] = VERS_MINOR;
	writemem((unsigned)cardinfo.serial, 2, blkbuf);
#endif
	return TRUE;
}

static void
SD_stopwrite(void)
{
	SPI_send(0xFD);
	SET_TIMEOUT(500);
	SPI_send(0xFF);
	while(!TIMEDOUT())
		if(SPI_read() == 0xFF)
			break;
}

// commence a read operation

static bit
SD_startread(unsigned long blockno)
{
	if(SD_cmd(17, blockno << 9)) {
		error_code = READ_FAIL;
		return FALSE;
	}
	return TRUE;
}

static bit
SD_waitread(void)
{
	unsigned char	token;
	unsigned int	i;

	SET_TIMEOUT(200);
	do
		token = SPI_read();
	while(token != 0xFE && !TIMEDOUT());
	if(token & 1) {
		error_code = READ_FAIL;
		return FALSE;
	}
	return TRUE;
}

/*
// commence a write operation

static bit
SD_startwrite(unsigned long blockno)
{
	unsigned char	token;
	unsigned int	i;

	if(SD_cmd(24, blockno << 9)) {
		error_code = WRITE_FAIL;
		return FALSE;
	}
	SPI_send(0xFF);
	SPI_send(0xFE);			// start of write
	return TRUE;
}

static bit
SD_writeblock(unsigned long blockno, unsigned char xdata * buf)
{
	unsigned int	i;
	unsigned char	tok;

	if(!SD_startwrite(blockno))
		return FALSE;
	i = SD_BLKLEN;
	do
		SPI_send(*buf++);
	while(--i != 0);
	do
		tok = SPI_read();
	while(tok == 0xFF);
	tok &= 0x1F;
	if(tok != 5) {
		error_code = WRITE_FAIL;
		return FALSE;
	}
	// wait for completion
	while(SPI_read() != 0xFF)
		continue;
	return TRUE;
}
*/

// read one BLKLEN block from the card.

static bit
SD_readblock(unsigned long blockno, unsigned char xdata * buf)
{
	unsigned int	i;

	if(!SD_startread(blockno))
		return FALSE;
	if(!SD_waitread())
		return FALSE;
	i = 0;
	do
		*buf++ = SPI_read();
	while(++i != SD_BLKLEN);
	SPI_read();
	SPI_read();		// flush unwanted CRC
	return TRUE;
}

static unsigned long
get_long(unsigned char xdata * ptr)
{
	unsigned long	value;

	value = *ptr++;
	value += (unsigned short)*ptr++ << 8;
	value += (unsigned long)*ptr++ << 16;
	value += (unsigned long)*ptr++ << 24;
	return value;
}
static void
set_residue(unsigned long value)
{
	((unsigned char xdata *)&CSW.dCSWResidue)[0] = value;
	((unsigned char xdata *)&CSW.dCSWResidue)[1] = value >> 8;
	((unsigned char xdata *)&CSW.dCSWResidue)[2] = value >> 16;
	((unsigned char xdata *)&CSW.dCSWResidue)[3] = value >> 24;
	pad_bytes = value != 0;
}

// Return the minimum of the requested length and the desired length.
// Calculate the data residue, and write it to the appropriate place,
// this keeps the long arithmetic all in once place.

static unsigned short
residue(unsigned short available)
{
	unsigned long	requested = get_long(CBW.dCBWDataTransferLength);

	if(available > requested)
		available = requested;
	set_residue(requested-available);
	return available;
}

// same kind of thing, for block transfers

static unsigned short
block_residue(unsigned short blkcnt)
{
	unsigned long	requested = get_long(CBW.dCBWDataTransferLength);

	if(requested < (unsigned long)blkcnt*SD_BLKLEN) {
		CSW.bCSWStatus = CSW_PHASE;
		blkcnt = requested/SD_BLKLEN;
	}
	set_residue(requested - (unsigned long)blkcnt*SD_BLKLEN);
	return blkcnt;
}


// init the SD card
static bit
SD_init(void)
{
	unsigned char	i;
	unsigned long	sectors;

	part_offset = 0;
	card_size = 0;
	// all successful, ramp up the clock rate now.

	if(!SD_reset() || !SD_readcsd())
		return FALSE;
	if(file_format == 0) {
		if(!SD_readblock(0, blkbuf))
			return FALSE;
		if(blkbuf[SD_BLKLEN-1] == 0xAA && blkbuf[SD_BLKLEN-2] == 0x55) {
			xptr = blkbuf+0x1BE;		// point to start of partition table
#ifdef	LOGGING
			writemem((unsigned)cardinfo.part, sizeof cardinfo.part, xptr);
#endif
			i = 4;
			do {
				if(xptr[0] == 0 &&
						(xptr[4] == 1 || xptr[4] == 4 || xptr[4] == 6 || xptr[4] == 14)) {
					part_offset = get_long(xptr+8);
					//card_size = get_long(xptr+12)-1;
					card_size -= part_offset;
					break;
				}
			} while(--i != 0);
		}
	}
	return TRUE;
}

// check the filesystem type - set a flag if it is FAT32
static void
SD_fatchick(void)
{
	unsigned char	i;
	unsigned long	sectors;

	fat32 = 0;
	fat_check = 0;
	if(!SD_readblock(part_offset, blkbuf))
		return;
	if(blkbuf[0x13] != 0 || blkbuf[0x14] != 0)
		return;								// must be <= 32MB, quite safe
	sectors = get_long(blkbuf+0x20);		// get number of sectors
	i = blkbuf[0x0D];						// sectors per cluster
	while((i >>= 1) != 0)
		sectors >>= 1;
	if(sectors >= 65525)
		fat32 = 1;
}

static void
send_csw(void)
{
	writelog();
	pad_bytes = FALSE;
	mse_state = BM_CSW;
	/*
	if(CSW.dCSWResidue) {
		mse_state = BM_STALL;
		USB_halt(2);
	} */
	xptr = (xdata unsigned char *)&CSW;
	data_cnt = sizeof CSW;
}


static void
DESC_rwec(void)
{
	// Read/write error recovery mode page

	*xptr++ = 0x01;
	*xptr++ = 0x06;
	*xptr++ = 0xC8;
	*xptr++ = 0x16;
	xptr += 4;
}
static void
DESC_fdd(void)
{
	data_cnt = card_size/2048;
	*xptr++ = 0x05;		// flexible disk page
	*xptr++ = 0x1E;		// length of page
	*xptr++ = 0x80;		// transfer rate
	*xptr++ = 0x00;
	*xptr++ = 0x40;		// heads
	*xptr++ = 0x20;		// sectors per track
	*xptr++ = 0x02;		// block size
	*xptr++ = 0x00;
	*xptr++ = data_cnt >> 8;
	*xptr++ = data_cnt;		// cylinders - each cylinder is about 1MB
	xptr += 18;				// all zeroes
	*xptr++ = 0x0B;			// rotational speed
	*xptr++ = 0x7D;
	xptr += 2;
}

static void
DESC_caching(void)
{

	*xptr++ = 0x08;
	*xptr++ = 0x0A;
	*xptr++ = 0x04;
	*xptr++ = 0x00;
	*xptr++ = 0xFF;
	*xptr++ = 0xFF;
	*xptr++ = 0x00;
	*xptr++ = 0x00;
	*xptr++ = 0xFF;
	*xptr++ = 0xFF;
	*xptr++ = 0xFF;
	*xptr++ = 0xFF;
}

static void
DESC_2f(void)
{
	*xptr++ = 0x2F;
	*xptr++ = 0x04;
	*xptr++ = 0x5C;
	*xptr++ = 0x01;
	*xptr++ = 0x0F;
	*xptr++ = 0x0F;
}

static void
BUF_clr(unsigned char i)
{
	xptr = blkbuf;
	do
		*xptr++ = 0;
	while(--i);
}

static void
io_done(void)
{
	if(pad_bytes) {
		if(CBW.bmCBWFlags & 0x80)	// data in operation
			USB_halt(2);			// send no more data
		else
			USB_halt(1);
	}
	send_csw();
}

static void
io_ok(void)
{
	sense_key = NOSENSE;
	additional_sense = 0;
	io_done();
}

// common transfer failure routine

static void
xfer_fail(unsigned char sense, unsigned char add)
{
	sense_key = sense;
	additional_sense = add;
	CSW.bCSWStatus = CSW_FAIL;
	io_done();
}

// fail a general command
static void
cmd_fail(unsigned char sense, unsigned char add)
{
	residue(0);
	xfer_fail(sense, add);
}

static void
io_fail(unsigned short blocks, unsigned char sense, unsigned char add)
{
	set_residue((unsigned long)blocks*SD_BLKLEN + get_long((unsigned char xdata *)CSW.dCSWResidue));
	xfer_fail(sense, add);
}

static void
Endpoint_1(void)
{
	unsigned short	len;
	unsigned char	idx;

	len = USB_read_packet(1, (xdata unsigned char *)&CBW, sizeof CBW);
#ifdef	DEBUG
	printf("CBW: ");
	xptr = (unsigned char xdata *)&CBW;
	idx = sizeof CBW;
	do
		printf("%2.2X ", *xptr++);
	while(--idx);
	putch('\n');
#endif

	// check for valid CBW
	if(len != sizeof CBW || CBW.dCBWSignature != CBW_SIG ||
			CBW.bCBWCBLength == 0 || CBW.bCBWCBLength > sizeof CBW.CBWCB) {
		mse_state = BM_STALL;
		return;
	}
	CSW.dCSWTag = CBW.dCBWTag;		// copy the tag ready for response
	CSW.dCSWSignature = CSW_SIG;
	CSW.bCSWStatus = CSW_OK;
	residue(0);
	switch(CBW.CBWCB[0]) {

#ifdef	LOGGING
	case MAINTIN:
		cptr = (const unsigned char *)&cardinfo;
		data_cnt = residue(0x3C00-0x2A00);
		mse_state = BM_CDATA;
		break;
#endif

	case INQUIRY:
	
		if(avidyne || CBW.dCBWDataTransferLength[0] == 0x20) {
			cptr = iomega_data;
			avidyne = TRUE;
		} else
			cptr = inquiry_data;
		data_cnt = residue(sizeof inquiry_data);
		mse_state = BM_CDATA;
		break;

	case MAX_FORMAT:
		cptr = max_format_data;
		data_cnt = residue(sizeof max_format_data);
		mse_state = BM_CDATA;
		break;

	case MAX_CAP:
		if(card_state != CARD_READY) {
			cmd_fail(NOT_READY, MEDIUM_NOT_PRESENT);
			return;
		}
		blkbuf[0] = card_size >> 24;
		blkbuf[1] = card_size >> 16;
		blkbuf[2] = card_size >> 8;
		blkbuf[3] = card_size;
		blkbuf[4] = 0;
		blkbuf[5] = 0;
		blkbuf[6] = SD_BLKLEN >> 8;
		blkbuf[7] = SD_BLKLEN & 0xFF;
		xptr = blkbuf;
		data_cnt = residue(8);
		mse_state = BM_XDATA;
		break;

	case VERIFY:
		residue(0);
		if(CBW.CBWCB[1] & 2 && pad_bytes) {
			xfer_fail(ILLEGAL_REQ, 0x26);
		} else
			send_csw();
		break;

	case STARTSTOP:
		if(CBW.CBWCB[4] & 2) {		// LOEJ bit
			if((CBW.CBWCB[4] & 1)) {	// load request
				switch(card_state) {
				case CARD_EJECTED:
					card_state = CARD_READY;
				case CARD_READY:
					break;

				default:
					cmd_fail(NOT_READY, MEDIUM_NOT_PRESENT);
					return;
				}
			} else {
				switch(card_state) {
				case CARD_EJECTED:
				case CARD_MISSING:
					break;

				default:
					card_state = CARD_EJECTED;
					break;
				}
			}
		}
		send_csw();
		break;

	case PAMR:
		send_csw();
		break;

	case MAINTOUT:
		if(CBW.CBWCB[1] == MAINT_FIRM) {
			send_csw();
			SET_TIMEOUT(1000);
			while(!TIMEDOUT())
				continue;
			USB_detach();
			SET_TIMEOUT(1000);
			while(!TIMEDOUT())
				continue;
			RSTSRC |= 0x10;			// software reset
			break;
		}
		cmd_fail(ILLEGAL_REQ, 0);
		break;


	case READ_10:
		current_block = *(unsigned long xdata *)&CBW.CBWCB[2];
		if(avidyne)
			current_block += part_offset;
		block_count = *(unsigned short xdata *)&CBW.CBWCB[7];
		if(!(CBW.bmCBWFlags & 0x80)) {
			block_count = 0;
			CSW.bCSWStatus = CSW_PHASE;
		}
		block_count = block_residue(block_count);
		// finish now if no data to be read or direction mismatch

		if(block_count == 0) {
			io_ok();
			break;
		}
		if(card_state != CARD_READY) {
			io_fail(block_count, NOT_READY, MEDIUM_NOT_PRESENT);
			return;
		}
		data_cnt = SD_BLKLEN;
		if(idx = SD_cmd(18, current_block << 9)) {
			error_code = READ_FAIL;
			io_fail(block_count, MEDIUM_ERR, idx);
		} else
			mse_state = BM_DATAIN;
		break;

	case WRITE_10:
		current_block = *(unsigned long xdata *)&CBW.CBWCB[2];
		if(current_block == 0)
			fat_check = 1;
		if(avidyne)
			current_block += part_offset;
		block_count = *(unsigned short xdata *)&CBW.CBWCB[7];
		if(CBW.bmCBWFlags & 0x80) {
			block_count = 0;
			CSW.bCSWStatus = CSW_PHASE;
		}
		block_count = block_residue(block_count);
		if(block_count == 0) {
			io_ok();
			break;
		}
		if(card_state != CARD_READY) {
			io_fail(block_count, NOT_READY, MEDIUM_NOT_PRESENT);
			return;
		}
		if(write_protect) {
			io_fail(block_count, DATA_PROT, WRITE_PROT);
			return;
		}
		data_cnt = SD_BLKLEN;
		if(SD_cmd(ACMD, 0))
			error_code = ACMD_FAIL;
		else if(SD_cmd(23, block_count))
			error_code = ERASE_FAIL;
		else if(idx = SD_cmd(25, current_block << 9))		// write multiple command
			error_code = WRITE_FAIL;
		else {
			mse_state = BM_DATAOUT;
			break;
		}
#ifdef	LOGGING
		CBW.CBWCB[9] = current_block >> 8;
		CBW.CBWCB[10] = current_block;
		len = SD_readstatus();
		CBW.CBWCB[11] = len >> 8;
		CBW.CBWCB[12] = len;
		CBW.CBWCB[13] = idx;
#endif	LOGGING
		io_fail(block_count, MEDIUM_ERR, idx);
		break;

	case MODE_SELECT:
	case MODE_SELECT_10:
		xptr = blkbuf;
		data_cnt = residue(sizeof blkbuf);
		mse_state = BM_READ;
		break;

	case MODE_SENSE:
	case MODE_SENSE_10:
		xptr = blkbuf;
		mse_state = BM_XDATA;
		BUF_clr(128);
		blkbuf[3] = 8;					// len of block descriptor
		blkbuf[0xA] = SD_BLKLEN >> 8;	// block desc
		xptr = blkbuf + 0xC;

		switch(CBW.CBWCB[2]) {

		case 0x01:
			DESC_rwec();
			break;

		case 0x05:
			DESC_fdd();
			break;

		case 0x08:
			DESC_caching();
			break;

		case 0x2F:
			DESC_2f();
			break;

		case 0x3F:				// get 'em all
			DESC_rwec();
			DESC_fdd();
			DESC_caching();
			DESC_2f();
			break;

		default:
			cmd_fail(ILLEGAL_REQ, 0);
			return;
		}
		data_cnt = xptr-blkbuf;
		blkbuf[0] = data_cnt-1;
		data_cnt = residue(data_cnt);
		if(write_protect)
			blkbuf[2] |= 0x80;
		xptr = blkbuf;
		break;

	case REQUEST_SENSE:
		BUF_clr(25);
		blkbuf[0] = 0x70;
		blkbuf[2] = sense_key;
		blkbuf[7] = 25-7;
		blkbuf[12] = additional_sense;
		sense_key = NOSENSE;
		additional_sense = 0;
		xptr = blkbuf;
		data_cnt = residue(25);
		mse_state = BM_XDATA;
		break;

	case TEST_UNIT_READY:	
		residue(0);
		if(card_state != CARD_READY) {
			sense_key = NOT_READY;
			additional_sense = MEDIUM_NOT_PRESENT;
		}
		if(sense_key != NOSENSE)
			CSW.bCSWStatus = CSW_FAIL;
		io_done();
		break;

		// unrecognized command
	default:
		cmd_fail(ILLEGAL_REQ, 0);
		break;
	}
}

static void
mse_engine()
{
	unsigned char	tok, i;

	if(USB_status[0] & PROT_RESET) {		// if reset by host, reset state
		USB_status[0] &= ~PROT_RESET;
		//USB_flushin(2, FALSE);
		//USB_flushout(1);
		mse_state = BM_IDLE;
		sense_key = 0;
		additional_sense = 0;
		return;
	}
	switch(mse_state) {

	case BM_IDLE:
		if(USB_status[1] & RX_READY)
			Endpoint_1();
		else if(fat_check)
			SD_fatchick();
		break;

	case BM_CDATA:
		if(USB_status[2] & (TX_BUSY|EP_HALT))
			break;
		if(data_cnt) {
			USB_send_byte(2, *cptr++);
			data_cnt--;
		} else {
			USB_flushin(2, FALSE);
			if(pad_bytes) {
				USB_halt(2);
			}
			io_ok();
		}
		break;

	case BM_XDATA:
		if(USB_status[2] & (TX_BUSY|EP_HALT))
			break;
		if(data_cnt) {
			USB_send_byte(2, *xptr++);
			data_cnt--;
		} else {
			USB_flushin(2, FALSE);
			if(pad_bytes) {
				USB_halt(2);
			}
			io_ok();
		}
		break;

	case BM_STALL:
		USB_halt(2);
		USB_halt(1);
		break;

	case BM_CSW:
		if(USB_status[2] & (TX_BUSY|EP_HALT))
			break;
		USB_send_byte(2, *xptr++);
		if(--data_cnt == 0) {
			USB_flushin(2, FALSE);
			mse_state = BM_IDLE;
		}
		break;

	case BM_READ:
		if(data_cnt == 0)
			io_ok();
		else if(USB_status[1] & RX_READY)
			data_cnt -= USB_read_packet(1, xptr, data_cnt);
		break;

	case BM_DATAOUT:
		if(!(USB_status[1] & RX_READY))
			break;
		if(data_cnt == SD_BLKLEN) {
			SPI_send(0xFF);
			SPI_send(0xFC);			// start of write
		}
		i = 64;
		USB_INTOFF();
		USB0ADR = FIFO1 | BUSY | AUTORD;
		do {
			while(USB0ADR & BUSY)
				continue;
			while(TXBMT == 0)
				continue;
			SPI0DAT = USB0DAT;
		} while(--i != 0);
		USB_flushout(1);
		if((data_cnt -= 64) == 0) {
			current_block++;
			SET_TIMEOUT(500);
			do
				tok = SPI_read();
			while(tok == 0xFF && !TIMEDOUT());
			tok &= 0x1F;
			while(!TIMEDOUT())
				if(SPI_read() == 0xFF)
					break;
			if(TIMEDOUT() || (tok != 5 && tok != 0xA)) {
				error_code = WRITE_FAIL;
				io_fail(block_count*SD_BLKLEN, MEDIUM_ERR, 0);
				SD_stopwrite();
				break;
			}
			if(--block_count == 0) {
				io_ok();
				SD_stopwrite();
			} else
				data_cnt = SD_BLKLEN;
		}
		break;

	case BM_DATAIN:
		while((USB_status[2] & (TX_BUSY|EP_HALT)) == 0) {
			if(data_cnt == SD_BLKLEN) {
				if(!SD_waitread()) {
					io_fail(block_count*SD_BLKLEN, MEDIUM_ERR, 0);
					break;
				}
				SPI0DAT = 0xFF;			// start the SPI read
			}
			i = 64;
			USB_INTOFF();
			USB0ADR = FIFO2;
			while(USB0ADR & BUSY)	// wait for FIFO ready
				continue;
			do {
				while(SPI0CFG & SPIBUSY)
					continue;
				USB0DAT = SPI0DAT;
				SPI0DAT = 0xFF;			// preload SPI output buffer
			} while(--i != 0);
			USB_flushin(2, TRUE);			// also restores USB interrupts
			data_cnt -= 64;
			if(data_cnt == 0) {
				SPI_send(0xFF);		// flush unwanted CRC
				current_block++;
				if(--block_count == 0) {
					SD_send(12, 0);			// stop data transmission from card
					io_ok();
					break;
				} else
					data_cnt = SD_BLKLEN;
			}
		}
		break;
	}
}

#ifdef	DEBUG
static void
UART_start(void)
{
	ES0 = 0;
	if(!TI0) {
		ES0 = 1;
		return;
	}
	if(inptr != optr) {
		TI0 = 0;
		SBUF0 = logbuf[optr++];
		ES0 = 1;
	}
}

void
putch(char c)
{
	unsigned char	i;

	if(c == '\n')
		c = '\r';
	for(;;) {
		for(;;) {
			ES0 = 0;
			i = inptr+1;
			if(i == optr) {
				ES0 = 1;
				continue;
			}
			break;
		}
		logbuf[inptr++] = c;
		UART_start();
		if(c == '\r') {
			c = '\n';
			continue;
		}
		break;
	}
}

static void interrupt
UART_isr(void) @ SERIAL
{
	UART_start();
}
#endif

static void
CLK_init(void)
{
	// setup timer 2 as a periodic interrupt
	TMR2RLL = -T2LOAD & 0xFF;
	TMR2RLH = -T2LOAD/256;
	TR2 = 1;				// enable the timer - note first period will be extended.
	ET2 = 1;				// enable timer2 interrupts.
	EA = 1;					// enable global interrupts

#ifdef	DEBUG
	// Timer 1 is baud rate

	TH1 = 152;				// 115200 at 24MHz
	TMOD = 0B00100000;		// mode 2 timer
	CKCON = 0B1000;			// sysclk input to timer 1
	TR1 = 1;				// enable timer

	SCON0 = 0;				// default UART settings - rx not enabled
	TI0 = 1;				// flag tx buffer empty
#endif
}

static void
PORT_init(void)
{
	// init IO ports

	P1SKIP =  0b00101110;	// skip all except MISO and MOSI
	P0MDOUT = 0b01010101;	// bits 6,1 and 0 are push-pull output
	P1MDOUT = 0b00011000;	// bit 4 is SPI data out, bit 3 is slave select
#ifdef	DEBUG
	P0SKIP =  0b11001011;	// skip all except SPI clock and UART
	XBR0 = 	  0x3;			// enable SPI and UART pins
#else
	P0SKIP =  0b11111011;	// skip all except SPI clock
	XBR0 = 	  0x2;			// enable SPI pins
#endif
	XBR1 =	  0x40;			// enable crossbar
	P0 = 	  0b01111111;	// turn on power led, others off
}

extern USB_CONST USB_descriptor_table	zipcard_table;

void
main(void)
{
	PORT_init();
	CLK_init();
	card_state = CARD_MISSING;
	avidyne = FALSE;
#ifdef	LOGGING
	// find the next unused cmd log entry
	while(logoffset != BUFSIZE-1 && cmdlog[logoffset][0] != 0xFF)
		logoffset++;
#endif	// LOGGING
#ifdef	DEBUG
	printf("Starting\n");
#endif

	USB_init(&zipcard_table);
	fifo_cnt = 0;
	for(;;) {
		if(USB_status[0] & USB_SUSPEND) {
			// suspended - turn the LEDs off lest we draw too much current.
			ACTIVE(FALSE);
			GREEN(FALSE);
		}
		if(USB_status[0] & (RX_READY|USB_SUSPEND)) {
			USB_control();
			continue;
		}
		switch(card_state) {

		case CARD_MISSING:
			if(card_present) {		// card inserted
				active = 1;
				if(SD_init()) {
					card_state = CARD_READY;
					error_code = 0;
					fat_check = 1;
					sense_key = UNIT_ATTENTION;
					additional_sense = MEDIA_CHANGE;
				} else
					card_state = CARD_FAIL;
			}
			break;

		case CARD_FAIL:
		case CARD_READY:
		case CARD_EJECTED:
			if(!card_present)
				card_state = CARD_MISSING;
			break;
		}
		mse_engine();
		active = mse_state != BM_IDLE;
	}
}

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
