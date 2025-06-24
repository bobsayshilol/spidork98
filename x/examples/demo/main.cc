#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"

#include <cstdio>
#include <cstdlib>

#include <conio.h>

namespace {

void play() {
  if (!gpu::setup()) {
    printf("Failed to setup GPU\n");
    return;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

  // Remove any text.
  Funcs98::clear_screen();
  gpu::enable_text_layer(true); // log text for FPS

  // Load the palette.
  images::Palette bg_palette;
  if (!images::load_palette(bg_palette, "bg.pal")) {
    printf("Failed to load palette\n");
    return;
  }

  // Load the background.
  images::ImageData bg_data;
  if (!bg_data.load("bg.img")) {
    printf("Failed to load background data\n");
    return;
  }

  // Setup the palette that we'll slowly fade in.
  images::Palette lerped_palette = {bg_palette.num_colours, {}};
  images::set_palette(lerped_palette);
  int pal_idx = 0;
  const uclock_t fade_time = Funcs98::ticks_per_sec() / 2;
  uclock_t pal_timer = 0;

  // Draw it the first time.
  gpu::wait_for_vsync();
  images::draw_image(0, 0, bg_data);

  // Timing tracking.
  uclock_t last_time = Funcs98::ticks();

  int num_frames = 0;
  uclock_t num_ticks = 0;

  bool vsync = false;
  while (true) {
    // Break on user input.
    if (Funcs98::kb_hit()) {
      const char c = getch();
      if (c == 'v' || c == 'V') {
        vsync = !vsync;
      } else if (c == 'q' || c == 'Q' || c == 27) {
        break;
      }
    }

    // Calculate delta.
    const uclock_t now = Funcs98::ticks();
    const uclock_t dt = now - last_time;
    last_time = now;

    // Draw FPS if it's been a while.
    num_frames++;
    num_ticks += dt;
    if (num_ticks > Funcs98::ticks_per_sec()) {
      Funcs98::clear_screen();
      const unsigned fpks = (num_frames * Funcs98::ticks_per_sec() * 1000) / num_ticks;
      printf("%u.%uFPS\n", fpks / 1000, (fpks / 100) % 10);
      num_frames = 0;
      num_ticks = 0;
    }

    // Wait for vsync.
    if (vsync) {
      gpu::wait_for_vsync();
    }

    // Fade each colour in.
    if (pal_idx < lerped_palette.num_colours) {
      const u8 *bg_rgb = &bg_palette.rgb[3 * pal_idx];
      u8 *lerp_rgb = &lerped_palette.rgb[3 * pal_idx];

      pal_timer += dt;
      if (pal_timer > fade_time) {
        // Make colour full when ticking over to the next.
        lerp_rgb[0] = bg_rgb[0];
        lerp_rgb[1] = bg_rgb[1];
        lerp_rgb[2] = bg_rgb[2];

        pal_timer = 0;
        pal_idx++;
        if (pal_idx == lerped_palette.num_colours) {
          continue;
        }

        // Next colour.
        bg_rgb += 3;
        lerp_rgb += 3;
      }

      // Update colour.
      lerp_rgb[0] = (bg_rgb[0] * pal_timer) / fade_time;
      lerp_rgb[1] = (bg_rgb[1] * pal_timer) / fade_time;
      lerp_rgb[2] = (bg_rgb[2] * pal_timer) / fade_time;
      images::set_palette(lerped_palette); // could do just this colour...
    }
  }
}

} // namespace

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  play();

  printf("Finished\n");
  return EXIT_SUCCESS;
}
