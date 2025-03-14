#pragma once

namespace {

struct Note {
  static const short Silence = 0;
  static const short C4 = 262;
  static const short Cs4 = 277;
  static const short Ds4 = 311;
  static const short E4 = 330;
  static const short F4 = 349;
  static const short G4 = 392;
  static const short Gs4 = 415;
};

static inline short make_beat(short bar, short note, short half) {
  return (bar * 4 + note) * 2 + half;
}

} // namespace
