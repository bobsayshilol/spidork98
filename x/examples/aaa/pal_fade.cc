#include "pal_fade.h"

#include <cstring>

namespace game {

PaletteFader g_palette_fader;

PaletteFader::PaletteFader() { m_done = true; }

void PaletteFader::start_fade_in(u32 fade_time) {
  m_lerped_palette.num_colours = m_target_palette.num_colours;
  memset(m_lerped_palette.rgb, 0, m_lerped_palette.num_colours * 3);
  images::set_palette(m_lerped_palette);

  m_done = false;
  m_fade_in = true;
  m_timer = 0;
  m_fade_time = fade_time;
}

void PaletteFader::start_fade_out(u32 fade_time) {
  m_lerped_palette.num_colours = m_target_palette.num_colours;
  memcpy(m_lerped_palette.rgb, m_target_palette.rgb, m_lerped_palette.num_colours * 3);
  images::set_palette(m_lerped_palette);

  m_done = false;
  m_fade_in = false;
  m_timer = 0;
  m_fade_time = fade_time;
}

bool PaletteFader::tick(u32 dt) {
  if (m_done) return true;

  const u8 *bg_rgb = m_target_palette.rgb;
  u8 *lerp_rgb = m_lerped_palette.rgb;

  m_timer += dt;
  if (m_timer > m_fade_time) {
    if (m_fade_in) {
      images::set_palette(m_target_palette);
    } else {
      memset(m_lerped_palette.rgb, 0, m_lerped_palette.num_colours * 3);
      images::set_palette(m_lerped_palette);
    }
    m_done = true;
    return m_done;
  }

  // Divide once rather than for every palette entry.
  const u32 elapsed = m_fade_in ? m_timer : (m_fade_time - m_timer);
  const u32 scale = (static_cast<u64>(elapsed) << 10) / m_fade_time;

  const u8 num_colours = m_lerped_palette.num_colours;
  for (u8 i = 0; i < num_colours; i++) {
    lerp_rgb[0] = (bg_rgb[0] * scale) >> 10;
    lerp_rgb[1] = (bg_rgb[1] * scale) >> 10;
    lerp_rgb[2] = (bg_rgb[2] * scale) >> 10;
    bg_rgb += 3;
    lerp_rgb += 3;
  }

  images::set_palette(m_lerped_palette);
  return m_done;
}

} // namespace game
