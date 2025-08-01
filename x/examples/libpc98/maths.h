#ifndef MATHS_H
#define MATHS_H

#include "macros.h"
#include "types.h"

#ifdef WEB_BUILD
#include <cmath>
#endif

namespace maths {

#ifndef WEB_BUILD

extern "C" const signed char maths_sin_lookup[256];
extern "C" const signed char maths_cos_lookup[256];

// sin(2 pi x / 256)
// Returns in range [-128,127]
static FORCEINLINE i8 sin(u8 x) {
  return maths_sin_lookup[x];
}

// cos(2 pi x / 256)
// Returns in range [-128,127]
static FORCEINLINE i8 cos(u8 x) {
  return maths_cos_lookup[x];
}

#else

static FORCEINLINE i8 sin(u8 x) {
  // Same as sc_gen.cc
  float f = ::sin(x * (2 * 3.14159f / 256));
  return static_cast<int>(127 * f);
}

static FORCEINLINE i8 cos(u8 x) {
  return sin(x + 64);
}

#endif

// Pad to a power of 2.
template <u32 Pad>
static FORCEINLINE u32 pad_to(u32 x) {
  STATIC_ASSERT((Pad & (Pad - 1)) == 0);
  return ((x - 1) | (Pad - 1)) + 1;
}

} // namespace

#endif
