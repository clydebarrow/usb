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
#define	JT_INIT	        0x02
#define	JT_READMEM	0x03			// read memory
#define	JT_WRITEMEM	0x04			// write memory
#define	JT_READREG	0x05
#define	JT_WRITEREG	0x06
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
#define	JT_RUN		0x12			// Run the ARM Core
#define	JT_STOP		0x13			// Stop the ARM Core
#define	JT_DEBUG	0x14			// For debugging purpose
#define	JT_ISSTOP	0x15			// 
#define JT_EXECINSTR	0x18
#define JT_STARTFROM	0x19
#define JT_READPC	0x1A
#define JT_FDATA	0x1B
#define JT_ISRUNNING	0x1C
#define JT_VERSION	0x1D
#define JT_CHIPERASE	0x1E

#define	JT_OK		0x00			// 0 means ok
#define	JT_FAIL		0x01			// 1 means failed
// sense codes

#define	JT_BADREQ	0x01			// unknown request
#define	JT_NOCHIP	0x02			// couldn't connect
#define	JT_READFAIL	0x03			// read failure
#define	JT_WRITEFAIL	0x04			// write failure
#define JT_TIMEOUT	0x05			// Time out
#define JT_TAPERROR     0x06			//Error in the TAP state machine
#define	JT_STOPERR	0x07			// read failure
#define	JT_PROGFAIL	0x08			// read failure

// device codes - passed as the len argument in a JT_CONNECT command.

#define	JT_SILABSC2	0x01			// Silicon Labs/Cygnal 8051 series with C2 interface
#define	JT_SILABSJT	0x02			// Silicon Labs/Cygnal 8051 with JTAG interface
#define	JT_ARM7		0x03			// ARM7 Generic device - JTAG interface

// memory space codes - mapping between these and particular architectures can
// vary

#define	JT_FLASH	0x00			// program memory
#define	JT_RAM		0x01			// data memory (internal usually)
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
	reentrant unsigned char (*	writemem)(unsigned char xdata * buf, unsigned long len, 
					unsigned long addr, unsigned char mem);
											// write memory
	reentrant unsigned char (*	readmem)(unsigned char xdata * buf, unsigned long len, unsigned long addr);
	reentrant unsigned long (*	readreg)(unsigned long reg);
	reentrant unsigned long (*	execinstr)(unsigned long );
	reentrant unsigned long (*	fastdata)(unsigned long , unsigned char);
	reentrant BOOL 		(*	writereg)(unsigned long reg, unsigned long value);

	reentrant unsigned char	(*	stop)(void);


	
	reentrant unsigned char (*go)(void);
	reentrant unsigned char (*step)(void);
	reentrant unsigned char (*startfrom)(unsigned long);
	reentrant BOOL 		(*isrunning)(void);
	reentrant void 		(*chiperase)(void);

}	dev_int;

typedef struct
{
	unsigned long address;
	int num;
	unsigned char ishard_ware;
}break_points_st;

#define	SET_TIMEOUT(x)	(timeout = (((x)*128UL))/1000+1)
#define	TIMEDOUT()		(timeout == 0)
// externs for supported devices

extern unsigned char	timeout;		// timeout value in ticks
extern const dev_int	mips_interface;				// mips jtag interface
extern unsigned char connected;				// true when device connected
extern unsigned char error_code;
extern void mips_save_dbgsts(void);
extern void mips_restore_dbgsts(void);
