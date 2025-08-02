#include "funcs.h"
#include "gpuscrn.h"
#include "logs.h"

#include "web_common.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <cassert>
#include <array>
#include <chrono>
#include <memory>

namespace gpu {

namespace {

struct SDLDeleter {
  void operator()(SDL_Palette *p) {
    SDL_DestroyPalette(p);
  }
  void operator()(SDL_Surface *s) {
    SDL_DestroySurface(s);
  }
  void operator()(SDL_Window *w) {
    SDL_DestroyWindow(w);
  }
};

//

constexpr std::size_t MAX_PALETTE_SIZE = 256;

std::unique_ptr<SDL_Window, SDLDeleter> s_window;
std::unique_ptr<SDL_Palette, SDLDeleter> s_palette;
std::unique_ptr<SDL_Surface, SDLDeleter> s_back_buffer;
std::unique_ptr<SDL_Surface, SDLDeleter> s_front_buffer;

SDL_Surface *get_current_surface() {
  return g_draw_to == DrawTo::Front ? s_front_buffer.get() : s_back_buffer.get();
}

//

constexpr std::size_t TEXT_CHAR_WIDTH = GPU_WIDTH / ScreenCols_98();
constexpr std::size_t TEXT_CHAR_HEIGHT = GPU_HEIGHT / ScreenRows_98();
bool s_text_enabled;
std::unique_ptr<SDL_Surface, SDLDeleter> s_text_buffer;

#include "web_font.h"

} // namespace

DrawTo::E g_draw_to;

FASTCALL bool setup() {
  if (SDL_WasInit(SDL_INIT_VIDEO)) {
    logging::print(logging::Level::Warning, "GPU already setup");
    return false;
  }

  if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
    logging::print(logging::Level::Error, "Failed to init video: %s", SDL_GetError());
    return false;
  }

  s_window.reset(SDL_CreateWindow("SPIDORK98", GPU_WIDTH, GPU_HEIGHT, 0));
  if (!s_window) {
    logging::print(logging::Level::Error, "Failed to create window: %s", SDL_GetError());
    return false;
  }

  s_palette.reset(SDL_CreatePalette(MAX_PALETTE_SIZE));
  if (!s_palette) {
    logging::print(logging::Level::Error, "Failed to create palette: %s", SDL_GetError());
    return false;
  }

  s_front_buffer.reset(SDL_CreateSurface(GPU_WIDTH, GPU_HEIGHT, SDL_PIXELFORMAT_INDEX8));
  if (!s_front_buffer) {
    logging::print(logging::Level::Error, "Failed to create front surface: %s", SDL_GetError());
    return false;
  }
  s_back_buffer.reset(SDL_CreateSurface(GPU_WIDTH, GPU_HEIGHT, SDL_PIXELFORMAT_INDEX8));
  if (!s_back_buffer) {
    logging::print(logging::Level::Error, "Failed to create back surface: %s", SDL_GetError());
    return false;
  }
  s_text_buffer.reset(SDL_CreateSurface(GPU_WIDTH, GPU_HEIGHT, SDL_PIXELFORMAT_RGBA8888));
  if (!s_text_buffer) {
    logging::print(logging::Level::Error, "Failed to create text surface: %s", SDL_GetError());
    return false;
  }

  SDL_Surface * window_surface = SDL_GetWindowSurface(s_window.get());
  for (auto *surface : { window_surface, s_front_buffer.get(), s_back_buffer.get()}) {
    if (!SDL_SetSurfacePalette(surface, s_palette.get())) {
      logging::print(logging::Level::Error, "Failed to set palette on a surface: %s", SDL_GetError());
      return false;
    }
  }

  // Setup the default palette.
  {
    std::array<SDL_Color, MAX_PALETTE_SIZE> pal;
    pal.fill({ .r = 0, .g = 0, .b = 0, .a = 0xFF });
    if (!SDL_SetPaletteColors(s_palette.get(), pal.data(), 0, pal.size())) {
      logging::print(logging::Level::Error, "Failed to set palette colours: %s", SDL_GetError());
      return false;
    }
  }

  g_draw_to = DrawTo::Back;
  clear(0);
  g_draw_to = DrawTo::Front;
  clear(0);
  enable_text_layer(false);

  return true;
}

