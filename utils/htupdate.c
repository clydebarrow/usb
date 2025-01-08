#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <initguid.h>           // Required for GUID definition

#include	"scsi.h"

#include	<vector.h>
#include	<alloc.h>

void
plain_alert(char * s)
{
	fprintf(stderr, "%s\n", s);
	exit(1);
}

// {37ABC9BB-51FE-47bd-BACD-A974BC4111E8}
static const GUID GUID_INTERFACE_HTJTAG  =
{	0x37abc9bb, 0x51fe, 0x47bd, { 0xba, 0xcd, 0xa9, 0x74, 0xbc, 0x41, 0x11, 0xe8 }};

static const GUID GUID_DEVINTERFACE_DISK =
{	0x53f56307L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}};


void
cleanup(int i)
{
	exit(i);
}

// get a list of devices matching the supplied GUID
vector_t
enumDevices(const GUID * guid)
{
	BOOL bRet = FALSE;
	int	MemberIndex;
	vector_t	vec = vec_new();
	SP_DEVINFO_DATA	dvd;


	// Retrieve device list for GUID that has been specified.
	HDEVINFO hDevInfoList = SetupDiGetClassDevs (guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)); 

	if(hDevInfoList != NULL) {
		SP_DEVICE_INTERFACE_DATA deviceInfoData;

		for (MemberIndex = 0; MemberIndex != 127; MemberIndex++) {
			// Clear data structure
			ZeroMemory(&deviceInfoData, sizeof(deviceInfoData));
			deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

			// retrieves a context structure for a device interface of a device information set.
			if(SetupDiEnumDeviceInterfaces (hDevInfoList, 0, guid, MemberIndex, &deviceInfoData)) {
				// Must get the detailed information in two steps
				// First get the length of the detailed information and allocate the buffer
				// retrieves detailed information about a specified device interface.
				PSP_DEVICE_INTERFACE_DETAIL_DATA     functionClassDeviceData = NULL;
				ULONG  predictedLength, requiredLength;
				predictedLength = requiredLength = 0;
				SetupDiGetDeviceInterfaceDetail (
						hDevInfoList,
						&deviceInfoData,
						NULL,			// Not yet allocated
						0,				// Set output buffer length to zero 
						&requiredLength,// Find out memory requirement
						NULL);			

				predictedLength = requiredLength;
				functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)alloc(predictedLength);
				functionClassDeviceData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

				// Second, get the detailed information
				if(SetupDiGetDeviceInterfaceDetail(hDevInfoList,
							&deviceInfoData,
							functionClassDeviceData,
							predictedLength,
							&requiredLength,
							NULL)) {
					// add device name to list
					vec_add(vec, strdup(functionClassDeviceData->DevicePath));
				}
				dealloc( functionClassDeviceData, predictedLength );

			} else if(GetLastError() == ERROR_NO_MORE_ITEMS ) 
				break;
		}
		SetupDiDestroyDeviceInfoList (hDevInfoList);
	}

	// SetupDiDestroyDeviceInfoList() destroys a device information set
	// and frees all associated memory.

	return vec;
}

/*
 * 	Boot loader protocol.
 */

typedef struct
{
	unsigned long	sig;			// Boot load signature - HTBL
	unsigned long	tag;			// tag for response packet
	unsigned long	addr;			// address to download data
	unsigned char	cmd;			// command
	unsigned char	len;			// length of data
	unsigned char   pad[2];         // dummies to pad it out
}	BCMD;

/*
#define	BL_SIG		0x4854424c		// big endian
#define	BL_RES		0x48544252		// response sig
*/

#define	BL_SIG		0x4c425448		// little endian
#define	BL_RES		0x52425448		// little endian

typedef struct
{
	unsigned long	sig;			// response signature - HTBR
	unsigned long	tag;			// matching tag
	unsigned long	value;			// value if required
	unsigned char	status;			// status - 0 ok, 1 not ok
	unsigned char	sense;			// more info about what is wrong.
	unsigned char   pad[2];         // dummies to pad it out

}	BRES;

