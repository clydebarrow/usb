#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <initguid.h>           // Required for GUID definition

#include	"scsi.h"

#include	<vector.h>
#include	<alloc.h>

static void
message(char * s, ...)
{
	char	buf[1024];
	va_list	ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof buf, s, ap);
	MessageBox(0, buf, "ZipCard log extractor", MB_OK);
}

static void
error(char * s, ...)
{
	char	buf[1024];
	va_list	ap;

	va_start(ap, s);
	vsnprintf(buf, sizeof buf, s, ap);
	MessageBox(0, buf, "ZipCard log extractor", MB_OK);
	exit(1);
}

void
plain_alert(char * s)
{
	error(s);
	exit(1);
}

// {0180F8EA-07BD-4a91-9526-7AE198E75DCD}
static const GUID GUID_INTERFACE_HTSOFT  =
{ 0x180f8ea, 0x7bd, 0x4a91, { 0x95, 0x26, 0x7a, 0xe1, 0x98, 0xe7, 0x5d, 0xcd } };


static const GUID GUID_DEVINTERFACE_DISK =
{	0x53f56307L, 0xb6bf, 0x11d0, { 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b } };

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

		for (MemberIndex = 0; MemberIndex != 127; MemberIndex++)
		{
			// Clear data structure
			ZeroMemory(&deviceInfoData, sizeof(deviceInfoData));
			deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

			// retrieves a context structure for a device interface of a device information set.
			if(SetupDiEnumDeviceInterfaces (hDevInfoList, 0, guid, MemberIndex, &deviceInfoData)) 
			{
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
					dealloc( functionClassDeviceData, predictedLength );
				}
				free( functionClassDeviceData );

			} else if(GetLastError() == ERROR_NO_MORE_ITEMS ) 
				break;
		}
		SetupDiDestroyDeviceInfoList (hDevInfoList);
	}

	// SetupDiDestroyDeviceInfoList() destroys a device information set
	// and frees all associated memory.

	return vec;
}


#define	LOGSIZE	8192		// size of log buffer - device will not actually return this
							// much

main(int argc, char ** argv)
{
	int	i, j, k;
	BOOL	x;
	vector_t	devs;
	FILE *	fp;
	DWORD	count, start, finish;
	BY_HANDLE_FILE_INFORMATION	info;
	unsigned char	buf[512];
	char	outname[256], inname[256];
	char *	fname, * cp;
	unsigned char	* bp;

	cp = strdup( _pgmptr);
	if(strrchr(cp, '\\')) {
		*strrchr(cp, '\\') = 0;
		SetCurrentDirectory(cp);
	}
	strcpy(outname, cp);
	strcat(outname, "\\zipcard.logdata");
	message("Ensure ZipCard is connected and ready");
	devs = enumDevices(&GUID_DEVINTERFACE_DISK);
	VEC_ITERATE(devs, cp, char *) {
		if(strstr(cp, "usbstor#disk&ven_hi-tech&prod_zipcard")) {
			SCSI_PASS_THROUGH	* sptwb;
			DWORD				returned;
			HANDLE				hFile;

			sptwb = alloc(sizeof *sptwb + LOGSIZE*2);
			bp = (unsigned char *)(sptwb+1);
			message("About to clobber the filesystem on %s", cp);
			hFile = CreateFile(cp,
					GENERIC_WRITE | GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);
			if(!hFile)
				error("Createfile failed: %d", GetLastError());

			sptwb->Length = (sizeof *sptwb);
			sptwb->PathId = 0;
			sptwb->TargetId = 1;
			sptwb->Lun = 0;
			sptwb->CdbLength = 10;
			sptwb->SenseInfoLength = 0;
			sptwb->DataIn = SCSI_IOCTL_DATA_OUT;
			sptwb->DataTransferLength = LOGSIZE;
			sptwb->DataBufferOffset = sizeof *sptwb;
			sptwb->TimeOutValue = 5;
			sptwb->Cdb[0] = 0x2A;	// Write_10
			sptwb->Cdb[2] = 0;	// block number
			sptwb->Cdb[3] = 0;	// block number
			sptwb->Cdb[4] = 0;	// block number
			sptwb->Cdb[5] = 0;	// block number
			sptwb->Cdb[7] = 0;	// block count high
			sptwb->Cdb[8] = LOGSIZE/512;	//block count

			if(!DeviceIoControl(hFile,
						IOCTL_SCSI_PASS_THROUGH,
						sptwb,
						sizeof(SCSI_PASS_THROUGH)+LOGSIZE,
						sptwb,
						sizeof *sptwb+LOGSIZE,
						&returned,
						NULL))  {
				printf("buffer = %X %X %X\n", bp[0], bp[1], bp[2]);
				error("IOCTL_SCSI_PASS_THROUGH failed: %d", GetLastError());
			}
			message("Succeeded - remove card, reinsert and reformat now");
			exit(0);
		}
	}
	error("No devices found to read log file from");
	exit(1);
}

