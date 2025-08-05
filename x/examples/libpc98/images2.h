#ifndef IMAGES2_H
#define IMAGES2_H

// Had to be broken out to avoid an internal compiler error

#include "types.h"

namespace images {

struct SpriteScratch {
  u8 *data;
  u16 x_base;
  u16 y_base;
};

} // namespace images

#endif
