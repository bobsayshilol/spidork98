CC	= gcc.exe
STRIP	= strip.exe
CFLAGS	= -mcpu=i386 -march=i386 -O3 -Wall
LIBS	= #-lm
DEPS	=
OBJS	= test.o
EXE	= basic.exe

%.o: %.cc $(DEPS)
	$(CC) -x c++ -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJS)
	$(CC) $(LIBS) -o $@ $^
	$(STRIP) $@

.PHONY: clean

clean:
	del $(OBJS) $(EXE)
