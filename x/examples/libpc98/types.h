#ifndef TYPES_H
#define TYPES_H

#include "macros.h"

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned u32;
typedef signed i32;
typedef unsigned long long u64;
typedef signed long long i64;

STATIC_ASSERT(sizeof(u8) == 1);
STATIC_ASSERT(sizeof(i8) == 1);
STATIC_ASSERT(sizeof(u16) == 2);
STATIC_ASSERT(sizeof(i16) == 2);
STATIC_ASSERT(sizeof(u32) == 4);
STATIC_ASSERT(sizeof(i32) == 4);
STATIC_ASSERT(sizeof(u64) == 8);
STATIC_ASSERT(sizeof(i64) == 8);

#endif
