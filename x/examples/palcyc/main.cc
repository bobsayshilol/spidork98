#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"
#include "logs.h"
#include "utils.h"
#include "keyboard.h"
#include "maths.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define LOG_FILE "palcyc.txt"

// TODO: move into common header
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
template <typename Func>
static void run_at_fps(int fps, Func & func) {
  emscripten_set_main_loop_arg([](void *arg){
    (*static_cast<Func*>(arg))();
  }, &func, fps, true);
}
#endif

// TODO: move into a common header
#define KEY_UP 11
#define KEY_DOWN 10
#define KEY_LEFT 8
#define KEY_RIGHT 12
#define KEY_ENTER '\r'
#define KEY_SPACE ' '
#define KEY_ESCAPE 27

namespace {

bool g_had_error;
images::Palette s_palette;

#define QUAD_START (18 + 1) // reserve some colours
#define QUAD_SIZE 7 // 7x7 tile - 49 colours
STATIC_ASSERT(QUAD_START + QUAD_SIZE * QUAD_SIZE < IMAGES_MAX_PALETTE_SIZE);

void set_quad_pix(u8 x, u8 y, u8 r, u8 g, u8 b) {
  u8 p = QUAD_START + QUAD_SIZE * y + x;
  s_palette.rgb[3 * p + 0] = r;
  s_palette.rgb[3 * p + 1] = g;
  s_palette.rgb[3 * p + 2] = b;
}

void quad_clear(u8 col) {
  memset(s_palette.rgb + QUAD_START * 3, col, QUAD_SIZE * QUAD_SIZE * 3);
}

void quad_rotate_x() {
  for (int line = 0; line < QUAD_SIZE; line++) {
    u8 * line_start = s_palette.rgb + QUAD_START * 3 + line * QUAD_SIZE * 3;
    utils::rotate_left<3>(line_start, line_start + QUAD_SIZE * 3);
  }
}

void quad_rotate_y() {
  u8 * rgb = s_palette.rgb + QUAD_START * 3;
  utils::rotate_left<3 * QUAD_SIZE>(rgb, rgb + QUAD_SIZE * QUAD_SIZE * 3);
}

#define TEX_GRID 0
#define TEX_GRID_ROT 1
#define TEX_CROSS 2
#define TEX_WATER_NE 3
#define TEX_MAX 4

void set_quad(u8 tex) {
  switch (tex) {
    case TEX_GRID: {
      // Black background, white lines.
      quad_clear(0x00);
      for (int i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i, 0, 0xFF, 0xFF, 0xFF);
        set_quad_pix(0, i, 0xFF, 0xFF, 0xFF);
      }
    } break;

    case TEX_GRID_ROT: {
      // TODO: build and cache it here
    } break;

    case TEX_CROSS: {
      // White background, purple foreground.
      quad_clear(0xFF);
      for (int i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i, i, 0xC8, 0xC8, 0xFF);
      }
      for (int i = 1; i < QUAD_SIZE; i++) {
        set_quad_pix(QUAD_SIZE - i, i, 0xC8, 0xC8, 0xFF);
      }
    } break;

    case TEX_WATER_NE: {
      // https://www.color-hex.com/color-palette/2738
      // Blue background.
      for (int j = 0; j < QUAD_SIZE; j++) {
        for (int i = 0; i < QUAD_SIZE; i++) {
          set_quad_pix(i, j, 0x23, 0x89, 0xDA);
        }
      }
      // Wave direction.
      for (int i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i, i, 0x74, 0xCF, 0xF4);
      }
      for (int i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i % QUAD_SIZE, (i + 1) % QUAD_SIZE, 0x5A, 0xBC, 0xD8); // leading
        set_quad_pix((i + 1) % QUAD_SIZE , i % QUAD_SIZE, 0x1C, 0xA3, 0xEC); // trailing
      }
    } break;
  }
}

void draw_background(u8 tex) {
  u8 data[SCANLINE_PART_WIDTH_32];

  // All have the same base background.
  {
    const u8 shift = (tex == TEX_GRID_ROT) ? 3 : 2;
    for (u16 y = 0; y < GPU_HEIGHT; ++y) {
      u16 x = 0;
      for (u16 part = 0; part < GPU_WIDTH / SCANLINE_PART_WIDTH_32; ++part) {
        for (u8 i = 0; i < SCANLINE_PART_WIDTH_32; i++, x++) {
          u16 ux = x >> shift;
          u16 uy = y >> shift;
          data[i] = QUAD_START + (ux % QUAD_SIZE) + (uy % QUAD_SIZE) * QUAD_SIZE;
        }
        gpu::write_scanline_part_32(y, part, data);
      }
    }
  }

  switch(tex) {
    case TEX_GRID_ROT:
    case TEX_GRID: {
      // Nothing extra.
    } break;

    case TEX_CROSS: {
      // TODO: draw face
    } break;

    case TEX_WATER_NE: {
      // TODO: draw boats
    } break;
  }
}

