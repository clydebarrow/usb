/*
 * 	JTAG debug protocol
 */

#define	VERS_MAJOR	1
#define	VERS_MINOR	01

#define	BOOL	bit
#define	TRUE	1
#define	FALSE	0

#define	POWER_LED		(P0_BITS.B6)
#define	ACTIVITY_LED	(P0_BITS.B0)
#define	ACTIVE(x)		ACTIVITY_LED = !(x)
#define	GREEN(x)		POWER_LED = !(x)

// USB endpoints

#define	USB_TX	2		// endpoint 2 used for transmit data
#define	USB_RX	1		// 1 for receive data

typedef struct
{
	unsigned long	sig;			// Boot load signature - HTJT
	unsigned long	tag;			// tag for response packet
	unsigned long	addr;			// address to download data
	unsigned short	len;			// length of data
	unsigned char	cmd;			// command
	unsigned char	mem;			// Memory qualifier and data direction flag
}	JCMD_t;

#define	JT_SIG		0x48544a54		// big endian
#define	JT_RES		0x48544a52		// response sig

typedef struct
{
	unsigned long	sig;			// response signature - HTJR
	unsigned long	tag;			// matching tag
	unsigned long	value;			// value result
	unsigned short	resid;			// residual length not transferred
	unsigned char	status;			// status - 0 ok, 1 not ok
	unsigned char	sense;			// more info about what is wrong.
}	JRES_t;

#define	JT_IDENT	0x01			// ident command - response is product number
//#define	unused	0x02
#define	JT_READMEM	0x03			// read memory
#define	JT_WRITEMEM	0x04			// write memory
//#define	Unused	0x05
//#define	Unused	0x06
#define	JT_SETBRK	0x07			// set code breakpoint
#define	JT_CLRBRK	0x08			// clear breakpoint
#define	JT_STEP		0x09			// step one instruction
#define	JT_CONNECT	0x0A			// connect to target - response is chip ID
#define	JT_FIRMWARE	0x0B			// go into download mode
#define	JT_PINWRITE	0x0C			// write I/O pins and set modes
#define	JT_PINREAD	0x0D			// read I/O pins
#define	JT_LIST		0x0E			// list supported devices
#define	JT_DEVERASE	0x0F			// erase entire device memory
#define	JT_DISCONN	0x10			// Disconnect target device

#define	JT_OK		0x00			// 0 means ok
#define	JT_FAIL		0x01			// 1 means failed
// sense codes

#define	JT_BADREQ	0x01			// unknown request
#define	JT_BADADDR	0x02			// bad address
#define	JT_PROGFAIL	0x03			// programming failure
#define	JT_NOCHIP	0x04			// couldn't connect
#define	JT_READFAIL	0x05			// read failure

// device codes - passed as the len argument in a JT_CONNECT command.

#define	JT_SILABSC2	0x01			// Silicon Labs/Cygnal 8051 series with C2 interface
#define	JT_SILABSJT	0x02			// Silicon Labs/Cygnal 8051 with JTAG interface
#define	JT_PSOC		0x05			// Cypress PSoC ISSP 

// memory space codes - mapping between these and particular architectures can
// vary

#define	JT_PROGMEM	0x00			// program memory
#define	JT_DATAMEM	0x01			// data memory (internal usually)
#define	JT_EXTMEM	0x02			// external data memory
#define	JT_SFR		0x03			// special function regs
#define	JT_REG		0x04			// device registers
#define	JT_DEBREGS	0x05			// debug registers

extern unsigned long	dev_id;	// device it set by connect routine

// interface structure for device-specific routines

typedef struct
{
	unsigned int	dev;					// device code - see above
	reentrant BOOL (*	connect)(void);		// attempt connect. Sets device ID.
	reentrant BOOL (*	fullerase)(void);	// Erase the entire device memory
	reentrant BOOL (*	disconnect)(void);	// Disconnect the target device
	reentrant BOOL (*	writemem)(unsigned char xdata * buf, unsigned char len, unsigned long addr, unsigned char mem);
											// write memory
	reentrant BOOL (*	readmem)(unsigned char xdata * buf, unsigned char len, unsigned long addr, unsigned char mem);
											// read memory
}	dev_int;

#define	SET_TIMEOUT(x)	(timeout = (((x)*128UL))/1000+1)
#define	TIMEDOUT()		(timeout == 0)
// externs for supported devices

extern unsigned char	timeout;		// timeout value in ticks
extern const dev_int	si_c2;				// si labs C2 interface
extern const dev_int	si_psoc;			// psoc interface
extern bit	connected;				// true when device connected
extern unsigned char error_code;
