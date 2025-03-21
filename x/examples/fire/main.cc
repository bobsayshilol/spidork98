//
// Flame effect taken straight from every demo ever
//

#include "funcs.h"
#include "gpuscrn.h"

#include <cstdio>
#include <cstdlib>

#include <conio.h>

namespace {

// The scanline we'll update to give the fire effect.
gpu::u8 scanline_data[GPU_WIDTH];

// Hotspot locations.
const int num_hotspots = 20;
int hotspot_locations[num_hotspots];

void draw_hotspots() {
  // Blank out the bottom line.
  for (int x = 0; x < GPU_WIDTH; x++) {
      scanline_data[x] = 0;
  }
  
  // Fill in the current hotspots and their neighbours.
  const int blur_width = 10;
  const int blur_intensity[blur_width + 1] = {
    255, 200, 160, 130, 110, 100, 95, 90, 85, 80, 75,
  };
  for (int i = 0; i < num_hotspots; i++) {
    const int x = hotspot_locations[i];
    for (int dx = -blur_width; dx <= blur_width; dx++) {
      const int p = x + dx;
      if (0 <= p && p < GPU_WIDTH) {
        scanline_data[p] = blur_intensity[abs(dx)];
      }
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
  const int move_speed = 3;
  for (int i = 0; i < num_hotspots; i++) {
    int & x = hotspot_locations[i];
    x += rand_between(-move_speed, move_speed);
    if (x < 0) x += GPU_WIDTH;
    if (x >= GPU_WIDTH) x -= GPU_WIDTH;
  }
}

void draw_screen() {
  // Draw from the top down, excluding the bottom line.
  // The maximum value is 255 so we only need to process that many lines.
  for (int y = GPU_HEIGHT - 256; y < GPU_HEIGHT - 1; y++) {
    // Read the scanline data for the line below us.
    gpu::read_scanline(y + 1, scanline_data);

    // Decrement the intensity of each pixel.
    for (int x = 0; x < GPU_WIDTH; x++) {
      if (scanline_data[x] > 0) {
        scanline_data[x] -= 1;
      }
    }

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

  // Randomise the initial hotspots.
  srand(time(0));
  for (int i = 0; i < num_hotspots; i++) {
    hotspot_locations[i] = rand_between(0, GPU_WIDTH - 1);
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
  clock_t last_time = Funcs::now();
  int num_frames = 0;

  while (true) {
    // Keep going until the user hits a button.
    if (Funcs::kb_hit()) {
      break;
    }

    draw_hotspots();
    move_hotspots();
    draw_screen();

    // Report the FPS.
    const clock_t now = Funcs::now();
    num_frames++;
    if (now - last_time > CLOCKS_PER_SEC) {
      Funcs::clear_screen();
      const int fps = (num_frames * CLOCKS_PER_SEC * 1000) / (now - last_time);
      printf("%iFPKS\n", fps);
      num_frames = 0;
      last_time = now;
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
