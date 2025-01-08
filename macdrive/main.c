/*
*/         						 				
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <mach/mach.h>

#include <CoreFoundation/CFNumber.h>

#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>

//#define DEBUG 1
mach_port_t 				masterPort = 0;				// requires <mach/mach.h>
UInt8						inPipeRef = 0;
UInt8						outPipeRef = 0;
IOUSBInterfaceInterface **	intf;
pthread_t					reader, writer;
IOUSBDeviceInterface		**dev;
#if	DEBUG
FILE *						logger;
static void
debuglog(char * cp, ...)
{
	va_list	ap;

	va_start(ap, cp);
	vfprintf(logger, cp, ap);
	va_end(ap);
}
#else
static void
debuglog(char * cp, ...)
{
}
#endif



void *
readFunc(void * p)
{
	UInt32	size;
	char	inBuf[32768];

	debuglog("Readfunc begins\n");
	for(;;) {
		size = sizeof inBuf;
		if((*intf)->ReadPipe(intf, inPipeRef, inBuf, &size) != kIOReturnSuccess) {
			debuglog("ReadPipe returned error\n");
			break;
		}
		debuglog("Read %d from device\n",size);
		if(size != 0)
			if(write(1, inBuf, size) != size)
				break;
	}
	debuglog("Reader ends\n");
	close(1);
	(*dev)->ResetDevice(dev);
	pthread_kill(writer, SIGTERM);
	return NULL;
}

void *
writeFunc(void * p)
{
	UInt32	size, i;
	char	outBuf[32768];

	debuglog("writefunc begins\n");
	for(;;) {
		size = read(0, outBuf, sizeof outBuf);
		debuglog("Read %d from stdin\n",size);
		if(size <= 0)
			break;
		if((*intf)->WritePipe(intf, outPipeRef, outBuf, size) != kIOReturnSuccess)
			break;
		for(i = 0 ; i != 16 ; i++)
			debuglog("%2.2X ", outBuf[i] & 0xFF);
		debuglog("\n");
		for( ; i != 32 ; i++)
			debuglog("%2.2X ", outBuf[i] & 0xFF);
		debuglog("\n");
	}
	debuglog("Writer ends\n");
	(*dev)->ResetDevice(dev);
	pthread_kill(reader, SIGTERM);
	return NULL;
}



void
dealWithInterface(io_service_t usbInterfaceRef)
{
    IOReturn					err;
    IOCFPlugInInterface 		**iodev;		// requires <IOKit/IOCFPlugIn.h>
    SInt32						score;
    UInt8						numPipes;
	int							i;
	UInt8						direction, number, transferType, interval;
	UInt16						maxPacketSize;

    err = IOCreatePlugInInterfaceForService(usbInterfaceRef, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score);
    if(err || !iodev) {
		debuglog("dealWithInterface: unable to create plugin. ret = %08x, iodev = %p\n", err, iodev);
		return;
    }
    err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID), (LPVOID)&intf);
	IODestroyPlugInInterface(iodev);				// done with this
	
    if(err || !intf) {
		debuglog("dealWithInterface: unable to create a device interface. ret = %08x, intf = %p\n", err, intf);
		return;
    }
    err = (*intf)->USBInterfaceOpen(intf);
    if(err) {
		debuglog("dealWithInterface: unable to open interface. ret = %08x\n", err);
		return;
    }
    err = (*intf)->GetNumEndpoints(intf, &numPipes);
    if(err) {
		debuglog("dealWithInterface: unable to get number of endpoints. ret = %08x\n", err);
		(*intf)->USBInterfaceClose(intf);
		(*intf)->Release(intf);
		return;
    }
    
    debuglog("dealWithInterface: found %d pipes\n", numPipes);
	// pipes are one based, since zero is the default control pipe
	// loop until we find one in and one out pipe
	for (i=1; i <= numPipes && (inPipeRef == 0 || outPipeRef == 0) ; i++) {
		err = (*intf)->GetPipeProperties(intf, i, &direction, &number, &transferType, &maxPacketSize, &interval);
		if(err)
			continue;
		if(transferType != kUSBBulk)
			continue;
		if((direction == kUSBIn) && !inPipeRef)
			inPipeRef = i;
		if((direction == kUSBOut) && !outPipeRef)
			outPipeRef = i;
	}
	if(inPipeRef != 0 && outPipeRef != 0) {
		//pthread_create(&reader, NULL, readFunc, NULL);
		pthread_create(&writer, NULL, writeFunc, NULL);
		//pthread_join(reader, NULL);
		pthread_join(writer, NULL);
	}
    err = (*intf)->USBInterfaceClose(intf);
    if(err) {
		debuglog("dealWithInterface: unable to close interface. ret = %08x\n", err);
		return;
    }
    err = (*intf)->Release(intf);
    if(err) {
		debuglog("dealWithInterface: unable to release interface. ret = %08x\n", err);
		return;
    }
}


