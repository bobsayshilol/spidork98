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

#define FADE_TIME (Funcs98::ticks_per_sec())

void splash_menu_enter() {
  logging::print(logging::Level::Info, "Entering splash menu");

  // Draw the background to the backbuffer.
  {
    images::ImageData splash_bg;
    if (!splash_bg.load("splash.img")) {
      logging::print(logging::Level::Error, "Failed to load splash screen image");
      g_had_error = true;
      return;
    }

    gpu::g_draw_to = gpu::DrawTo::Back;
    images::draw_image(0, 0, splash_bg);
    gpu::g_draw_to = gpu::DrawTo::Front;
  }

  // Load the palette.
  memcpy(&g_palette_fader.target_palette(), &images::default_palette_16, sizeof(images::Palette));

  // Copy to the front buffer.
  gpu::undraw_quad(0, 0, GPU_WIDTH, GPU_HEIGHT);

  // Start the fade in.
  g_palette_fader.start_fade_in(FADE_TIME);
  s_splash_state = SplashState::FadeIn;
}

const MenuScreen *splash_menu_update(u32 dt) {
  switch (s_splash_state) {
    case SplashState::FadeIn:
      if (g_palette_fader.tick(dt)) {
        s_splash_state = SplashState::Input;
        flush_kb_buffer();
      }
      break;

    case SplashState::Input:
      if (kbhit_98()) {
        const int ch = getch();
        if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == ' ' || ch == '\r') {
          g_palette_fader.start_fade_out(FADE_TIME);
          s_splash_state = SplashState::FadeOut;
        }
      }
      break;

    case SplashState::FadeOut:
      if (g_palette_fader.tick(dt)) {
        return &g_main_menu;
      }
      break;
  }

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
