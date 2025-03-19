#include "gpuscrn.h"

#include <go32.h>
#include <pc.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>

// Logical to physical address.
#define SEG2REAL(a, b) (a * 0x10 + b)

// Flip-flop2 control port + commands.
#define PORT_FLIPFLOP2_CTRL 0x6A
#define FLIPFLOP2_LOCK 0x06
#define FLIPFLOP2_UNLOCK 0x07
#define FLIPFLOP2_STANDARD_MODE 0x20
#define FLIPFLOP2_EXTENDED_MODE 0x21 // 256 colours
#define FLIPFLOP2_DUAL_SCREEN 0x68
#define FLIPFLOP2_SINGLE_SCREEN 0x69

// Another flip-flop2 port?
#define PORT_SCANLINE_COUNT 0x09A8
#define SCANLINE_COUNT_400 0x00
#define SCANLINE_COUNT_480 0x01

// Palette ports.
#define PORT_PALETTE_NUM 0xA8
#define PORT_PALETTE_GREEN 0xAA
#define PORT_PALETTE_RED 0xAC
#define PORT_PALETTE_BLUE 0xAE

// GDC ports + commands.
// Note that these are dual purpose:
// parameter read is status, command read is data.
#define PORT_GDC_TEXT_PARAMETER 0x60
#define PORT_GDC_GPU_PARAMETER 0xA0
#define PORT_GDC_TEXT_COMMAND 0x62
#define PORT_GDC_GPU_COMMAND 0xA2
#define GDC_COMMAND_STOP 0x0C
#define GDC_COMMAND_START 0x0D

// Memory map options.
#define MEMORY_MAP_WINDOW0_BANK_ADDR SEG2REAL(0xE000, 0x0004)
#define MEMORY_MAP_WINDOW1_BANK_ADDR SEG2REAL(0xE000, 0x0006)
#define MEMORY_MAP_MODE_ADDR SEG2REAL(0xE000, 0x0100)
#define MEMORY_MAP_PACKED 0x00
#define MEMORY_MAP_PLANES 0x01

// Windows bank mappings (where banks are mapped to)
#define WINDOW0_DATA_ADDR SEG2REAL(0xA800, 0x0000)
#define WINDOW1_DATA_ADDR SEG2REAL(0xB000, 0x0000)
#define WINDOW_BANK_SIZE (32 * 1024)


// TODO: libstdc++ is very broken due to 8.3 filenames
//#include <utility>
template <typename T>
static __inline__ void SWAP(T&l,T&r) {
  T t = l;
  l = r;
  r = t;
}


// Based on https://www.delorie.com/djgpp/doc/brennan/brennan_access_vga.html
// And https://web.archive.org/web/20041225164712/http://www2.muroran-it.ac.jp/circle/mpc/program/pc98dos/index.html

namespace gpu {

namespace {

// C++11-ish enum.
struct Window { enum E { Window0, Window1 }; };

// Currently mapped banks.
u8 *g_windows[2];

// Map in a new bank to one of the windows.
void __inline__ set_bank(Window::E window, short bank) {
  _farpokew(_dos_ds, window == Window::Window1 ? MEMORY_MAP_WINDOW1_BANK_ADDR : MEMORY_MAP_WINDOW0_BANK_ADDR, bank);
}

// Convert a pixel location to an address.
#define pixel_to_addr(x, y) (static_cast<unsigned>(y) * GPU_WIDTH + static_cast<unsigned>(x))

// Convert a pixel location to a bank and offset into that bank.
struct BankInfo { int bank, offset; };
BankInfo __inline__ pixel_to_bank(int x, int y) {
  BankInfo info;
  const unsigned addr = pixel_to_addr(x, y);
  info.bank = addr / WINDOW_BANK_SIZE;
  info.offset = addr % WINDOW_BANK_SIZE;
  return info;
}

} // namespace raw

bool setup() {
  // Need near pointers to actually access memory.
  if (__djgpp_nearptr_enable() == 0) {
    return false;
  }
  g_windows[0] = (u8*)(__djgpp_conventional_base + WINDOW0_DATA_ADDR);
  g_windows[1] = (u8*)(__djgpp_conventional_base + WINDOW1_DATA_ADDR);

  // Change to 256 colours, dual screen.
  outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_UNLOCK);
  outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_EXTENDED_MODE);
  outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_DUAL_SCREEN);
  outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_LOCK);
  
  // 400 lines (24kHz).
  outportb(PORT_SCANLINE_COUNT, SCANLINE_COUNT_400);

  // Packed pixel format.
  _farpokeb(_dos_ds, MEMORY_MAP_MODE_ADDR, MEMORY_MAP_PACKED);

  // Clear the palette and the screen.
  for (int i = 0; i < 256; i++) {
    set_palette_colour(i, 0, 0, 0);
  }
  clear(0);

  // Disable text mode.
  enable_text_layer(false);

  // Setup initial mappings.
  set_bank(Window::Window0, 0);
  set_bank(Window::Window1, 1);

  return true;
}

