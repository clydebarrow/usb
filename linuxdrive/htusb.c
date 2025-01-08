/*
*/         										
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>


#include	<usb.h> 

//#define DEBUG 1

pthread_t					reader, writer;
int				timeout = 5000;			// timeout in ms
usb_dev_handle *	devh;






#if     DEBUG
FILE *                                          logger;
static void
debuglog(char * cp, ...)
{
	va_list ap;

	va_start(ap, cp);
	vfprintf(logger, cp, ap);
	va_end(ap);
	fflush(logger);
}
#else
static void
debuglog(char * cp, ...)
{
}
#endif

void *
readFunc(void * a)
{
	int		size, i;
	char	inBuf[16384];

	debuglog("Readfunc begins\n");
	for(;;) {
			
		size = usb_bulk_read(devh, 0x2, inBuf, sizeof inBuf, 0);		
		if((size == -EAGAIN) || (size == -ETIMEDOUT) || (size == 0) ){
			debuglog("\nReader continues %d", size);			
			continue; 
		}
		if(size < 0) {
			debuglog("usb_bulk_read returned error %d\n", size);
			break;
		}
		if(size != 0) {
			debuglog("Read %d from device\n",size);
			for(i = 0 ; i != size && i != 16 ; i++)
				debuglog("%2.2X ", inBuf[i] & 0xFF);
			debuglog("\n");
			for( ; i < size && i != 32 ; i++)
				debuglog("%2.2X ", inBuf[i] & 0xFF);
			debuglog("\n");
			if(write(1, inBuf, size) != size)
				break;
		}
		debuglog("\nReader Resumes");
		
	}
	debuglog("Reader ends\n");
	close(1);
	usb_reset(devh);
	pthread_kill(writer, SIGTERM);	
	return NULL;
}

void *
writeFunc(void * a)
{
	int				res;
	unsigned int	size, i;
	char			outBuf[16384];

	debuglog("writefunc begins\n");
	for(;;) {
		
		size = read(0, outBuf, sizeof outBuf);
		debuglog("Writer : Read %d from stdin\n",size);
		if(size <= 0)
			break;
		for(i = 0 ; i != 16 ; i++)
			debuglog("%2.2X ", outBuf[i] & 0xFF);
		debuglog("\n");
		for( ; i != 32 ; i++)
			debuglog("%2.2X ", outBuf[i] & 0xFF);
		debuglog("\n");
		res = usb_bulk_write(devh, 1, outBuf, size, timeout);		
		if(res  != size) {
			debuglog("writer returns %d\n", res);
			break;
		}
		
		
	}
	debuglog("Writer ends\n");
	usb_reset(devh);	
	pthread_kill(reader, SIGTERM);
	return NULL;
}

void
readwrite(void)
{
	pthread_attr_t attr;
	
	
	

	
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&reader, &attr, readFunc, NULL);
	pthread_create(&writer, &attr, writeFunc, NULL);
	
	pthread_attr_destroy(&attr);
	
	
	pthread_join(reader, NULL);
	pthread_join(writer, NULL);
	
}



int
main(int argc, const char * argv[])
{
	int					idVendor;
	int					idProduct;
	int						scan = 0, pipeit = 0;
	char *					cp;
	const char *			serno;
	struct usb_bus *busses; 
	struct usb_bus *bus;   

#if	DEBUG
	//logger = stderr;
	//logger = fopen("/dev/tty", "w");
	logger = fopen("htusberrors.log", "w");
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
		if(argc < 4)
			return 4;
		pipeit = 1;
		serno = argv[3];
	} else {
		fprintf(stderr, "unknown command %s\n", argv[1]);
		return 2;
	}

	usb_init(); 
	usb_find_busses(); 
	usb_find_devices(); 
	busses = usb_get_busses(); 

	for (bus = busses; bus; bus = bus->next) { 
		struct usb_device *dev; 


		char		serno[256];
		

		for (dev = bus->devices; dev; dev = dev->next) { 
			debuglog("VID:PID = %X:%X\n", dev->descriptor.idVendor, dev->descriptor.idProduct);
			if(dev->descriptor.idVendor == idVendor &&
					dev->descriptor.idProduct == idProduct) {
				devh = usb_open(dev);

				if(devh != NULL) {
					if(usb_set_configuration(devh, 1) != 0){
						debuglog("Config set %s\n", usb_strerror());
					}
					if(usb_claim_interface(devh, 0) != 0){
						debuglog("Claim  %s\n", usb_strerror());
					}

					
					if(usb_get_string_simple(devh, dev->descriptor.iSerialNumber, serno, sizeof serno) >= 0)
						if(scan){
							debuglog("%s\n", serno);							
							printf("%s\n", serno);
						}
						else {

							readwrite();
							break;
						}
					usb_release_interface(devh, 0);
					usb_close(devh);
				}
			}
		} 
	}
	
	return 0;
}
