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

  // Set the palette for the animations.
  images::set_palette(images::default_palette_16);

  // Load the animation.
  images::AnimationData anim_data;
  if (!anim_data.load("ayy.ani")) {
    printf("Failed to load animated sprite data\n");
    return;
  }

  // Make some sprites.
  images::AnimatedGif gif1(320 - 32, 100, anim_data);
  images::AnimatedGif gif2(160 - 32, 100, anim_data);
  images::AnimatedGif gif3(480 - 32, 100, anim_data);

  // Draw it the first time.
  gpu::wait_for_vsync();
  gif1.draw();
  gif2.draw();
  gif3.draw();

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

    // Advance the gifs.
    gif1.tick(dt);
    gif2.tick(dt);
    gif3.tick(dt);
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
