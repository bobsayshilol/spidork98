#include "bgm.h"
#include "funcs.h"

#include <cstdio>
#include <cstdlib>

template <typename Funcs>
static void play() {
  Funcs::ScreenClear();
  printf("Running on %s\n", Funcs::name());

  const int ms_per_beat = 200;
  int beat = bgm_data[1] - 1; // begin a beat before the start
  int note_idx = 0;
  while (note_idx < num_bgm_notes) {
    int bgm_freq = bgm_data[2 * note_idx];
    int bgm_beat = bgm_data[2 * note_idx + 1];
    if (bgm_beat <= beat) {
      Funcs::sound(bgm_freq);
      note_idx++;
    }
    beat++;
    Funcs::delay(ms_per_beat);
  }

  // Silence playback.
  Funcs::sound(0);
}

int main() {
  int m = __crt0_mtype;
  if (ISPCAT(m)) {
    play<FuncsAT>();
  } else if (ISPC98(m)) {
    play<Funcs98>();
  } else {
    printf("Unknown CPU type: %d\n", m);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
