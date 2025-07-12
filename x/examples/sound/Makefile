# Common flags
include ..\\libpc98\\flags.mk

# What we're building
DEPS	+= $(LIBPC98)
OBJS	= main.o
TARGET	= sound.exe

# Copy from the bgm example
CFLAGS	+= -I..\\bgm

# Build it
include ..\\libpc98\\exe.mk