#define	BL_IDENT	0x01			// ident command - response is product number
#define	BL_DNLD		0x02			// download data
#define	BL_DONE		0x03			// download complete
#define	BL_QBASE	0x04			// query base for download

// sense codes

#define	BL_BADREQ	0x01			// unknown request
#define	BL_BADADDR	0x02			// bad address
#define	BL_PROGFAIL	0x03			// programming failure

typedef struct
{
	unsigned long	tag;
	unsigned long	product;
	unsigned long	version;
	unsigned long	addr;
	unsigned long	size;
}	firmware;

#define	FW_TAG		0x554322BF

static BCMD	cmd;
static BRES	res;

static HANDLE	hFile, inpipe;
static void
message(char * s, ...)
{
	char	buf[1024];
	va_list	ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof buf, s, ap);
	MessageBox(0, buf, "Firmware update", MB_OK);
}

static void
error(char * s, ...)
{
	char	buf[1024];
	va_list	ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof buf, s, ap);
	MessageBox(0, buf, "Firmware update", MB_OK);
	exit(1);
}

static BOOL
send_cmd(void)
{
	DWORD	count;

	if(!WriteFile(hFile, (LPCVOID)&cmd, sizeof cmd, &count, NULL)) {
		error("Writefile failed - %d\n", GetLastError());
		return FALSE;
	}
}
static BOOL
get_resp(void)
{
	DWORD	count;
	if(!ReadFile(inpipe, (LPVOID)&res, sizeof res, &count, NULL)) {
		error("ReadFile failed - %d\n", GetLastError());
		return FALSE;
	}
	if(res.sig != BL_RES || res.tag != cmd.tag) {
		error("sig or tag did not match\n");
		return FALSE;
	}
	cmd.tag--;
}

