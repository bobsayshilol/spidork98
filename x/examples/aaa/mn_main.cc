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

void main_menu_enter() {
  // Wait for vsync.
  gpu::wait_for_vsync();

}

const MenuScreen *main_menu_update(u32) {
  if (kbhit_98()) {
      //const char ch = getch();
      //switch (ch) {
      //  case // up, down, left, right
      //}
      return NULL;
  }

  return &g_main_menu;
}

void main_menu_leave() {
  
}

} // namespace

const MenuScreen g_main_menu = {
  main_menu_enter,
  main_menu_update,
  main_menu_leave,
};

} // namespace menus
} // namespace game