void quad_rot_set(u8 ticker) {
  // TODO: cache this

  // Clear background
  quad_clear(0x20);

  // Speed up rotation speed.
  ticker <<= 1;
  ticker &= 0x7F; // half circle
  const i8 s = maths::sin(ticker);
  const i8 c = maths::cos(ticker);

  // Center shifted into sin/cos space.
  const int cx = (QUAD_SIZE / 2) << 7;
  const int cy = (QUAD_SIZE / 2) << 7;

  // Rotate around center point.
  u8 col = 0xC0;
  for (int i = -QUAD_SIZE/2; i < 0; i++) {
    const u8 x = (cx + s * i) >> 7;
    const u8 y = (cy + c * i) >> 7;
    set_quad_pix(x, y, col, col, col);
    col -= 0xC0 / ((QUAD_SIZE + 1)/2);
  }
  for (int i = 0; i <= QUAD_SIZE/2; i++) {
    const u8 x = (cx + s * i) >> 7;
    const u8 y = (cy + c * i) >> 7;
    set_quad_pix(x, y, col, col, col);
    col += 0xC0 / ((QUAD_SIZE + 1)/2);
  }

#if 1
  // Bucko bouncing.
  const u8 y0 = (s * QUAD_SIZE / 2) >> 7;
  set_quad_pix(QUAD_SIZE - 1, QUAD_SIZE / 2 + y0 - 1, 0xA0, 0x30, 0xB0);
  set_quad_pix(QUAD_SIZE - 1, QUAD_SIZE - 1, 0x00, 0xFF, 0x00);

  // FF dropping.
  ticker = (ticker - 0x80 / 4) & 0x7F; // offset animation of drops
  const u8 y1 = (ticker * QUAD_SIZE) >> 7;
  set_quad_pix(1, y1, 0x50, 0x90, 0xF0);
#else
  // Dots on both sides.
  u8 x0 = (cx + c * QUAD_SIZE/2) >> 7;
  u8 y0 = (cy - s * QUAD_SIZE/2) >> 7;
  set_quad_pix(x0, y0, 0x00, 0xFF, 0x00);
  //x0 = (cx - c * QUAD_SIZE/2) >> 7;
  //y0 = (cy + s * QUAD_SIZE/2) >> 7;
  //set_quad_pix(x0, y0, 0x00, 0xFF, 0x00);
#endif
}

bool update_quad(u8 tex, u8 ticker) {
  switch (tex) {
    case TEX_GRID: {
      if ((ticker & 3) == 0) { quad_rotate_x(); }
      if ((ticker & 1) == 1) quad_rotate_y();
      return true;
    } break;

    case TEX_GRID_ROT: {
      quad_rot_set(ticker);
      return true;
    } break;

    case TEX_CROSS: {
      if (ticker & 1) {
        quad_rotate_x();
        return true;
      }
    } break;

    case TEX_WATER_NE: {
      if (ticker & 1) {
        quad_rotate_x();
        return true;
      }
    } break;
  }
  return false;
}

void play() {
  logging::init(LOG_FILE);

  const unsigned seed = time(0);
  logging::print(logging::Level::Info, "RNG seed: %u", seed);
  srand(seed);

  // GPU go!
  if (!gpu::setup()) {
    logging::print(logging::Level::Error, "Failed to setup GPU\n");
    g_had_error = true;
    return;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

#ifndef WEB_BUILD
  // No cursor unless we need it.
  _setcursortype_98(_NOCURSOR);
  DEFER(void *, p, NULL, (_setcursortype_98(_NORMALCURSOR)));
#endif

  // No text.
  gpu::enable_text_layer(false);

  //

  // Setup palette and background.
  u8 quad_tex = TEX_GRID;
  s_palette.num_colours = QUAD_START + QUAD_SIZE * QUAD_SIZE;
  memset(s_palette.rgb, 0, sizeof(s_palette.rgb));
  set_quad(quad_tex);
  images::set_palette(s_palette);
  draw_background(quad_tex);

  bool adjust = true;
  bool step = false;
  bool vsync = true;

  u8 quad_tick = 0;

  //uclock_t last_time = Funcs98::ticks();

#ifdef __EMSCRIPTEN__
  auto run_one = [&]
#else
  while (!g_had_error)
#endif
  {
    if (kbhit_98()) {
      const int ch = getch_98();
      if (ch == 'q' || ch == 'Q' || ch == KEY_ESCAPE) {
        break;
      } else if (ch == KEY_SPACE) {
        adjust = !adjust;
        step = false;
      } else if (ch == 't' || ch == 't') {
        adjust = true;
        step = true;
      } else if (ch == 'e' || ch == 'E') {
        quad_tex = (quad_tex + 1) % TEX_MAX;
        set_quad(quad_tex);
        images::set_palette(s_palette);
        draw_background(quad_tex);
      } else if (ch == 'v' || ch == 'V') {
        vsync = !vsync;
      }
    }

    // Adjust palette.
    if (adjust) {
      // Pause if we're doing a single step.
      if (step) {
        adjust = false;
        step = false;
      }

      // Update quad.
      quad_tick++;
      const bool changed = update_quad(quad_tex, quad_tick);

      // Update the palette if it changed.
      if (changed) {
        images::set_palette(s_palette);
      }
    }

    // Wait for display.
    if (vsync) {
      gpu::wait_for_vsync();
    }
  }
#ifdef __EMSCRIPTEN__
  ;
  run_at_fps(60, run_one);
#endif
}

} // namespace

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  play();

  Funcs98::clear_screen();
  if (g_had_error) {
    printf("Error encountered\nSee " LOG_FILE " for more info\n\n");
  }
  printf("Thanks for playing!\n");
  return EXIT_SUCCESS;
}
