#ifndef IMAGES_H
#define IMAGES_H

//
// GPU must be inited first before using these functions.
//

#include "types.h"

#include <time.h>

namespace images {

// Upper 128 are reserved
#define IMAGES_MAX_PALETTE_SIZE 128

struct AnimatedGif;
struct AnimationData;
struct ImageData;
struct Palette;
struct SpriteScratch; // see images2.h

// Default palettes.
extern const Palette default_palette_16;
extern const Palette default_palette_32;
extern const Palette default_palette_64;

// Load a palette.
FASTCALL bool load_palette(Palette & palette, const char *path);

// Use the provided palette.
FASTCALL void set_palette(const Palette & palette);

// Draw an image at the given co-ordinates.
// x must be a multiple of 16.
FASTCALL void draw_image(u16 x, u16 y, ImageData const & data);

// Draw a sprite with a given mask.
FASTCALL void draw_sprite(u16 x, u16 y, ImageData const & sprite, ImageData const & mask);
FASTCALL void draw_sprite_64(u16 x, u16 y, ImageData const & sprite, ImageData const & mask, SpriteScratch & scratch);

// Itchy.
FASTCALL void free_scratch();
FASTCALL void free_scratch_64(SpriteScratch & scratch);

// A predefined palette to use.
struct Palette {
  u8 num_colours; // limited to IMAGES_MAX_PALETTE_SIZE
  u8 rgb[IMAGES_MAX_PALETTE_SIZE * 3];
};

// Precomputed image data.
struct ImageData {
  ImageData();
  ~ImageData();

  // Try and load a file.
  bool load(const char *path);

  // Reset image.
  void clear();

private:
  ImageData(ImageData const&);
  ImageData&operator=(ImageData const&);

public: // read only
  const u8 *m_data;
  u16 m_width;
  u16 m_height;
};

// Precomputed animation data.
struct AnimationData {
  AnimationData();
  ~AnimationData();

  // Try and load a file.
  bool load(const char *path);

  FORCEINLINE u16 width() const { return m_data.m_width; }
  FORCEINLINE u16 height() const { return m_data.m_height; }

private:
  AnimationData(AnimationData const&);
  AnimationData&operator=(AnimationData const&);

private:
  ImageData m_data;
  u8 m_num_frames;
  u8 m_fps;
  bool m_pingpong; // whether or not to play backwards on loop

  friend struct AnimatedGif;
};

struct AnimatedGif {
  // Create a gif at the screen co-ordinates with the given animation data.
  // x must be a multiple of 16.
  AnimatedGif(u16 x, u16 y, AnimationData const & data);

  // Tick the animation by tick count.
  // This will automatically redraw if necessary.
  FASTCALL void tick(uclock_t dt);

  // Advance to the next frame.
  // This will automatically redraw if necessary.
  FASTCALL void advance();

  // Draw the current frame of the animation.
  void draw();

private:
  AnimatedGif(AnimatedGif const&);
  AnimatedGif&operator=(AnimatedGif const&);

private:
  uclock_t m_ticks;
  uclock_t m_ticks_per_frame; // ticks_per_sec() / fps
  i16 m_frame_idx;
  i16 m_frame_dir;
  u16 m_x;
  u16 m_y;
  AnimationData const & m_data;
};

} // namespace images

#endif
