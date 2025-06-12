#include "funcs.h"
#include "pcm.h"
#include "maths.h"

#include <cstdio>
#include <cstdlib>

namespace {

// I think there's a bug in the emulator since audio continues playing after quitting.
// But it's equally likely that there's a hole in my understanding of the FIFO.
// For now, use bigger buffer sizes to reduce hitching (it's still bad though).
#define AUDIO_BUFFER_SAMPLES (1024 * 2)
STATIC_ASSERT(AUDIO_BUFFER_SAMPLES < 0x8000);
signed char s_audio_buffer[AUDIO_BUFFER_SAMPLES];

#define AUDIO_DEBUG 0

unsigned char generate_audio(pcm::SamplingRate::E rate, pcm::Format::E format, unsigned char last_t, int num_samples) {
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
  signed char *out = s_audio_buffer;
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
static void play() {
  printf("Running on %s\n", Funcs::name());

  const pcm::SamplingRate::E pcm_rate = pcm::SamplingRate::kHz_8_3;
  const pcm::Format::E pcm_format = pcm::Format::fmt_mono;
  if (!pcm::init(pcm_rate, pcm_format, s_audio_buffer)) {
    printf("Failed to init PCM\n");
    return;
  }

  const int hz = pcm::to_hz(pcm_rate);
  printf("Audio buffer size: %u samples (%llims @ %iHz)\n", AUDIO_BUFFER_SAMPLES, AUDIO_BUFFER_SAMPLES * 1000LL / hz, hz);

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
      last_t = generate_audio(pcm_rate, pcm_format, last_t, AUDIO_BUFFER_SAMPLES);
#if 0 // debugging
if (iteration & 7)
#endif
      pcm::filled(AUDIO_BUFFER_SAMPLES);
      printf("Regened %i samples [%u - %u]\n", AUDIO_BUFFER_SAMPLES, refills, iteration);
    }

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
