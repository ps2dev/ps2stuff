EE_LIB = libps2stuff.a

EE_LDFLAGS  += -L. -L$(PS2SDK)/ports/lib
EE_INCS     += -I./include -I$(PS2SDK)/ports/include
EE_CFLAGS   += -D_DEBUG
EE_CXXFLAGS += -D_DEBUG
# VU0 code is broken so disable for now
EE_CFLAGS   += -DNO_VU0_VECTORS
EE_CXXFLAGS += -DNO_VU0_VECTORS

EE_OBJS = \
	src/core.o \
	src/cpu_matrix.o \
	src/displayenv.o \
	src/drawenv.o \
	src/eetimer.o \
	src/gs.o \
	src/gsmem.o \
	src/imagepackets.o \
	src/math.o \
	src/matrix.o \
	src/packet.o \
	src/perfmon.o \
	src/ps2stuff.o \
	src/sprite.o \
	src/texture.o \
	src/timer.o \
	src/utils.o

all: $(EE_LIB)

install: all
	mkdir -p $(PS2SDK)/ports/include
	mkdir -p $(PS2SDK)/ports/lib
	cp -rf include/ps2s $(PS2SDK)/ports/include
	cp -f  $(EE_LIB) $(PS2SDK)/ports/lib

clean:
	rm -f $(EE_OBJS_LIB) $(EE_OBJS) $(EE_BIN) $(EE_LIB)

realclean: clean
	rm -rf $(PS2SDK)/ports/include/ps2s
	rm -f  $(PS2SDK)/ports/lib/$(EE_LIB)

include $(PS2SDK)/Defs.make
include ../Makefile.eeglobal
