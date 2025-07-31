#include "game.h"
#include "menus.h"
#include "pal_fade.h"

#include "funcs.h"
#include "images.h"
#include "gpuscrn.h"
#include "macros.h"
#include "logs.h"
#include "pcm.h"
#include "utils.h"

#include <conio.h>

// TODO: lots of stuff copied from the main menu

namespace game {
namespace menus {

namespace {

#define SELECTED_COLOUR COLOUR_YELLOW
#define UNSELECTED_COLOUR COLOUR_WHITE

#define NUM_UNLOCKED_LEVELS (g_unlocked_levels + 1) // counting tutorial as level 0
#define NUM_UI_ELEMS (NUM_UNLOCKED_LEVELS + 1)
#define SELECTION_BACK (NUM_UI_ELEMS - 1)

const char * const s_selection_texts[] = {
  "T R A I N I N G",
  "F I R S T   O P E R A T I O N",
  "A   N E W   E N E M Y",
  "O U T   O F   H A N D",
};
STATIC_ASSERT(COUNT_OF(s_selection_texts) == NUM_LEVELS);
int s_selection_idx;

//                            >|< about here is the limit (26 chars)
struct DescText {
  const char *const * lines;
  int num_lines;
};
const char * const s_tutorial_lines[] = {
  "Learn the controls against a",
  "training dummy.",
  "No time to write an in-level",
  "tutorial, so:",
  "Shoot the enemy until it drops",
  "buckos, then swoop in and save",
  "them. Some enemies will have",
  "multiple stages.",
};
const char * const s_level1_lines[] = {
  "Aliens have been sighted",
  "stealing buckos.",
  "Head out there and stop the",
  "aliens before they abduct",
  "any more!",
};
const char * const s_level2_lines[] = {
  "A new foe - the Highly Armoured",
  "Weaponised Creature (H.A.W.C)",
  "- has been spotted.",
  "It appears to be abducting",
  "buckos too!",
};
const char * const s_level3_lines[] = {
  "H.A.W.C.s - two of them -",
  "have been spotted on the",
  "radar. It's up to you to",
  "stop them!",
  "(This is the last level)",
};
DescText const s_selection_descs[] = {
  { s_tutorial_lines, COUNT_OF(s_tutorial_lines), },
  { s_level1_lines, COUNT_OF(s_level1_lines), },
  { s_level2_lines, COUNT_OF(s_level2_lines), },
  { s_level3_lines, COUNT_OF(s_level3_lines), },
};
STATIC_ASSERT(COUNT_OF(s_selection_descs) == NUM_LEVELS);

const char * const s_back_lines[] = {
  "Return to the MAIN MENU",
};
DescText const s_back_desc = {
  s_back_lines, COUNT_OF(s_back_lines),
};

const MenuScreen *s_next_screen;

//

void redraw_text_ui() {
  // TODO: this could be a lot faster if we had initial full draw + redraw delta
  Funcs98::clear_screen();

  // Screen sizes.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24
  const u8 first_y = (num_rows - 2 * NUM_UI_ELEMS) / 2;

  // Draw the selectables.
  for (u8 i = 0; i < NUM_UI_ELEMS; i++) {
    const char *text = (i == SELECTION_BACK) ? "B A C K" : s_selection_texts[i];
    const int text_len = strlen(text);
    const int y = first_y + i * 2;
    const int x = num_cols / 4 - text_len / 2;
    const bool selected = i == s_selection_idx;
    ScreenPutString_98(text, selected ? SELECTED_COLOUR | COLOUR_UNDERLINE : UNSELECTED_COLOUR, x, y);

    // Draw markers around the selection too.
    if (selected) {
      ScreenPutChar_98('\xA2', SELECTED_COLOUR, x - 2, y);
      ScreenPutChar_98('\xA3', SELECTED_COLOUR, x + text_len + 1, y);
    }
  }

  // Border is in the image.
  const u8 desc_y = num_rows / 2 - 3;
  const u8 desc_x = num_cols / 2 + 2;

  // Draw the desc.
  const DescText selection_desc = (s_selection_idx == SELECTION_BACK) ? s_back_desc : s_selection_descs[s_selection_idx];
  for (int line = 0; line < selection_desc.num_lines; line++) {
    const char *text = selection_desc.lines[line];
    const int y = desc_y + line;
    ScreenPutString_98(text, COLOUR_WHITE, desc_x + 2, y);
  }
}

//

void on_enter_pressed() {
  if (s_selection_idx == SELECTION_BACK) {
    s_next_screen = &g_main_menu;
    return;
  }

  g_level_selected = s_selection_idx;
  s_next_screen = &g_playing_menu;
}

//

void level_select_enter() {
  logging::print(logging::Level::Info, "Entering level select menu");

  // Load the background.
  // TODO
  gpu::set_palette_colour(0, 0, 0, 0);
  gpu::clear(0);
  gpu::wait_for_vsync();

  // Setup UI state.
  gpu::enable_text_layer(true);
  s_next_screen = &g_level_select_menu;
  s_selection_idx = 0;
  redraw_text_ui();

  // Reset keyboard input before relying on it in the main update loop.
  flush_kb_buffer();
}

const MenuScreen *level_select_update(u32) {
  if (kbhit_98()) {
      const char ch = getch();
      bool did_thing = false;
      switch (ch) {
        case KEY_UP: case 'W': case 'w':
          s_selection_idx = (s_selection_idx + NUM_UI_ELEMS - 1) % NUM_UI_ELEMS;
          did_thing = true;
          break;
        case KEY_DOWN: case 'S': case 's':
          s_selection_idx = (s_selection_idx + 1) % NUM_UI_ELEMS;
          did_thing = true;
          break;
        case KEY_ENTER: case KEY_SPACE: case 'E': case 'e':
          on_enter_pressed();
          did_thing = true;
          break;
        case KEY_ESCAPE: case 'Q': case 'q':
          s_next_screen = &g_main_menu;
          did_thing = true;
          break;
      }
      if (did_thing) {
        redraw_text_ui();
        play_click_sound();
      }
  }

  gpu::wait_for_vsync();
  return s_next_screen;
}

void level_select_leave() {
  logging::print(logging::Level::Info, "Leaving level select menu");
  gpu::enable_text_layer(false);
}

} // namespace

const MenuScreen g_level_select_menu = {
  level_select_enter,
  level_select_update,
  level_select_leave,
};

} // namespace menus
} // namespace game
