# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = hello.elf

EE_OBJS = main.o
EE_CFLAGS = -Os
EE_CFLAGS += -fdata-sections -ffunction-sections
EE_LDFLAGS += -s
EE_LDFLAGS += -Wl,--gc-sections

ifeq ($(DUMMY_TIMEZONE), 1)
   EE_CFLAGS += -DDUMMY_TIMEZONE
endif

ifeq ($(DUMMY_LIBC_INIT), 1)
   EE_CFLAGS += -DDUMMY_LIBC_INIT
endif

all: $(EE_BIN)

clean:
	rm -rf $(EE_OBJS) $(EE_BIN)

# Include makefiles
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal