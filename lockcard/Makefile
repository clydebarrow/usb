#
#	Makefile for the Lock controller firmware
#

.SUFFIXES:
.SUFFIXES:	.c .obj .as .usb
COM	= ../common
-include ~/.htdefaults.mk
HITECH	= /Applications/HI-TECH/hc51/pro/9.70
AR	= libr r
C	= c51
AS	= c51
ASFLAGS	= -P
CHIP=c8051f320
DEBUG	= -UDEBUG -ULOGGING
ROM=--rom=0-2FFF
CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=all -g --asmlist -I. $(DEBUG) --opt=-asm
NOOPT_CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=none
USBCONFIG=../utils/usbconfig

LOCKCARD_H = lockcard.h

VERS_MINOR := $(shell sed -n 's/\#define.*VERS_MINOR *\(.*\)/\1/p' < $(LOCKCARD_H))
VERS_MAJOR := $(shell sed -n 's/\#define.*VERS_MAJOR *\(.*\)/\1/p' < $(LOCKCARD_H))
VERS := $(shell printf '%d.%2.2d' $(VERS_MAJOR) $(VERS_MINOR))
HVERS := $(shell printf '0x%d%2.2d' $(VERS_MAJOR) $(VERS_MINOR))


LD	= $(BIN)/link
RMAKE	= $(MAKE) -f Makefile

%.obj:	%.as
	$(C) $(CFLAGS) $(ASFLAGS) -c $<
%.obj:	%.c
	$(C) $(CFLAGS) -c $<
#%.obj:	%.usb
#	$(C) $(CFLAGS) -c $<
%.c:	%.usb
	$(USBCONFIG) < $< > $@
%.p1:	%.c
	$(C) $(CFLAGS) --pass1 $<


SRCS	 = \
	   main.c bootlib.as

OBJS	 = \
		main.p1

all:	  lockcard.hex

lockcard.hex:	$(OBJS)
	$(C) $(CFLAGS) -olockcard.hex -mlockcard.map $(OBJS) --runtime=+keep
	../utils/firmware -c0 -v$(HVERS) -p320 lockcard.hex >lockcard.fmw
#	$(C) --chip=$(CHIP) -olockcard.hex -mlockcard.map $(OBJS)

srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst *.hex *.omf *.rlf *.hxl

clean:		rmobjs
		@rm -f lockcard.c lockcard.fmw
		
main.obj:	main.c lockcard.h Makefile
