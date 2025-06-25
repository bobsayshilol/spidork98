//
// Flame effect taken straight from every demo ever
//

#include "funcs.h"
#include "gpuscrn.h"
#include "profile.h"

#include <cstdio>
#include <cstdlib>

#include <conio.h>

// Add gaps between pixels for performance (and it looks nicer).
#define PIXEL_SPACING_X 2
#define PIXEL_SPACING_Y 2

namespace {

// Profiling sections.
PROFILE_DECLARE_SECTION(draw_hotspots);
PROFILE_DECLARE_SECTION(move_hotspots);
PROFILE_DECLARE_SECTION(draw_screen);

// The scanline we'll update to give the fire effect.
ALIGNAS(4) u8 scanline_data[GPU_WIDTH];

// Hotspot locations.
const int num_hotspots = 64;
int hotspot_locations[num_hotspots];

// Lookup table for the next iteration step of the main loop.
u8 decrement_if_positive_lookup[256];

// Inclusive range.
int rand_between(int a, int b) {
  return a + (rand() % (b - a + 1));
}

// We only need different shades of red... no, purple!
// Taken from https://icolorpalette.com/color/a020f0#LightDarkshades
const u32 purple_palette[64] = {
  0xf7ecfe,0xf4e4fd,0xf0ddfd,0xedd5fc,0xeacdfc,0xe7c6fb,0xe3befb,0xe0b6fa,
  0xddaffa,0xdaa7f9,0xd69ff9,0xd398f8,0xd090f8,0xcd88f7,0xc981f7,0xc679f6,
  0xc372f5,0xbf6af5,0xbc62f4,0xb95bf4,0xb653f3,0xb24bf3,0xaf44f2,0xac3cf2,
  0xa934f1,0xa52df1,0xa225f0,0x9f1df0,0x9c16ef,0x9810ed,0x930fe5,0x8e0fde,
  0x890ed6,0x840ece,0x7f0dc7,0x7b0dbf,0x760cb7,0x710cb0,0x6c0ba8,0x670ba1,
  0x620a99,0x5d0a91,0x58098a,0x530982,0x4e087a,0x4a0873,0x45076b,0x400763,
  0x3b065c,0x360654,0x31054c,0x2c0545,0x27043d,0x220436,0x1d032e,0x190326,
  0x14021f,0x0f0217,0x0a010f,0x050108,
};
const u32 red_palette[64] = {
  0xffebeb,0xffe2e2,0xffdada,0xffd2d2,0xffcaca,0xffc2c2,0xffbaba,0xffb1b1,
  0xffa9a9,0xffa1a1,0xff9999,0xff9191,0xff8989,0xff8181,0xff7878,0xff7070,
  0xff6868,0xff6060,0xff5858,0xff5050,0xff4747,0xff3f3f,0xff3737,0xff2f2f,
  0xff2727,0xff1f1f,0xff1616,0xff0e0e,0xff0606,0xfd0000,0xf50000,0xed0000,
  0xe40000,0xdc0000,0xd40000,0xcc0000,0xc40000,0xbc0000,0xb40000,0xab0000,
  0xa30000,0x9b0000,0x930000,0x8b0000,0x830000,0x7a0000,0x720000,0x6a0000,
  0x620000,0x5a0000,0x520000,0x490000,0x410000,0x390000,0x310000,0x290000,
  0x210000,0x180000,0x100000,0x080000,
};
const u32 blue_palette[64] = {
  0xebebff,0xe2e2ff,0xdadaff,0xd2d2ff,0xcacaff,0xc2c2ff,0xbabaff,0xb1b1ff,
  0xa9a9ff,0xa1a1ff,0x9999ff,0x9191ff,0x8989ff,0x8181ff,0x7878ff,0x7070ff,
  0x6868ff,0x6060ff,0x5858ff,0x5050ff,0x4747ff,0x3f3fff,0x3737ff,0x2f2fff,
  0x2727ff,0x1f1fff,0x1616ff,0x0e0eff,0x0606ff,0x0000fd,0x0000f5,0x0000ed,
  0x0000e4,0x0000dc,0x0000d4,0x0000cc,0x0000c4,0x0000bc,0x0000b4,0x0000ab,
  0x0000a3,0x00009b,0x000093,0x00008b,0x000083,0x00007a,0x000072,0x00006a,
  0x000062,0x00005a,0x000052,0x000049,0x000041,0x000039,0x000031,0x000029,
  0x000021,0x000018,0x000010,0x000008,
};
const u32 green_palette[64] = {
  0xebffeb,0xe2ffe2,0xdaffda,0xd2ffd2,0xcaffca,0xc2ffc2,0xbaffba,0xb1ffb1,
  0xa9ffa9,0xa1ffa1,0x99ff99,0x91ff91,0x89ff89,0x81ff81,0x78ff78,0x70ff70,
  0x68ff68,0x60ff60,0x58ff58,0x50ff50,0x47ff47,0x3fff3f,0x37ff37,0x2fff2f,
  0x27ff27,0x1fff1f,0x16ff16,0x0eff0e,0x06ff06,0x00fd00,0x00f500,0x00ed00,
  0x00e400,0x00dc00,0x00d400,0x00cc00,0x00c400,0x00bc00,0x00b400,0x00ab00,
  0x00a300,0x009b00,0x009300,0x008b00,0x008300,0x007a00,0x007200,0x006a00,
  0x006200,0x005a00,0x005200,0x004900,0x004100,0x003900,0x003100,0x002900,
  0x002100,0x001800,0x001000,0x000800,
};
const u32 grey_palette[64] = {
  0xf5f5f5,0xf1f1f1,0xededed,0xe9e9e9,0xe4e4e4,0xe0e0e0,0xdcdcdc,0xd8d8d8,
  0xd4d4d4,0xd0d0d0,0xcccccc,0xc8c8c8,0xc4c4c4,0xc0c0c0,0xbcbcbc,0xb8b8b8,
  0xb4b4b4,0xafafaf,0xababab,0xa7a7a7,0xa3a3a3,0x9f9f9f,0x9b9b9b,0x979797,
  0x939393,0x8f8f8f,0x8b8b8b,0x878787,0x838383,0x7e7e7e,0x7a7a7a,0x767676,
  0x727272,0x6e6e6e,0x6a6a6a,0x666666,0x626262,0x5e5e5e,0x5a5a5a,0x565656,
  0x525252,0x4e4e4e,0x494949,0x454545,0x414141,0x3d3d3d,0x393939,0x353535,
  0x313131,0x2d2d2d,0x292929,0x252525,0x212121,0x1d1d1d,0x181818,0x141414,
  0x101010,0x0c0c0c,0x080808,0x040404,
};

const u32 *const palettes[] = {
  purple_palette, red_palette, blue_palette, green_palette, grey_palette,
};

void change_palette() {
  static int idx = 0;
  const u32 *const palette = palettes[idx];
  idx = (idx + 1) % COUNT_OF(palettes);

  // 64 entries (in reverse) but we'll just duplicate them.
  for (int i = 0; i < 64; i++) {
    const u32 rgb = palette[63 - i];
    // Extract colours.
    const u8 r = (rgb >> 16) & 0xff;
    const u8 g = (rgb >>  8) & 0xff;
    const u8 b = (rgb >>  0) & 0xff;
    // Set the palette.
    gpu::set_palette_colour(4 * i + 0, r, g, b);
    gpu::set_palette_colour(4 * i + 1, r, g, b);
    gpu::set_palette_colour(4 * i + 2, r, g, b);
    gpu::set_palette_colour(4 * i + 3, r, g, b);
  }
}

void build_lookup_table() {
  // When we space out pixels in Y we need to jump further back.
  for (int i = 0; i < PIXEL_SPACING_Y; i++) {
    decrement_if_positive_lookup[i] = 0;
  }
  for (int j = PIXEL_SPACING_Y; j < 256; j++) {
    decrement_if_positive_lookup[j] = j - PIXEL_SPACING_Y;
  }
}

void setup_hotspots() {
  // Spread out the initial hotspots.
  for (int i = 0; i < num_hotspots; i++) {
    hotspot_locations[i] = i * GPU_WIDTH / num_hotspots;
  }
}

void draw_hotspots() {
  PROFILE_TIME_SECTION(draw_hotspots);

  // Blank out the bottom line.
  for (int x = 0; x < GPU_WIDTH; x++) {
      scanline_data[x] = 0;
  }

  // Fill in the current hotspots and their neighbours.
#define BLUR_WIDTH 20
#define FIRE_POWER 4
  for (int i = 0; i < num_hotspots; i++) {
    const int h = hotspot_locations[i];
    STATIC_ASSERT((BLUR_WIDTH % PIXEL_SPACING_X) == 0);
    for (int dx = -BLUR_WIDTH; dx <= BLUR_WIDTH; dx += PIXEL_SPACING_X) {
      const int adx = dx < 0 ? -dx : dx;
      int p = h + dx;
      // Handle wrap around.
      if (p < 0) p += GPU_WIDTH;
      if (p >= GPU_WIDTH) p -= GPU_WIDTH;
      // Add the contribution from this hotspot.
      STATIC_ASSERT(FIRE_POWER * BLUR_WIDTH < 255);
      const unsigned delta = FIRE_POWER * (BLUR_WIDTH - adx);
      unsigned pixel = scanline_data[p];
      pixel += delta;
      scanline_data[p] = pixel > 255 ? 255 : pixel;
    }
  }

  // Write it to the bottom line.
  gpu::write_scanline(GPU_HEIGHT - 1, scanline_data);
}

void move_hotspots() {
  PROFILE_TIME_SECTION(move_hotspots);

  const int move_speed = 2;
  for (int i = 0; i < num_hotspots; i++) {
    int & x = hotspot_locations[i];
    // Add a bias so they don't all move in the same direction.
    const int bias = i & 1;
    x += rand_between(-move_speed + bias - 1, move_speed + bias) * PIXEL_SPACING_X;
    STATIC_ASSERT((GPU_WIDTH % PIXEL_SPACING_X) == 0);
    if (x < 0) x += GPU_WIDTH;
    if (x >= GPU_WIDTH) x -= GPU_WIDTH;
  }
}

void draw_screen() {
  PROFILE_TIME_SECTION(draw_screen);

  // Draw from the top down, excluding the bottom line.
  // The maximum value is 255 so we only need to process that many lines.
  for (int ys = PIXEL_SPACING_Y; ys < 256; ys += PIXEL_SPACING_Y) {
    const int y = GPU_HEIGHT - 1 - ys;
    // Read the scanline data for the line below us.
    gpu::read_scanline(y + PIXEL_SPACING_Y, scanline_data);

    // Decrement the intensity of each pixel.
#if 0 // Basic implementation.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING_X) {
      if (scanline_data[x] >= PIXEL_SPACING_Y) {
        scanline_data[x] -= PIXEL_SPACING_Y;
      }
    }
#else // Unroll it 16 times for much better performance.
    for (int x = 0; x < GPU_WIDTH; x += PIXEL_SPACING_X * 16) {
#define DECREMENT_IF_POSITIVE(o) \
  { \
    u8 &pixel = scanline_data[(x) + PIXEL_SPACING_X * (o)]; \
    pixel = decrement_if_positive_lookup[pixel]; \
  }
      DECREMENT_IF_POSITIVE(0)  DECREMENT_IF_POSITIVE(1)  DECREMENT_IF_POSITIVE(2)  DECREMENT_IF_POSITIVE(3)
      DECREMENT_IF_POSITIVE(4)  DECREMENT_IF_POSITIVE(5)  DECREMENT_IF_POSITIVE(6)  DECREMENT_IF_POSITIVE(7)
      DECREMENT_IF_POSITIVE(8)  DECREMENT_IF_POSITIVE(9)  DECREMENT_IF_POSITIVE(10) DECREMENT_IF_POSITIVE(11)
      DECREMENT_IF_POSITIVE(12) DECREMENT_IF_POSITIVE(13) DECREMENT_IF_POSITIVE(14) DECREMENT_IF_POSITIVE(15)
    }
#endif

    // Write it back to the current line.
    gpu::write_scanline(y, scanline_data);
  }
}

