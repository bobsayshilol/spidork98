#ifndef GAME_PALETTE_FADER_H
#define GAME_PALETTE_FADER_H

#include "images.h"

namespace game {

struct PaletteFader {
  PaletteFader();

  images::Palette &target_palette() { return m_target_palette; }

  void start_fade_in(u32 fade_time);
  void start_fade_out(u32 fade_time);

  // Returns whether or not we're at the target.
  bool tick(u32 dt);

private:
  bool m_done;
  bool m_fade_in;
  u32 m_timer;
  u32 m_fade_time;
  images::Palette m_target_palette;
  images::Palette m_lerped_palette;
};

extern PaletteFader g_palette_fader;

} // namespace game

#endif
