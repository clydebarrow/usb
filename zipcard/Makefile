#
#	Makefile for the Zipcard firmware
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

ZIPCARD_H = zipcard.h

VERS_MINOR := $(shell sed -n 's/\#define.*VERS_MINOR *\(.*\)/\1/p' < $(ZIPCARD_H))
VERS_MAJOR := $(shell sed -n 's/\#define.*VERS_MAJOR *\(.*\)/\1/p' < $(ZIPCARD_H))
VERS := $(shell printf '%d.%2.2d' $(VERS_MAJOR) $(VERS_MINOR))
HVERS := $(shell printf '0x%d%2.2d' $(VERS_MAJOR) $(VERS_MINOR))


LD	= $(BIN)/link
RMAKE	= $(MAKE) -f Makefile

%.obj:	%.as
	$(C) $(CFLAGS) $(ASFLAGS) -c $<
%.obj:	%.c
	$(C) $(CFLAGS) -c $<
%.p1:	$(COM)/%.c
	$(C) $(CFLAGS) --pass1 $<
%.c:	%.usb
	$(USBCONFIG) < $< > $@


SRCS	 = main.c zipcard.usb bootlib.as

OBJS	 = main.obj zipcard.obj bootlib.obj

all:	  zipcard.hex

zipcard.hex:	$(OBJS)
	$(C) --rom=default,-0-fff --codeoffset=1000h --chip=$(CHIP) -ozipcard.hex -mzipcard.map $(OBJS)
	firmware -c1000 -v$(HVERS) -p320 zipcard.hex >zipcard.fmw

bugfix.hex:	main.obj zipcard.obj usbfix.obj
	$(C) --rom=default,-0-fff --codeoffset=1000h --chip=$(CHIP) -obugfix.hex -mbugfix.map $^
	firmware -c1000 -v$(HVERS) -p320 bugfix.hex >bugfix.fmw

debug.omf:	main.obj zipcard.obj usb32x.obj
	$(C) --chip=$(CHIP) -odebug.hex -mdebug.map $^

srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst *.hex *.omf

clean:		rmobjs
		@rm -f zipcard.c
		
main.obj:	main.c htusb.h zipcard.h Makefile
usbfix.obj:	usbfix.c htusb.h zipcard.h Makefile
	$(C) $(CFLAGS) $(ASFLAGS) -c usbfix.c
