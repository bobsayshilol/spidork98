#include "funcs.h"
#include "gpuscrn.h"
#include "loading.h"
#include "logs.h"
#include "sound.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>

namespace game {

void loader(LoadingProgress::E progress) {
  (void)progress;
}

void play() {
  logging::init("gamelog.txt");

  const unsigned seed = time(0);
  logging::print(logging::Level::Info, "RNG seed: %u", seed);
  srand(seed);

  // No cursor unless we need it.
  _setcursortype_98(_NOCURSOR);
  DEFER(void *, p, NULL, (_setcursortype_98(_NORMALCURSOR)));

  // Kick off a loading screen.
  loading_screen(loader);
}

} // namespace game

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  game::play();

  Funcs98::clear_screen();
  printf("Thanks for playing!\n");
  return EXIT_SUCCESS;
}
