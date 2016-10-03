

ifndef CC
CC	= gcc
endif

all: ps2stuff.o # test - DON'T USE TEST - IT'S BROKEN AND WILL LOCK YOUR MACHINE

clean:
	rm -f ps2stuff.o test

# to enable debug output, build with 'make CFLAGS=-D_DEBUG'
ps2stuff.o: ps2stuff.c Makefile ps2stuff_kmodule.h
	$(CC) $(CFLAGS) -mno-abicalls -fno-pic -mlong-calls -mcpu=r5900 -mips1 -pipe \
	-fomit-frame-pointer -O2 -G 0 -c ps2stuff.c -o ps2stuff.o

test: test.c Makefile ps2stuff_kmodule.h
	$(CC) -O2 -fomit-frame-pointer -o test test.c


