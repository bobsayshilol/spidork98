extern "C" {
#include "puzzles.h"
}

#include "funcs.h"
#include "gpuscrn.h"
#include "logs.h"

#include <stdarg.h>

#define LOG_FILE "puzlog.txt"

// Available games.
extern "C" const game mines;



//
// Frontend functions.
//

void frontend_default_colour(frontend *fe, float *output) {
  // TODO
  (void)fe;
  (void)output;
}

void get_random_seed(void **randseed, int *randseedsize) {
  // TODO
  char *c = snewn(1, char);
  *c = 0;
  *randseed = c;
  *randseedsize = 1;
}

void deactivate_timer(frontend *fe) {
  // TODO
  (void)fe;
}

void activate_timer(frontend *fe) {
  // TODO
  (void)fe;
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

void document_add_puzzle(document *doc, const game *game, game_params *par, game_ui *ui, game_state *st, game_state *st2) {
  // TODO
  (void)doc;
  (void)game;
  (void)par;
  (void)ui;
  (void)st;
  (void)st2;
}



namespace {

//
// Drawing
//

void fe98_draw_text(drawing *dr, int x, int y, int fonttype, int fontsize, int align, int colour, const char *text) {
  // TODO
  (void)dr;
  (void)x;
  (void)y;
  (void)fonttype;
  (void)fontsize;
  (void)align;
  (void)colour;
  (void)text;
}

void fe98_draw_rect(drawing *dr, int x, int y, int w, int h, int colour) {
  // TODO
  (void)dr;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)colour;
}

void fe98_draw_line(drawing *dr, int x1, int y1, int x2, int y2, int colour) {
  // TODO
  (void)dr;
  (void)x1;
  (void)y1;
  (void)x2;
  (void)y2;
  (void)colour;
}

void fe98_draw_polygon(drawing *dr, const int *coords, int npoints, int fillcolour, int outlinecolour) {
  // TODO
  (void)dr;
  (void)coords;
  (void)npoints;
  (void)fillcolour;
  (void)outlinecolour;
}

void fe98_draw_circle(drawing *dr, int cx, int cy, int radius, int fillcolour, int outlinecolour) {
  // TODO
  (void)dr;
  (void)cx;
  (void)cy;
  (void)radius;
  (void)fillcolour;
  (void)outlinecolour;
}

//void draw_update(drawing *dr, int x, int y, int w, int h);

void fe98_clip(drawing *dr, int x, int y, int w, int h) {
  // TODO
  (void)dr;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

void fe98_unclip(drawing *dr) {
  // TODO
  (void)dr;
}

void fe98_start_draw(drawing *dr) {
  // TODO
  (void)dr;
}

void fe98_end_draw(drawing *dr) {
  // TODO
  (void)dr;
}

void fe98_status_bar(drawing *dr, const char *text) {
  // TODO
  (void)dr;
  (void)text;
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
  return bl;
}

void fe98_blitter_free(drawing *dr, blitter *bl) {
  // TODO
  (void)dr;
  sfree(bl);
}

void fe98_blitter_save(drawing *dr, blitter *bl, int x, int y) {
  // TODO
  (void)dr;
  (void)bl;
  (void)x;
  (void)y;
}

void fe98_blitter_load(drawing *dr, blitter *bl, int x, int y) {
  // TODO
  (void)dr;
  (void)bl;
  (void)x;
  (void)y;
}

void fe98_begin_doc(drawing *dr, int pages) {
  // TODO
  (void)dr;
  (void)pages;
}

void fe98_begin_page(drawing *dr, int number) {
  // TODO
  (void)dr;
  (void)number;
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
}

void fe98_end_puzzle(drawing *dr) {
  // TODO
  (void)dr;
}

void fe98_end_page(drawing *dr, int number) {
  // TODO
  (void)dr;
  (void)number;
}

void fe98_end_doc(drawing *dr) {
  // TODO
  (void)dr;
}

void fe98_line_width(drawing *dr, float width) {
  // TODO
  (void)dr;
  (void)width;
}

void fe98_line_dotted(drawing *dr, bool dotted) {
  // TODO
  (void)dr;
  (void)dotted;
}

char *fe98_text_fallback(drawing *dr, const char *const *strings, int nstrings) {
  // TODO
  (void)dr;
  (void)nstrings;
  return dupstr(strings[0]);
}

//void fe98_draw_thick_line(drawing *dr, float thickness, float x1, float y1, float x2, float y2, int colour);

} // namespace



int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  logging::init(LOG_FILE);

  if (!gpu::setup()) {
    logging::print(logging::Level::Error, "Failed to setup GPU\n");
    return EXIT_FAILURE;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));


  const drawing_api drapi {
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

  midend *me = midend_new(NULL, &mines, &drapi, NULL);
  midend_new_game(me);

  int w = INT_MAX;
  int h = INT_MAX;
  midend_size(me, &w, &h, false, 1);
  midend_redraw(me);

  midend_free(me);

  return EXIT_SUCCESS;
}
