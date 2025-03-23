//
// Flame effect taken straight from every demo ever
//

#include "funcs.h"
#include "gpuscrn.h"
#include "profile.h"

#include <cstdio>
#include <cstdlib>

#include <conio.h>

// Add gaps between pixels for performance (and it looks nicer).
#define PIXEL_SPACING_X 2
#define PIXEL_SPACING_Y 2

namespace {

// Profiling sections.
PROFILE_DECLARE_SECTION(draw_hotspots);
PROFILE_DECLARE_SECTION(move_hotspots);
PROFILE_DECLARE_SECTION(draw_screen);

// The scanline we'll update to give the fire effect.
ALIGNAS(4) gpu::u8 scanline_data[GPU_WIDTH];

// Hotspot locations.
const int num_hotspots = 64;
int hotspot_locations[num_hotspots];

// Lookup table for the next iteration step of the main loop.
gpu::u8 decrement_if_positive_lookup[256];

// Inclusive range.
int rand_between(int a, int b) {
  return a + (rand() % (b - a + 1));
}

void setup_palette() {
  // We only need different shades of red... no, purple!
  // Taken from https://icolorpalette.com/color/a020f0#LightDarkshades
  const unsigned palette[64] = {
    0xf7ecfe,0xf4e4fd,0xf0ddfd,0xedd5fc,0xeacdfc,0xe7c6fb,0xe3befb,0xe0b6fa,
    0xddaffa,0xdaa7f9,0xd69ff9,0xd398f8,0xd090f8,0xcd88f7,0xc981f7,0xc679f6,
    0xc372f5,0xbf6af5,0xbc62f4,0xb95bf4,0xb653f3,0xb24bf3,0xaf44f2,0xac3cf2,
    0xa934f1,0xa52df1,0xa225f0,0x9f1df0,0x9c16ef,0x9810ed,0x930fe5,0x8e0fde,
    0x890ed6,0x840ece,0x7f0dc7,0x7b0dbf,0x760cb7,0x710cb0,0x6c0ba8,0x670ba1,
    0x620a99,0x5d0a91,0x58098a,0x530982,0x4e087a,0x4a0873,0x45076b,0x400763,
    0x3b065c,0x360654,0x31054c,0x2c0545,0x27043d,0x220436,0x1d032e,0x190326,
    0x14021f,0x0f0217,0x0a010f,0x050108,
  };
  // 64 entries (in reverse) but we'll just duplicate them.
  for (int i = 0; i < 64; i++) {
    const unsigned rgb = palette[63 - i];
    // Extract colours.
    const gpu::u8 r = (rgb >> 16) & 0xff;
    const gpu::u8 g = (rgb >>  8) & 0xff;
    const gpu::u8 b = (rgb >>  0) & 0xff;
    // Set the palette.
    gpu::set_palette_colour(4 * i + 0, r, g, b);
    gpu::set_palette_colour(4 * i + 1, r, g, b);
    gpu::set_palette_colour(4 * i + 2, r, g, b);
    gpu::set_palette_colour(4 * i + 3, r, g, b);
  }
}

void build_lookup_table() {
  // When we space out pixels in Y we need to jump further back.
  for (int i = 0; i < PIXEL_SPACING_Y; i++) {
    decrement_if_positive_lookup[i] = 0;
  }
  for (int i = PIXEL_SPACING_Y; i < 256; i++) {
    decrement_if_positive_lookup[i] = i - PIXEL_SPACING_Y;
  }
}

void setup_hotspots() {
  // Spread out the initial hotspots.
  for (int i = 0; i < num_hotspots; i++) {
    hotspot_locations[i] = i * GPU_WIDTH / num_hotspots;
  }
}

void draw_hotspots() {
  PROFILE_TIME_SECTION(draw_hotspots);

  // Blank out the bottom line.
  for (int x = 0; x < GPU_WIDTH; x++) {
      scanline_data[x] = 0;
  }

  // Fill in the current hotspots and their neighbours.
#define BLUR_WIDTH 20
#define FIRE_POWER 4
  for (int i = 0; i < num_hotspots; i++) {
    const int x = hotspot_locations[i];
    STATIC_ASSERT((BLUR_WIDTH % PIXEL_SPACING_X) == 0);
    for (int dx = -BLUR_WIDTH; dx <= BLUR_WIDTH; dx += PIXEL_SPACING_X) {
      const int adx = dx < 0 ? -dx : dx;
      int p = x + dx;
      // Handle wrap around.
      if (p < 0) p += GPU_WIDTH;
      if (p >= GPU_WIDTH) p -= GPU_WIDTH;
      // Add the contribution from this hotspot.
      STATIC_ASSERT(FIRE_POWER * BLUR_WIDTH < 255);
      const unsigned delta = FIRE_POWER * (BLUR_WIDTH - adx);
      unsigned pixel = scanline_data[p];
      pixel += delta;
      scanline_data[p] = pixel > 255 ? 255 : pixel;
    }
  }

  // Write it to the bottom line.
  gpu::write_scanline(GPU_HEIGHT - 1, scanline_data);
}

void move_hotspots() {
  PROFILE_TIME_SECTION(move_hotspots);

  const int move_speed = 2;
  for (int i = 0; i < num_hotspots; i++) {
    int & x = hotspot_locations[i];
    // Add a bias so they don't all move in the same direction.
    const int bias = i & 1;
    x += rand_between(-move_speed + bias - 1, move_speed + bias) * PIXEL_SPACING_X;
    STATIC_ASSERT((GPU_WIDTH % PIXEL_SPACING_X) == 0);
    if (x < 0) x += GPU_WIDTH;
    if (x >= GPU_WIDTH) x -= GPU_WIDTH;
  }
}

void draw_screen() {
  PROFILE_TIME_SECTION(draw_screen);

  // Draw from the top down, excluding the bottom line.
  // The maximum value is 255 so we only need to process that many lines.
  for (int ys = PIXEL_SPACING_Y; ys < 256; ys += PIXEL_SPACING_Y) {
    const int y = GPU_HEIGHT - 1 - ys;
    // Read the scanline data for the line below us.
    gpu::read_scanline(y + PIXEL_SPACING_Y, scanline_data);

    // Decrement the intensity of each pixel.
#if 0 // Basic implementation.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING_X) {
      if (scanline_data[x] >= PIXEL_SPACING_Y) {
        scanline_data[x] -= PIXEL_SPACING_Y;
      }
    }
#else // Unroll it 16 times for much better performance.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING_X * 16) {
#define DECREMENT_IF_POSITIVE(o) \
  { \
    gpu::u8 &pixel = scanline_data[(x) + PIXEL_SPACING_X * (o)]; \
    pixel = decrement_if_positive_lookup[pixel]; \
  }
      DECREMENT_IF_POSITIVE(0)  DECREMENT_IF_POSITIVE(1)  DECREMENT_IF_POSITIVE(2)  DECREMENT_IF_POSITIVE(3)
      DECREMENT_IF_POSITIVE(4)  DECREMENT_IF_POSITIVE(5)  DECREMENT_IF_POSITIVE(6)  DECREMENT_IF_POSITIVE(7)
      DECREMENT_IF_POSITIVE(8)  DECREMENT_IF_POSITIVE(9)  DECREMENT_IF_POSITIVE(10) DECREMENT_IF_POSITIVE(11)
      DECREMENT_IF_POSITIVE(12) DECREMENT_IF_POSITIVE(13) DECREMENT_IF_POSITIVE(14) DECREMENT_IF_POSITIVE(15)
    }
#endif

