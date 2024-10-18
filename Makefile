# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2022, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = hello.elf


EE_OBJS = main.o
EE_CFLAGS += -O3 -g
EE_LIBS = -lps2_drivers -lpatches
EE_LDFLAGS += -L$(PS2SDK)/ports/lib
EE_INCS += -I$(PS2SDK)/ports/include

all: $(EE_BIN)

clean:
	rm -rf $(EE_OBJS) $(EE_BIN)

# Include makefiles
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
