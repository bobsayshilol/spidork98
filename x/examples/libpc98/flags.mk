# Executables
CC		= gcc.exe
LD		= gcc.exe
AR		= ar.exe
RANLIB	= ranlib.exe
STRIP	= strip.exe

# Compiler options
CFLAGS	= -g -Wall -Werror
CFLAGS	+= -mcpu=i386 -march=i386 -O3

# LIBPC98 target
LIBPC98	= ..\\libpc98\\libpc98.a
CFLAGS	+= -I..\\libpc98
LDFLAGS	=
DEPS	= ..\\libpc98\\funcs.h ..\\libpc98\\gpuscrn.h $(LIBPC98)
