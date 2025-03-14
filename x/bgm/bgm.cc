#include "bgm.h"
#include "notes.h"

// https://playxylo.com/?scale=chromatic
// pairs of [freq, beat]
const short bgm_data[] = {
  Note::C4,       make_beat(0, 3, 0),
  Note::Cs4,      make_beat(0, 3, 1),
  Note::Ds4,      make_beat(1, 0, 0),

  Note::Silence,  make_beat(1, 2, 0),

  Note::Gs4,      make_beat(2, 1, 0),
  Note::G4,       make_beat(2, 2, 0),
  Note::Ds4,      make_beat(2, 3, 0),
  Note::Ds4,      make_beat(3, 0, 0),

  Note::Silence,  make_beat(3, 2, 0),

  Note::Cs4,      make_beat(3, 2, 1),
  Note::C4,       make_beat(3, 3, 0),
  Note::Cs4,      make_beat(3, 3, 1),

  Note::Silence,  make_beat(4, 1, 0),

// C
// D#
// D#
// D#
// E
// F
// G
// G#
};
const int num_bgm_notes = sizeof(bgm_data) / sizeof(bgm_data[0]) / 2;
