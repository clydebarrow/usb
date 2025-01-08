/*
 * 	Read a descriptive text file and produce from this
 * 	a data structure representing a USB descriptor set.
 */

/*
 * 	Syntax of the descriptive file:
 *
 * 	All lines are of the form KEYWORD=value
 * 	Braces and the bar are meta-symbols, e.g.
 *
 * 	USB={1.1|2.0} means that the lines
 * 	USB=1.1
 * 	and
 * 	USB=2.0
 * 	are permitted.
 *
 * 	Angle brackets are used to represent required values, e.g.
 * 	<max> should be replaced by a number.
 *
 * 	Numbers may be in hex (0xnn) or decimal format.
 * 	Lines beginning with # or ; are ignored.
 *
 * 	The class/subclass/protocol code will be placed in the interface descriptor,
 * 	not the device descriptor.
 *
 *

USB={1.1|2.0}
CLASS=<class>,<subclass>,<protocol>
PACKETSIZE=<max>
VID=<vendor id>
PID=<product id>
RELEASE=<release no>
MANUFACTURER=<manufacturer name>
PRODUCT=<product name>
SERIAL=<serial number>

; Configuration info

CONFIG=<configuration name>
POWER={bus|self},<milliamps>
WAKEUP={yes|no}

; Interface info

INTERFACE=<interface name>
CLASS=<class>,<subclass>,<protocol>

;Endpoints

ENDPOINT=<number>
DIR={in|out}
TYPE={bulk|interrupt|iso|control}
PACKETSIZE=<max>
INTERVAL=<milliseconds>

; Device Firmware Upgrade function
DFU_FUNCTION
DETACH={yes|no}
TOLERANT={yes|no}
UPLOAD={yes|no}
DOWNLOAD={yes|no}
TIMEOUT=<number>
TRANSFER=<number>
VERSION=1.1
ENDFUNCTION



ENDENDPOINT
ENDINTERFACE
ENDCONFIG
ENDDEVICE		(optional)

*/

#include	<stdlib.h>
#include	<vector.h>
#include	<alloc.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<math.h>

typedef struct device
{
	char *			name;
	unsigned int	class, subclass, protocol;
	unsigned int	packetsize;
	unsigned int	version;		// USB version
	unsigned int	vid, pid, release;
	unsigned int	manufacturer, product, serial;		// string indices
	vector_t		configurations, strings;
}	device_t;

typedef struct configuration
{
	vector_t		interfaces;
	unsigned int	attributes;
	unsigned int	power;
	unsigned int	id;
	unsigned int	name;
	unsigned int	total;
	unsigned int	offset;
}	configuration_t;

typedef struct interface
{
	vector_t		endpoints;
	vector_t		functions;
	unsigned int	id;
	unsigned int	class, subclass, protocol;
	unsigned int	name;			// string index
	unsigned int	offset;
}	interface_t;

typedef struct
{
	unsigned char	length;
	unsigned char	type;
	unsigned char *	body;
}	function_t;



typedef enum {
	control = 0,
	iso = 1,
	bulk = 2,
	interrupt = 3
}	transfer_t;

typedef struct endpoint
{
	unsigned int	no;
	unsigned int	dir;		// out == 0
	transfer_t		type;
	unsigned int	packetsize;
	unsigned int	interval;
	unsigned int	offset;
}	endpoint_t;

vector_t			devices;
device_t *			current_device;
unsigned int		errcnt, lineno;
unsigned int		maxconfsize;
char				buf[4096], * name, * value;
char *				global_serial;

unsigned int	interface_no = 0, configuration_no = 1;

//	Sizes of various descriptors

#define	DEVICE_SIZE			18
#define	ENDPOINT_SIZE		7
#define	INTERFACE_SIZE		9
#define	CONFIGURATION_SIZE	9
#define	DFU_SIZE			9

static void
error(char * s, ...)
{
	va_list	ap;

	fprintf(stderr, "%3d: ", lineno);
	va_start(ap, s);
	vfprintf(stderr, s, ap);
	fprintf(stderr, "\n");
	errcnt++;
}

static unsigned int
getyesno(char * s)
{
	if(strcasecmp(s, "yes") == 0)
		return 1;
	if(strcasecmp(s, "no") == 0)
		return 0;
	error("Expected either yes or no");
	return 0;
}

static unsigned int
tobcd(unsigned int i)
{
	return (i/10)*16 + i%10;
}

