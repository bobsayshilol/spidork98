#ifndef LOADING_H
#define LOADING_H

namespace game {

struct LoadingProgress { enum E { BarStart, BarTick, FadeOutStart, FadeOutTick, FadeInStart, FadeInTick, Done }; };

// Loading screen thing.
// tick_progress() will be called at ~20fps.
void loading_screen(void (*tick_progress)(LoadingProgress::E progress));

} // namespace game

#endif
