#include "funcs.h"
#include "pcm.h"
#include "maths.h"

#include "bgm.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

STATIC_ASSERT(sizeof(unsigned) == 4);

unsigned (*generate_audio)(pcm::SamplingRate::E rate, unsigned state, signed char *out, int num_samples);

unsigned generate_audio_tone(pcm::SamplingRate::E rate, unsigned state, signed char *out, int num_samples) {
  unsigned char const start_t = state;

  const unsigned long sampling_rate = pcm::to_hz(rate);
  const unsigned long freq1 = 523; // C
  const unsigned long freq2 = 660; // E
  const unsigned long freq3 = 783; // G
  const unsigned long freq4 = 988; // B
  for (int t = start_t; t < num_samples + start_t; t++) {
    int val = 0;
    val += maths::sin(t * (freq1 * 256) / sampling_rate);
    val += maths::sin(t * (freq2 * 256) / sampling_rate);
    val += maths::sin(t * (freq3 * 256) / sampling_rate);
    val += maths::sin(t * (freq4 * 256) / sampling_rate);
    val /= 4;
    *out++ = val;
  }

  return num_samples + start_t;
}

unsigned generate_audio_debug(pcm::SamplingRate::E rate, unsigned flip, signed char *out, int num_samples) {
  for (int t = 0; t < num_samples; t++) {
    *out++ = flip ? -t : t;
  }
  return ~flip;
}

unsigned generate_audio_bgm(pcm::SamplingRate::E rate, unsigned state, signed char *out, int num_samples) {
  unsigned bgm_idx = (state >> 24) & 0xFF;
  unsigned sample_count = state & 0x00FFFFFF;
  unsigned char const start_t = sample_count & 0xFF;

  const unsigned long bpm = 120;
  const unsigned long freq = bgm_data[bgm_idx * 2 + 0];
  const unsigned long next_note = bgm_data[bgm_idx * 2 + 1 + 2];

  const unsigned long sampling_rate = pcm::to_hz(rate);
  const unsigned long next_note_samples =
    sampling_rate // samples per second
    * 60 // seconds per minute
    / bpm // beats per minute
    / 2 // notes per beat
    * next_note // note
  ;

  // TODO: doesn't handle split notes
  for (int t = start_t; t < num_samples + start_t; t++) {
    *out++ = maths::sin(t * (freq * 256) / sampling_rate);
  }

  // Jump to next note.
  sample_count += num_samples;
  if (sample_count > next_note_samples) {
    bgm_idx++;
    if (bgm_idx >= static_cast<unsigned>(num_bgm_notes - 1)) {
      bgm_idx = 0;
      sample_count = 0;
    }
  }

  return (bgm_idx << 24) | sample_count;
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
  unsigned generator_state = 0;
  while (true) {
    iteration++;

    // Refill if needed.
    if (pcm::is_empty()) {
      refills++;

      // Running low, generate more audio.
      generator_state = generate_audio(pcm_rate, generator_state, buffer, buffer_size);
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
  } else if (argc != 3) {
    printf(
      "Usage:\n"
      "  %s <type> <size>\n"
      "<type> is \"tone\", \"bgm\", or \"debug\"\n"
      "<size> is buffer size and must be a power of 2\n"
      "       (512, 1024, 2048 are good choices)\n"
      , argv[0]
    );
    return EXIT_FAILURE;
  }

  char const *const type = argv[1];
  if (strcmp(type, "tone") == 0) {
    generate_audio = generate_audio_tone;
  } else if (strcmp(type, "bgm") == 0) {
    generate_audio = generate_audio_bgm;
  } else if (strcmp(type, "debug") == 0) {
    generate_audio = generate_audio_debug;
  } else {
    printf("Unknown type: %s\n", type);
    return EXIT_FAILURE;
  }

  int const buffer_size = atoi(argv[2]);
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
