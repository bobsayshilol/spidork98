#include "game.h"
#include "menus.h"
#include "pal_fade.h"

#include "funcs.h"
#include "images.h"
#include "gpuscrn.h"
#include "logs.h"

#include <conio.h>

namespace game {
namespace menus {

namespace {

struct SplashState { enum E { FadeIn, Input, FadeOut }; };
SplashState::E s_splash_state;

i32 s_continue_dir;
i32 s_continue_tick;

#define FADE_IN_TIME (Funcs98::ticks_per_sec())
#define FADE_OUT_TIME (Funcs98::ticks_per_sec() * 2 / 3)
#define FLASHER_TIME (Funcs98::ticks_per_sec() / 2)

void splash_menu_enter() {
  logging::print(logging::Level::Info, "Entering splash menu");

  // Draw the background to the backbuffer.
  {
    images::ImageData splash_bg;
    if (!splash_bg.load(GAME_DATA_PATH("splash.img"))) {
      logging::print(logging::Level::Error, "Failed to load splash screen image");
      g_had_error = true;
      return;
    }

    gpu::g_draw_to = gpu::DrawTo::Back;
    images::draw_image(0, 0, splash_bg);
    gpu::g_draw_to = gpu::DrawTo::Front;
  }

  // Copy to the front buffer.
  gpu::undraw_quad(0, 0, GPU_WIDTH, GPU_HEIGHT);

  // Load the palette.
#define FLASHER_PALETTE_IDX 64
  g_palette_fader.target_palette() = images::default_palette_64;

  // Start the fade in.
  g_palette_fader.start_fade_in(FADE_IN_TIME);
  s_splash_state = SplashState::FadeIn;
  s_continue_tick = 0;
  s_continue_dir = 1;
}

const MenuScreen *splash_menu_update(u32 dt) {
  switch (s_splash_state) {
    case SplashState::FadeIn:
      if (g_palette_fader.tick(dt)) {
        s_splash_state = SplashState::Input;
        flush_kb_buffer();
      }
      break;

    case SplashState::Input: {
      // Flash the continue button.
      s_continue_tick += s_continue_dir * static_cast<i32>(dt);
      if (s_continue_tick > FLASHER_TIME) {
        s_continue_tick = FLASHER_TIME;
        s_continue_dir = -1;
      } else if (s_continue_tick < 0) {
        s_continue_tick = 0;
        s_continue_dir = 1;
      }

      // Flicker the flasher.
      const u8 flash_rgb = s_continue_tick * 255 / FLASHER_TIME; // >>8 has flickers, >>7 too dark
      gpu::set_palette_colour(FLASHER_PALETTE_IDX, flash_rgb, flash_rgb, flash_rgb);

      if (kbhit_98()) {
        const int ch = getch();
        if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == KEY_SPACE || ch == KEY_ENTER || ch == KEY_ESCAPE) {
          // Add the hidden colour.
          images::Palette & target_palette = g_palette_fader.target_palette();
          u8 *pal = &target_palette.rgb[3 * target_palette.num_colours];
          *pal++ = flash_rgb;
          *pal++ = flash_rgb;
          *pal++ = flash_rgb;
          target_palette.num_colours++;

          // Start the fade.
          g_palette_fader.start_fade_out(FADE_OUT_TIME);
          s_splash_state = SplashState::FadeOut;
        }
      }
    } break;

    case SplashState::FadeOut:
      if (g_palette_fader.tick(dt)) {
        return &g_main_menu;
      }
      break;
  }

  gpu::wait_for_vsync();
  return &g_splash_menu;
}

void splash_menu_leave() {
  logging::print(logging::Level::Info, "Leaving splash menu");
}

} // namespace

const MenuScreen g_splash_menu = {
  splash_menu_enter,
  splash_menu_update,
  splash_menu_leave,
};

} // namespace menus
} // namespace game
