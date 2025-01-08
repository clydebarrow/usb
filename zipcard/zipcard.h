/*	Bulk-only mass storage defines
 */


#define	VERS_MAJOR	3
#define	VERS_MINOR	1

#define	CBW_SIG			0x55534243	// signature in CBW packets
#define	CSW_SIG			0x55534253	// signature in CSW packets


typedef struct
{
	unsigned long	dCBWSignature;
	unsigned long	dCBWTag;
	unsigned char	dCBWDataTransferLength[4];		// little endian
	unsigned char	bmCBWFlags;						// flag bits
	unsigned char	bCBWLUN;
	unsigned char	bCBWCBLength;					// command length
	unsigned char	CBWCB[16];						// command
}	CBW_t;

typedef struct
{
	unsigned long	dCSWSignature;
	unsigned long	dCSWTag;
	unsigned long	dCSWResidue;					// data residue - little endian
	unsigned char	bCSWStatus;					// status flags
}	CSW_t;

//	CSW status values

#define	CSW_OK		0x00		// all good
#define	CSW_FAIL	0x01		// command failed
#define	CSW_PHASE	0x02		// phase error

//	SCSI commands

#define	INQUIRY		0x12
#define	MAX_FORMAT	0x23		// max formatted capacity enquiry
#define	MAX_CAP		0x25		// max current capacity enquiry
#define	READ_10		0x28
#define	WRITE_10	0x2A
#define	MODE_SENSE	0x1A
#define	MODE_SENSE_10	0x5A
#define	REQUEST_SENSE	0x03
#define	TEST_UNIT_READY	0x00
#define	MODE_SELECT	0x15
#define	MODE_SELECT_10	0x55
#define	VERIFY		0x2F
#define	PAMR		0x1E		// prevent/allow medium removal
#define	STARTSTOP	0x1B		// eject command, basically
#define	MAINTOUT	0xA4		// maintenance out command
#define	MAINTIN		0xA3		// maintenance in command
#define	MAINT_FIRM	0x06		// maintenance out code we use to trigger download of firmware

// 	Sense keys

#define	NOSENSE		0x0			// yes we have no bananas
#define	RECOVERED_ERR	0x01	// error was recovered
#define	NOT_READY	0x02		// we're not ready, please wait
#define	MEDIUM_ERR	0x03		// error in data medium
#define	HW_ERR		0x04		// error in our hardware
#define	ILLEGAL_REQ	0x05		// ask a stupid question....
#define	UNIT_ATTENTION	0x06	// Media has changed...
#define	DATA_PROT	0x07		// data protected

// Additional sense codes

#define	DRV_NOT_RDY	0x04
#define	MEDIUM_NOT_PRESENT	0x3A
#define	WRITE_PROT	0x27
#define	MEDIA_CHANGE	0x28


// USB0ADR bits

#define	BUSY		0x80		// data engine busy
#define	AUTORD		0x40		// automatically read next byte

