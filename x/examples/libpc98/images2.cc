#include "images.h"
#include "images2.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "logs.h"
#include "macros.h"
#include "maths.h"
#include "memory.h"
#include "utils.h"

#include <cstdlib>
#include <cstdio>

namespace images {

FASTCALL void draw_sprite_64(u16 img_x, u16 img_y, ImageData const & sprite, ImageData const & mask, SpriteScratch & scratch) {
  // Determine where the back buffer for this sprite.
  STATIC_ASSERT(SCANLINE_PART_WIDTH_32 == 32);
  const u16 scratch_w = SCANLINE_PART_WIDTH_32 + SCANLINE_PART_WIDTH_32;
  const u16 scratch_h = SCANLINE_PART_WIDTH_32 + SCANLINE_PART_WIDTH_32;
  const u16 x_base = img_x & ~static_cast<u16>(SCANLINE_PART_WIDTH_32 - 1);
  const u16 y_base = img_y & ~static_cast<u16>(SCANLINE_PART_WIDTH_32 - 1);
  const u16 offset_x = img_x - x_base;
  const u16 offset_y = img_y - y_base;
  const u16 img_w = sprite.m_width;
  const u16 img_h = sprite.m_height;

  // Allocate if required.
  if (!scratch.data) {
    // Sanity check in here since it shouldn't happen often.
    if (sprite.m_width != mask.m_width || sprite.m_height != mask.m_height) {
      logging::print(logging::Level::Error, "Mask data doesn't match sprite data");
      return;
    } else if (sprite.m_width > 32 || sprite.m_height > 32) {
      logging::print(logging::Level::Error, "Sprite width or height is more than 32x32: %u x %u", sprite.m_width, sprite.m_height);
      return;
    }

    const u32 full_size = scratch_w * scratch_h;
    scratch.data = memory::alloc4<u8>(full_size);
    if (!scratch.data) {
      logging::print(logging::Level::Error, "Failed to allocate temporary sprite buffer");
      return;
    }
  }

  // Copy the backbuffer to the scratch buffer if required.
  if ( (scratch.x_base != x_base) | (scratch.y_base != y_base))
  {
    const u16 last_y = utils::min(y_base + scratch_h, GPU_HEIGHT);
    STATIC_ASSERT((GPU_WIDTH % scratch_w) == 0);

    gpu::g_draw_to = gpu::DrawTo::Back;
    u8 *part_data = scratch.data;
    for (u16 line = y_base; line < last_y; line++) {
      for (u16 part = x_base >> 5; part < (x_base + scratch_w) >> 5; part++) {
        gpu::read_scanline_part_32(line, part, part_data);
        part_data += SCANLINE_PART_WIDTH_32;
      }
    }
    gpu::g_draw_to = gpu::DrawTo::Front;

    scratch.x_base = x_base;
    scratch.y_base = y_base;
  }

  // Mask the backbuffer and add the sprite.
  {
    const u8 * mask_data = mask.m_data;
    const u8 * sprite_data = sprite.m_data;
    const u8 * back_data = scratch.data + offset_y * scratch_w;
    u8 temp_scanline[scratch_w];
    for (u32 j = 0; j < img_h; j++) {
      const u8 * line_data = back_data;
      u8 * output = temp_scanline;

      // Blanket copy of first part.
#if 0  // unrolled for perf
      for (u32 k1 = 0; k1 < offset_x; k1++)
#else
      for (u32 k0 = 0; k0 < (offset_x & ~3U); k0 += 4) {
        *reinterpret_cast<u32*>(output) = *reinterpret_cast<const u32*>(line_data);
        output += 4;
        line_data += 4;
      }
      for (int k1 = 0; k1 < (offset_x & 3); k1++)
#endif
      {
        *output++ = *line_data++;
      }

      // TODO: output and line_data will be unaligned if offset_x isn't a multiple of 4...

      // If mask is set, select the sprite. Otherwise select the backbuffer.
      for (u32 u = 0; u < (img_w & ~3U); u += 4) { // unrolled for perf
        const u32 m = *reinterpret_cast<const u32*>(mask_data); mask_data += 4;
        const u32 s = *reinterpret_cast<const u32*>(sprite_data); sprite_data += 4;
        u32 pal = *reinterpret_cast<const u32*>(line_data); line_data += 4; // UNALIGNED
        pal = (pal & ~m) | (s & m);
        *reinterpret_cast<u32*>(output) = pal; output += 4; // UNALIGNED
      }
      for (int i = 0; i < (img_w & 3); i++) {
        const u8 m = *mask_data++;
        const u8 s = *sprite_data++;
        u8 pal = *line_data++;
        pal = (pal & ~m) | (s & m);
        *output++ = pal;
      }

      // Blanket copy of remaining area.
      const u16 remaining = scratch_w - (offset_x + img_w);
#if 0  // unrolled for perf
      for (u32 k3 = 0; k3 < remaining; k3++)
#else
      for (u32 k2 = 0; k2 < (remaining & ~3U); k2 += 4) {
        *reinterpret_cast<u32*>(output) = *reinterpret_cast<const u32*>(line_data); // UNALIGNED
        output += 4;
        line_data += 4;
      }
      for (int k3 = 0; k3 < (remaining & 3); k3++)
#endif
      {
        *output++ = *line_data++;
      }

      // Write the scanlines to the front buffer.
      const u32 scan_x = x_base >> 5;
      const u32 scan_y = img_y + j;
      gpu::write_scanline_part_32(scan_y, scan_x, temp_scanline);
      gpu::write_scanline_part_32(scan_y, scan_x + 1, temp_scanline + SCANLINE_PART_WIDTH_32);

      back_data += scratch_w;
    }
  }
}

FASTCALL void free_scratch_64(SpriteScratch & scratch) {
  memory::free4(scratch.data);
  scratch.data = 0;
  scratch.x_base = ~0;
  scratch.y_base = ~0;
}

} // namespace images