void shutdown() {
  // If we haven't setup a mapping then it's a no-op.
  if (!g_windows[0]) {
    return;
  }

  // Clear the screen.
  set_palette_colour(0, 0, 0, 0);
  clear(0);

  // Change back to standard mode.
	outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_UNLOCK);
	outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_STANDARD_MODE);
	outportb(PORT_FLIPFLOP2_CTRL, FLIPFLOP2_LOCK);

  // Bring back text mode.
  enable_text_layer(true);
  
  // Undo mappings.
  g_windows[0] = 0;
  g_windows[1] = 0;
  __djgpp_nearptr_disable();
}

void swap() {
  // TODO: double buffering with dual screen
}

void enable_text_layer(bool show) {
  outportb(PORT_GDC_TEXT_COMMAND, show ? GDC_COMMAND_START : GDC_COMMAND_STOP);
}

void set_palette_colour(u8 pal_col, u8 r, u8 g, u8 b) {
	outportb(PORT_PALETTE_NUM, pal_col);
	outportb(PORT_PALETTE_GREEN, r);
	outportb(PORT_PALETTE_RED, g);
	outportb(PORT_PALETTE_BLUE, b);
}

void clear(u8 pal_col) {
  u8 volatile *const window0 = g_windows[0];
  u8 volatile *const window1 = g_windows[1];

  for (int bank = 0; bank < 4; bank++) {
    set_bank(Window::Window0, 2 * bank + 0);
    set_bank(Window::Window1, 2 * bank + 1);
    for (int i = 0; i < WINDOW_BANK_SIZE; i++) {
      window0[i] = pal_col;
      window1[i] = pal_col;
    }
  }
}

#if 0
void draw_line(int x0, int y0, int x1, int y1, u8 pal_col) {
  // TODO: implement
}
#endif

void draw_quad(int x0, int y0, int x1, int y1, u8 pal_col) {
  // TODO: use both banks
  u8 volatile *const window0 = g_windows[0];

  // Render from top to bottom.
  if (x0 > x1) SWAP(x0, x1);
  if (y0 > y1) SWAP(y0, y1);

  // TODO: go a bank at a time filling them in
  //const BankInfo first_bank = pixel_to_bank(x0, y0);
  //const BankInfo last_bank = pixel_to_bank(x1, y1);

  // For now, basic but works.
  int current_bank = -1;
  int bank_start = 0;
  for (int y = y0; y < y1; y++) {
    // TODO: read/write word size at a time
    for (int x = x0; x < x1; x++) {
      // Change bank if we need to.
      const BankInfo bank_info = pixel_to_bank(x, y);
      if (bank_info.bank != current_bank) {
        set_bank(Window::Window0, bank_info.bank);
        current_bank = bank_info.bank;
        bank_start = current_bank * WINDOW_BANK_SIZE;
      }
      // Set pixel.
      const int addr = pixel_to_addr(x, y) - bank_start;
      window0[addr] = pal_col;
    }
  }
}

} // namespace gpu
