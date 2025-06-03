#ifndef MATHS_H
#define MATHS_H

#include "macros.h"

namespace maths {

extern "C" const signed char maths_sin_lookup[256];
extern "C" const signed char maths_cos_lookup[256];

// sin(2 pi x / 256)
// Returns in range [-128,127]
static FORCEINLINE signed char sin(unsigned char x) {
  return maths_sin_lookup[x];
}

// cos(2 pi x / 256)
// Returns in range [-128,127]
static FORCEINLINE signed char cos(unsigned char x) {
  return maths_cos_lookup[x];
}

} // namespace

#endif
