#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <initguid.h>           // Required for GUID definition

#include	"scsi.h"

#include	<vector.h>
#include	<alloc.h>


static int	verbose;

void
plain_alert(const char * s)
{
	fprintf(stderr, "%s\n", s);
	exit(1);
}

static GUID *
strtoGuid(const char * s)
{
	char *			cp, buf[3];
	GUID *			gp;
	unsigned int	i;

	cp = strdup(s);
	cp = strtok(cp, "{}-");
	if(!cp)
		return NULL;
	gp = alloc(sizeof *gp);
	gp->Data1 = strtoul(cp, NULL, 16);
	cp = strtok(0, "{}-");
	if(!cp)
		return NULL;
	gp->Data2 = strtoul(cp, NULL, 16);
	cp = strtok(0, "{}-");
	if(!cp)
		return NULL;
	gp->Data3 = strtoul(cp, NULL, 16);
	cp = strtok(0, "{}-");
	if(!cp || strlen(cp) != 4)
		return NULL;
	for(i = 0 ; i != 2 ; i++) {
		buf[0] = *cp++;
		buf[1] = *cp++;
		buf[2] = 0;
		gp->Data4[i] = strtoul(buf, NULL, 16);
	}
	cp = strtok(0, "{}-");
	if(!cp || strlen(cp) != 12)
		return NULL;
	for( ; i != sizeof gp->Data4/sizeof gp->Data4[0] ; i++) {
		buf[0] = *cp++;
		buf[1] = *cp++;
		buf[2] = 0;
		gp->Data4[i] = strtoul(buf, NULL, 16);
	}
	return gp;
}


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

	// Retrieve device list for GUID that has been specified.
	HDEVINFO hDevInfoList = SetupDiGetClassDevs (guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)); 

	if(hDevInfoList != NULL) {
		SP_DEVICE_INTERFACE_DATA deviceInfoData;

		if(verbose)
			fprintf(stderr, "Got non-null hDevInfoList\n");

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
				if(verbose)
					fprintf(stderr, "Enumed interfaces for entry %d\n", MemberIndex);
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
					if(verbose)
						fprintf(stderr, "Got detail for entry %d - %s\n", MemberIndex, functionClassDeviceData->DevicePath);
					vec_add(vec, strdup(functionClassDeviceData->DevicePath));
					dealloc( functionClassDeviceData, predictedLength );
				}

				free( functionClassDeviceData );

			} else {
				if(GetLastError() == ERROR_NO_MORE_ITEMS ) 
					break;
				else if(verbose)
					fprintf(stderr, "Enum failed on %d: error code %d\n", MemberIndex, GetLastError());
			}
		}
		SetupDiDestroyDeviceInfoList (hDevInfoList);
	}

	// SetupDiDestroyDeviceInfoList() destroys a device information set
	// and frees all associated memory.

	return vec;
}

main(int argc, char ** argv)
{
	vector_t	devs;
	char *		cp;
	GUID *		gp;

	if(argc > 1 && strcmp(argv[1], "-v") == 0) {
		verbose++;
		argc--;
		argv++;
	}
	if(argc < 2)
		exit(1);
	gp = strtoGuid(argv[1]);
	if(!gp) {
		fprintf(stderr, "bad GUID format\n");
		exit(2);
	}
	devs = enumDevices(gp);
	if(!devs)
		exit(1);
	VEC_ITERATE(devs, cp, char *) {
		printf("%s\n", cp);
	}
	exit(0);
}

