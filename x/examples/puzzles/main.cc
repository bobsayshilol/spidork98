#include "fe98.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "keyboard.h"
#include "logs.h"
#include "utils.h"

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


#define LOG_FILE "puzlog.txt"

#define GAME_SQUARE_SIZE GPU_HEIGHT


namespace {

void print_help(const game *ourgame) {
  if (ourgame == &mines) {
    const int top_left_x = ScreenCols_98() * 9 / 16 + 2;
    const int top_left_y = ScreenRows_98() / 4;
    ScreenPutString_98("Controls:",                     COLOUR_GREEN, top_left_x, top_left_y);
    ScreenPutString_98("WASD or arrows to move",        COLOUR_GREEN, top_left_x, top_left_y + 2);
    ScreenPutString_98("Space to add a flag",           COLOUR_GREEN, top_left_x, top_left_y + 4);
    ScreenPutString_98("Enter or E to uncover a tile",  COLOUR_GREEN, top_left_x, top_left_y + 6);
    ScreenPutString_98("R to restart",                  COLOUR_GREEN, top_left_x, top_left_y + 8);
#ifndef __EMSCRIPTEN__
    ScreenPutString_98("Q to quit",                     COLOUR_GREEN, top_left_x, top_left_y + 10);
#endif
  }
}

midend * new_game(const game *ourgame) {
  midend *me = midend_new(NULL, ourgame, &fe98::g_drapi, NULL);
  midend_new_game(me);

  // Clear any text.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();

  // Setup the palette.
  {
    int num_colours = 0;
    float *colours = midend_colours(me, &num_colours);
    for (int i = 0; i < num_colours; i++) {
      u8 r = static_cast<u8>(colours[3 * i + 0] * 255);
      u8 g = static_cast<u8>(colours[3 * i + 1] * 255);
      u8 b = static_cast<u8>(colours[3 * i + 2] * 255);
      gpu::set_palette_colour(i, r, g, b);
    }
    for (int j = num_colours; j < 32; j++) {
      gpu::set_palette_colour(j, 0, 0, 0);
    }
    sfree(colours);
  }

  // Tell the game where it can draw.
  int w = GAME_SQUARE_SIZE;
  int h = GAME_SQUARE_SIZE;
  midend_size(me, &w, &h, true, 1);
  logging::print(logging::Level::Info, "Using screen size %i x %i", w, h);
  midend_force_redraw(me);

  // Show some help on the side.
  print_help(ourgame);

  return me;
}

void free_game(midend *me) {
  midend_free(me);
}

bool update_loop(midend * & me, uclock_t & last_time) {
  // Deal with input.
  if (kbhit_98()) {
    const char ch = getch_98();
    switch (ch) {
      case KEY_UP: case 'W': case 'w':
        midend_process_key(me, 0, 0, CURSOR_UP);
        break;
      case KEY_DOWN: case 'S': case 's':
        midend_process_key(me, 0, 0, CURSOR_DOWN);
        break;
      case KEY_LEFT: case 'A': case 'a':
        midend_process_key(me, 0, 0, CURSOR_LEFT);
        break;
      case KEY_RIGHT: case 'D': case 'd':
        midend_process_key(me, 0, 0, CURSOR_RIGHT);
        break;
      case KEY_ENTER: case 'E': case 'e':
        midend_process_key(me, 0, 0, CURSOR_SELECT);
        break;
      case KEY_SPACE:
        midend_process_key(me, 0, 0, CURSOR_SELECT2);
        break;
#ifndef __EMSCRIPTEN__
      case KEY_ESCAPE: case 'Q': case 'q':
        return false;
#endif
      case 'R': case 'r': {
        const game *ourgame = midend_which_game(me);
        free_game(me);
        me = new_game(ourgame);
      } break;
    }
  }

  // Tick the timer.
  const uclock_t now = Funcs98::ticks();
  if (fe98::g_timer_active) {
    const float dt = static_cast<float>(now - last_time) / Funcs98::ticks_per_sec();
    midend_timer(me, dt);
  }
  last_time = now;

  // Update the UI.
  midend_redraw(me);
  gpu::wait_for_vsync();

  return true;
}

} // namespace



int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  logging::init(LOG_FILE);

  const unsigned seed = time(0);
  logging::print(logging::Level::Info, "RNG seed: %u", seed);
  srand(seed);

  if (!gpu::setup()) {
    logging::print(logging::Level::Error, "Failed to setup GPU\n");
    return EXIT_FAILURE;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

  midend *me = new_game(&mines);
  uclock_t last_time = Funcs98::ticks();

#if defined(__EMSCRIPTEN__)
  auto run_one = [&]{ update_loop(me, last_time); };
  run_at_fps(60, run_one);
#else
  while (true) {
    const bool finished = !update_loop(me, last_time);
    if (finished) {
      break;
    }
  }
#endif

  free_game(me);

  return EXIT_SUCCESS;
}
