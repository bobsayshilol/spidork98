#include "pcm.h"

#include "web_common.h"

namespace pcm {

FASTCALL bool init(SamplingRate::E rate, Format::E format, const i8 *buffer) {
  // TODO
  (void)rate;
  (void)format;
  (void)buffer;
  return false;
}

FASTCALL void shutdown() {
  // TODO
}

FASTCALL int to_hz(SamplingRate::E rate) {
  switch (rate) {
    case SamplingRate::kHz_44_1: return 44100;
    case SamplingRate::kHz_33:   return 33080;
    case SamplingRate::kHz_22:   return 22050;
    case SamplingRate::kHz_16_5: return 16540;
    case SamplingRate::kHz_11:   return 11030;
    case SamplingRate::kHz_8_3:  return 8270;
    case SamplingRate::kHz_5_5:  return 5520;
    case SamplingRate::kHz_4_1:  return 4130;
  }
  return -1;
}

FASTCALL void set_volume(Volume::E volume) {
  // TODO
  (void)volume;
}

FASTCALL bool is_empty() {
  // TODO
  return false;
}

FASTCALL void filled(u16 buffer_elems) {
  // TODO
  (void)buffer_elems;
}

} // namespace pcm
