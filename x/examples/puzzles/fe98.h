#ifndef FE98_H
#define FE98_H

extern "C" {
#include "puzzles.h"
#undef min
#undef max
}

namespace fe98 {

// Configurable options.
extern bool g_render_polygons_with_fallback;
extern float g_background_colour[3];

// frontend timer
extern bool g_timer_active;

// drawing_api
extern const drawing_api g_drapi;

} // namespace fe98

#endif