main(int argc, char ** argv)
{
	int	i, j, k;
	BOOL	x;
	vector_t	devs;
	time_t	started, now;
	FILE *	fp;
	DWORD	count, start, finish, xcode;
	BY_HANDLE_FILE_INFORMATION	info;
	unsigned char	buf[512];
	firmware		fw;
	char	outname[256], inname[256];
	char *	fname, * cp, * devtype;

	cp = strdup( _pgmptr);
	if(strrchr(cp, '\\')) {
		*strrchr(cp, '\\') = 0;
		SetCurrentDirectory(cp);
	}
	snprintf(outname, sizeof outname, "%s\\htusb.inf", cp);
	SetupCopyOEMInf(outname, NULL, SPOST_PATH, 0, 0, 0, 0, 0);
	if(argc != 2)
		fname = "jtag.fmw";
	else
		fname = argv[1];
	if(!(fp = fopen(fname, "rb"))) {
		error("Could not open firmware file '%s': %s", fname, strerror(errno));
	}
	if(fread(&fw, sizeof fw, 1, fp) != 1 || fw.tag != FW_TAG) {
		error("%s does not appear to be a firmware file\n");
		exit(1);
	}
	snprintf(buf, sizeof buf, "&prod_%s", fname);
	cp = strrchr(buf, '.');
	if(strcasecmp(cp, ".fmw") == 0)
		*cp = 0;

	devtype = strdup(buf);
	//message("Ensure the HI-TECH JTAG interface is plugged into your computer");
	printf("looking for %s\n", devtype);
	devs = enumDevices(&GUID_DEVINTERFACE_DISK);
	VEC_ITERATE(devs, cp, char *) {
		printf("device %s\n", cp);
		if(strstr(cp, devtype)) {
			SCSI_PASS_THROUGH	sptwb;
			DWORD				returned;

			hFile = CreateFile(cp,
					GENERIC_WRITE | GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);
			if(!hFile)
				error("Createfile failed: %d", GetLastError());

			ZeroMemory(&sptwb,sizeof(SCSI_PASS_THROUGH));

			sptwb.Length = sizeof(SCSI_PASS_THROUGH);
			sptwb.PathId = 0;
			sptwb.TargetId = 1;
			sptwb.Lun = 0;
			sptwb.CdbLength = CDB6GENERIC_LENGTH;
			sptwb.SenseInfoLength = 0;
			sptwb.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
			sptwb.DataTransferLength = 0;
			sptwb.TimeOutValue = 5;
			sptwb.Cdb[0] = 0xA4;	// MAINTOUT
			sptwb.Cdb[1] = 0x06;	// MAINT_FIRM
			sptwb.Cdb[4] = 0;

			DeviceIoControl(hFile,
					IOCTL_SCSI_PASS_THROUGH,
					&sptwb,
					sizeof(SCSI_PASS_THROUGH),
					NULL,
					0,
					&returned,
					NULL);
			break;
		}
	}
	//message("Wait for device to reconnect - if 'New Hardware Wizard' appears,\nselect 'don't search', then 'install automatically'.\nClick 'Continue Anyway'\nif warned about an unsigned device driver");
	time(&started);
	do {
		devs = enumDevices(&GUID_INTERFACE_HTJTAG);
		VEC_ITERATE(devs, cp, char *) {
			if(strstr(cp, "vid_1725&pid_ffff")) {
				strcpy(outname, cp);
				strcpy(inname, cp);
				strcat(outname, "\\0#5000");
				strcat(inname, "\\1#5000");
				hFile = CreateFile(outname,
						GENERIC_WRITE | GENERIC_READ,
						FILE_SHARE_WRITE | FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

				if(hFile == INVALID_HANDLE_VALUE) {
					error("Createfile of %s failed\n", outname);
					exit(1);
				}
				inpipe = CreateFile(inname,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

				if(inpipe == INVALID_HANDLE_VALUE) {
					error("Createfile of %s failed\n", inname);
					exit(1);
				}
				cmd.sig = BL_SIG;
				cmd.tag = 0xFFFFFFFF;
				cmd.cmd = BL_IDENT;
				cmd.len = 0;
				cmd.addr = 0;
				send_cmd();
				get_resp();
				if(res.status != 0 || res.value != fw.product) {
					error("This firmware is not suitable for this product\n");
					exit(1);
				}
				cmd.cmd = BL_QBASE;
				send_cmd();
				get_resp();
				if(res.status != 0 || res.value != fw.addr) {
					error("This firmware is not suitable for this product\n");
					exit(1);
				}
				//message("Upgrading device to firmware V%X\n", fw.version);
				start = GetTickCount();
				cmd.cmd = BL_DNLD;
				cmd.addr = fw.addr;
				for(i = 0 ; i != fw.size ; ) {
					if(fw.size-i < 64)
						cmd.len = fw.size-i;
					else
						cmd.len = 64;
					send_cmd();
					if(fread(buf, cmd.len, 1, fp) != 1) {
						error("Firmware file appears to be corrupt - device may be in an inconsistent state\n");
						exit(1);
					}
					if(!WriteFile(hFile, (LPCVOID)buf, cmd.len, &count, NULL)) {
						error("Writefile failed - %d\n", GetLastError());
						exit(1);
					}
					get_resp();
					if(res.status != 0) {
						error("Firmware upgrade failed - error code %d\n", res.sense);
						exit(1);
					}
					i += cmd.len;
					cmd.addr += cmd.len;
				}
				cmd.cmd = BL_DONE;
				send_cmd();
				get_resp();
				finish = GetTickCount();
				message("Success!! Firmware upgraded to V%lX.%2.2lX in %ldms\n", fw.version/0x100, fw.version & 0xFF, finish-start);
				exit(0);
			}
		}
		sleep(1);
		time(&now);
	} while(now-started < 60);
	error("No suitable devices for firmware upgrade found.");
	exit(1);
}
