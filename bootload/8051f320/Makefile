#
#	Makefile for the USB bootload firmware
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
USBCONFIG=../../utils/usbconfig


#define the base of the application program. The bootloader must reside
#below this

USER_START	= 0x1000
CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) -DUSER_START=$(USER_START) -g --asmlist -I. -ULOGGING
NOOPT_CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=none
BOOTLOAD_H = bootload.h

VERS_MINOR := $(shell sed -n 's/\#define.*VERS_MINOR *\(.*\)/\1/p' < $(BOOTLOAD_H))
VERS_MAJOR := $(shell sed -n 's/\#define.*VERS_MAJOR *\(.*\)/\1/p' < $(BOOTLOAD_H))
VERS := $(shell printf '%d.%2.2d' $(VERS_MAJOR) $(VERS_MINOR))
HVERS := $(shell printf '0x%d%2.2d' $(VERS_MAJOR) $(VERS_MINOR))

LD	= $(BIN)/link
RMAKE	= $(MAKE) -f Makefile

%.obj:	%.as
	$(C) -P $(CFLAGS) $(ASFLAGS) -c $<
%.obj:	%.c
	$(C) $(CFLAGS) -c $<
%.p1:	$(COM)/%.c
	$(C) $(CFLAGS) --pass1 $<
%.c:	%.usb
	$(USBCONFIG) < $< > $@



SRCS	 = \
	   bootload.usb usb320.c main.c vectors.as

OBJS	 = \
	   bootload.obj usb320.obj main.obj vectors.obj

all:	  bootload.hex

bootload.hex:	$(OBJS)
	$(C) --chip=$(CHIP) -obootload.hex -mbootload.map $(OBJS)
	firmware -c0 -v$(HVERS) -p320 bootload.hex >bootload.fmw


srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst *.hex *.omf

clean:		rmobjs
		@rm -f bootload.c
		
usb320.obj:	usb320.c Makefile bootload.h
