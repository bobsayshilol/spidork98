#include "gpuscrn.h"
#include "logs.h"

#include "web_common.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>

#include <array>
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

constexpr std::size_t MAX_PALETTE_SIZE = 256;

std::unique_ptr<SDL_Window, SDLDeleter> s_window;
std::unique_ptr<SDL_Palette, SDLDeleter> s_palette;
std::unique_ptr<SDL_Surface, SDLDeleter> s_back_buffer;
std::unique_ptr<SDL_Surface, SDLDeleter> s_front_buffer;

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

  s_front_buffer.reset(SDL_CreateSurface(GPU_WIDTH, GPU_HEIGHT, SDL_PIXELFORMAT_RGBX8888));
  if (!s_front_buffer) {
    logging::print(logging::Level::Error, "Failed to create front surface: %s", SDL_GetError());
    return false;
  }
  s_back_buffer.reset(SDL_CreateSurface(GPU_WIDTH, GPU_HEIGHT, SDL_PIXELFORMAT_RGBX8888));
  if (!s_back_buffer) {
    logging::print(logging::Level::Error, "Failed to create back surface: %s", SDL_GetError());
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
    pal.fill({});
    if (!SDL_SetPaletteColors(s_palette.get(), pal.data(), 0, pal.size())) {
      logging::print(logging::Level::Error, "Failed to set palette colours: %s", SDL_GetError());
      return false;
    }
  }

  return true;
}

FASTCALL void shutdown() {
  if (!SDL_WasInit(SDL_INIT_VIDEO)) return;

  s_back_buffer.reset();
  s_front_buffer.reset();
  s_window.reset();

  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

FASTCALL void wait_for_vsync() {
  SDL_Surface * window_surface = SDL_GetWindowSurface(s_window.get());
  SDL_BlitSurface(s_front_buffer.get(), nullptr, window_surface, nullptr);
  SDL_UpdateWindowSurface(s_window.get());
}

FASTCALL void enable_text_layer(bool show) {
  // TODO
  (void)show;
}

FASTCALL void set_palette_colour(u8 pal_col, u8 r, u8 g, u8 b) {
  // TODO
  (void)pal_col;
  (void)r;
  (void)g;
  (void)b;
}

FASTCALL void clear(u8 pal_col) {
  // TODO
  (void)pal_col;
}

FASTCALL void draw_quad(int x0, int y0, int x1, int y1, u8 pal_col) {
  // TODO
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  (void)pal_col;
}

FASTCALL void undraw_quad(int x0, int y0, int x1, int y1) {
  // TODO
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
}

FASTCALL void read_scanline(int line, u8 *data) {
  // TODO
  (void)line;
  (void)data;
}

FASTCALL void write_scanline(int line, const u8 *data) {
  // TODO
  (void)line;
  (void)data;
}

FASTCALL void read_scanline_part_16(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_16*/) {
  // TODO
  (void)line;
  (void)part;
  (void)data;
}

FASTCALL void write_scanline_part_16(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_16*/) {
  // TODO
  (void)line;
  (void)part;
  (void)data;
}

FASTCALL void read_scanline_part_32(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_32*/) {
  // TODO
  (void)line;
  (void)part;
  (void)data;
}

FASTCALL void write_scanline_part_32(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_32*/) {
  // TODO
  (void)line;
  (void)part;
  (void)data;
}

} // namespace gpu
