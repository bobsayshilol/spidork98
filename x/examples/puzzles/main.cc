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

#define GAME_SQUARE_SIZE (GPU_HEIGHT * 95 / 100) // 1 row = 1/24 ~= 4%

// Available games.
extern "C" const game mines;
extern "C" const game flood;


namespace {

struct LoopMenu { enum E { Quit, Main, Preset, Game, }; };
void preset_enter(midend * me);
void main_enter();

//
// Gameplay menu.
//

void game_print_help(const game *ourgame) {
  const int top_left_x = ScreenCols_98() * GAME_SQUARE_SIZE / GPU_WIDTH;
  const int top_left_y = ScreenRows_98() / 8;
  int cur_y = top_left_y;

  ScreenPutString_98(  "Description:",                  COLOUR_GREEN, top_left_x, cur_y); cur_y += 2;
  if (ourgame == &mines) {
    ScreenPutString_98("It's minesweeper, 'nuf said.",  COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
  } else if (ourgame == &flood) {
    ScreenPutString_98("Goal is to make every tile",    COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("the same colour by repeatedly", COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("flood-filling from the top",    COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("left corner.",                  COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
  }

  cur_y += 2;
  ScreenPutString_98(  "Controls:",                     COLOUR_GREEN, top_left_x, cur_y); cur_y += 2;
  if (ourgame == &mines) {
    ScreenPutString_98("WASD or arrows to move.",       COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("Space to add a flag.",          COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("Enter or E to uncover a tile.", COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
  } else if (ourgame == &flood) {
    ScreenPutString_98("WASD or arrows to move.",       COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("Enter or E to use the",         COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
    ScreenPutString_98("selected tile's colour.",       COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
  }
  ScreenPutString_98(  "U/R for undo/redo.",            COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
  ScreenPutString_98(  "Q or Escape to quit.",          COLOUR_GREEN, top_left_x, cur_y); cur_y += 1;
}

void game_force_redraw(midend *me) {
  // Clear any text.
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

  // Draw the game.
  midend_force_redraw(me);

  // Show some help on the side.
  game_print_help(midend_which_game(me));
}

void game_new(midend *me, game_params *params) {
  // Clear any text.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();

  // Display a loading while we wait for the game to be created.
  const char *text = "Generating new game...";
  ScreenPutString_98(text, COLOUR_WHITE, (ScreenCols_98() - strlen(text)) / 2, ScreenRows_98() / 2);

  // Create the game.
  midend_set_params(me, params);
  midend_new_game(me);

  // Remove the loading text.
  Funcs98::clear_screen();

  // Tell the game where it can draw.
  int w = GAME_SQUARE_SIZE;
  int h = GAME_SQUARE_SIZE;
  midend_size(me, &w, &h, true, 1);
  logging::print(logging::Level::Info, "Using screen size %i x %i", w, h);

  game_force_redraw(me);
}

LoopMenu::E game_update_loop(midend * me, uclock_t dt) {
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
      case 'U': case 'u':
        midend_process_key(me, 0, 0, UI_UNDO);
        // Force a redraw of everything since text won't be cleared in mines.
        game_force_redraw(me);
        break;
      case 'R': case 'r':
        midend_process_key(me, 0, 0, UI_REDO);
        break;
      case KEY_ESCAPE: case 'Q': case 'q':
        preset_enter(me);
        return LoopMenu::Preset;
    }
  }

  // Tick the timer.
  if (fe98::g_timer_active) {
    const float dtf = static_cast<float>(dt) / Funcs98::ticks_per_sec();
    midend_timer(me, dtf);
  }

  // Update the UI.
  midend_redraw(me);

  return LoopMenu::Game;
}



//
// Preset menu
//

int s_preset_idx;
preset_menu *s_preset_menu;
#define NUM_PRESETS (s_preset_menu->n_entries)
#define PRESET_LIST (s_preset_menu->entries)

void preset_redraw() {
  // Clear any text.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();

  // Clear the background too.
  gpu::set_palette_colour(0, 0, 0, 0);
  gpu::clear(0);

  const int mid_x = ScreenCols_98() / 2;
  const int mid_y = ScreenRows_98() / 2;

  int cur_y = mid_y - NUM_PRESETS;
  const char * text = "Pick a preset:";
  ScreenPutString_98(text, COLOUR_WHITE, mid_x - strlen(text) / 2, cur_y);
  cur_y += 2;

  // Draw the list of games.
  for (u8 idx = 0; idx < NUM_PRESETS + 1; idx++) {
    const char *const title = idx == NUM_PRESETS ? "Back" : PRESET_LIST[idx].title;
    const bool selected = idx == s_preset_idx;
    ScreenPutString_98(title, selected ? COLOUR_GREEN : COLOUR_WHITE, mid_x - strlen(title) / 2, cur_y);
    cur_y += 2;
  }
}

void preset_enter(midend * me) {
  // Determine preset options.
  int id_limit;
  s_preset_menu = midend_get_presets(me, &id_limit);
  s_preset_idx = 0;

  preset_redraw();
}

LoopMenu::E preset_update_loop(midend * & me) {
  // Deal with input.
  if (kbhit_98()) {
    const char ch = getch_98();
    switch (ch) {
      case KEY_UP: case 'W': case 'w':
        s_preset_idx = (s_preset_idx - 1 + (NUM_PRESETS + 1)) % (NUM_PRESETS + 1);
        preset_redraw();
        break;
      case KEY_DOWN: case 'S': case 's':
        s_preset_idx = (s_preset_idx + 1) % (NUM_PRESETS + 1);
        preset_redraw();
        break;
      case KEY_ENTER: case KEY_SPACE: case 'E': case 'e':
        if (s_preset_idx == NUM_PRESETS) {
          midend_free(me);
          me = NULL;
          main_enter();
          return LoopMenu::Main;
        } else {
          game_new(me, PRESET_LIST[s_preset_idx].params);
          return LoopMenu::Game;
        }
    }
  }

  return LoopMenu::Preset;
}



//
// Main menu
//

struct GameEntry {
  const game *g;
  const char *info;
};
const GameEntry s_games[] = {
  { &mines, "Small lag on first selection. Plays fine after.", },
  { &flood, "Lags when generating a game. Plays fine.", },
#ifndef __EMSCRIPTEN__
  { NULL, NULL, }, // quit
#endif
};
u8 s_game_idx = 0;

void main_redraw() {
  // Clear any text.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();

  // Clear the background too.
  gpu::set_palette_colour(0, 0, 0, 0);
  gpu::clear(0);

  const int mid_x = ScreenCols_98() / 2;
  const int mid_y = ScreenRows_98() / 2;

  int cur_y = mid_y - 2 * COUNT_OF(s_games);
  const char * text = "BUZZLES!!";
  ScreenPutString_98(text, COLOUR_WHITE, mid_x - strlen(text) / 2, cur_y);
  cur_y += 2;

  // Draw the list of games.
  for (u8 idx = 0; idx < COUNT_OF(s_games); idx++) {
    const GameEntry entry = s_games[idx];
    const char *const name = entry.g ? entry.g->name : "Quit";
    const bool selected = idx == s_game_idx;
    ScreenPutString_98(name, selected ? COLOUR_GREEN : COLOUR_WHITE, mid_x - strlen(name) / 2, cur_y);
    cur_y += 1;

    // Display extra info about the game.
    if (selected && entry.info) {
      ScreenPutString_98(entry.info, COLOUR_GREEN, mid_x - strlen(entry.info) / 2, cur_y);
      cur_y += 1;
    }
    cur_y += 1;
  }
}

void main_enter() {
  main_redraw();
}

LoopMenu::E main_update_loop(midend * & me) {
  // Deal with input.
  if (kbhit_98()) {
    const char ch = getch_98();
    switch (ch) {
      case KEY_UP: case 'W': case 'w':
        s_game_idx = (s_game_idx - 1 + COUNT_OF(s_games)) % COUNT_OF(s_games);
        main_redraw();
        break;
      case KEY_DOWN: case 'S': case 's':
        s_game_idx = (s_game_idx + 1) % COUNT_OF(s_games);
        main_redraw();
        break;
      case KEY_ENTER: case KEY_SPACE: case 'E': case 'e': {
        const GameEntry entry = s_games[s_game_idx];
        if (entry.g == NULL) {
          return LoopMenu::Quit;
        } else {
          // Make a new midend and move to selecting a preset for it.
          me = midend_new(NULL, entry.g, &fe98::g_drapi, NULL);
          preset_enter(me);
          return LoopMenu::Preset;
        }
      } break;
    }
  }

  return LoopMenu::Main;
}



//
//
//

struct LoopState {
  uclock_t last_time;
  midend *me;
  LoopMenu::E menu;

  LoopState() : last_time(), me(NULL), menu(LoopMenu::Main) {}
};

bool update_loop(LoopState & state) {
  const uclock_t now = Funcs98::ticks();
  const uclock_t dt = (now - state.last_time);
  state.last_time = now;

  // Handle state changes.
  switch (state.menu) {
    case LoopMenu::Quit:
      return false;
    case LoopMenu::Main:
      state.menu = main_update_loop(state.me);
      break;
    case LoopMenu::Preset:
      state.menu = preset_update_loop(state.me);
      break;
    case LoopMenu::Game:
      state.menu = game_update_loop(state.me, dt);
      break;
  }

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

  fe98::g_timer_active = false;
  fe98::g_background_colour[0] = 0.2f;
  fe98::g_background_colour[1] = 0.2f;
  fe98::g_background_colour[2] = 0.2f;

  LoopState state;
  main_redraw();

#if defined(__EMSCRIPTEN__)
  auto run_one = [&]{ update_loop(state); };
  run_at_fps(60, run_one);
#else
  while (true) {
    const bool finished = !update_loop(state);
    if (finished) {
      break;
    }
  }
#endif

  return EXIT_SUCCESS;
}
