/*
 * SoftFP Library
 * 
 * Copyright (c) 2016 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cutils.h"
#include "softfp.h"

#ifdef FE98_BUILD
static __inline__ int __builtin_clz(uint32_t a) {
  int r = 0;
  __asm__ __volatile__ (
    "bsr %1, %0\n"
    "xor $31, %0\n"
    : "=r" (r)
    : "rm" (a)
  );
  return r;
}

static __inline__ int __builtin_clzll(uint64_t a) {
  int r = 0;
  uint32_t bits = a >> 32;
  if (bits == 0) {
    bits = a;
    r = 32;
  }
  return r + __builtin_clz(bits);
}
#endif

static inline int clz32(uint32_t a)
{
    int r;
    if (a == 0) {
        r = 32;
    } else {
        r = __builtin_clz(a);
    }
    return r;
}

static inline int clz64(uint64_t a)
{
    int r;
    if (a == 0) {
        r = 64;
    } else 
    {
        r = __builtin_clzll(a);
    }
    return r;
}

#ifdef HAVE_INT128
static inline int clz128(uint128_t a)
{
    int r;
    if (a == 0) {
        r = 128;
    } else 
    {
        uint64_t ah, al;
        ah = a >> 64;
        al = a;
        if (ah != 0)
            r = __builtin_clzll(ah);
        else
            r = __builtin_clzll(al) + 64;
    }
    return r;
}
#endif

#define F_SIZE 32
#ifdef FE98_BUILD
#include "softfp_t.h"
#else
#include "softfp_template.h"
#endif

#define F_SIZE 64
#ifdef FE98_BUILD
#include "softfp_t.h"
#else
#include "softfp_template.h"
#endif

#ifdef HAVE_INT128

#define F_SIZE 128
#include "softfp_template.h"

#endif

