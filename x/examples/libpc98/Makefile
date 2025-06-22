# Common flags
include ..\\libpc98\\flags.mk

# What we're building
DEPS	+=
OBJS	= gpuscrn.o profile.o sincos.o pcm.o images.o
# pcmi.o
TARGET	= libpc98.a

# Build it
include ..\\libpc98\\lib.mk


# Build the tools too
tools: sc_gen.exe sc_test.exe snd_conv.exe ;

sc_gen.exe sc_gen.cc:
	gcc -x c++ sc_gen.cc -o sc_gen.exe

sc_test.exe: $(TARGET) sc_test.cc
	gcc -x c++ -c sc_test.cc -o sc_test.o
	gcc sc_test.o sincos.o -o sc_test.exe

snd_conv.exe: $(TARGET) snd_conv.cc
	gcc -x c++ snd_conv.cc -o snd_conv.exe
