# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.

EE_BIN = hello.elf

EE_OBJS = main.o
EE_LIBS = -lpatches

#Include preferences
# include $(PS2SDK)/samples/Makefile.pref
# include $(PS2SDK)/samples/Makefile.eeglobal

#Include UJ preferences
include $(PS2SDKUJ)/samples/Makefile.pref
include $(PS2SDKUJ)/samples/Makefile.eeglobal