#ifndef PCM_H
#define PCM_H

#include "macros.h"
#include "types.h"

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
    bits_8 = 1 << 6,
    bits_16 = 0 << 6,
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

struct Format {
  enum E {
    fmt_mono = 1,
    fmt_stereo = 0,
  };
};

struct Volume {
  // These values match the raw value you'd write to the port.
  enum E {
    vol_max = 0x0,
    vol_3_quater = 0x4,
    vol_half = 0x8,
    vol_1_quater = 0xB,
    vol_min = 0xF,
  };
};

namespace detail {
extern "C" bool g_pcm_buffer_empty;
} // detail

// Init the PCM system.
// Data will be read from the provided buffer, which should be filled() when it's empty.
// |buffer| must be aligned to 4 bytes minimum.
FASTCALL bool init(SamplingRate::E rate, Format::E format, const i8 *buffer);
FASTCALL bool init(SamplingRate::E rate, Format::E format, const i16 *buffer); // not implemented

// Stop any playback and shutdown the PCM system.
FASTCALL void shutdown();

// Convert an enum to its Hz.
FASTCALL int to_hz(SamplingRate::E rate);

// Change the volume.
FASTCALL void set_volume(Volume::E volume);

// Needs checking often to see if we need more data filling the read buffer.
FASTCALL bool is_empty(); // { return detail::g_pcm_buffer_empty; }

// Report to the system that the buffer has been filled with this many elements.
// Should only be called when is_empty(), with a max size of 32KB.
FASTCALL void filled(u16 buffer_elems);

} // namespace pcm

#endif
