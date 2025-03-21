#ifndef GPU_SCREEN_H
#define GPU_SCREEN_H

#include <dpmi.h>

#define GPU_WIDTH 640
#define GPU_HEIGHT 400

namespace gpu {

typedef unsigned char u8;

// Setup and shutdown the GPU.
bool setup();
void shutdown();

// Swap screen buffers.
void swap();

// Enable/disable text layer.
void enable_text_layer(bool show);

// Set a palette colour.
void set_palette_colour(u8 pal_col, u8 r, u8 g, u8 b);

// Clear the screen to a specific colour.
void clear(u8 pal_col);

// Draw a line from point (x0, y0) -> (x1, y1)
void draw_line(int x0, int y0, int x1, int y1, u8 pal_col);

// Draw a quad from corner (x0, y0) -> (x1, y1)
void draw_quad(int x0, int y0, int x1, int y1, u8 pal_col);

// Read/write a scanline (GPU_WIDTH in size).
void read_scanline(int line, u8 *data);
void write_scanline(int line, const u8 *data);

} // namespace gpu

#endif
