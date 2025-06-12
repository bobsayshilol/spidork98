#include "funcs.h"
#include "pcm.h"
#include "maths.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

#define AUDIO_DEBUG 0

unsigned char generate_audio(pcm::SamplingRate::E rate, pcm::Format::E format, unsigned char last_t, signed char *out, int num_samples) {
  (void)format; // assume mono for now

#if AUDIO_DEBUG
  static bool flip = true;
  flip ^= true;
#endif // AUDIO_DEBUG

  const unsigned long sampling_rate = pcm::to_hz(rate);
  const unsigned long freq1 = 523; // C
  const unsigned long freq2 = 660; // E
  const unsigned long freq3 = 783; // G
  const unsigned long freq4 = 988; // B
  for (int t = last_t; t < num_samples + last_t; t++) {
    int val = 0;
    val += maths::sin(t * (freq1 * 256) / sampling_rate);
    val += maths::sin(t * (freq2 * 256) / sampling_rate);
    val += maths::sin(t * (freq3 * 256) / sampling_rate);
    val += maths::sin(t * (freq4 * 256) / sampling_rate);
    val /= 4;
#if AUDIO_DEBUG
    val = flip ? -t : t;
#endif // AUDIO_DEBUG
    *out++ = val;
  }

  return num_samples + last_t;
}

} // namespace

template <typename Funcs>
static bool play(int buffer_size) {
  printf("Running on %s\n", Funcs::name());

  // Allocate a new buffer for us to work with.
  signed char *buffer = static_cast<signed char*>(malloc(buffer_size));
  if (!buffer) {
    printf("Failed to allocate data\n");
    return false;
  }
  DEFER(signed char *, ptr, buffer, free(ptr));

  // Clear it out.
  memset(buffer, 0, buffer_size);

  // Setup PCM.
  const pcm::SamplingRate::E pcm_rate = pcm::SamplingRate::kHz_8_3;
  const pcm::Format::E pcm_format = pcm::Format::fmt_mono;
  if (!pcm::init(pcm_rate, pcm_format, buffer)) {
    printf("Failed to init PCM\n");
    return false;
  }

  const int hz = pcm::to_hz(pcm_rate);
  printf("Audio buffer size: %u samples (%llims @ %iHz)\n", buffer_size, buffer_size * 1000LL / hz, hz);

  printf("Press any key to stop\n");
  unsigned iteration = 0;
  unsigned refills = 0;
  unsigned char last_t = 0;
  while (true) {
    iteration++;

    // Refill if needed.
    if (pcm::is_empty()) {
      refills++;

      // Running low, generate more audio.
      last_t = generate_audio(pcm_rate, pcm_format, last_t, buffer, buffer_size);
#if 0 // debugging
if (refills & 7)
#endif
      pcm::filled(buffer_size);
      printf("Regened %i samples [%u - %u]\n", buffer_size, refills, iteration);
    }

    // Check for user input.
    if (Funcs::kb_hit()) {
      break;
    }
  }

  // Reset PCM state.
  pcm::shutdown();
  return true;
}

int main(int argc, const char **argv) {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  } else if (argc != 2) {
    printf("Usage: %s <size>\nsize must be a power of 2\n512, 1024, 2048 are good choices", argv[0]);
    return EXIT_FAILURE;
  }

  int const buffer_size = atoi(argv[1]);
  if (buffer_size < 0 || (buffer_size & (buffer_size - 1))) {
    printf("Buffer size isn't a power of 2: %i\n", buffer_size);
    return EXIT_FAILURE;
  }

  if (!play<Funcs98>(buffer_size)) {
    return EXIT_FAILURE;
  }

  printf("Finished\n");
  return EXIT_SUCCESS;
}
