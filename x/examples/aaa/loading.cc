#include "loading.h"

#include "funcs.h"
#include "utils.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>

namespace game {

void loading_screen(void (*tick_progress)(LoadingProgress::E progress)) {
  // Remove any text.
  Funcs98::clear_screen();

  // Show something to confirm that it's actually running.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24

  // Reset attributes on function leave.
  //const u8 old_attr = ScreenAttrib;
  //DEFER(u8, o, old_attr, (ScreenAttrib = o));

  // Can't do much with attributes.
  //ScreenAttrib = GREEN;

  {
    const char first_line[] = "S.P.I.D.O.R.K. 0.9.8 INITIALISING";
    const int spacing = (num_cols - sizeof(first_line)) / 2;
    ScreenPutString_98(first_line, GREEN, spacing, num_rows / 3);
  }

  const u8 pb_x = num_cols / 4;
  const u8 pb_y = num_rows / 2;
  const u8 pb_w = num_cols / 2;

  // Progress bar.
  tick_progress(LoadingProgress::BarStart);
  {
    ScreenPutChar_98('[', GREEN, pb_x - 1, pb_y);
    ScreenPutChar_98(']', GREEN, pb_x + pb_w, pb_y);
    for (u8 i = 0; i < pb_w; i++) {
      tick_progress(LoadingProgress::BarTick);
      Funcs98::delay_ms(50);
      tick_progress(LoadingProgress::BarTick);
      Funcs98::delay_ms(50);
      ScreenPutChar_98('#', GREEN, pb_x + i, pb_y);
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
      memmove(heights + idx, heights + idx + 1, (num_hs - idx) * sizeof(ChPos));
    } else {
      ScreenPutChar_98('#', GREEN, pos.x, pb_y + pos.dy);
      ScreenPutChar_98('#', GREEN, pos.x, pb_y - pos.dy);
    }

    // Spread them out.
    do_next++;
    if (do_next > 40) { // ~80ms
      do_next = 0;
      tick_progress(LoadingProgress::FadeOutTick);

      if (next_dx < pb_x) {
        ChPos & chl = heights[num_hs];
        chl.x = pb_x - 1 - next_dx;
        chl.dy = 0;
        ScreenPutChar_98('#', GREEN, chl.x, pb_y);
        num_hs++;

        ChPos & chr = heights[num_hs];
        chr.x = pb_x + pb_w + next_dx;
        chr.dy = 0;
        ScreenPutChar_98('#', GREEN, chr.x, pb_y);
        num_hs++;

        next_dx++;
      }
    }

    Funcs98::delay_ms(2);
  }


  // Remove them.
  num_hs = pb_w;
  for (int k = 0; k < num_hs; k++) {
    heights[k].x = pb_x + k;
    heights[k].dy = 0;
    ScreenPutChar_98(' ', 0, heights[k].x, pb_y);
  }

  next_dx = 0;
  do_next = 0;

  tick_progress(LoadingProgress::FadeInStart);
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
    if (do_next > 20) { // ~40ms
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
