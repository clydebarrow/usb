#
#	Makefile for the JTAG firmware
#

.SUFFIXES:
.SUFFIXES:	.c .obj .as .usb
-include ~/.htdefaults.mk
ifdef	INSTALL_DIR
HITECH=$(shell echo $(INSTALL_DIR) | sed 's^/bin^^' )
else
HITECH	= /usr/hitech
endif
BIN	= $(HITECH)/bin
LIB	= $(HITECH)/lib
AR	= $(BIN)/libr r
C	= $(BIN)/c51
AS	= $(BIN)/c51
ASFLAGS	= -P
CHIP=c8051f320
USBCONFIG=../utils/usbconfig
DEBUG	= -UDEBUG -ULOGGING -UUSB_LOGGING
CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=all -g --asmlist -I. $(DEBUG)
NOOPT_CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=none

JTAG_H = jtag.h

VERS_MINOR := $(shell sed -n 's/\#define.*VERS_MINOR *\(.*\)/\1/p' < $(JTAG_H))
VERS_MAJOR := $(shell sed -n 's/\#define.*VERS_MAJOR *\(.*\)/\1/p' < $(JTAG_H))
VERS := $(shell printf '%d.%2.2d' $(VERS_MAJOR) $(VERS_MINOR))


LD	= $(BIN)/link
RMAKE	= $(MAKE) -f Makefile
FIRMWARE = ../utils/firmware

%.obj:	%.as
	$(C) $(CFLAGS) $(ASFLAGS) -c $<
%.obj:	%.c
	$(C) $(CFLAGS) -c $<
%.p1:	$(COM)/%.c
	$(C) $(CFLAGS) --pass1 $<
%.c:	%.usb
	$(USBCONFIG) < $< > $@


SRCS	 = \
	   main.c jtag.usb silabs.c psoc.c  bootlib.as

OBJS	 = \
		main.obj jtag.obj silabs.obj psoc.obj

all:	  jtag.hex

jtag.hex:	$(OBJS)
	$(C) --rom=default,-0-fff --codeoffset=1000h --chip=$(CHIP) -ojtag.hex -mjtag.map $(OBJS) bootlib.as
	$(FIRMWARE) -c1000 -v$(VERS) -p320 jtag.hex >jtag.fmw
#	$(C) --chip=$(CHIP) -ojtag.hex -mjtag.map $(OBJS)

srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst *.hex *.omf

clean:		rmobjs
		@rm -f jtag.c
		
main.obj:	main.c htusb.h jtag.h Makefile
