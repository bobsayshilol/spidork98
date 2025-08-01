#include "loading.h"

#include "funcs.h"
#include "game.h"
#include "gpuscrn.h"
#include "utils.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace game {

#if 1
#define LEFT_2CHAR " ["
#define RIGHT_2CHAR "] "
#else
#define LEFT_2CHAR "\x81\x73" // <<
#define RIGHT_2CHAR "\x81\x74" // >>
#endif

void loading_screen(void (*tick_progress)(LoadingProgress::E progress)) {
  // Remove any text.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();

  // Show something to confirm that it's actually running.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24

  // Reset attributes on function leave.
  //const u8 old_attr = ScreenAttrib;
  //DEFER(u8, o, old_attr, (ScreenAttrib = o));

  // Can't do much with attributes.
  //ScreenAttrib = COLOUR_GREEN;

  // Info line bits.
  const char info_line[] = "\xA2 S.P.I.D.O.R.K. 0.9.8 INITIALISING \xA3";
  const int info_line_x = (num_cols - sizeof(info_line)) / 2;
  const int info_line_y = num_rows / 3;
  ScreenPutString_98(info_line, COLOUR_GREEN, info_line_x, info_line_y);
  //const char loading_chars_l[] = "|/-`";
  //const char loading_chars_r[] = "|`-/";

  // Progress bar stuff.
  const u8 pb_x = num_cols / 4;
  const u8 pb_y = num_rows / 2;
  const u8 pb_w = num_cols / 2;

  // yen symbol
  const char fill_char = 0x5C; //(rand() & 0x10) ? '#' : 0x5C;

  // Progress bar.
  tick_progress(LoadingProgress::BarStart);
  {
    //u8 loading_char_idx = 0;

    ScreenPutString_98(LEFT_2CHAR, COLOUR_GREEN, pb_x - 2, pb_y);
    ScreenPutString_98(RIGHT_2CHAR, COLOUR_GREEN, pb_x + pb_w, pb_y);
    for (u8 i = 0; i < pb_w; i++) {
      for (int j = 0; j < 2; j++) {
        //ScreenPutChar_98(loading_chars_l[loading_char_idx], COLOUR_GREEN, info_line_x - 2, info_line_y);
        //ScreenPutChar_98(loading_chars_r[loading_char_idx], COLOUR_GREEN, info_line_x + sizeof(info_line) + 1, info_line_y);
        //loading_char_idx = (loading_char_idx + 1) & 3;
        Funcs98::delay_ms(10);
        tick_progress(LoadingProgress::BarTick);
      }

      ScreenPutChar_98(fill_char, COLOUR_GREEN, pb_x + i, pb_y);
    }
  }

  struct ChPos { u8 x, dy; };
  int num_hs = pb_w;
  ChPos heights[num_cols];
  for (int j = 0; j < num_hs; j++) {
    heights[j].x = pb_x + j;
    heights[j].dy = 0;
  }

  u8 next_dx = 0;
  u8 do_next = 0;


  // Make them spread out.
  tick_progress(LoadingProgress::FadeOutStart);
  while (num_hs > 0) {
    const int idx = rand() % num_hs;
    ChPos & pos = heights[idx];

    // Spread them up.
    pos.dy++;
    if (pos.dy > pb_y) {
      num_hs--;
      //memmove(heights + idx, heights + idx + 1, (num_hs - idx) * sizeof(ChPos));
      heights[idx] = heights[num_hs];
    } else {
      ScreenPutChar_98(fill_char, COLOUR_GREEN, pos.x, pb_y + pos.dy);
      ScreenPutChar_98(fill_char, COLOUR_GREEN, pos.x, pb_y - pos.dy);
    }

    // Spread them out.
    do_next++;
    if (do_next == 20) tick_progress(LoadingProgress::FadeOutTick);
    if (do_next > 40) { // ~80ms
      do_next = 0;
      tick_progress(LoadingProgress::FadeOutTick);

      if (next_dx < pb_x) {
        ChPos & chl = heights[num_hs];
        chl.x = pb_x - 1 - next_dx;
        chl.dy = 0;
        ScreenPutChar_98(fill_char, COLOUR_GREEN, chl.x, pb_y);
        num_hs++;

        ChPos & chr = heights[num_hs];
        chr.x = pb_x + pb_w + next_dx;
        chr.dy = 0;
        ScreenPutChar_98(fill_char, COLOUR_GREEN, chr.x, pb_y);
        num_hs++;

        next_dx++;
      }
    }

    Funcs98::delay_ms(2);
  }


  // Remove them.
  tick_progress(LoadingProgress::FadeInStart);
  num_hs = pb_w;
  for (int k = 0; k < num_hs; k++) {
    heights[k].x = pb_x + k;
    heights[k].dy = 0;
    ScreenPutChar_98(' ', 0, heights[k].x, pb_y);
  }

  next_dx = 0;
  do_next = 0;

  while (num_hs > 0) {
    const int idx = rand() % num_hs;
    ChPos & pos = heights[idx];

    // Spread them up.
    pos.dy++;
    if (pos.dy > pb_y) {
      num_hs--;
      memmove(heights + idx, heights + idx + 1, (num_hs - idx) * sizeof(ChPos));
    } else {
      ScreenPutChar_98(' ', 0, pos.x, pb_y + pos.dy);
      ScreenPutChar_98(' ', 0, pos.x, pb_y - pos.dy);
    }

    // Spread them out.
    do_next++;
    if (do_next > 15) { // ~30ms
      do_next = 0;
      tick_progress(LoadingProgress::FadeInTick);

      if (next_dx < pb_x) {
        ChPos & chl = heights[num_hs];
        chl.x = pb_x - 1 - next_dx;
        chl.dy = 0;
        ScreenPutChar_98(' ', 0, chl.x, pb_y);
        num_hs++;

        ChPos & chr = heights[num_hs];
        chr.x = pb_x + pb_w + next_dx;
        chr.dy = 0;
        ScreenPutChar_98(' ', 0, chr.x, pb_y);
        num_hs++;

        next_dx++;
      }
    }

    Funcs98::delay_ms(2);
  }

  tick_progress(LoadingProgress::Done);
}

} // namespace game
