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

namespace game {
namespace menus {

namespace {

#define SELECTION_LEVEL_SELECT 0
#define SELECTION_TOGGLE_SOUND 1
#define SELECTION_TOGGLE_VOLUME 2
#define SELECTION_CREDITS 3
#define SELECTION_QUIT 4
#define NUM_SELECTIONS 5

#define SELECTED_COLOUR COLOUR_YELLOW
#define UNSELECTED_COLOUR COLOUR_WHITE

const char * s_selection_texts[] = {
  "L E V E L  S E L E C T",
  "",
  "",
  "C R E D I T S",
  "Q U I T  G A M E",
};
STATIC_ASSERT(COUNT_OF(s_selection_texts) == NUM_SELECTIONS);
int s_selection_idx;

#define MAX_VOLUME 4
int s_volume_level = MAX_VOLUME / 2;

const MenuScreen *s_next_screen;

//

void redraw_text_ui() {
  // TODO: this could be a lot faster if we had initial full draw + redraw delta
  Funcs98::clear_screen();

  // Update what we display depending on flags.
  if (g_sound_enabled) {
    s_selection_texts[SELECTION_TOGGLE_SOUND] = "S O U N D :   O N";
    switch (s_volume_level) {
      case 0: s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :       0 %"; break;
      case 1: s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :     2 5 %"; break;
      case 2: s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :     5 0 %"; break;
      case 3: s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :     7 5 %"; break;
      case 4: s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :   1 0 0 %"; break;
    }
  } else {
    s_selection_texts[SELECTION_TOGGLE_SOUND] = "S O U N D :   N O";
    s_selection_texts[SELECTION_TOGGLE_VOLUME] = "V O L U M E :     O F F";
  }

  // Screen sizes.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24
  const u8 first_y = (num_rows - 2 * NUM_SELECTIONS) / 2;

  // Draw it.
  for (u8 i = 0; i < NUM_SELECTIONS; i++) {
    const char *text = s_selection_texts[i];
    const int text_len = strlen(text);
    const int y = first_y + i * 2;
    const int x = (num_cols - text_len) / 2;
    const bool selected = i == s_selection_idx;
    ScreenPutString_98(text, selected ? SELECTED_COLOUR | COLOUR_UNDERLINE : UNSELECTED_COLOUR, x, y);

    // Draw markers around the selection too.
    if (selected) {
      ScreenPutChar_98('\xA2', SELECTED_COLOUR, x - 2, y);
      ScreenPutChar_98('\xA3', SELECTED_COLOUR, x + text_len + 1, y);
    }
  }
}

void update_volume() {
  if (!g_sound_enabled) return;
  switch (s_volume_level) {
    case 0: pcm::set_volume(pcm::Volume::vol_min); break;
    case 1: pcm::set_volume(pcm::Volume::vol_1_quater); break;
    case 2: pcm::set_volume(pcm::Volume::vol_half); break;
    case 3: pcm::set_volume(pcm::Volume::vol_3_quater); break;
    case 4: pcm::set_volume(pcm::Volume::vol_max); break;
  }
}

//

void on_left_pressed() {
  switch (s_selection_idx) {
    case SELECTION_TOGGLE_SOUND:
      toggle_audio();
      update_volume();
      break;

    case SELECTION_TOGGLE_VOLUME:
      s_volume_level = utils::max(s_volume_level - 1, 0);
      update_volume();
      break;
  }
}

void on_right_pressed() {
  switch (s_selection_idx) {
    case SELECTION_TOGGLE_SOUND:
      toggle_audio();
      update_volume();
      break;

    case SELECTION_TOGGLE_VOLUME:
      s_volume_level = utils::min(s_volume_level + 1, MAX_VOLUME);
      update_volume();
      break;
  }
}

void on_enter_pressed() {
  switch (s_selection_idx) {
    case SELECTION_LEVEL_SELECT:
      s_next_screen = &g_level_select_menu;
      break;

    case SELECTION_TOGGLE_SOUND:
      toggle_audio();
      redraw_text_ui();
      break;

    case SELECTION_CREDITS:
      s_next_screen = &g_credits_menu;
      break;

    case SELECTION_QUIT:
      s_next_screen = NULL;
      break;
  }
}

//

void main_menu_enter() {
  logging::print(logging::Level::Info, "Entering main menu");

  // Load the background.
  // TODO
  gpu::set_palette_colour(0, 0, 0, 0);
  gpu::clear(0);
  gpu::wait_for_vsync();

  // Setup UI state.
  gpu::enable_text_layer(true);
  s_next_screen = &g_main_menu;
  s_selection_idx = 0;
  redraw_text_ui();

  // Reset keyboard input before relying on it in the main update loop.
  flush_kb_buffer();
}

const MenuScreen *main_menu_update(u32) {
  if (kbhit_98()) {
      const char ch = getch();
      bool did_thing = false;
      switch (ch) {
        case KEY_UP: case 'W': case 'w':
          s_selection_idx = (s_selection_idx + NUM_SELECTIONS - 1) % NUM_SELECTIONS;
          did_thing = true;
          break;
        case KEY_DOWN: case 'S': case 's':
          s_selection_idx = (s_selection_idx + 1) % NUM_SELECTIONS;
          did_thing = true;
          break;
        case KEY_LEFT: case 'A': case 'a':
          on_left_pressed();
          did_thing = true;
          break;
        case KEY_RIGHT: case 'D': case 'd':
          on_right_pressed();
          did_thing = true;
          break;
        case KEY_ENTER: case KEY_SPACE: case 'E': case 'e':
          on_enter_pressed();
          did_thing = true;
          break;
        case KEY_ESCAPE: case 'Q': case 'q':
          s_next_screen = NULL;
          did_thing = true;
          break;

        // Cheats.
        case 'U': case 'u':
          g_unlocked_levels = NUM_LEVELS - 1;
          break;
        case 'I': case 'i':
          g_invincible = true;
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

void main_menu_leave() {
  logging::print(logging::Level::Info, "Leaving main menu");
  gpu::enable_text_layer(false);
}

} // namespace

const MenuScreen g_main_menu = {
  main_menu_enter,
  main_menu_update,
  main_menu_leave,
};

} // namespace menus
} // namespace game
