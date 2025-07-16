#ifndef GAME_MENUS_H
#define GAME_MENUS_H

#include "types.h"

namespace game {
namespace menus {

// TODO: enum-based might be faster on a 486
// TODO: or do the looping in each state and avoid this completely
struct MenuScreen {
  void (*enter)();
  const MenuScreen * (*update)(u32 delta_ticks);
  void (*leave)();
};

extern const MenuScreen g_splash_menu;
extern const MenuScreen g_main_menu;
extern const MenuScreen g_options_menu;
extern const MenuScreen g_credits_menu;
extern const MenuScreen g_level_select_menu;
extern const MenuScreen g_playing_menu;

} // namespace menus
} // namespace game

#endif