    // Write it back to the current line.
    gpu::write_scanline(y, scanline_data);
  }
}

template <typename Funcs>
void play() {
  if (!gpu::setup()) {
    printf("Failed to setup GPU\n");
    return;
  }
  Funcs::clear_screen();
  gpu::enable_text_layer(true);

  // Seed the RNG.
  srand(time(0));

  setup_palette();
  build_lookup_table();
  setup_hotspots();

  // Timings to calculate FPS.
  uclock_t last_time = Funcs::ticks();
  int num_frames = 0;

  while (true) {
    // Keep going until the user hits a button.
    if (Funcs::kb_hit()) {
      break;
    }

    draw_hotspots();
    move_hotspots();
    draw_screen();

    // Report the FPS once a second.
    const uclock_t now = Funcs::ticks();
    num_frames++;
    if (now - last_time > Funcs::ticks_per_sec()) {
      Funcs::clear_screen();
      const int fps = (num_frames * Funcs::ticks_per_sec() * 1000) / (now - last_time);
      printf("%iFPKS\n", fps);
      num_frames = 0;
      last_time = now;
      // Log the timings too.
      PROFILE_PRINT_TIMINGS();
    }
  }

  // Cleanup.
  gpu::shutdown();
  Funcs::clear_screen();
}

} // namespace

int main() {
  int m = __crt0_mtype;
  if (ISPCAT(m)) {
    printf("Can't run on PC-AT\n");
    return EXIT_FAILURE;
  } else if (ISPC98(m)) {
    play<Funcs98>();
  } else {
    printf("Unknown CPU type: %d\n", m);
    return EXIT_FAILURE;
  }

  printf("Finished\n");
  return EXIT_SUCCESS;
}
