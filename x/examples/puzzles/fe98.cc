#include "fe98.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "keyboard.h"
#include "logs.h"
#include "utils.h"

#include <stdarg.h>


#define FE98_UNIMPLEMENTED() logging::print(logging::Level::Warning, "Unimplemented: %s", __func__)



//
// Frontend functions.
//

namespace fe98 {

bool g_timer_active = false;

} // namespace fe98

void frontend_default_colour(frontend *, float *output) {
  output[0] = 0;
  output[1] = 0;
  output[2] = 0;
}

void get_random_seed(void **randseed, int *randseedsize) {
  const int elem_size = utils::max(sizeof(int), sizeof(time_t));
  char *data = snewn(elem_size * 2, char);
  *reinterpret_cast<int*>(data) = rand();
  *reinterpret_cast<time_t*>(data + elem_size) = time(NULL);
  *randseed = data;
  *randseedsize = elem_size * 2;
}

void deactivate_timer(frontend *) {
  fe98::g_timer_active = false;
}

void activate_timer(frontend *) {
  fe98::g_timer_active = true;
}

void fatal(const char *fmt, ...) {
  logging::print(logging::Level::Error, "Fatal error from game");

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap); // TODO
  //logging::print(logging::Level::Error, fmt, ap);
  va_end(ap);

  // TODO: longjmp instead
  gpu::shutdown();
  exit(1);
}

void debug_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap); // TODO
  //logging::print(logging::Level::Info, fmt, ap);
  va_end(ap);
}

void document_add_puzzle(document *doc, const struct game *game_, game_params *par, game_ui *ui, game_state *st, game_state *st2) {
  // TODO
  (void)doc;
  (void)game_;
  (void)par;
  (void)ui;
  (void)st;
  (void)st2;
  FE98_UNIMPLEMENTED();
}



//
// Misc API functions
//

namespace {

char *fe98_text_fallback(drawing *, const char *const *strings, int) {
  FE98_UNIMPLEMENTED();
  return dupstr(strings[0]);
}

void fe98_status_bar(drawing *, const char *text) {
  ScreenPutString_98(text, COLOUR_GREEN, ScreenCols_98() / 8, ScreenRows_98() - 1);
}

} // namespace



//
// Drawing
//

