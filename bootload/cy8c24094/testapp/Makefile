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
C	= cpsoc
AS	= cpsoc
ASFLAGS	= -P
CHIP=CY8C24094
USBCONFIG=../../../utils/usbconfig
FIRMWARE=../../../utils/firmware


#define the base of the application program. The bootloader must reside
#below this
ROM		= --ROM=default,-000-FFF
USER_START	= 0x1000
CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) -DUSER_START=$(USER_START) -g --asmlist -I. -ULOGGING -Dxdata= -DDEBUG=1
NOOPT_CFLAGS	= -q --chip=$(CHIP) $(DBL) $(ROM) --opt=none


LD	= $(BIN)/link
RMAKE	= $(MAKE) -f Makefile

%.obj:	%.as
	$(C) -P $(CFLAGS) $(ASFLAGS) -c $<
%.obj:	%.c
	$(C) $(CFLAGS) -c $<
%.p1:	%.c
	$(C) $(CFLAGS) --pass1 $<
%.c:	%.usb
	$(USBCONFIG) < $< > $@



SRCS	 = \
	   psoctest.usb  main.c lcd.c  
OBJS	 = \
	   psoctest.p1  main.p1 lcd.p1 

all:	  bootload.hex

bootload.hex:	$(OBJS)
	$(C) $(CFLAGS)      -opsoctest.hex  -mpsoctest.map $(OBJS) bootlib.as flashsecurity.txt
	$(FIRMWARE) -c1000-8000  -v$(HVERS) -p494 psoctest.hex >psoctest.fmw


srclist:
	 @echo $(SRCS)

objlist:
	 @echo $(OBJS)

rmobjs:
		@rm -f $(OBJS) *.map *.sym *.sdb *.lst psoctest.hex *.omf

clean:		rmobjs
		@rm -f psoctest.c
		
usbx94.p1:	usbx94.c Makefile bootload.h
