
#default OS
OS=linux64

all:	htusb
clean clobber:	
	@rm -rf htusb build

ifeq ($(OS), linux64)
htusb: htusb.c
	mkdir -p build/linux64
	gcc -m64 htusb.c -static -lusb -lpthread -obuild/linux64/htusb
	ln -sf build/linux64/htusb
endif
ifeq ($(OS), linux)
htusb: htusb.c
	mkdir -p build/linux
	gcc -m32 htusb.c -static -lusb -lpthread -obuild/linux/htusb
endif


