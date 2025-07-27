#include "game.h"
#include "menus.h"
#include "pal_fade.h"

#include "funcs.h"
#include "images.h"
#include "gpuscrn.h"
#include "logs.h"

#include <conio.h>

namespace game {
namespace menus {

namespace {

const char * const s_text[] = {
  "Design, code, music: me",
  "",
  "Press any key to return",
};

void redraw_text_ui() {
  Funcs98::clear_screen();

  // Screen sizes.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24
  const u8 num_texts = COUNT_OF(s_text);
  const u8 first_y = (num_rows - num_texts) / 2;

  // Draw it.
  for (u8 i = 0; i < num_texts; i++) {
    const char *text = s_text[i];
    const int text_len = strlen(text);
    const int y = first_y + i;
    const int x = (num_cols - text_len) / 2;
    ScreenPutString_98(text, COLOUR_WHITE, x, y);
  }
}

//

void credits_menu_enter() {
  logging::print(logging::Level::Info, "Entering credits menu");

  gpu::enable_text_layer(true);
  redraw_text_ui();
}

const MenuScreen *credits_menu_update(u32) {
  if (kbhit_98()) {
    const int ch = getch();
    if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == KEY_SPACE || ch == KEY_ENTER || ch == KEY_ESCAPE) {
      return &g_main_menu;
    }
  }

  gpu::wait_for_vsync();
  return &g_credits_menu;
}

void credits_menu_leave() {
  logging::print(logging::Level::Info, "Leaving credits menu");

  gpu::enable_text_layer(false);
}

} // namespace

const MenuScreen g_credits_menu = {
  credits_menu_enter,
  credits_menu_update,
  credits_menu_leave,
};

} // namespace menus
} // namespace game
