#include "funcs.h"
#include "sound.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>

namespace {

bool play(pcm::SamplingRate::E pcm_rate, const char **filenames, int num_files, const bool *loop_mask) {
  // Setup sound system.
  if (!soundsystem::init(pcm_rate)) {
    printf("Failed to init sound system\n");
    return false;
  }

  for (soundsystem::Handle h = 0; h < num_files; h++) {
    if (!soundsystem::load_sound(h, filenames[h], loop_mask[h])) {
      printf("Failed to load %s\n", filenames[h]);
    }
  }

  printf(
    "1-5 to control volume\n"
    "ASDF to play sound file 0-4 (ie S plays the second file)\n"
    "ZXCV to stop sound file 0-4 (ie X stop the second file)\n"
    "T for timings\n"
    "Q to quit\n"
  );

  bool verbose = false;
  while (true) {
    // Tick the sound system.
    const uclock_t update_start = Funcs98::ticks();
    soundsystem::update();
    const uclock_t update_dt = Funcs98::ticks() - update_start;

    if (verbose) {
      printf("Sound system tick took %llu ticks (%llums)\n"
        , update_dt
        , update_dt * 1000 / Funcs98::ticks_per_sec()
      );
    }

    // Check for user input.
    if (Funcs98::kb_hit()) {
      bool quit = false;
      const char ch = getch();
      switch (ch) {
        case '1': pcm::set_volume(pcm::Volume::vol_min); break;
        case '2': pcm::set_volume(pcm::Volume::vol_1_quater); break;
        case '3': pcm::set_volume(pcm::Volume::vol_half); break;
        case '4': pcm::set_volume(pcm::Volume::vol_3_quater); break;
        case '5': pcm::set_volume(pcm::Volume::vol_max); break;

        STATIC_ASSERT(MAX_SOUNDS == 4);
        case 'a': case 'A': if (num_files >= 1) { soundsystem::play(0); } break;
        case 's': case 'S': if (num_files >= 2) { soundsystem::play(1); } break;
        case 'd': case 'D': if (num_files >= 3) { soundsystem::play(2); } break;
        case 'f': case 'F': if (num_files >= 4) { soundsystem::play(3); } break;

        case 'z': case 'Z': if (num_files >= 1) { soundsystem::stop(0); } break;
        case 'x': case 'X': if (num_files >= 2) { soundsystem::stop(1); } break;
        case 'c': case 'C': if (num_files >= 3) { soundsystem::stop(2); } break;
        case 'v': case 'V': if (num_files >= 4) { soundsystem::stop(3); } break;

        case 'q': case 'Q': quit = true; break;
        case 't': case 'T': verbose = !verbose; break;
      }
      if (quit) {
        break;
      }
    }

    // Emulate rendering a frame.
    Funcs98::delay_ms(60);
  }

  // Shutdown sound system.
  soundsystem::shutdown();
  return true;
}

} // namespace

int main(int argc, const char **argv) {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  } else if (argc <= 3 || argc > 3 + MAX_SOUNDS) {
    printf(
      "Usage:\n"
      "  %s <rate> <loop> <file> ...\n"
      "<rate> is sampling rate in kHz (8,11,16)\n"
      "<loop> bitmask of which files to loop\n"
      "       (5 would mean loop the first and third files)\n"
      "<file> a space separated list of files to play\n"
      "       Can be no longer than MAX_SOUNDS (%u) files\n"
      , argv[0]
      , MAX_SOUNDS
    );
    return EXIT_FAILURE;
  }

  int const sampling_rate_int = atoi(argv[1]);
  pcm::SamplingRate::E sampling_rate;
  switch (sampling_rate_int) {
    case 8: sampling_rate = pcm::SamplingRate::kHz_8_3; break;
    case 11: sampling_rate = pcm::SamplingRate::kHz_11; break;
    case 16: sampling_rate = pcm::SamplingRate::kHz_16_5; break;
    default:
      printf("Sampling rate given isn't supported: %i\n", sampling_rate_int);
      return EXIT_FAILURE;
  }
  
  int const mask = atoi(argv[2]);
  bool loop_mask[MAX_SOUNDS] = {};
  for (int i = 0; i < MAX_SOUNDS; i++) {
    loop_mask[i] = mask & (1 << i);
  }

  if (!play(sampling_rate, argv + 3, argc - 3, loop_mask)) {
    return EXIT_FAILURE;
  }

  printf("Finished\n");
  return EXIT_SUCCESS;
}
