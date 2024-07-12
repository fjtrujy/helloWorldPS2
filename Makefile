# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2022, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = gprof.elf

EE_OBJS = main.o
EE_LIBS += -lprof -pg

EE_INCS = -I$(PS2SDK)/ports/include
EE_LDFLAGS += -L$(PS2SDK)/ports/lib -L.

EE_CFLAGS += -Wno-implicit-function-declaration -Wno-strict-aliasing -Wno-unused-function

# ifeq ($(DEBUG), 1)
  EE_CFLAGS += -DDEBUG -O0 -g -pg
# else 
#   EE_CFLAGS += -Os
#   EE_LDFLAGS += -s
# endif

all: $(EE_BIN)

clean:
	rm -rf $(EE_OBJS) $(EE_BIN)

analyze: $(EE_BIN)
	mips64r5900el-ps2-elf-gprof -b $(EE_BIN) gmon.out > profile.txt

# Include makefiles
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
