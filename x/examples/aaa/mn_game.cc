#include "game.h"
#include "menus.h"
#include "pal_fade.h"
#include "unordvec.h"

#include "funcs.h"
#include "images.h"
#include "gpuscrn.h"
#include "logs.h"
#include "maths.h"

#include <conio.h>

namespace game {
namespace menus {

namespace {

struct GameState { enum E { Loading, Playing, GameOver, }; };
GameState::E s_game_state;

//

template <typename uHalf, typename iHalf>
union Vec2T {
  struct {
    iHalf x;
    iHalf y;
  } i;
  struct {
    uHalf x;
    uHalf y;
  } u;
};

typedef Vec2T<u8, i8> Vec2_8; // 2 u8s
STATIC_ASSERT(sizeof(Vec2_8) == 2);
typedef Vec2T<u16, i16> Vec2_16; // 2 u16s
STATIC_ASSERT(sizeof(Vec2_16) == 4);

//

#define PLAY_AREA_BORDER_X 64
#define PLAY_AREA_BORDER_Y 32
#define PLAY_AREA_WIDTH (GPU_WIDTH - PLAY_AREA_BORDER_X * 2)
#define PLAY_AREA_HEIGHT (GPU_HEIGHT - PLAY_AREA_BORDER_Y * 2)

// First 64 are reserved for background.
#define GAME_PALETTE_BLACK (64 + 1)
#define GAME_PALETTE_WHITE (64 + 2)
#define GAME_PALETTE_DEBUG (64 + 3)

//

// 12 per ring x 6 rings x 2 launchers = 144, 1/3 off screen
#define MAX_BULLETS 100
StaticUnorderedVector<Vec2_16, MAX_BULLETS> s_bullet_positions;
StaticUnorderedVector<Vec2_8, MAX_BULLETS> s_bullet_velocities;

//

#define MAX_HEALTH 3
i8 s_player_health;

Vec2_16 s_player_position;

//

void play_menu_enter() {
  logging::print(logging::Level::Info, "Entering gameplay menu, level %u", g_level_selected);

  // Reset gameplay state.
  s_player_health = MAX_HEALTH;
  s_player_position.i.x = PLAY_AREA_WIDTH / 2;
  s_player_position.i.y = PLAY_AREA_HEIGHT / 2;
  s_bullet_positions.clear();
  s_bullet_velocities.clear();

  // Probably a good enough palette.
  images::set_palette(images::default_palette_64);
  gpu::set_palette_colour(GAME_PALETTE_BLACK, 0, 0, 0);
  gpu::set_palette_colour(GAME_PALETTE_WHITE, 255, 255, 255);
  gpu::set_palette_colour(GAME_PALETTE_DEBUG, 255, 0, 195);

  // Clear everything. We'll load images later.
  gpu::g_draw_to = gpu::DrawTo::Back;
  gpu::clear(GAME_PALETTE_BLACK);
  gpu::g_draw_to = gpu::DrawTo::Front;
  gpu::clear(GAME_PALETTE_BLACK);

  // TODO: loading screen for music and stuff, should happen in update loop
}

i32 s_spawner;
const MenuScreen *play_menu_update(u32 dt) {
  s_spawner += dt;
  if (s_spawner >= 0) {
    s_spawner = -Funcs98::ticks_per_sec() / 10;

    Vec2_16 *pos = s_bullet_positions.try_add();
    if (pos) {
      const u8 angle = (rand() >> 4);
      pos->i.x = (rand() >> 4) % PLAY_AREA_WIDTH;
      pos->i.y = (rand() >> 4) % PLAY_AREA_HEIGHT;
      Vec2_8 &vel = s_bullet_velocities.add();
      vel.i.x = maths::cos(angle) / (1 << 6);
      vel.i.y = maths::sin(angle) / (1 << 6);
    }
  }


  Vec2_8 velocity;
  velocity.i.x = velocity.i.y = 0;

  if (kbhit_98()) {
      const char ch = getch();
      switch (ch) {
// TODO: this isn't the right way of detecting input
        case KEY_UP: case 'W': case 'w':
          break;
        case KEY_DOWN: case 'S': case 's':
          break;
        case KEY_LEFT: case 'A': case 'a':
          break;
        case KEY_RIGHT: case 'D': case 'd':
          break;

        case KEY_ENTER: case KEY_SPACE: case 'E': case 'e':
          break;

        case KEY_ESCAPE: case 'Q': case 'q':
          return &g_main_menu;
      }
  }

  //

  // Undraw current bullet position.
  {
    const u32 count = s_bullet_positions.count;
    const Vec2_16 *positions = s_bullet_positions.raw;
    for (u32 i = 0; i < count; i++) {
      const Vec2_16 pos = *positions++;
      // Undraw around current position to mask out the area we just moved from.
      // This does more work than it needs to, but meh.
      gpu::undraw_quad(
        PLAY_AREA_BORDER_X + pos.i.x - 1, PLAY_AREA_BORDER_Y + pos.i.y - 1,
        PLAY_AREA_BORDER_X + pos.i.x + 2, PLAY_AREA_BORDER_Y + pos.i.y + 2
      );
    }
  }

  // Bullet movement.
  {
    const u32 count = s_bullet_positions.count;
    Vec2_16 *positions = s_bullet_positions.raw;
    const Vec2_8 *velocities = s_bullet_velocities.raw;
    for (u32 i = 0; i < count; i++) {
      Vec2_16 pos = *positions;
      const Vec2_8 vel = *velocities++;
      // TODO: SWAR?
      pos.i.x += vel.i.x;
      pos.i.y += vel.i.y;
      *positions++ = pos;
    }
  }

  // Collisions.
  {
    u32 count = s_bullet_positions.count;
    const Vec2_16 *positions = s_bullet_positions.raw;
    for (u32 i = 0; i < count; i++) {
      const Vec2_16 pos = *positions;

      // Detect flying off screen.
      if ((pos.u.x > PLAY_AREA_WIDTH) | (pos.u.y > PLAY_AREA_HEIGHT)) {
        s_bullet_velocities.erase_at(i);
        s_bullet_positions.erase_at(i);
        --count;
        --i;
        continue;
      }

      // We kept this one, go to next.
      positions++;
    }
  }

  //

  // Render.
  {
    const u32 count = s_bullet_positions.count;
    const Vec2_16 *positions = s_bullet_positions.raw;
    for (u32 i = 0; i < count; i++) {
      const Vec2_16 pos = positions[i];

      // Draw the thingy.
      gpu::draw_quad(
        PLAY_AREA_BORDER_X + pos.i.x - 1, PLAY_AREA_BORDER_Y + pos.i.y - 1,
        PLAY_AREA_BORDER_X + pos.i.x + 2, PLAY_AREA_BORDER_Y + pos.i.y + 2,
        GAME_PALETTE_WHITE
      );
    }
  }

  gpu::wait_for_vsync();
  return &g_playing_menu;
}

void play_menu_leave() {
  logging::print(logging::Level::Info, "Leaving gameplay menu");
#if DEBUG_CHECK_VECTOR
  s_bullet_positions.log("bullets");
#endif

  // Reinstate the audio handles.
  load_menu_audio();
}

} // namespace

const MenuScreen g_playing_menu = {
  play_menu_enter,
  play_menu_update,
  play_menu_leave,
};

} // namespace menus
} // namespace game