static unsigned int
getBCD(char * s)
{
	unsigned int	maj, min;
	double			f;

	maj = 0;
	min = 0;
	if(sscanf(s, "%lf", &f) <= 0)
		error("malformed version number: %s", s);
	maj = floor(f);
	f -= maj;
	maj = tobcd(maj);
	min = f*100;
	min = tobcd(min);
	return (maj << 8) + min;
}

static unsigned int
addstring(char * s)
{
	char *	cp;

	VEC_ITERATE(current_device->strings, cp, char *) {
		if(strcmp(cp, s) == 0)
			return vec_indexOf(current_device->strings, cp)+1;
	}
	s = strdup(s);
	vec_add(current_device->strings, s);
	return vec_indexOf(current_device->strings, s)+1;
}


#define	_stringize(x)	#x
#define	is(x)	if(strcasecmp(_stringize(x), name) == 0)
#define	tok(x)	value = strtok(value, _stringize(x) " ")

static int
nextline(void)
{
	while(fgets(buf, sizeof buf, stdin)) {
		lineno++;
		name = strtok(buf, "\n");
		if(!name)
			continue;
		while(isspace(*name))
			name++;
		if(*name == 0 || *name == ';' || *name == '#')
			continue;
		name = strtok(name, " =");
		if(!name)
			continue;
		value = strtok(0, "=");
		return 1;
	}
	return 0;
}

static int
get_dfu(interface_t * interface)
{
	function_t *		function;
	unsigned int		i;

	function = alloc(sizeof *function);
	function->body = alloc(7);
	function->length = DFU_SIZE;
	function->type = 0x21;
	vec_add(interface->functions, function);

	while(nextline()) {
		is(DETACH) {
			if(getyesno(value))
				function->body[0] |= 0x08;
			continue;
		}
		is(TOLERANT) {
			if(getyesno(value))
				function->body[0] |= 0x04;
			continue;
		}
		is(UPLOAD) {
			if(getyesno(value))
				function->body[0] |= 0x02;
			continue;
		}
		is(DOWNLOAD) {
			if(getyesno(value))
				function->body[0] |= 0x01;
			continue;
		}
		is(TIMEOUT) {
			i = strtol(value, 0, 0);
			function->body[1] = i & 0xFF;
			function->body[2] = (i >> 8) & 0xFF;
			continue;
		}
		is(TRANSFER) {
			i = strtol(value, 0, 0);
			if(i > current_device->packetsize)
				error("Transfer size must be <= packetsize (%d)", current_device->packetsize);
			function->body[3] = i & 0xFF;
			function->body[4] = (i >> 8) & 0xFF;
			continue;
		}
		is(VERSION) {
			i = getBCD(value);
			function->body[5] = i & 0xFF;
			function->body[6] = (i >> 8) & 0xFF;
			continue;
		}
		is(ENDFUNCTION)
			return 1;
		error("Unrecognized endpoint keyword \"%s\"", name);
	}
	return 0;
}

static int
get_endpoint(interface_t * interface, char * number)
{
	endpoint_t *		endpoint;

	endpoint = alloc(sizeof *endpoint);
	vec_add(interface->endpoints, endpoint);

	if(strtol(number, 0, 0) == 0)
		error("Endpoint 0 should not be specified");
	else
		endpoint->no = strtol(number, 0, 0);
	while(nextline()) {
		is(DIR) {
			if(strcasecmp(value, "in") == 0)
				endpoint->dir = 0x80;
			else if(strcasecmp(value, "out") == 0)
				endpoint->dir = 0;
			else
				error("Invalid argument to DIR");
			continue;
		}
		is(TYPE) {
			if(strcasecmp(value, "control") == 0)
				endpoint->type = control;
			else if(strcasecmp(value, "iso") == 0 || strcasecmp(value, "isosynchronous") == 0)
				endpoint->type = iso;
			else if(strcasecmp(value, "bulk") == 0)
				endpoint->type = bulk;
			else if(strcasecmp(value, "interrupt") == 0)
				endpoint->type = interrupt;
			else
				error("Invalid argument to TYPE");
			continue;
		}
		is(PACKETSIZE) {
			endpoint->packetsize = strtol(value, 0, 0);
			if(endpoint->packetsize > 1024)
				error("Invalid packet size");
			continue;
		}
		is(INTERVAL) {
			endpoint->interval = strtol(value, 0, 0);
			if(endpoint->interval > 255)
				error("Invalid interval");
			continue;
		}
		is(ENDENDPOINT)
			return 1;
		error("Unrecognized endpoint keyword \"%s\"", name);
	}
	return 0;
}

