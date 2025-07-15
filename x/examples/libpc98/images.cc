#include "images.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "logs.h"
#include "macros.h"
#include "memory.h"

#include <cstdlib>
#include <cstdio>

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

//

struct ImgType { enum E { Image, Anim }; };

bool load_common(const char *path, ImgType::E type, u16 &width, u16 &height, u16 &flags, u8 *&data, u8 &fps, u8 &num_frames) {
  FILE * input = fopen(path, "rb");
  if (!input) {
    logging::print(logging::Level::Error, "Failed to open %s", path);
    return false;
  }
  DEFER(FILE *, p, input, fclose(p));

  // Check magic.
  u32 magic = 0;
  if (fread(&magic, 4, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  } else if (magic == FOURCC('I', 'M', '9', '8')) {
    if (type != ImgType::Image) {
      logging::print(logging::Level::Error, "File isn't an image: %s", path);
      return false;
    }

    fps = 0;
    num_frames = 1;

  } else if (magic == FOURCC('A', 'N', '9', '8')) {
    if (type != ImgType::Anim) {
      logging::print(logging::Level::Error, "File isn't an animation: %s", path);
      return false;
    }

    // Read the fps and frame count.
    if (fread(&fps, 1, 1, input) != 1 || fread(&num_frames, 1, 1, input) != 1) {
      logging::print(logging::Level::Error, "Failed to read %s", path);
      return false;
    }

  } else {
    logging::print(logging::Level::Error, "File isn't an image or animation: %s", path);
    return false;
  }

  // Read off width, height, flags.
  if (fread(&width, 2, 1, input) != 1 || fread(&height, 2, 1, input) != 1 || fread(&flags, 2, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  }

  // Allocate space for the image data.
  const unsigned size = width * static_cast<unsigned>(height) * num_frames;
  data = memory::alloc4<u8>(size);
  if (!data) {
    logging::print(logging::Level::Error, "Failed to allocate space for image: %s", path);
    return false;
  }

  // Read it in.
  if (fread(data, 1, size, input) != size) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    memory::free4(data);
    data = 0;
    return false;
  }

  // All done.
  logging::print(logging::Level::Info, "Loaded image/animation %s", path);
  return true;
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

// Taken from https://lospec.com/palette-list/vivid-ega-64
const Palette default_palette_64 = {
  64,
  {
    0xFF, 0xFF, 0xFF,
    0xD5, 0xD5, 0xD5,
    0xAA, 0xAA, 0xAA,
    0x7F, 0x7F, 0x7F,
    0x55, 0x55, 0x55,
    0x2A, 0x2A, 0x2A,
    0x00, 0x00, 0x01,
    0x2A, 0x00, 0x00,
    0x55, 0x00, 0x00,
    0x7F, 0x00, 0x00,
    0xAA, 0x00, 0x00,
    0xFF, 0x00, 0x00,
    0xFF, 0x7F, 0x7F,
    0xFF, 0xAA, 0xAA,
    0xFF, 0xAA, 0x7F,
    0xD5, 0x7F, 0x55,
    0x98, 0x4F, 0x08,
    0x55, 0x2A, 0x00,
    0x85, 0x41, 0x15,
    0xCE, 0x58, 0x1D,
    0xFF, 0x55, 0x00,
    0xFF, 0x7F, 0x00,
    0xFF, 0xAA, 0x00,
    0xFF, 0xD4, 0x00,
    0xFF, 0xFF, 0x00,
    0xD5, 0xFF, 0x00,
    0xAA, 0xFF, 0x00,
    0x86, 0xAF, 0x17,
    0x7F, 0x7F, 0x00,
    0x55, 0x55, 0x00,
    0x2A, 0x2A, 0x00,
    0x00, 0x35, 0x00,
    0x00, 0x7A, 0x00,
    0x00, 0xC6, 0x00,
    0x0B, 0xFF, 0x0B,
    0x6E, 0xF7, 0xB2,
    0xA0, 0xF4, 0xD1,
    0xAA, 0xFF, 0xFF,
    0x00, 0xFF, 0xFF,
    0x00, 0xC2, 0xC6,
    0x00, 0x7F, 0x7F,
    0x00, 0x55, 0x55,
    0x01, 0x84, 0x94,
    0x01, 0x99, 0xCA,
    0x02, 0x53, 0x98,
    0x04, 0x2C, 0x72,
    0x00, 0x00, 0x55,
    0x00, 0x00, 0xAA,
    0x00, 0x00, 0xFF,
    0x2D, 0x2D, 0xF7,
    0x57, 0x57, 0xFF,
    0xAA, 0xAA, 0xFF,
    0xCD, 0xAB, 0xFB,
    0xA4, 0x61, 0xFF,
    0x55, 0x00, 0xAA,
    0x2A, 0x00, 0x2A,
    0x55, 0x00, 0x55,
    0x7F, 0x00, 0x7F,
    0xAA, 0x00, 0xAA,
    0xFF, 0x00, 0xFF,
    0xFF, 0x82, 0xFF,
    0xFF, 0xAA, 0xD5,
    0xFF, 0xD5, 0xD5,
    0xFB, 0xED, 0xED,
  }
};

//

FASTCALL bool load_palette(Palette & palette, const char *path) {
  palette.num_colours = 0;

  FILE * input = fopen(path, "rb");
  if (!input) {
    logging::print(logging::Level::Error, "Failed to open %s", path);
    return false;
  }
  DEFER(FILE *, p, input, fclose(p));

  // Check magic.
  u32 magic = 0;
  if (fread(&magic, 4, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  } else if (magic != FOURCC('P', 'L', '9', '8')) {
    logging::print(logging::Level::Error, "File isn't an palette: %s", path);
    return false;
  }

  // Read off count.
  if (fread(&palette.num_colours, 1, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  } else if (palette.num_colours > IMAGES_MAX_PALETTE_SIZE) {
    logging::print(logging::Level::Error, "Invalid colour count in palette: %u", palette.num_colours);
    return false;
  }

  // Read off data.
  if (fread(palette.rgb, 3, palette.num_colours, input) != palette.num_colours) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  }

  // Done.
  logging::print(logging::Level::Info, "Loaded palette %s", path);
  return true;
}

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
  clear();
}

void ImageData::clear() {
  if (m_data != s_invalid_data) {
    memory::free4(const_cast<u8*>(m_data));
    m_data = s_invalid_data;
    m_width = COUNT_OF(s_invalid_data);
    m_height = 1;
  }
}

bool ImageData::load(const char *path) {
  clear();

  u16 w = 0, h = 0, flags = 0;
  u8 *data = 0;
  u8 fps = 0, num_frames = 0;
  if (!load_common(path, ImgType::Image, w, h, flags, data, fps, num_frames)) {
    return false;
  }

  // All done.
  m_width = w;
  m_height = h;
  m_data = data;
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

bool AnimationData::load(const char *path) {
  m_data.clear();

  u16 w = 0, h = 0, flags = 0;
  u8 *data = 0;
  u8 fps = 0, num_frames = 0;
  if (!load_common(path, ImgType::Anim, w, h, flags, data, fps, num_frames)) {
    return false;
  }

  // All done.
  m_data.m_width = w;
  m_data.m_height = h;
  m_data.m_data = data;
  m_num_frames = num_frames;
  m_fps = fps;
  m_pingpong = flags & 1;
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
  }
  draw();
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

