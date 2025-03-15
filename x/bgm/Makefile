CC	= gcc.exe
STRIP	= strip.exe
CFLAGS	= -mcpu=i386 -march=i386 -O3 -Wall
LIBS	= #-lm
DEPS	= bgm.h funcs.h notes.h
OBJS	= main.o bgm.o
EXE	= bgm.exe

%.o: %.cc $(DEPS)
	$(CC) -x c++ -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJS)
	$(CC) $(LIBS) -o $@ $^
	$(STRIP) $@

.PHONY: clean

clean:
	del $(OBJS) $(EXE)