static int
get_interface(configuration_t * conf, char * intname)
{
	interface_t *		interface;

	interface = alloc(sizeof *interface);
	vec_add(conf->interfaces, interface);
	interface->name = addstring(intname);
	interface->endpoints = vec_new();
	interface->functions = vec_new();
	interface->id = interface_no++;
	conf->total += INTERFACE_SIZE;
	if(conf->total > maxconfsize)
		maxconfsize = conf->total;

	while(nextline()) {
		is(ENDPOINT) {
			conf->total += ENDPOINT_SIZE;
			if(get_endpoint(interface, value))
				continue;
			break;
		}
		is(DFU_FUNCTION) {
			if(get_dfu(interface)) {
				conf->total += DFU_SIZE;
				continue;
			}
			break;
		}
		is(CLASS) {
			value = strtok(value, ",");
			if(!value) {
				error("Invalid CLASS line");
				continue;
			}
			interface->class = strtol(value, 0, 0);
			value = strtok(0, ",");
			if(value) {
				interface->subclass = strtol(value, 0, 0);
				value = strtok(0, ",");
				if(value)
					interface->protocol = strtol(value, 0, 0);
			}
			continue;
		}
		is(ENDINTERFACE)
			return 1;
		error("Unrecognized interface keyword \"%s\"", name);
	}
	return 0;
}

static int
get_config(device_t * device, char * confname)
{
	configuration_t *		conf;

	conf = alloc(sizeof *conf);
	vec_add(device->configurations, conf);
	conf->name = addstring(confname);
	conf->interfaces = vec_new();
	conf->id = configuration_no++;
	conf->attributes = 0x80;
	conf->total = CONFIGURATION_SIZE;

	while(nextline()) {
		is(POWER) {
			value = strtok(value, ",");
			if(value && strcasecmp(value, "bus") == 0)
				/* ok */ ;
			else if(value && strcasecmp(value, "self") == 0)
				conf->attributes |= 0x40;
			else
				error("Invalid POWER argument");
			value = strtok(0, ",");
			if(!value || strtol(value, 0, 0) == 0)
				error("Invalid mA value in POWER");
			else
				conf->power = (strtol(value, 0, 0)+1)/2;
			continue;
		}
		is(INTERFACE) {
			if(get_interface(conf, value))
				continue;
			break;
		}
		is(WAKEUP) {
			if(strcasecmp(value, "yes") == 0)
				conf->attributes |= 0x20;
			else if(strcasecmp(value, "no") == 0)
				/* ok */ ;
			else
				error("Invalid argument to WAKEUP");
			continue;
		}
		is(ENDCONFIG)
			return 1;
		error("Unrecognized configuration keyword \"%s\"", name);
	}
	return 0;
}

void
cleanup(int i)
{
	exit(i);
}


int
get_device(char * devname)
{
	device_t *	device;

	device = alloc(sizeof *device);
	current_device = device;
	vec_add(devices, device);
	device->configurations = vec_new();
	device->name = strdup(devname);
	device->strings = vec_new();
	interface_no = 0;
	configuration_no = 1;

	device->packetsize = 64;
	while(nextline()) {
		is(USB) {
			device->version = getBCD(value);
			continue;
		}
		is(CLASS) {
			value = strtok(value, ",");
			if(!value) {
				error("Invalid CLASS line");
				continue;
			}
			device->class = strtol(value, 0, 0);
			value = strtok(0, ",");
			if(value) {
				device->subclass = strtol(value, 0, 0);
				value = strtok(0, ",");
				if(value)
					device->protocol = strtol(value, 0, 0);
			}
			continue;
		}
		is(PACKETSIZE) {
			device->packetsize = strtol(value, 0, 0);
			if(device->packetsize > 1024)
				error("Invalid packet size");
			continue;
		}
		is(VID) {
			device->vid = strtol(value, 0, 0);
			continue;
		}
		is(PID) {
			device->pid = strtol(value, 0, 0);
			continue;
		}
		is(RELEASE) {
			device->release = strtol(value, 0, 0);
			continue;
		}
		is(MANUFACTURER) {
			device->manufacturer = addstring(value);
			continue;
		}
		is(PRODUCT) {
			device->product = addstring(value);
			continue;
		}
		is(SERIAL) {
			device->serial = addstring(value);
			continue;
		}
		is(CONFIG) {
			if(get_config(device, value))
				continue;
			break;
		}
		is(ENDDEVICE) {
			if(device->serial == 0 && global_serial)
				device->serial = addstring(global_serial);
			return 1;
		}
		error("Unrecognized keyword \"%s\"", name);
	}
	return 0;
}

