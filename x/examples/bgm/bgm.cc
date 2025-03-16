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

  Note::C4,       make_beat(4, 2, 0),
  Note::Cs4,      make_beat(4, 3, 0),
  Note::Ds4,      make_beat(5, 0, 0),
  Note::Silence,  make_beat(5, 0, 1),
  Note::Ds4,      make_beat(5, 1, 0),
  Note::Ds4,      make_beat(5, 1, 1),
  Note::Silence,  make_beat(5, 2, 0),
  Note::Ds4,      make_beat(5, 2, 1),
  Note::Silence,  make_beat(5, 3, 0),
  Note::E4,       make_beat(5, 3, 1),

  Note::Silence,  make_beat(6, 0, 0),
  Note::E4,       make_beat(6, 1, 0),
  Note::Silence,  make_beat(6, 1, 1),
  Note::F4,       make_beat(6, 2, 0),
  Note::Silence,  make_beat(6, 2, 1),
  Note::G4,       make_beat(6, 3, 0),
  Note::Silence,  make_beat(6, 3, 1),
  Note::G4,       make_beat(7, 0, 0),

  Note::Silence,  make_beat(7, 1, 0),

  Note::F4,       make_beat(7, 2, 1),
  Note::G4,       make_beat(7, 3, 0),
  Note::Gs4,      make_beat(7, 3, 1),

  Note::Silence,  make_beat(8, 1, 0),

  Note::Gs4,      make_beat(8, 3, 0),
  Note::As4,      make_beat(8, 3, 1),
  Note::C5,       make_beat(9, 0, 0),
  Note::Silence,  make_beat(9, 0, 1),
  Note::C5,       make_beat(9, 1, 0),
  Note::C5,       make_beat(9, 1, 1),
  Note::Silence,  make_beat(9, 2, 0),
  Note::Cs5,      make_beat(9, 2, 1),
  Note::Silence,  make_beat(9, 3, 0),
  Note::As4,      make_beat(9, 3, 1),
  Note::Silence,  make_beat(10, 0, 1),

  Note::G4,       make_beat(10, 2, 0),
  Note::Silence,  make_beat(10, 2, 1),
  Note::Gs4,      make_beat(10, 3, 0),
  Note::Silence,  make_beat(10, 3, 1),
  Note::As4,      make_beat(11, 0, 0),
  Note::Silence,  make_beat(11, 0, 1),
  Note::As4,      make_beat(11, 1, 0),
  Note::As4,      make_beat(11, 1, 1),
  Note::Silence,  make_beat(11, 2, 0),
  Note::C5,       make_beat(11, 2, 1),
  Note::Silence,  make_beat(11, 3, 0),
  Note::Gs4,      make_beat(11, 3, 1),
  Note::Silence,  make_beat(12, 0, 1),

  Note::F4,       make_beat(12, 2, 0),
  Note::Silence,  make_beat(12, 2, 1),
  Note::G4,       make_beat(12, 3, 0),
  Note::Silence,  make_beat(12, 3, 1),
  Note::Gs4,      make_beat(13, 0, 0),
  Note::Silence,  make_beat(13, 0, 1),
  Note::Gs4,      make_beat(13, 1, 0),
  Note::Gs4,      make_beat(13, 1, 1),
  Note::Silence,  make_beat(13, 2, 0),
  Note::C5,       make_beat(13, 2, 1),
  Note::Silence,  make_beat(13, 3, 0),
  Note::As4,      make_beat(13, 3, 1),
  Note::Silence,  make_beat(14, 0, 1),

  Note::As4,      make_beat(14, 1, 1),
  Note::As4,      make_beat(14, 2, 0),
  Note::Gs4,      make_beat(14, 2, 1),
  Note::Silence,  make_beat(14, 3, 0),
  Note::G4,       make_beat(14, 3, 1),
  Note::Gs4,      make_beat(15, 0, 0),
  Note::Silence,  make_beat(15, 1, 0),
};
const int num_bgm_notes = sizeof(bgm_data) / sizeof(bgm_data[0]) / 2;
