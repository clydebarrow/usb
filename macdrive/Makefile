.PHONY: all htusb

all: htusb

clean clobber:
	@rm -rf htusb
htusb: main.c
	gcc-4.0 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc -framework IOKit -framework CoreFoundation main.c -o htusb
