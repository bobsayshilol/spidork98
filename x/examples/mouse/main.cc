#include "mouse.h"

#include <cstdio>
#include <cstdlib>

static void play() {
  mouse::init();

  i16 x = 0, y = 0;
  while (true) {
    // Read movement.
    i8 dx = 0, dy = 0;
    mouse::read_delta(dx, dy);
    x += dx;
    y += dy;

    // Read button presses.
    const mouse::ButtonsState state = mouse::read_buttons();
    const bool left = mouse::is_button_pressed(state, mouse::Button::Left);
    const bool middle = mouse::is_button_pressed(state, mouse::Button::Middle);
    const bool right = mouse::is_button_pressed(state, mouse::Button::Right);

    printf("(%i, %i) [%i, %i] - %i %i %i\n", x, y, dx, dy, left, middle, right);

    // Check for user input.
    if (Funcs98::kb_hit()) {
      break;
    }
  }
}

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  play();

  printf("Finished\n");
  return EXIT_SUCCESS;
}
