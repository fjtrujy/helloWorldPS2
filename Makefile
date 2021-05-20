# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = test.elf

EE_OBJS = testSuite.o
EE_INCS += -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ports/lib -L.
EE_LIBS += -lgtest -lgtest_main

all: $(EE_BIN)

clean:
	rm -rf $(EE_OBJS) $(EE_BIN)

reset:
	ps2client -h 192.168.1.10 reset

run:
	ps2client -h 192.168.1.10 netdump
	ps2client -h 192.168.1.10 execee host:test.elf



# Include makefiles
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal_cpp