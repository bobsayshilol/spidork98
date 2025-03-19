CC	= gcc.exe
STRIP	= strip.exe
CFLAGS	= -mcpu=i386 -march=i386 -O3 -Wall
LIBS	= #-lm
DEPS	= funcs.h gpuscrn.h
OBJS	= main.o gpuscrn.o
EXE	= gfx.exe

%.o: %.cc $(DEPS)
	$(CC) -x c++ -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJS) $(LIBS)
	$(CC) -o $@ $^ $(LIBS)
	$(STRIP) $@

.PHONY: clean

clean:
	del $(OBJS) $(EXE)