void
dealWithDevice(io_service_t usbDeviceRef)
{
    IOReturn						err;
    IOCFPlugInInterface				**iodev;		// requires <IOKit/IOCFPlugIn.h>
    SInt32							score;
    UInt8							numConf;
    IOUSBConfigurationDescriptorPtr	confDesc;
    IOUSBFindInterfaceRequest		interfaceRequest;
    io_iterator_t					iterator;
    io_service_t					usbInterfaceRef;
	
	fprintf(stderr, "In Deal with device");
    
    err = IOCreatePlugInInterfaceForService(usbDeviceRef, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score);
    if(err || !iodev) {
		fprintf(stderr, "dealWithDevice: unable to create plugin. ret = %08x, iodev = %p\n", err, iodev);
		return;
    }
    err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID), (LPVOID)&dev);
	IODestroyPlugInInterface(iodev);				// done with this

    if(err || !dev) {
		fprintf(stderr, "dealWithDevice: unable to create a device interface. ret = %08x, dev = %p\n", err, dev);
		return;
    }
    err = (*dev)->USBDeviceOpen(dev);
    if(err) {
		fprintf(stderr, "dealWithDevice: unable to open device. ret = %08x\n", err);
		return;
    }
    err = (*dev)->GetNumberOfConfigurations(dev, &numConf);
    if(err || !numConf) {
		fprintf(stderr, "dealWithDevice: unable to obtain the number of configurations. ret = %08x\n", err);
        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
		return;
    }
    fprintf(stderr, "dealWithDevice: found %d configurations\n", numConf);
    err = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &confDesc);			// get the first config desc (index 0)
    if(err) {
		fprintf(stderr, "dealWithDevice:unable to get config descriptor for index 0\n");
        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
		return;
    }
    err = (*dev)->SetConfiguration(dev, confDesc->bConfigurationValue);
    if(err) {
		fprintf(stderr, "dealWithDevice: unable to set the configuration\n");
        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
		return;
    }
    
    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;		// requested class
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;		// requested subclass
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;		// requested protocol
    interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;		// requested alt setting
    
    err = (*dev)->CreateInterfaceIterator(dev, &interfaceRequest, &iterator);
    if(err) {
		fprintf(stderr, "dealWithDevice: unable to create interface iterator\n");
        (*dev)->USBDeviceClose(dev);
        (*dev)->Release(dev);
		return;
    }
    
    while ((usbInterfaceRef = IOIteratorNext(iterator))) {
		debuglog("found interface: %p\n", (void*)usbInterfaceRef);
		dealWithInterface(usbInterfaceRef);
		IOObjectRelease(usbInterfaceRef);				// no longer need this reference
    }
    
    IOObjectRelease(iterator);
    iterator = 0;

    err = (*dev)->USBDeviceClose(dev);
    if(err) {
		fprintf(stderr, "dealWithDevice: error closing device - %08x\n", err);
		(*dev)->Release(dev);
		return;
    }
    err = (*dev)->Release(dev);
    if(err) {
		fprintf(stderr, "dealWithDevice: error releasing device - %08x\n", err);
		return;
    }
}



