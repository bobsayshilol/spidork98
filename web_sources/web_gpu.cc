#include "gpuscrn.h"

#include "web_common.h"

namespace gpu {

DrawTo::E g_draw_to;

FASTCALL bool setup() {
  // TODO
  return false;
}

FASTCALL void shutdown() {
  // TODO
}

FASTCALL void wait_for_vsync() {
  // TODO
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
