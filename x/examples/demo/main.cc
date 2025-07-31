#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>

namespace {

struct State {
  enum E {
    IntroFadeIn,
    IntroFadeOut,
    AnimFadeIn,
    AnimPlay,
  };
};

struct FadeInPalette {
  FadeInPalette() : m_palette_idx(0), m_palette_timer(0) {}

  bool load(const char *path) {
    if (!images::load_palette(m_bg_palette, path)) {
      printf("Failed to load palette\n");
      return false;
    }

    m_lerped_palette.num_colours = m_bg_palette.num_colours;
    memset(m_lerped_palette.rgb, 0, sizeof(m_lerped_palette.rgb));
    images::set_palette(m_lerped_palette);
    return true;
  }

  bool fade_in_single(uclock_t dt) {
    if (m_palette_idx >= m_lerped_palette.num_colours) {
      return false;
    }

    const uclock_t fade_time = Funcs98::ticks_per_sec() / 2;

    const u8 *bg_rgb = &m_bg_palette.rgb[3 * m_palette_idx];
    u8 *lerp_rgb = &m_lerped_palette.rgb[3 * m_palette_idx];

    m_palette_timer += dt;
    if (m_palette_timer > fade_time) {
      // Make colour full when ticking over to the next.
      lerp_rgb[0] = bg_rgb[0];
      lerp_rgb[1] = bg_rgb[1];
      lerp_rgb[2] = bg_rgb[2];

      m_palette_timer = 0;
      m_palette_idx++;

    } else {
      // Update current colour.
      lerp_rgb[0] = (bg_rgb[0] * m_palette_timer) / fade_time;
      lerp_rgb[1] = (bg_rgb[1] * m_palette_timer) / fade_time;
      lerp_rgb[2] = (bg_rgb[2] * m_palette_timer) / fade_time;
    }

    images::set_palette(m_lerped_palette); // could do just this colour...
    return true;
  }

  bool fade_out_all(uclock_t dt) {
    if (m_palette_idx == 0) {
      return false;
    }

    const uclock_t fade_time = Funcs98::ticks_per_sec() / 2;

    const u8 *bg_rgb = m_bg_palette.rgb;
    u8 *lerp_rgb = m_lerped_palette.rgb;

    m_palette_timer += dt;
    if (m_palette_timer > fade_time) {
      // All white.
      for (u8 i = 0; i < m_lerped_palette.num_colours; i++) {
        lerp_rgb[0] = 0xFF;
        lerp_rgb[1] = 0xFF;
        lerp_rgb[2] = 0xFF;
        lerp_rgb += 3;
      }

      m_palette_timer = 0;
      m_palette_idx = 0;

    } else {
      // Brighten colours to white.
      const u32 scale = (m_palette_timer * (1 << 10)) / fade_time;
      for (u8 i = 0; i < m_lerped_palette.num_colours; i++) {
        lerp_rgb[0] = bg_rgb[0] + (((0xFF - bg_rgb[0]) * scale) >> 10);
        lerp_rgb[1] = bg_rgb[1] + (((0xFF - bg_rgb[1]) * scale) >> 10);
        lerp_rgb[2] = bg_rgb[2] + (((0xFF - bg_rgb[2]) * scale) >> 10);
        bg_rgb += 3;
        lerp_rgb += 3;
      }
    }

    images::set_palette(m_lerped_palette);
    return true;
  }

  bool fade_in_all(uclock_t dt) {
    if (m_palette_idx < 0) {
      return false;
    }

    const uclock_t fade_time = Funcs98::ticks_per_sec() * 2;

    const u8 *bg_rgb = m_bg_palette.rgb;
    u8 *lerp_rgb = m_lerped_palette.rgb;

    m_palette_timer += dt;
    if (m_palette_timer > fade_time) {
      // Proper background colours.
      memcpy(m_lerped_palette.rgb, m_bg_palette.rgb, sizeof(m_lerped_palette.rgb));

      m_palette_timer = 0;
      m_palette_idx = -1;

    } else {
      // Fade back from white to the expected colours.
      const u32 scale = (m_palette_timer * (1 << 10)) / fade_time;
      for (u8 i = 0; i < m_lerped_palette.num_colours; i++) {
        lerp_rgb[0] = 0xFF - (((0xFF - bg_rgb[0]) * scale) >> 10);
        lerp_rgb[1] = 0xFF - (((0xFF - bg_rgb[1]) * scale) >> 10);
        lerp_rgb[2] = 0xFF - (((0xFF - bg_rgb[2]) * scale) >> 10);
        bg_rgb += 3;
        lerp_rgb += 3;
      }
    }

    images::set_palette(m_lerped_palette);
    return true;
  }

private:
  int m_palette_idx; // >0: fading in, =0: fading out, <0, fading in all
  uclock_t m_palette_timer;

  images::Palette m_bg_palette;
  images::Palette m_lerped_palette;
};

struct FPSPrinter {
  FPSPrinter() : m_num_frames(0), m_num_ticks(0) {}

  void tick(uclock_t dt) {
    m_num_frames++;
    m_num_ticks += dt;
    if (m_num_ticks > Funcs98::ticks_per_sec()) {
      Funcs98::clear_screen();
      const unsigned fpks = (m_num_frames * Funcs98::ticks_per_sec() * 1000) / m_num_ticks;
      printf("%u.%uFPS\n", fpks / 1000, (fpks / 100) % 10);
      m_num_frames = 0;
      m_num_ticks = 0;
    }
  }

private:
  int m_num_frames;
  uclock_t m_num_ticks;
};

void play() {
  if (!gpu::setup()) {
    printf("Failed to setup GPU\n");
    return;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

  // Remove any text.
  Funcs98::clear_screen();
  gpu::enable_text_layer(true); // log text for FPS

  // Setup the palette fader.
  FadeInPalette fader;
  if (!fader.load("demo/bg.pal")) {
    return;
  }

  // Load the background.
  images::ImageData bg_data;
  if (!bg_data.load("demo/bg.img")) {
    printf("Failed to load background data\n");
    return;
  }

  // Load the animation.
  images::AnimationData anim_data;
  if (!anim_data.load("demo/ayy.ani")) {
    printf("Failed to load animated sprite data\n");
    return;
  }

  // Make some sprites, but don't draw them yet.
  images::AnimatedGif gif1(320 - 64, 100, anim_data);
  images::AnimatedGif gif2(160 - 64, 100, anim_data);
  images::AnimatedGif gif3(480 - 64, 100, anim_data);

  // Draw the intro image.
  gpu::wait_for_vsync();
  images::draw_image(0, 0, bg_data);

  // Timing tracking.
  FPSPrinter fpser;
  uclock_t last_time = Funcs98::ticks();

  State::E state = State::IntroFadeIn;

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
    fpser.tick(dt);

    // Wait for vsync.
    if (vsync) {
      gpu::wait_for_vsync();
    }

    // Tick state machine.
    switch (state) {
      case State::IntroFadeIn:
        if (!fader.fade_in_single(dt)) {
          state = State::IntroFadeOut;
        }
        break;
      case State::IntroFadeOut:
        if (!fader.fade_out_all(dt)) {
          state = State::AnimFadeIn;
        }
        break;
      case State::AnimFadeIn:
        if (!fader.fade_in_all(dt)) {
          state = State::AnimPlay;
        }
        // fallthrough
      case State::AnimPlay:
        gif1.tick(dt);
        gif2.tick(dt);
        gif3.tick(dt);
        break;
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