FASTCALL void shutdown() {
  if (!SDL_WasInit(SDL_INIT_VIDEO)) return;

  s_palette.reset();
  s_text_buffer.reset();
  s_back_buffer.reset();
  s_front_buffer.reset();
  s_window.reset();

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

FASTCALL void wait_for_vsync() {
  SDL_PumpEvents();
  web::update_screen();

  // Fake 60fps.
#ifndef __EMSCRIPTEN__
  const auto now = std::chrono::system_clock::now();
  static auto last_time = now;
  const auto delay = std::chrono::duration_cast<std::chrono::milliseconds>((last_time + std::chrono::milliseconds(16) - now));
  last_time = now;
  if (delay > std::chrono::milliseconds(0)) {
    SDL_Delay(delay.count());
  }
#endif
}

FASTCALL void enable_text_layer(bool show) {
  s_text_enabled = show;
}

FASTCALL void set_palette_colour(u8 pal_idx, u8 r, u8 g, u8 b) {
  SDL_Color col { .r = r, .g = g, .b = b, .a = 0xFF };
  const bool success = SDL_SetPaletteColors(s_palette.get(), &col, pal_idx, 1);
  assert(success);
}

FASTCALL void clear(u8 pal_col) {
  auto *surface = get_current_surface();
  const bool success = SDL_FillSurfaceRect(surface, nullptr, pal_col);
  assert(success);
}

FASTCALL void draw_quad(int x0, int y0, int x1, int y1, u8 pal_col) {
  if (x1 < x0) std::swap(x0, x1);
  if (y1 < y0) std::swap(y0, y1);

  auto *surface = get_current_surface();
  const SDL_Rect rect{
    .x = x0, .y = y0,
    .w = x1 - x0, .h = y1 - y0,
  };
  const bool success = SDL_FillSurfaceRect(surface, &rect, pal_col);
  assert(success);
}

FASTCALL void undraw_quad(int x0, int y0, int x1, int y1) {
  if (x1 < x0) std::swap(x0, x1);
  if (y1 < y0) std::swap(y0, y1);

  const SDL_Rect rect{
    .x = x0, .y = y0,
    .w = x1 - x0, .h = y1 - y0,
  };
  const bool success = SDL_BlitSurface(s_back_buffer.get(), &rect, s_front_buffer.get(), &rect);
  assert(success);
}

FASTCALL void read_scanline_part_16(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_16*/) {
  auto *surface = get_current_surface();
  const bool locked = SDL_LockSurface(surface);
  assert(locked);
  DEFER( SDL_Surface* , s , surface , SDL_UnlockSurface(s) );

  const u8 *pix = static_cast<const u8*>(surface->pixels);
  pix += line * GPU_WIDTH;
  pix += part * SCANLINE_PART_WIDTH_16;
  memcpy(data, pix, SCANLINE_PART_WIDTH_16);
}

FASTCALL void write_scanline_part_16(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_16*/) {
  auto *surface = get_current_surface();
  const bool locked = SDL_LockSurface(surface);
  assert(locked);
  DEFER( SDL_Surface* , s , surface , SDL_UnlockSurface(s) );

  u8 *pix = static_cast<u8*>(surface->pixels);
  pix += line * GPU_WIDTH;
  pix += part * SCANLINE_PART_WIDTH_16;
  memcpy(pix, data, SCANLINE_PART_WIDTH_16);
}

FASTCALL void read_scanline_part_32(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_32*/) {
  auto *surface = get_current_surface();
  const bool locked = SDL_LockSurface(surface);
  assert(locked);
  DEFER( SDL_Surface* , s , surface , SDL_UnlockSurface(s) );

  const u8 *pix = static_cast<const u8*>(surface->pixels);
  pix += line * GPU_WIDTH;
  pix += part * SCANLINE_PART_WIDTH_32;
  memcpy(data, pix, SCANLINE_PART_WIDTH_32);
}

FASTCALL void write_scanline_part_32(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_32*/) {
  auto *surface = get_current_surface();
  const bool locked = SDL_LockSurface(surface);
  assert(locked);
  DEFER( SDL_Surface* , s , surface , SDL_UnlockSurface(s) );

  u8 *pix = static_cast<u8*>(surface->pixels);
  pix += line * GPU_WIDTH;
  pix += part * SCANLINE_PART_WIDTH_32;
  memcpy(pix, data, SCANLINE_PART_WIDTH_32);
}

} // namespace gpu

namespace web {

void clear_text_layer() {
  if (gpu::s_text_buffer) {
    const bool success = SDL_FillSurfaceRect(gpu::s_text_buffer.get(), nullptr, 0);
    assert(success);
  }
}

void print_text_layer(char ch, u8 col, int x, int y) {
  auto *surface = gpu::s_text_buffer.get();
  const bool locked = SDL_LockSurface(surface);
  assert(locked);
  DEFER( SDL_Surface* , s , surface , SDL_UnlockSurface(s) );

  // Map the colour to one we can use.
  const bool underline = col & 8;
  const bool r = col & 4;
  const bool g = col & 2;
  const bool b = col & 1;
  const bool a = r | g | b;

  // TODO: palette would probably be better.
  const u32 rgba8888 =
    (r ? 0xFF << 24 : 0) |
    (g ? 0xFF << 16 : 0) |
    (b ? 0xFF <<  8 : 0) |
    (a ? 0xFF <<  0 : 0) ;

  u32 *const corner = static_cast<u32*>(surface->pixels)
    + y * GPU_WIDTH * gpu::TEXT_CHAR_HEIGHT
    + x * gpu::TEXT_CHAR_WIDTH
    ;

  // Draw the character.
  auto &char_data = gpu::get_font_data(ch);
  for (std::size_t j = 0; j < gpu::TEXT_CHAR_HEIGHT; j++) {
    u32 *const pix = corner + j * GPU_WIDTH;
    u8 mask = char_data.rows[j];
    for (std::size_t i = 0; i < gpu::TEXT_CHAR_WIDTH; i++) {
      pix[i] = (mask & 0x80) ? rgba8888 : 0;
      mask <<= 1;
    }
  }

  // Give it an underline.
  if (underline) {
    u32 *const pix = corner + ((7 * gpu::TEXT_CHAR_HEIGHT - 8) / 8) * GPU_WIDTH;
    for (std::size_t i = 0; i < gpu::TEXT_CHAR_WIDTH; i++) {
      pix[i] = rgba8888;
    }
  }
}

void update_screen() {
  SDL_Surface * window_surface = SDL_GetWindowSurface(gpu::s_window.get());
  SDL_BlitSurfaceScaled(gpu::s_front_buffer.get(), nullptr, window_surface, nullptr, SDL_ScaleMode::SDL_SCALEMODE_NEAREST);
  if (gpu::s_text_enabled) {
    SDL_BlitSurfaceScaled(gpu::s_text_buffer.get(), nullptr, window_surface, nullptr, SDL_ScaleMode::SDL_SCALEMODE_NEAREST);
  }
  SDL_UpdateWindowSurface(gpu::s_window.get());
}

} // namespace web