static char *
get_string(vector_t str, unsigned int idx)
{
	if(idx == 0 || idx > vec_size(str))
		return "";
	return (char *)vec_elementAt(str, idx-1);
}

static void
write_devices()
{
	device_t *			device;
	configuration_t *	conf;
	interface_t *		interface;
	endpoint_t *		endpoint;
	function_t *		function;
	unsigned int		offset;
	vector_t			interface_list, string_idx;
	char *				cp;
	unsigned int		len, i;

	interface_list = vec_new();
	string_idx = vec_new();
	printf("#ifndef USB_CONST\n#define	USB_CONST	const\n#endif\n\n\n");
	printf("#include	<htusb.h>\n\n");
	printf("/* This structure is defined in htusb.h, here for reference only\n\n");
	printf(
			"typedef struct\n{\n"
			"	USB_CONST unsigned char *				device,\n"
			"							* USB_CONST *	configurations,\n"
			"							* USB_CONST *	strings;\n"
			"	USB_CONST unsigned short*				conf_sizes;\n"
			"	unsigned char							conf_cnt, string_cnt;\n"
			"}	USB_descriptor_table;\n");
	printf("*/\n\n");

	VEC_ITERATE(devices, device, device_t *) {
		offset = 0;
		vec_removeAll(interface_list);
		vec_removeAll(string_idx);
		printf("// USB Device \"%s\"\n\n", device->name);
		printf("static USB_CONST unsigned char %s_data[] = {\n", device->name);
		printf("	%d, 1,			// %d byte device descriptor \n", DEVICE_SIZE, DEVICE_SIZE);
		printf("	0x%2.2X, 0x%2.2X,		// USB version\n",
				device->version & 0xFF, device->version >> 8);
		printf("	0x%2.2X,0x%2.2X,0x%2.2X,		// class,subclass,protocol\n",
				device->class, device->subclass, device->protocol);
		printf("	%d,			// max packet size\n", device->packetsize);
		printf("	0x%2.2X, 0x%2.2X,		// Vendor ID: 0x%X/%d\n",
				device->vid & 0xFF, device->vid >> 8,
				device->vid, device->vid);
		printf("	0x%2.2X, 0x%2.2X,		// Product ID: 0x%X/%d\n",
				device->pid & 0xFF, device->pid >> 8,
				device->pid, device->pid);
		printf("	0x%2.2X, 0x%2.2X,		// Product release: 0x%X/%d\n",
				device->release & 0xFF, device->release >> 8,
				device->release, device->release);
		printf("	%d,			// manufacturer: \"%s\"\n", device->manufacturer,
				get_string(device->strings, device->manufacturer));
		printf("	%d,			// product: \"%s\"\n", device->product,
				get_string(device->strings, device->product));
		if(device->serial == 0)
			printf("	%d,			// Custom serial\n", vec_size(device->strings)+1);
		else
			printf("	%d,			// serial: \"%s\"\n", device->serial,
					get_string(device->strings, device->serial));
		printf("	%d,			// number of configurations\n\n",
				vec_size(device->configurations));
		offset += DEVICE_SIZE;
		VEC_ITERATE(device->configurations, conf, configuration_t *) {
			conf->offset = offset;
			printf("	// Configuration %d\n\n", conf->id);
			printf("		%d, 2,		// %d byte configuration descriptor \n", CONFIGURATION_SIZE, CONFIGURATION_SIZE);
			printf("		0x%2.2X, 0x%2.2X,	// total size of conf/interface/endpoint descriptors = %d\n",
					conf->total & 0xFF, conf->total >> 8, conf->total);
			printf("		%d,		// number of interfaces in this configuration\n",
					vec_size(conf->interfaces));
			printf("		%d,		// configuration identifier\n", conf->id);
			printf("		%d,		// configuration name: \"%s\"\n", conf->name,
					get_string(device->strings, conf->name));
			printf("		0x%2.2X,		// attributes: ", conf->attributes);
			if(conf->attributes & 0x20)
				printf("Wakeup=yes; ");
			else
				printf("Wakeup=no; ");
			if(conf->attributes & 0x40)
				printf("self powered\n");
			else
				printf("bus powered\n");
			printf("		0x%2.2X,		// power=%dmA\n\n", conf->power, conf->power*2);
			offset += CONFIGURATION_SIZE;
			VEC_ITERATE(conf->interfaces, interface, interface_t *) {
				vec_add(interface_list, interface);
				interface->offset = offset;
				printf("		// Interface %d\n", interface->id);
				printf("			%d, 4,		// %d byte interface descriptor \n", INTERFACE_SIZE, INTERFACE_SIZE);
				printf("			%d,		// interface identifier\n", interface->id);
				printf("			0,		// alternate identifier\n");
				printf("			%d,		// number of endpoints\n",
						vec_size(interface->endpoints));
				printf("			0x%2.2X,0x%2.2X,0x%2.2X,	// class,subclass,protocol\n",
						interface->class, interface->subclass, interface->protocol);
				printf("			%d,		// interface name: \"%s\"\n", interface->name,
						get_string(device->strings, interface->name));
				offset += INTERFACE_SIZE;
				VEC_ITERATE(interface->endpoints, endpoint, endpoint_t *) {
					endpoint->offset = offset;
					printf("			// Endpoint %d\n", endpoint->no);
					printf("				%d, 5,		// %d byte endpoint descriptor \n",
							ENDPOINT_SIZE, ENDPOINT_SIZE);
					printf("				0x%2.2X,		// Endpoint number (%s)\n",
							endpoint->no|endpoint->dir, endpoint->dir ? "in" : "out");
					printf("				0x%2.2X,		//", endpoint->type);
					switch(endpoint->type) {

						case iso:
							printf(" isosynchronous");
							break;

						case bulk:
							printf(" bulk");
							break;

						case interrupt:
							printf(" interrupt");
							break;

						case control:
							printf(" control");
							break;
					}
					printf("\n");
					printf("				0x%2.2X, 0x%2.2X,	// Max packet size %d\n",
							endpoint->packetsize & 0xFF, endpoint->packetsize >> 8,
							endpoint->packetsize);
					printf("				%d,		// Latency/polling interval\n", endpoint->interval);
					offset += ENDPOINT_SIZE;
				}
				VEC_ITERATE(interface->functions, function, function_t *) {
					printf("\n			// Function descriptor\n");
					printf("				%d, 0x%X,	// length and type\n			",
							function->length, function->type);
					for(i = 0 ; i != function->length-2 ; i++)
						printf("0x%2.2X, ", function->body[i]);
					printf("\n\n");
					offset += function->length;
				}
			}
		}
		i = 0;
		printf("	// string descriptor 0: language ids\n");
		printf("	4, 3, 9, 4,\n");
		vec_setAt(string_idx, (void *)(long)offset, i);
		offset += 4;
		VEC_ITERATE(device->strings, cp, char *) {
			len = strlen(cp);
			i++;
			printf("	// string descriptor %d: \"%s\"\n", i, get_string(device->strings, i));
			printf("	%d, 3, ", len*2+2);
			while(*cp)
				printf("%d, 0, ", *cp++);
			printf("\n");
			vec_add(string_idx, (void *)(long)offset);
			offset += len*2+2;
		}
		if(device->serial == 0)
			vec_add(string_idx, (void *)0xFFFF);
		printf("\n};\n");
		printf("// sizeof table above should be %u\n\n", offset);

		printf("// Access tables for this device\n");
		printf("\nstatic USB_CONST unsigned char * USB_CONST	%s_configurations[] =\n{\n", device->name);
		VEC_ITERATE(device->configurations, conf, configuration_t *)
			printf("	%s_data+%u,\n", device->name, conf->offset);
		printf("};\n");
		printf("\nstatic USB_CONST unsigned short %s_conf_sizes[] =\n{\n", device->name);
		VEC_ITERATE(device->configurations, conf, configuration_t *)
			printf("	%u,\n", conf->total);
		printf("};\n");
		printf("\nstatic USB_CONST unsigned char * USB_CONST	%s_strings[] =\n{\n", device->name);
		VEC_ITERATE(string_idx, i, unsigned)
			if(i == 0xFFFF)
				printf("	0,\n");
			else
				printf("	%s_data+%u,\n", device->name, i);
		printf("};\n");
		printf("USB_CONST USB_descriptor_table	%s_table =\n{\n", device->name);
		printf("	%s_data,\n\t%s_configurations,\n\t%s_strings,\n\t%s_conf_sizes,",
				device->name, device->name, device->name, device->name);
		printf("	%u, %u,\n};\n",
				vec_size(device->configurations), vec_size(string_idx));
	}
}


int
main(int argc, char ** argv)
{
	devices = vec_new();
	while(nextline()) {
		is(DEVICE) {
			get_device(value);
			continue;
		}
		is(SERIAL) {
			global_serial = strdup(value);
			continue;
		}
		error("Unrecognized keyword \"%s\"", name);
	}
	write_devices();
	cleanup(errcnt != 0);
	return 1;
}