namespace {

void fe98_draw_text(drawing *, int x, int y, int fontttype, int fontsize, int align, int colour, const char *text) {
  (void)fontttype;
  (void)fontsize;
  (void)align;
  (void)colour;
  x = (x * ScreenCols_98() + GPU_WIDTH / 2) / GPU_WIDTH;
  y = (y * ScreenRows_98() + GPU_HEIGHT / 2) / GPU_HEIGHT;
  ScreenPutString_98(text, COLOUR_RED, x, y);
}

void fe98_draw_rect(drawing *, int x, int y, int w, int h, int colour) {
  gpu::draw_quad(x, y, x + w, y + h, colour);
}

void fe98_draw_line(drawing *dr, int x1, int y1, int x2, int y2, int colour) {
  // TODO
  (void)dr;
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
  (void)colour;
  FE98_UNIMPLEMENTED();
}

#define FE98_DRAW_SHIFT 8

FORCEINLINE void fe98_draw_triangle_half(int start_x, int end_x, int start_y, int end_y, int dsx, int dex, int fillcolour, int outlinecolour) {
  int y = start_y;
  while (y < end_y) {
    const int sx = start_x >> FE98_DRAW_SHIFT;
    const int ex = end_x >> FE98_DRAW_SHIFT;

    // TODO: why are we inclusive on x but not y...
    gpu::draw_quad(sx, y, ex, y + 1, fillcolour);
    gpu::draw_quad(sx, y, sx, y + 1, outlinecolour);
    gpu::draw_quad(ex, y, ex, y + 1, outlinecolour);

    start_x += dsx;
    end_x += dex;
    ++y;
  }
}

struct Point { int x, y; };

void fe98_draw_triangle(Point (&pts)[3], int fillcolour, int outlinecolour) {
  // TODO: move this (or polygon) into gpu so we don't have to draw_quad()

  //
  //          x <- p0
  //         /|
  //        / |
  // p1 -> x--x <- m
  //        \ |
  //         \|
  //          x <- p2
  //

  // Sort by y.
  if (pts[1].y < pts[0].y) utils::swap(pts[1], pts[0]);
  if (pts[2].y < pts[0].y) utils::swap(pts[2], pts[0]);
  if (pts[2].y < pts[1].y) utils::swap(pts[2], pts[1]);

  // All 3 points lie on a line, discard.
  if (pts[0].y == pts[2].y) return;

  const int start_y = pts[0].y;
  const int mid_y = pts[1].y;
  const int end_y = pts[2].y;

  if (start_y == mid_y) {
    // p0+p1 are flat on top.
    int start_x = pts[0].x << FE98_DRAW_SHIFT;
    int end_x = pts[1].x << FE98_DRAW_SHIFT;
    if (end_x < start_x) utils::swap(end_x, start_x);
    const int x2 = pts[2].x << FE98_DRAW_SHIFT;
    const int dsx = (x2 - start_x) / (end_y - start_y);
    const int dex = (x2 - end_x) / (end_y - start_y);
    fe98_draw_triangle_half(start_x, end_x, start_y, end_y, dsx, dex, fillcolour, outlinecolour);

  } else if (mid_y == end_y) {
    // p1+p2 are flat on bottom
    int start_x = pts[0].x << FE98_DRAW_SHIFT;
    int end_x = start_x;
    int x1 = pts[2].x << FE98_DRAW_SHIFT;
    int x2 = pts[2].x << FE98_DRAW_SHIFT;
    if (x2 < x1) utils::swap(x2, x1);
    const int dsx = (x1 - start_x) / (end_y - start_y);
    const int dex = (x2 - end_x) / (end_y - start_y);
    fe98_draw_triangle_half(start_x, end_x, start_y, end_y, dsx, dex, fillcolour, outlinecolour);

  } else {
    // Find intersection point.
    const int dmy = pts[2].y - pts[0].y;
    const int dmx = pts[2].x - pts[0].x;
    int m_x = pts[0].x + dmx * (pts[1].y - pts[0].y) / dmy;

    // m is bigger.
    if (m_x < pts[1].x) utils::swap(m_x, pts[1].x);

    int start_x = pts[0].x << FE98_DRAW_SHIFT;
    int end_x = start_x;

    const int p1x = pts[1].x << FE98_DRAW_SHIFT;
    const int p2x = pts[2].x << FE98_DRAW_SHIFT;
    const int mx = m_x << FE98_DRAW_SHIFT;

    const int dsx1 = (p1x - start_x) / (mid_y - start_y);
    const int dex1 = (mx - end_x) / (mid_y - start_y);
    const int dsx2 = (p2x - p1x) / (end_y - mid_y);
    const int dex2 = (p2x - mx) / (end_y - mid_y);

    fe98_draw_triangle_half(start_x, end_x, start_y, mid_y, dsx1, dex1, fillcolour, outlinecolour);
    fe98_draw_triangle_half(p1x, mx, mid_y, end_y, dsx2, dex2, fillcolour, outlinecolour);
  }
}

void fe98_draw_polygon(drawing *,const int *coords, int npoints, int fillcolour, int outlinecolour) {
  // Decompose into tris because lazy.
  // This is wrong: polygon isn't necessarily concave...
  Point pts[3];
  for (int pt = 2; pt < npoints; pt++) {
    pts[0].x = coords[0];
    pts[0].y = coords[1];
    pts[1].x = coords[2];
    pts[1].y = coords[3];
    pts[2].x = coords[2 * pt + 0];
    pts[2].y = coords[2 * pt + 1];
    fe98_draw_triangle(pts, fillcolour, outlinecolour);
  }
}

void fe98_draw_circle(drawing *dr, int cx, int cy, int radius, int fillcolour, int outlinecolour) {
  // TODO
  (void)dr;
  (void)cx;
  (void)cy;
  (void)radius;
  (void)fillcolour;
  (void)outlinecolour;
  FE98_UNIMPLEMENTED();
}

void fe98_line_width(drawing *dr, float width) {
  // TODO
  (void)dr;
  (void)width;
  FE98_UNIMPLEMENTED();
}

void fe98_line_dotted(drawing *dr, bool dotted) {
  // TODO
  (void)dr;
  (void)dotted;
  FE98_UNIMPLEMENTED();
}

void fe98_clip(drawing *dr, int x, int y, int w, int h) {
  // TODO
  (void)dr;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  FE98_UNIMPLEMENTED();
}

void fe98_unclip(drawing *dr) {
  // TODO
  (void)dr;
  FE98_UNIMPLEMENTED();
}

void fe98_start_draw(drawing *) {
  // Nothing to do
}

void fe98_end_draw(drawing *) {
  // Nothing to do
}

} // namespace



