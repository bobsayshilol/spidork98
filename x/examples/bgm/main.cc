#include "bgm.h"
#include "funcs.h"

#include <cstdio>
#include <cstdlib>

template <typename Funcs>
static void play() {
  Funcs::clrscr();
  printf("Running on %s\n", Funcs::name());
  printf("Press any key to stop\n");

  const int ms_per_beat = 250;
  int beat = bgm_data[1] - 1; // begin a beat before the start
  int note_idx = 0;
  while (note_idx < num_bgm_notes) {
    // Read the current note.
    int bgm_freq = bgm_data[2 * note_idx];
    int bgm_beat = bgm_data[2 * note_idx + 1];

    // Wait until it's time to play this note.
    if (bgm_beat <= beat) {
      if (bgm_freq > 0) {
        Funcs::sound(bgm_freq);
      } else {
        nosound();
      }
      note_idx++;
    }

    // Move to the next beat.
    beat++;
    Funcs::delay(ms_per_beat);

    // Check for user input.
    if (Funcs::kbhit()) {
      break;
    }
  }

  // Silence playback.
  nosound();
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
