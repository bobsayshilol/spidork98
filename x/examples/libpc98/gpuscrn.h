#ifndef GPU_SCREEN_H
#define GPU_SCREEN_H

#include "macros.h"
#include "types.h"

#define GPU_WIDTH 640
#define GPU_HEIGHT 400

namespace gpu {

// Setup and shutdown the GPU.
FASTCALL bool setup();
FASTCALL void shutdown();

// Swap screen buffers.
FASTCALL void swap();

// Wait for vsync.
FASTCALL void wait_for_vsync();

// Enable/disable text layer.
FASTCALL void enable_text_layer(bool show);

// Set a palette colour.
FASTCALL void set_palette_colour(u8 pal_col, u8 r, u8 g, u8 b);

// Clear the screen to a specific colour.
FASTCALL void clear(u8 pal_col);

// Draw a line from point (x0, y0) -> (x1, y1)
FASTCALL void draw_line(int x0, int y0, int x1, int y1, u8 pal_col);

// Draw a quad from corner (x0, y0) -> (x1, y1)
FASTCALL void draw_quad(int x0, int y0, int x1, int y1, u8 pal_col);

// Read/write a scanline (GPU_WIDTH in size).
FASTCALL void read_scanline(int line, u8 *data);
FASTCALL void write_scanline(int line, const u8 *data);

// Read/write part of a scanline starting part*SCANLINE_PART_WIDTH into the line.
#define SCANLINE_PART_WIDTH_16 16
FASTCALL void read_scanline_part_16(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_16*/);
FASTCALL void write_scanline_part_16(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_16*/);
#define SCANLINE_PART_WIDTH_32 32
FASTCALL void read_scanline_part_32(u16 line, u16 part, u8 *data /*SCANLINE_PART_WIDTH_32*/);
FASTCALL void write_scanline_part_32(u16 line, u16 part, const u8 *data /*SCANLINE_PART_WIDTH_32*/);

} // namespace gpu

#endif
