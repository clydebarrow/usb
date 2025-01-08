#
#	Makefile for usbconfig.
#	Copyright (C) HI-TECH Software, Pty. Ltd.
#
#	$Header$
#

.PHONY: all
all:	targets

COMMON	= ../../common
PRIMARY	= usbconfig
SRCS	= usbconfig.c \
	  $(COMMON)/osdep.c \
	  $(COMMON)/vector.c $(COMMON)/alloc.c

LIBXML_ON_UNIX=1
include $(COMMON)/common.mk
include $(COMMON)/target.mk
LDFLAGS += -lm

htupdate.exe: htupdate.c
	mingw-gcc  -Xlinker -Map -Xlinker htupdate.map -s -mwindows \
		-o htupdate.exe -I../../common htupdate.c \
		../../common/vector.c ../../common/osdep.c ../../common/alloc.c -lsetupapi

enumdev.exe: enumdev.c
	mingw-gcc -s -o enumdev.exe -I$(COMMON) enumdev.c \
		$(COMMON)/vector.c $(COMMON)/osdep.c $(COMMON)/alloc.c \
		-L$(COMMON)/lib/win/ \
		-lsetupapi -lcrypto
