#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"
#include "logs.h"
#include "utils.h"
#include "keyboard.h"
#include "maths.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define LOG_FILE "palcyc.txt"

// TODO: move into common header
#ifndef WEB_BUILD
#include <conio.h>
#define getch_98() getch()
#endif

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

// Image gets the first 19 entries, one for sky, rest for quad.
#define PAL_SKY 19
#define QUAD_START (PAL_SKY + 1)
#define QUAD_SIZE 8 // 8x8 tile - 64 colours
STATIC_ASSERT(QUAD_START + QUAD_SIZE * QUAD_SIZE < IMAGES_MAX_PALETTE_SIZE);

void set_sky_col(u8 idx) {
  images::Palette const & pal = images::default_palette_16;
  idx = idx % pal.num_colours;
  const u8 * col = pal.rgb + idx * 3;
  u8 * rgb = s_palette.rgb + PAL_SKY * 3;
  for (int i = 0; i < 3; i++) {
    *rgb++ = *col++;
  }
}

void set_quad_pix(u8 x, u8 y, u8 r, u8 g, u8 b) {
  u8 p = QUAD_START + QUAD_SIZE * y + x;
  u8 p3 = p + (p << 1);
  u8 * rgb = s_palette.rgb + p3;
  *rgb++ = r;
  *rgb++ = g;
  *rgb++ = b;
}

void quad_clear(u8 col) {
  memset(s_palette.rgb + QUAD_START * 3, col, QUAD_SIZE * QUAD_SIZE * 3);
}

void quad_rotate_x(bool left) {
  u8 * rgb = s_palette.rgb + QUAD_START * 3;
  for (int line = 0; line < QUAD_SIZE; line++) {
    if (left) {
      utils::rotate_left<3>(rgb, rgb + QUAD_SIZE * 3);
    } else {
      utils::rotate_right<3>(rgb, rgb + QUAD_SIZE * 3);
    }
    rgb += QUAD_SIZE * 3;
  }
}

void quad_rotate_y(bool up) {
  u8 * rgb = s_palette.rgb + QUAD_START * 3;
  if (up) {
    utils::rotate_left<3 * QUAD_SIZE>(rgb, rgb + QUAD_SIZE * QUAD_SIZE * 3);
  } else {
    utils::rotate_right<3 * QUAD_SIZE>(rgb, rgb + QUAD_SIZE * QUAD_SIZE * 3);
  }
}

#define TEX_GRID 0
#define TEX_GRID_ROT 1
#define TEX_CROSS 2
#define TEX_NEON 3
#define TEX_WATER_NE 4
#define TEX_MAX 5

#define TEX_CROSS_PAL_START 13 // 6 of the colours are for scrolling
#define TEX_CROSS_PAL_SIZE 19
STATIC_ASSERT(TEX_CROSS_PAL_SIZE == PAL_SKY);
const u8 s_fah_palette_data[] = {
#if 1 // Taken from fire demo.
  0xc3, 0x72, 0xf5,
  0xaf, 0x44, 0xf2,
  0x9c, 0x16, 0xef,
  0x7f, 0x0d, 0xc7,
  0x62, 0x0a, 0x99,
  0x45, 0x07, 0x6b,
#elif 0 // looks bad on white background
  0xA0, 0xB0, 0xE0,
  0xB0, 0x90, 0xE0,
  0xF0, 0xE0, 0xE0,
  0xE0, 0xF0, 0xF0,
  0xFF, 0xA0, 0xC0,
  0xD0, 0xC0, 0xFF,
#else
  0xFF, 0x00, 0x00, // red
  0xFF, 0x7F, 0x00, // orange
  //0xFF, 0xFF, 0x00, // yellow
  //0x00, 0xFF, 0x00, // green
  0x00, 0x00, 0xFF, // blue
  0x4B, 0x00, 0x82, // indigo
  0x94, 0x00, 0xD3, // violet
  0xFF, 0x14, 0x93, // pink
#endif
};

