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
#define PIXEL_SPACING 2

namespace {

// Profiling sections.
PROFILE_DECLARE_SECTION(draw_hotspots);
PROFILE_DECLARE_SECTION(move_hotspots);
PROFILE_DECLARE_SECTION(draw_screen);

// The scanline we'll update to give the fire effect.
ALIGNAS(4) gpu::u8 scanline_data[GPU_WIDTH];

// Hotspot locations.
const int num_hotspots = 32;
int hotspot_locations[num_hotspots];

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
    STATIC_ASSERT((BLUR_WIDTH % PIXEL_SPACING) == 0);
    for (int dx = -BLUR_WIDTH; dx <= BLUR_WIDTH; dx += PIXEL_SPACING) {
      const int adx = dx < 0 ? -dx : dx;
      int p = x + dx;
      // Handle wrap around.
      if (p < 0) p += GPU_WIDTH;
      if (p >= GPU_WIDTH) p -= GPU_WIDTH;
      // Add the contribution from this hotspot.
      STATIC_ASSERT(FIRE_POWER * BLUR_WIDTH < 255);
      const unsigned delta = FIRE_POWER * BLUR_WIDTH - adx;
      unsigned pixel = scanline_data[p];
      pixel += delta;
      scanline_data[p] = pixel > 255 ? 255 : pixel;
    }
  }

  // Write it to the bottom line.
  gpu::write_scanline(GPU_HEIGHT - 1, scanline_data);
}

// Inclusive range.
int rand_between(int a, int b) {
  return a + (rand() % (b - a + 1));
}

void move_hotspots() {
  PROFILE_TIME_SECTION(move_hotspots);

  const int move_speed = 2;
  for (int i = 0; i < num_hotspots; i++) {
    int & x = hotspot_locations[i];
    x += rand_between(-move_speed, move_speed) * PIXEL_SPACING;
    STATIC_ASSERT((GPU_WIDTH % PIXEL_SPACING) == 0);
    if (x < 0) x += GPU_WIDTH;
    if (x >= GPU_WIDTH) x -= GPU_WIDTH;
  }
}

// Lookup table for the next iteration step of the main loop.
gpu::u8 decrement_if_positive_lookup[256];

void draw_screen() {
  PROFILE_TIME_SECTION(draw_screen);

  // Draw from the top down, excluding the bottom line.
  // The maximum value is 255 so we only need to process that many lines.
  for (int y = GPU_HEIGHT - 256; y < GPU_HEIGHT - 1; y++) {
    // Read the scanline data for the line below us.
    gpu::read_scanline(y + 1, scanline_data);

    // Decrement the intensity of each pixel.
#if 0 // Basic implementation.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING) {
      if (scanline_data[x] > 0) {
        scanline_data[x] -= 1;
      }
    }
#else // Unroll it 16 times for much better performance.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING * 16) {
#define DECREMENT_IF_POSITIVE(o) \
  { \
    gpu::u8 &pixel = scanline_data[(x) + PIXEL_SPACING * (o)]; \
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

  // Build lookup table.
  decrement_if_positive_lookup[0] = 0;
  for (int i = 1; i < 256; i++) {
    decrement_if_positive_lookup[i] = i - 1;
  }

  // Spread out the initial hotspots.
  for (int i = 0; i < num_hotspots; i++) {
    hotspot_locations[i] = i * GPU_WIDTH / num_hotspots;
  }

  // We only need different shades of red... no, purple!
  const int r = 0xA0;
  const int g = 0x20;
  const int b = 0xF0;
  for (int i = 0; i < 256; i++) {
    gpu::set_palette_colour(i,
      (i * r) >> 8,
      (i * g) >> 8,
      (i * b) >> 8
    );
  }

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
