#include "game.h"
#include "menus.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"
#include "logs.h"
#include "sound.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <conio.h>
#include <dpmi.h>
#include <go32.h>

namespace game {

bool g_had_error;
bool g_sound_enabled;
u8 g_level_selected;
u8 g_unlocked_levels;

#define LOG_FILE "gamelog.txt"

//

void toggle_audio() {
  if (g_sound_enabled) {
    soundsystem::shutdown();
    g_sound_enabled = false;
  } else {
    if (soundsystem::init(pcm::SamplingRate::kHz_16_5)) {
      g_sound_enabled = true;
    } else {
      logging::print(logging::Level::Error, "Failed to setup sound system\n");
    }
  }
}

void load_menu_audio() {
  soundsystem::free_handle(VOICE_HANDLE_MENU_BGM);
  if (!soundsystem::load_sound(VOICE_HANDLE_MENU_BGM, GAME_DATA_PATH("song3.pcm"), true)) {
    logging::print(logging::Level::Warning, "Missing menu bgm");
  }

  soundsystem::free_handle(VOICE_HANDLE_MENU_CLICK);
  if (!soundsystem::load_sound(VOICE_HANDLE_MENU_CLICK, GAME_DATA_PATH("boop.pcm"), false)) {
    logging::print(logging::Level::Warning, "Missing click");
  }

  // Kick off the BGM too.
  soundsystem::play(VOICE_HANDLE_MENU_BGM);
}

namespace {

void easter_egg() {
  const unsigned char ee[10] = {
    0x20, 0x41, 0x4D, 0x49, 0x20,
    0x43, 0x55, 0x54, 0x45, 0x20,
  };

  for (int i = 1; i <= 10; i++) {
    // Read current text using the transfer buffer.
    __dpmi_regs regs;
    regs.h.cl = 0x0C;
    regs.x.ax = i;
    regs.x.ds = __tb >> 4;
    regs.x.dx = __tb & 0x0F;
    __dpmi_int(0xDC, &regs);

    // Update it.
    unsigned char temp[16] = {};
    dosmemget(__tb, 16, temp);
    temp[1] = ' ';
    temp[2] = ' ';
    temp[3] = ee[i - 1];
    temp[4] = ' ';
    temp[5] = ' ';
    dosmemput(temp, 16, __tb);

    // Write it back
    regs.h.cl = 0x0D;
    regs.x.ax = i;
    regs.x.ds = __tb >> 4;
    regs.x.dx = __tb & 0x0F;
    __dpmi_int(0xDC, &regs);
  }
}

void play() {
  logging::init(LOG_FILE);

  const unsigned seed = time(0);
  logging::print(logging::Level::Info, "RNG seed: %u", seed);
  srand(seed);

  // GPU go!
  if (!gpu::setup()) {
    logging::print(logging::Level::Error, "Failed to setup GPU\n");
    g_had_error = true;
    return;
  }
  DEFER(void*, p, NULL, (gpu::shutdown()));

  // Sound go!
  g_sound_enabled = false;
  toggle_audio();
  DEFER(void*, p, NULL, (soundsystem::shutdown()));

  load_menu_audio();
  // TODO: unload

  // No cursor unless we need it.
  _setcursortype_98(_NOCURSOR);
  DEFER(void *, p, NULL, (_setcursortype_98(_NORMALCURSOR)));

  easter_egg();

  //

  uclock_t last_time = Funcs98::ticks();

  const menus::MenuScreen * current_menu = &menus::g_splash_menu;
  current_menu->enter();

  while (current_menu && !g_had_error) {
    // Calculate delta.
    const uclock_t now = Funcs98::ticks();
    const u32 dt = now - last_time; // ticks should be small enough to fit into 32bit
    last_time = now;

    // Main update.
    soundsystem::update();
    const menus::MenuScreen *next_menu = current_menu->update(dt);
    soundsystem::update();

    // Do state machine logic.
    if (next_menu != current_menu) {
      if (current_menu->leave) current_menu->leave();
      current_menu = next_menu;
      if (current_menu && current_menu->enter) current_menu->enter();

      // Make sure the sound system is ticked often.
      soundsystem::update();
    }
  }
}

} // namespace

} // namespace game

int main() {
  if (!ISPC98(__crt0_mtype)) {
    printf("Can only run on PC-98\n");
    return EXIT_FAILURE;
  }

  game::play();

  Funcs98::clear_screen();
  if (game::g_had_error) {
    printf("Error encountered\nSee " LOG_FILE " for more info\n\n");
  }
  printf("Thanks for playing!\n");
  return EXIT_SUCCESS;
}