const u8 s_neon_palette_data[8 * 8 * 3] = {
  #define W 0xFF, 0xFF, 0xFF,
  #define P 0xFF, 0x00, 0xFF,
  #define E 0x00, 0x00, 0x00,
  #define B 0xA0, 0x30, 0xB0,
  P P P P P P P P
  P W W W W W W W
  P W B W W W B W
  P W W B B B W W
  P W B E B E B W
  P W W B B B W W
  P W B W W W B W
  P W W W W W W W
};
STATIC_ASSERT(sizeof(s_neon_palette_data) == QUAD_SIZE * QUAD_SIZE * 3);

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
      int i;
      for (i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i, i, 0xC8, 0xC8, 0xFF);
      }
      for (i = 1; i < QUAD_SIZE; i++) {
        set_quad_pix(QUAD_SIZE - i, i, 0xC8, 0xC8, 0xFF);
      }
    } break;

    case TEX_WATER_NE: {
      // https://www.color-hex.com/color-palette/2738
      // Blue background.
      int i;
      for (int j = 0; j < QUAD_SIZE; j++) {
        for (i = 0; i < QUAD_SIZE; i++) {
          set_quad_pix(i, j, 0x23, 0x89, 0xDA);
        }
      }
      // Wave direction.
      for (i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i, i, 0x74, 0xCF, 0xF4);
      }
      for (i = 0; i < QUAD_SIZE; i++) {
        set_quad_pix(i % QUAD_SIZE, (i + 1) % QUAD_SIZE, 0x5A, 0xBC, 0xD8); // leading
        set_quad_pix((i + 1) % QUAD_SIZE , i % QUAD_SIZE, 0x1C, 0xA3, 0xEC); // trailing
      }
    } break;

    case TEX_NEON: {
      memcpy(s_palette.rgb + QUAD_START * 3, s_neon_palette_data, sizeof(s_neon_palette_data));
    } break;
  }
}

