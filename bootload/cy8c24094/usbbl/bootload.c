#ifndef USB_CONST
#define	USB_CONST	const
#endif

#define xdata
#include	<htusb.h>

/* This structure is defined in htusb.h, here for reference only

typedef struct
{
	USB_CONST unsigned char *				device,
							* USB_CONST *	configurations,
							* USB_CONST *	strings;
	USB_CONST unsigned short*				conf_sizes;
	unsigned char							conf_cnt, string_cnt;
}	USB_descriptor_table;
*/

// USB Device "bootload"

static USB_CONST unsigned char bootload_data[] = {
	18, 1,			// 18 byte device descriptor 
	0x01, 0x01,		// USB version
	0x00,0x00,0x00,		// class,subclass,protocol
	8,			// max packet size
	0x25, 0x17,		// Vendor ID: 0x1725/5925
	0xFF, 0xFF,		// Product ID: 0xFFFF/65535
	0x01, 0x12,		// Product release: 0x1201/4609
	1,			// manufacturer: "HI-TECH Software"
	2,			// product: "USB Firmware upgrade"
	5,			// Custom serial
	1,			// number of configurations

	// Configuration 1

		9, 2,		// 9 byte configuration descriptor 
		0x20, 0x00,	// total size of conf/interface/endpoint descriptors = 32
		1,		// number of interfaces in this configuration
		1,		// configuration identifier
		3,		// configuration name: "Default"
		0x80,		// attributes: Wakeup=no; bus powered
		0x32,		// power=100mA

		// Interface 0
			9, 4,		// 9 byte interface descriptor 
			0,		// interface identifier
			0,		// alternate identifier
			2,		// number of endpoints
			0xFF,0xFF,0x00,	// class,subclass,protocol
			4,		// interface name: "Bootload"
			// Endpoint 1
				7, 5,		// 7 byte endpoint descriptor 
				0x01,		// Endpoint number (out)
				0x02,		// bulk
				0x40, 0x00,	// Max packet size 64
				0,		// Latency/polling interval
			// Endpoint 2
				7, 5,		// 7 byte endpoint descriptor 
				0x82,		// Endpoint number (in)
				0x02,		// bulk
				0x40, 0x00,	// Max packet size 64
				0,		// Latency/polling interval
	// string descriptor 0: language ids
	4, 3, 9, 4,
	// string descriptor 1: "HI-TECH Software"
	34, 3, 72, 0, 73, 0, 45, 0, 84, 0, 69, 0, 67, 0, 72, 0, 32, 0, 83, 0, 111, 0, 102, 0, 116, 0, 119, 0, 97, 0, 114, 0, 101, 0, 
	// string descriptor 2: "USB Firmware upgrade"
	42, 3, 85, 0, 83, 0, 66, 0, 32, 0, 70, 0, 105, 0, 114, 0, 109, 0, 119, 0, 97, 0, 114, 0, 101, 0, 32, 0, 117, 0, 112, 0, 103, 0, 114, 0, 97, 0, 100, 0, 101, 0, 
	// string descriptor 3: "Default"
	16, 3, 68, 0, 101, 0, 102, 0, 97, 0, 117, 0, 108, 0, 116, 0, 
	// string descriptor 4: "Bootload"
	18, 3, 66, 0, 111, 0, 111, 0, 116, 0, 108, 0, 111, 0, 97, 0, 100, 0, 

};
// sizeof table above should be 164

// Access tables for this device

static USB_CONST unsigned char * USB_CONST	bootload_configurations[] =
{
	bootload_data+18,
};

static USB_CONST unsigned short bootload_conf_sizes[] =
{
	32,
};

static USB_CONST unsigned char * USB_CONST	bootload_strings[] =
{
	bootload_data+50,
	bootload_data+54,
	bootload_data+88,
	bootload_data+130,
	bootload_data+146,
	0,
};
USB_CONST USB_descriptor_table	bootload_table =
{
	bootload_data,
	bootload_configurations,
	bootload_strings,
	bootload_conf_sizes,	1, 6,
};
