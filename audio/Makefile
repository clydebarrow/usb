#
#	Makefile for the Lock controller firmware
#

.SUFFIXES:
.SUFFIXES:	.c .obj .as .usb
COM	= ../common
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
USBCONFIG=../utils/usbconfig
ASFLAGS	= -P
CHIP=c8051f320
DEBUG	= -UDEBUG -ULOGGING
CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=all -g --asmlist -I. $(DEBUG)
NOOPT_CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=none

audiogw_H = audiogw.h

VERS_MINOR := $(shell sed -n 's/\#define.*VERS_MINOR *\(.*\)/\1/p' < $(audiogw_H))
VERS_MAJOR := $(shell sed -n 's/\#define.*VERS_MAJOR *\(.*\)/\1/p' < $(audiogw_H))
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



SRCS	 = \
	   main.c bootlib.as

OBJS	 = \
		main.obj bootlib.obj

all:	  audiogw.hex

audiogw.hex:	$(OBJS)
	$(C) --chip=$(CHIP) -oaudiogw.hex -maudiogw.map $(OBJS)
	firmware -c0 -v$(HVERS) -p320 audiogw.hex >audiogw.fmw
#	$(C) --chip=$(CHIP) -oaudiogw.hex -maudiogw.map $(OBJS)

srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst *.hex *.omf *.rlf *.hxl

clean:		rmobjs
		@rm -f audiogw.c audiogw.fmw
		
main.obj:	main.c audiogw.h Makefile
