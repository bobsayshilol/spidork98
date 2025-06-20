#include "images.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "macros.h"

#include <cstdlib>

namespace images {

namespace {

const u8 s_invalid_data[16] = {
  0, 1, 2, 3, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15,
};
STATIC_ASSERT((COUNT_OF(s_invalid_data) % SCANLINE_PART_WIDTH_16) == 0);

//

FASTCALL void draw_internal(u16 x_start, u16 x_end, u16 y_start, u16 y_end, const u8 *data) {
  STATIC_ASSERT(SCANLINE_PART_WIDTH_16 == 16);
  x_start >>= 4;
  x_end >>= 4;

  for (u16 y = y_start; y < y_end; y++) {
    for (u16 x = x_start; x < x_end; x++) {
      gpu::write_scanline_part_16(y, x, data);
      data += SCANLINE_PART_WIDTH_16;
    }
  }
}

} // namespace

const Palette default_palette_16 = {
  16,
  {
    0x00, 0x00, 0x00,
    0x7F, 0x7F, 0x7F,
    0xBF, 0xBF, 0xBF,
    0xFF, 0xFF, 0xFF,

    0xFF, 0x00, 0x00,
    0x00, 0xFF, 0x00,
    0x00, 0x00, 0xFF,

    0x7F, 0x00, 0x00,
    0x00, 0x7F, 0x00,
    0x00, 0x00, 0x7F,

    0xFF, 0xFF, 0x00,
    0x00, 0xFF, 0xFF,
    0xFF, 0x00, 0xFF,

    0x7F, 0x7F, 0x00,
    0x00, 0x7F, 0x7F,
    0x7F, 0x00, 0x7F,
  }
};

//

FASTCALL void set_palette(const Palette & palette) {
  const int num_colours = palette.num_colours;
  const u8 *rgb = palette.rgb;
  for (int p = 0; p < num_colours; p++) {
    gpu::set_palette_colour(p, rgb[0], rgb[1], rgb[2]);
    rgb += 3;
  }
}

FASTCALL void draw_image(u16 x, u16 y, ImageData const & data) {
  draw_internal(x, x + data.m_width, y, y + data.m_height, data.m_data);
}

//

ImageData::ImageData()
  : m_data(s_invalid_data)
  , m_width(COUNT_OF(s_invalid_data)), m_height(1)
{
}

ImageData::~ImageData() {
  if (m_data != s_invalid_data) {
    free(const_cast<u8*>(m_data));
  }
}

bool ImageData::load() {
  // TODO
  return true;
}

//

AnimationData::AnimationData()
  : m_data()
  , m_num_frames(1), m_fps(1)
  , m_pingpong(false)
{
}

AnimationData::~AnimationData() {
}

bool AnimationData::load() {
  // TODO
  return true;
}

//

AnimatedGif::AnimatedGif(u16 x, u16 y, AnimationData const & data)
  : m_ticks(0), m_ticks_per_frame(Funcs98::ticks_per_sec() / data.m_fps)
  , m_frame_idx(0), m_frame_dir(1)
  , m_x(x), m_y(y)
  , m_data(data)
{
}

FASTCALL void AnimatedGif::tick(uclock_t dt) {
  m_ticks += dt;
  if (m_ticks > m_ticks_per_frame) {
    m_ticks -= m_ticks_per_frame;
    advance();
  }
}

FASTCALL void AnimatedGif::advance() {
  m_frame_idx += m_frame_dir;
  if (m_frame_idx < 0 || m_frame_idx >= m_data.m_num_frames) {
    if (m_data.m_pingpong) {
      m_frame_dir = -m_frame_dir;
      m_frame_idx += m_frame_dir + m_frame_dir;
    } else {
      m_frame_idx = 0;
    }
    draw();
  }
}

void AnimatedGif::draw() {
  const u32 width = m_data.m_data.m_width;
  const u32 height = m_data.m_data.m_height;
  const u32 offset = width * height * m_frame_idx;
  draw_internal(
    m_x, m_x + width,
    m_y, m_y + height,
    m_data.m_data.m_data + offset
  );
}

} // namespace images