template <typename Funcs>
void play() {
  if (!gpu::setup()) {
    printf("Failed to setup GPU\n");
    return;
  }
  Funcs::clear_screen();
  gpu::enable_text_layer(true);

  // Seed the RNG.
  srand(time(0));

  change_palette();
  build_lookup_table();
  setup_hotspots();

  // Timings to calculate FPS.
  uclock_t last_time = Funcs::ticks();
  int num_frames = 0;

  bool vsync = false;
  bool paused = false;
  bool show_help = true;
  while (true) {
    // Keep going until the user hits a button.
    if (Funcs::kb_hit()) {
      const char c = getch();
      if (c == 'v' || c == 'V') {
        vsync = !vsync;
      } else if (c == 'c' || c == 'C') {
        change_palette();
      } else if (c == 'p' || c == 'P' || c == ' ') {
        paused = !paused;
      } else if (c == 'h' || c == 'H') {
        show_help = !show_help;
      } else if (c == 'r' || c == 'R') {
        setup_hotspots();
      } else if (c == 'q' || c == 'Q' || c == 27) {
        break;
      }
    }

    if (!paused) {
      draw_hotspots();
      move_hotspots();
      draw_screen();
    }

    // Report the FPS once a second.
    const uclock_t now = Funcs::ticks();
    num_frames++;
    if (now - last_time > Funcs::ticks_per_sec()) {
      Funcs::clear_screen();
      const unsigned fpks = (num_frames * Funcs::ticks_per_sec() * 1000) / (now - last_time);
      printf("%u.%uFPS\n", fpks / 1000, (fpks / 100) % 10);
      num_frames = 0;
      last_time = now;

      // Add some help text.
      if (show_help) {
        printf("Toggle vsync: V\n");
        printf("Cycle colours: C\n");
        printf("Pause: P or Space\n");
        printf("Toggle help: H\n");
        printf("Reset fires: R\n");
        printf("Quit: Q or Escape\n");
      }

      // Log the timings too.
      PROFILE_PRINT_TIMINGS();
    }

    // Wait for the vsync.
    if (vsync) {
      gpu::wait_for_vsync();
    }
  }

  // Cleanup.
  gpu::shutdown();
  Funcs::clear_screen();
}

} // namespace

int main() {
  int m = __crt0_mtype;
  if (ISPCAT(m)) {
    printf("Can't run on PC-AT\n");
    return EXIT_FAILURE;
  } else if (ISPC98(m)) {
    play<Funcs98>();
  } else {
    printf("Unknown CPU type: %d\n", m);
    return EXIT_FAILURE;
  }

  printf("Finished\n");
  return EXIT_SUCCESS;
}