void draw_background(u8 tex, bool face, bool mode7, u16 horizon) {
  // All have the same base background.
  {
    gpu::g_draw_to = gpu::DrawTo::Back;
    u8 data[SCANLINE_PART_WIDTH_32];

    if (mode7) {
      horizon = utils::clamp<u16>(horizon, 1, GPU_HEIGHT - 1);

      // Clear out everything beyond the horizon.
      memset(data, PAL_SKY, sizeof(data));
      u16 y;
      for (y = 0; y < horizon; ++y) {
        for (u16 part = 0; part < GPU_WIDTH / SCANLINE_PART_WIDTH_32; ++part) {
          gpu::write_scanline_part_32(y, part, data);
        }
      }

      for (y = horizon + 1; y < GPU_HEIGHT; ++y) {
        // Perspective divide.
        // d = (y - horizon) / GPU_HEIGHT
        // z = 1 / d
        // tx = (x - GPU_WIDTH / 2) / z
#define Z_SHIFT 9 // 512, >GPU_HEIGHT
        const u32 z = (GPU_HEIGHT << Z_SHIFT) / (y - horizon);
        i32 xz = -GPU_WIDTH / 2 * z;
        for (u16 part = 0; part < GPU_WIDTH / SCANLINE_PART_WIDTH_32; ++part) {
          for (u8 i = 0; i < SCANLINE_PART_WIDTH_32; i++, xz += z) {
            // Scale by 1/16 (x) and 1/4 (y).
            const i16 tx = (xz) >> (Z_SHIFT + 4);
            // -ve causes a seam at 0 unless power of 2
            STATIC_ASSERT((QUAD_SIZE & (QUAD_SIZE - 1)) == 0);
            const u16 ux = tx;
            const u16 uy = y >> 2;
            data[i] = QUAD_START + (ux % QUAD_SIZE) + (uy % QUAD_SIZE) * QUAD_SIZE;
          }
          gpu::write_scanline_part_32(y, part, data);
        }
      }
    } else {
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
    gpu::g_draw_to = gpu::DrawTo::Front;
  }
  // Copy from backbuffer to front buffer.
  // There needs to be a copy of the data in the backbuffer for sprites to work.
  gpu::undraw_quad(0, 0, GPU_WIDTH, GPU_HEIGHT);

  if (face) {
    // Draw the face over the top.
    images::Palette pal;
    images::ImageData img, mask;
    if (images::load_palette(pal, "AF.PAL") && pal.num_colours == TEX_CROSS_PAL_SIZE && img.load("AF.IMG") && mask.load("AF_M.IMG")) {
      // Copy over the image's palette and draw it.
      memcpy(s_palette.rgb, pal.rgb, pal.num_colours * 3);
      const u16 x = (GPU_WIDTH - img.m_width) / 2;
      const u16 y = (GPU_HEIGHT - img.m_height) / 2;
      images::draw_sprite(x, y, img, mask);

      // Replace the scrolling palette.
      STATIC_ASSERT(sizeof(s_fah_palette_data) == (TEX_CROSS_PAL_SIZE - TEX_CROSS_PAL_START) * 3);
      memcpy(s_palette.rgb + TEX_CROSS_PAL_START * 3, s_fah_palette_data, (TEX_CROSS_PAL_SIZE - TEX_CROSS_PAL_START) * 3);
    } else {
      logging::print(logging::Level::Error, "Failed to load background for cross");
    }
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
  int i;
  for (i = -QUAD_SIZE/2; i < 0; i++) {
    const u8 x = (cx + s * i) >> 7;
    const u8 y = (cy + c * i) >> 7;
    set_quad_pix(x, y, col, col, col);
    col -= 0xC0 / ((QUAD_SIZE + 1)/2);
  }
  for (i = 0; i <= QUAD_SIZE/2; i++) {
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
  ticker = (ticker - 0x40 / 4) & 0x7F; // offset animation of drops
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

bool update_quad(u8 tex, u8 ticker, u32 buttons) {
  // Update scrolling palette.
  if ((ticker & 7) == 0) {
    utils::rotate_right<3>(s_palette.rgb + TEX_CROSS_PAL_START * 3, s_palette.rgb + TEX_CROSS_PAL_SIZE * 3);
  }

  // Update the quad.
  switch (tex) {
    case TEX_NEON:
    case TEX_GRID: {
      if ((buttons & (KB_STATE_LEFT | KB_STATE_RIGHT | KB_STATE_A | KB_STATE_D)) ) {
        quad_rotate_x(buttons & (KB_STATE_RIGHT | KB_STATE_D));
      }
      if ((buttons & (KB_STATE_UP | KB_STATE_DOWN | KB_STATE_W | KB_STATE_S)) ) {
        quad_rotate_y(buttons & (KB_STATE_DOWN | KB_STATE_S));
      }
      return true;
    } break;

    case TEX_GRID_ROT: {
      quad_rot_set(ticker);
      return true;
    } break;

    case TEX_CROSS: {
      if (ticker & 1) {
        quad_rotate_x(true);
        return true;
      }
    } break;

    case TEX_WATER_NE: {
      if (ticker & 1) {
        quad_rotate_x(true);
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

  // Display some help text.
  gpu::enable_text_layer(true);
  {
    printf("Controls:\n");
    printf("  q - [q]uit\n");
    printf("  e/r - n[e]xt/p[r]evious screen\n");
    printf("  space - pause\n");
    printf("  y - toggle vsync\n");
    printf("  arrow keys/wasd - movement (on grid screen)\n");
    printf("  u - face on/off\n");
    printf("  i - mode change\n");
    printf("  o - sky c[o]lour\n");
    printf("  p - horizon\n");
    printf("  t - single s[t]ep (for debuggin)\n");
    printf("Press any key to continue\n");
#ifndef WEB_BUILD // TODO
    getch_98();
#endif
  }

  // No text.
  gpu::enable_text_layer(false);

  //

  // Setup palette and background.
  u8 quad_tex = TEX_GRID;
  u8 sky_col = 0;
  u16 horizon = GPU_HEIGHT / 3;
  bool face = false;
  bool mode7 = false;
  s_palette.num_colours = QUAD_START + QUAD_SIZE * QUAD_SIZE;
  memset(s_palette.rgb, 0, sizeof(s_palette.rgb));
  set_quad(quad_tex);
  set_sky_col(sky_col);
  draw_background(quad_tex, face, mode7, horizon);
  images::set_palette(s_palette);

  bool adjust = true;
  bool step = false;
  bool vsync = true;
  bool pressed = false;

  u8 quad_tick = 0;

  //uclock_t last_time = Funcs98::ticks();

#ifdef __EMSCRIPTEN__
  auto run_one = [&]
#else
  while (!g_had_error)
#endif
  {
    const u32 buttons = read_keyboard_state();
    if (buttons && !pressed) {
      bool update = false;
      if (buttons & KB_STATE_Q) { // q = quit
        break;
      } else if (buttons & KB_STATE_SPACE) { // space = pause
        adjust = !adjust;
        step = false;
      } else if (buttons & KB_STATE_T) { // t = single step
        adjust = true;
        step = true;
      } else if (buttons & KB_STATE_E) { // e = next
        quad_tex = (quad_tex + 1) % TEX_MAX;
        update = true;
      } else if (buttons & KB_STATE_R) { // r = prev
        quad_tex = (quad_tex + TEX_MAX - 1) % TEX_MAX;
        update = true;
      } else if (buttons & KB_STATE_U) { // u = face
        face = !face;
        update = true;
      } else if (buttons & KB_STATE_I) { // i = mode7
        mode7 = !mode7;
        update = true;
      } else if (buttons & KB_STATE_Y) { // y = vsync
        vsync = !vsync;
      } else if (buttons & KB_STATE_O) { // o = sky colour
        sky_col++;
        update = true;
      } else if (buttons & KB_STATE_P) { // p = horizon
        horizon = (horizon + GPU_HEIGHT / 8) % GPU_HEIGHT;
        update = true;
      }

      if (update) {
        set_quad(quad_tex);
        set_sky_col(sky_col);
        draw_background(quad_tex, face, mode7, horizon);
        images::set_palette(s_palette);
      }
    }
    pressed = buttons;

    // Adjust palette.
    if (adjust) {
      // Pause if we're doing a single step.
      if (step) {
        adjust = false;
        step = false;
      }

      // Update quad.
      quad_tick++;
      const bool changed = update_quad(quad_tex, quad_tick, buttons);

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