//
// Blitting
//

struct blitter { int w, h; };

namespace {

blitter *fe98_blitter_new(drawing *dr, int w, int h) {
  // TODO
  (void)dr;
  blitter *bl = snew(blitter);
  bl->w = w;
  bl->h = h;
  FE98_UNIMPLEMENTED();
  return bl;
}

void fe98_blitter_free(drawing *dr, blitter *bl) {
  // TODO
  (void)dr;
  sfree(bl);
  FE98_UNIMPLEMENTED();
}

void fe98_blitter_save(drawing *dr, blitter *bl, int x, int y) {
  // TODO
  (void)dr;
  (void)bl;
  (void)x;
  (void)y;
  FE98_UNIMPLEMENTED();
}

void fe98_blitter_load(drawing *dr, blitter *bl, int x, int y) {
  // TODO
  (void)dr;
  (void)bl;
  (void)x;
  (void)y;
  FE98_UNIMPLEMENTED();
}

} // namespace



//
// Printing
//

namespace {

void fe98_begin_doc(drawing *dr, int pages) {
  // TODO
  (void)dr;
  (void)pages;
  FE98_UNIMPLEMENTED();
}

void fe98_begin_page(drawing *dr, int number) {
  // TODO
  (void)dr;
  (void)number;
  FE98_UNIMPLEMENTED();
}

void fe98_begin_puzzle(drawing *dr, float xm, float xc, float ym, float yc, int pw, int ph, float wmm) {
  // TODO
  (void)dr;
  (void)xm;
  (void)xc;
  (void)ym;
  (void)yc;
  (void)pw;
  (void)ph;
  (void)wmm;
  FE98_UNIMPLEMENTED();
}

void fe98_end_puzzle(drawing *dr) {
  // TODO
  (void)dr;
  FE98_UNIMPLEMENTED();
}

void fe98_end_page(drawing *dr, int number) {
  // TODO
  (void)dr;
  (void)number;
  FE98_UNIMPLEMENTED();
}

void fe98_end_doc(drawing *dr) {
  // TODO
  (void)dr;
  FE98_UNIMPLEMENTED();
}

} // namespace



//
// drawing_api
//

namespace fe98 {

const drawing_api g_drapi = {
  1,
  fe98_draw_text,
  fe98_draw_rect,
  fe98_draw_line,
  fe98_draw_polygon,
  fe98_draw_circle,
  NULL /* draw_update */,
  fe98_clip,
  fe98_unclip,
  fe98_start_draw,
  fe98_end_draw,
  fe98_status_bar,
  fe98_blitter_new,
  fe98_blitter_free,
  fe98_blitter_save,
  fe98_blitter_load,
  fe98_begin_doc,
  fe98_begin_page,
  fe98_begin_puzzle,
  fe98_end_puzzle,
  fe98_end_page,
  fe98_end_doc,
  fe98_line_width,
  fe98_line_dotted,
  fe98_text_fallback,
  NULL /* draw_thick_line */
};

} // namespace fe98