int
main(int argc, const char * argv[])
{
    kern_return_t			err;
    CFMutableDictionaryRef 	matchingDictionary = 0;		// requires <IOKit/IOKitLib.h>
    SInt32					idVendor;
    SInt32					idProduct;
    CFNumberRef				numberRef;
    io_iterator_t			iterator = 0;
    io_service_t			usbDeviceRef;
	int						scan = 0, pipeit = 0;
	char *					cp;
   	const char *			serno;
    
#if	DEBUG
	logger = fopen("/dev/tty", "w");
#endif
	debuglog("htusb started\n");
	
	if(argc < 3) {
		fprintf(stderr, "Usage: htusb {enum|open} vid:pid [serial]\n");
		return 1;
	}
	cp = strdup(argv[2]);
	cp = strtok(cp, ":");
	if(cp == NULL) {
		fprintf(stderr, "Missing vendor ID\n");
		return 3;
	}
	idVendor = strtoul(cp, NULL, 0);
	cp = strtok(NULL, ":");
	if(cp == NULL) {
		fprintf(stderr, "Missing product ID\n");
		return 3;
	}
	idProduct = strtoul(cp, NULL, 0);
	if(strcasecmp(argv[1], "enum") == 0) {
		scan = 1;
	} else if(strcasecmp(argv[1], "open") == 0) {
		if(argc < 4){
			debuglog("Here");
			return 4;
			}
		pipeit = 1;
		serno = argv[3];
	} else {
		fprintf(stderr, "unknown command %s\n", argv[1]);
		return 2;
	}
	

    err = IOMasterPort(MACH_PORT_NULL, &masterPort);				
    if(err) {
		
        fprintf(stderr, "HTUSB: could not create master port, err = %08x\n", err);
        return err;
    }
    matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
    if(!matchingDictionary) {
        fprintf(stderr, "HTUSB: could not create matching dictionary\n");
        return -1;
    }
    numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor);
    if(!numberRef) {
        fprintf(stderr, "HTUSB: could not create CFNumberRef for vendor\n");
        return -1;
    }
	
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBVendorID), numberRef);
    CFRelease(numberRef);
    numberRef = 0;
    numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct);
    if(!numberRef) {
        fprintf(stderr, "HTUSB: could not create CFNumberRef for product\n");
        return -1;
    }
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBProductID), numberRef);
    CFRelease(numberRef);
    numberRef = 0;
    
    err = IOServiceGetMatchingServices(masterPort, matchingDictionary, &iterator);
    matchingDictionary = 0;			// this was consumed by the above call
	
	debuglog("**Here %d\n", err);   
	
	
	while((usbDeviceRef = IOIteratorNext(iterator))) {
		CFTypeRef	serialNumber;
		char		sn[256];
		

		serialNumber = IORegistryEntryCreateCFProperty(usbDeviceRef,
				CFSTR("USB Serial Number"), kCFAllocatorDefault, 0);
		debuglog("***Here %d\n", serialNumber);   
		sn[0] = 0;
		CFStringGetCString(serialNumber, sn, sizeof sn,  kCFStringEncodingASCII);
		CFRelease(serialNumber);
		
		
		
		if(scan)
			printf("%s\n", sn);
		if(pipeit && strcmp(sn, serno) == 0) {
			dealWithDevice(usbDeviceRef);
			IOObjectRelease(usbDeviceRef);
			break;
		}
		IOObjectRelease(usbDeviceRef);			// no longer need this reference
	}
    
    IOObjectRelease(iterator);
    iterator = 0;
    mach_port_deallocate(mach_task_self(), masterPort);
	return 0;
}
