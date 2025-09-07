# Common flags
include ..\\libpc98\\flags.mk


CFLAGS += -DCOMBINED

PUZZLE_DEPS = \
	puzzles/grid.h \
	puzzles/gtk.h \
	puzzles/hat.h \
	puzzles/hat-internal.h \
	puzzles/hat-tables.h \
	puzzles/latin.h \
	puzzles/loopgen.h \
	puzzles/matching.h \
	puzzles/penrose.h \
	puzzles/penrose-internal.h \
	puzzles/penrose-legacy.h \
	puzzles/puzzles.h \
	puzzles/spectre.h \
	puzzles/spectre-internal.h \
	puzzles/spectre-tables-auto.h \
	puzzles/spectre-tables-manual.h \
	puzzles/tree234.h \
	puzzles/version.h

PUZZLE_COMMON_OBJS = \
	puzzles/combi.o \
	puzzles/divvy.o \
	puzzles/draw-poly.o \
	puzzles/drawing.o \
	puzzles/dsf.o \
	puzzles/findloop.o \
	puzzles/grid.o \
	puzzles/hat.o \
	puzzles/latin.o \
	puzzles/laydomino.o \
	puzzles/loopgen.o \
	puzzles/malloc.o \
	puzzles/matching.o \
	puzzles/midend.o \
	puzzles/misc.o \
	puzzles/penrose.o \
	puzzles/penrose-legacy.o \
	puzzles/ps.o \
	puzzles/random.o \
	puzzles/sort.o \
	puzzles/spectre.o \
	puzzles/tdq.o \
	puzzles/tree234.o \
	puzzles/version.c

PUZZLE_GAME_OBJS = \
	puzzles/mines.o


# What we're building
DEPS	+= $(LIBPC98) $(PUZZLE_DEPS)
OBJS	= fe98.o $(PUZZLE_COMMON_OBJS) $(PUZZLE_GAME_OBJS)
TARGET	= puzzles.exe


# Build it
include ..\\libpc98\\exe.mk
