#include "game.h"
#include "menus.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"
#include "logs.h"
#include "sound.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>

namespace game {

bool g_had_error;
#define LOG_FILE "gamelog.txt"

//

void play() {
  logging::init(LOG_FILE);

  const unsigned seed = time(0);
  logging::print(logging::Level::Info, "RNG seed: %u", seed);
  srand(seed);

  // GPU go!
  if (!gpu::setup()) {
    logging::print(logging::Level::Error, "Failed to setup GPU\n");
    g_had_error = true;
    return;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

  // Sound go!
  if (!soundsystem::init(pcm::SamplingRate::kHz_16_5)) {
    logging::print(logging::Level::Error, "Failed to setup sound system\n");
    g_had_error = true;
    return;
  }
  DEFER(void*, p, NULL, (soundsystem::shutdown()));

  // No cursor unless we need it.
  _setcursortype_98(_NOCURSOR);
  DEFER(void *, p, NULL, (_setcursortype_98(_NORMALCURSOR)));

  //

  uclock_t last_time = Funcs98::ticks();

  const menus::MenuScreen * current_menu = &menus::g_splash_menu;
  current_menu->enter();

  while (current_menu && !g_had_error) {
    // Calculate delta.
    const uclock_t now = Funcs98::ticks();
    const u32 dt = now - last_time; // ticks should be small enough to fit into 32bit
    last_time = now;

    // Main update.
    soundsystem::update();
    const menus::MenuScreen *next_menu = current_menu->update(dt);
    soundsystem::update();

    // Do state machine logic.
    if (next_menu != current_menu) {
      if (current_menu->leave) current_menu->leave();
      current_menu = next_menu;
      if (current_menu && current_menu->enter) current_menu->enter();

      // Make sure the sound system is ticked often.
      soundsystem::update();
    }
  }
}

} // namespace game

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  game::play();

  Funcs98::clear_screen();
  if (game::g_had_error) {
    printf("Error encountered\nSee " LOG_FILE " for more info\n\n");
  }
  printf("Thanks for playing!\n");
  return EXIT_SUCCESS;
}
