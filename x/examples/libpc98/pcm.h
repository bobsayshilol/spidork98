#ifndef PCM_H
#define PCM_H

namespace pcm {

struct SamplingRate {
  // These values match the raw value you'd write to the port.
  enum E {
    kHz_44_1 = 0,
    kHz_33 = 1,
    kHz_22 = 2,
    kHz_16_5 = 3,
    kHz_11 = 4,
    kHz_8_3 = 5,
    kHz_5_5 = 6,
    kHz_4_1 = 7,
  };
};

struct SampleSize {
  // These values match the raw value you'd write to the port.
  enum E {
    bits_8 = 0 << 6,
    bits_16 = 1 << 6,
  };
};

struct Panning {
  // These values match the raw value you'd write to the port.
  enum E {
    pan_left = 1 << 5,
    pan_right = 1 << 4,
    pan_stereo = (1 << 5) | (1 << 4),
  };
};


// Init/shutdown the PCM system.
bool init();
void shutdown();

// Generate some testing data.
void generate_audio();

} // namespace pcm

#endif
