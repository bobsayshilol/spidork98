#include "funcs.h"
#include "pcm.h"

#include <cstdio>
#include <cstdlib>

template <typename Funcs>
static void play() {
  printf("Running on %s\n", Funcs::name());

  if (!pcm::init(pcm::SamplingRate::kHz_8_3, pcm::SampleSize::bits_8, pcm::Panning::pan_stereo)) {
    printf("Failed to init PCM\n");
    return;
  }

  printf("Press any key to stop\n");
  while (true) {
#if 1
    Funcs::delay_ms(10);
#else
    const unsigned char status = inportb(PORT_FIFO_STATUS);
    if (status & 0x40) {
      // Empty, generate more audio.
      printf("regen=0x%x\n", status);
      pcm::generate_audio();
      printf("status=0x%x\n", inportb(PORT_FIFO_STATUS));
    } else {
      Funcs::delay_ms(10);
    }
#endif

    // Check for user input.
    if (Funcs::kb_hit()) {
      break;
    }
  }

  // Reset PCM state.
  pcm::shutdown();
}

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
