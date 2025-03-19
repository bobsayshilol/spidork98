#include "funcs.h"
#include "gpuscrn.h"

#include <cstdio>
#include <cstdlib>

#include <conio.h>

namespace {

void wait_for_any_key() {
  printf("Press any key to continue\n");
  getch();
}

template <typename Funcs>
void play() {
  Funcs::clrscr();
  printf("Running on %s\n", Funcs::name());

  if (!gpu::setup()) {
    printf("Failed to setup GPU\n");
    return;
  }

  gpu::enable_text_layer(true);
  printf("GPU works!\n");
  wait_for_any_key();

  const int N = 5;
  srand(time(0));

  // Random colours.
  for (int i = 0; i < N; i++) {
    Funcs::clrscr();
    printf("Colour test %i/%i\n", i + 1, N);
    const int r = rand() % 255;
    const int g = rand() % 255;
    const int b = rand() % 255;
    printf("Random colour #%02X%02X%02X\n", r, g, b);

    gpu::set_palette_colour(i, r, g, b);
    gpu::clear(i);

    wait_for_any_key();
  }

  // Random rectangles.
  for (int i = 0; i < N; i++) {
    Funcs::clrscr();
    printf("Quad test %i/%i\n", i + 1, N);
    const int x0 = rand() % GPU_WIDTH;
    const int y0 = rand() % GPU_HEIGHT;
    const int x1 = rand() % GPU_WIDTH;
    const int y1 = rand() % GPU_HEIGHT;
    printf("Random co-ords (%i,%i) to (%i,%i)\n", x0, y0, x1, y1);

    gpu::draw_quad(x0, y0, x1, y1, i);

    wait_for_any_key();
  }

  // Cleanup.
  gpu::shutdown();
  Funcs::clrscr();
}

} // namespace

int main() {
  int m = __crt0_mtype;
  if (ISPCAT(m)) {
    printf("Can't run on PC-AT\n");
    return EXIT_FAILURE;
  } else if (ISPC98(m)) {
    play<Funcs98>();
  } else {
    printf("Unknown CPU type: %d\n", m);
    return EXIT_FAILURE;
  }

  printf("Finished\n");
  return EXIT_SUCCESS;
}
