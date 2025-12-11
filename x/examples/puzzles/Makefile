# Common flags
include ..\\libpc98\\flags.mk


CFLAGS += -DCOMBINED -DNO_TGMATH_H -DNO_STDINT_H
CFLAGS += -DFE98_BUILD
CFLAGS += -Ipuzzles
CFLAGS += -DNDEBUG # TODO

PUZZLE_DEPS = \
	puzzles/grid.h \
	puzzles/gtk.h \
	puzzles/hat.h \
	puzzles/hat-int.h \
	puzzles/hat-tab.h \
	puzzles/latin.h \
	puzzles/loopgen.h \
	puzzles/matching.h \
	puzzles/penrose.h \
	puzzles/pen-int.h \
	puzzles/pen-leg.h \
	puzzles/puzzles.h \
	puzzles/spectre.h \
	puzzles/spec-int.h \
	puzzles/spectaba.h \
	puzzles/spectabm.h \
	puzzles/tree234.h \
	puzzles/version.h \
	\
	puzzles/stdbool.h

PUZZLE_COMMON_OBJS = \
	puzzles/combi.o \
	puzzles/divvy.o \
	puzzles/draw-ply.o \
	puzzles/drawing.o \
	puzzles/dsf.o \
	puzzles/findloop.o \
	puzzles/grid.o \
	puzzles/hat.o \
	puzzles/latin.o \
	puzzles/laydomin.o \
	puzzles/loopgen.o \
	puzzles/malloc.o \
	puzzles/matching.o \
	puzzles/midend.o \
	puzzles/misc.o \
	puzzles/penrose.o \
	puzzles/pen-leg.o \
	puzzles/ps.o \
	puzzles/random.o \
	puzzles/sort.o \
	puzzles/spectre.o \
	puzzles/tdq.o \
	puzzles/tree234.o \
	puzzles/version.o

PUZZLE_GAME_OBJS = \
	puzzles/mines.o \
	puzzles/flood.o


# What we're building
DEPS	+= $(LIBPC98) $(PUZZLE_DEPS)
OBJS	= fe98.o $(PUZZLE_COMMON_OBJS) $(PUZZLE_GAME_OBJS)
TARGET	= puzzles.exe


# Build it
include ..\\libpc98\\exe.mk
