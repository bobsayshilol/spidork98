# Executables
CC		= gcc.exe
LD		= gcc.exe
AR		= ar.exe
RANLIB	= ranlib.exe
STRIP	= strip.exe

# Compiler options
CFLAGS	= -g -Wall -Werror -W -Wparentheses -Winline -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn
CFLAGS	+= -mcpu=i486 -march=i486 -O3
#CFLAGS	+= -mcpu=i386 -march=i386 -O3
#CFLAGS	+= -fomit-frame-pointer -fexpensive-optimizations -fgcse -frerun-loop-opt
#CFLAGS	+= -finline-limit-50000 # 10000 is default

# LIBPC98 target
LIBPC98	= ..\\libpc98\\libpc98.a
CFLAGS	+= -I..\\libpc98
LDFLAGS	= -g
DEPS	= \
	..\\libpc98\\funcs.h	\
	..\\libpc98\\gpuscrn.h	\
	..\\libpc98\\images.h	\
	..\\libpc98\\macros.h	\
	..\\libpc98\\maths.h	\
	..\\libpc98\\mouse.h	\
	..\\libpc98\\pcm.h		\
	..\\libpc98\\profile.h	\
	..\\libpc98\\progress.h	\
	..\\libpc98\\types.h 	\
	..\\libpc98\\utils.h	\
	# line left blank
