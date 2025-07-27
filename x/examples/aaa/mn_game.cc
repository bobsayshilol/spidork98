#include "game.h"
#include "menus.h"
#include "pal_fade.h"
#include "unordvec.h"
#include "loading.h"

#include "funcs.h"
#include "images.h"
#include "gpuscrn.h"
#include "logs.h"
#include "maths.h"
#include "keyboard.h"

#include <conio.h>
#include <cstdio>

#define DEBUG_PRINT_FPS 0

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
#define GAME_PALETTE_RED (64 + 4)
#define GAME_PALETTE_YELLOW (64 + 5)

//

#define NUM_STARS 16
Vec2_16 s_stars[NUM_STARS];

#define STAR_SHIFT 1

//

#define BULLET_MOVE_SPEED 2

// 12 per ring x 6 rings x 2 launchers = 144, 1/3 off screen
#define MAX_ENEMY_BULLETS 128
StaticUnorderedVector<Vec2_16, MAX_ENEMY_BULLETS> s_bullet_positions;
StaticUnorderedVector<Vec2_8, MAX_ENEMY_BULLETS> s_bullet_velocities;
StaticUnorderedVector<u8, MAX_ENEMY_BULLETS> s_bullet_metadata;

#define BULLET_METADATA_HURTS_PLAYER (1 << 0)

//

#define MAX_HEALTH 3
i8 s_player_health;

#define SHIP_MOVE_SPEED 3
Vec2_16 s_player_position;

#define PLAYER_SHOOT_TIMEOUT 10 // in frames
u32 s_since_last_shot;

//

void tick_loader(LoadingProgress::E) {
  // TODO: loading screen for music and stuff

  // Make sure to tick the audio system since we're blocking in here.
  soundsystem::update();
}

const MenuScreen *run_loading() {
  // Note: this is blocking!
  loading_screen(tick_loader);
  gpu::enable_text_layer(DEBUG_PRINT_FPS);
  s_game_state = GameState::Playing;
  return &g_playing_menu;
}

//

bool emit_bullet(u16 x, u16 y, u8 angle, bool hurts_player) {
  Vec2_16 *pos = s_bullet_positions.try_add();
  if (!pos) {
    return false;
  }
  pos->u.x = x;
  pos->u.y = y;

  Vec2_8 &vel = s_bullet_velocities.add();
  STATIC_ASSERT(BULLET_MOVE_SPEED == 2); // 1 bit sign + 1 bit move, so not quite 2
  vel.i.x = maths::cos(angle) / (1 << 6);
  vel.i.y = maths::sin(angle) / (1 << 6);

  u8 metadata = 0;
  metadata |= hurts_player ? BULLET_METADATA_HURTS_PLAYER : 0;
  s_bullet_metadata.add() = metadata;

  return true;
}

//

void play_menu_enter() {
  logging::print(logging::Level::Info, "Entering gameplay menu, level %u", g_level_selected);

  // Reset gameplay state.
  s_player_health = MAX_HEALTH;
  s_player_position.i.x = PLAY_AREA_WIDTH / 2;
  s_player_position.i.y = PLAY_AREA_HEIGHT / 2;
  s_bullet_positions.clear();
  s_bullet_velocities.clear();
  s_bullet_metadata.clear();

  for (int i = 0; i < NUM_STARS; i++) {
    Vec2_16 &pos = s_stars[i];
    pos.u.x = (( i * 5 * PLAY_AREA_WIDTH / 7 + ((rand() & 7) << 5) ) % PLAY_AREA_WIDTH) << STAR_SHIFT;
    pos.u.y = (PLAY_AREA_HEIGHT * (2 * i + 1)) / (2 * NUM_STARS) + ((rand() & 3) << 2);
  }

  // Probably a good enough palette.
  images::set_palette(images::default_palette_64);
  gpu::set_palette_colour(GAME_PALETTE_BLACK, 0, 0, 0);
  gpu::set_palette_colour(GAME_PALETTE_WHITE, 255, 255, 255);
  gpu::set_palette_colour(GAME_PALETTE_DEBUG, 255, 0, 195);
  gpu::set_palette_colour(GAME_PALETTE_RED, 255, 0, 0);
  gpu::set_palette_colour(GAME_PALETTE_YELLOW, 255, 255, 0);

  // Clear everything. We'll load images later.
  gpu::g_draw_to = gpu::DrawTo::Back;
  gpu::clear(GAME_PALETTE_BLACK);
  gpu::g_draw_to = gpu::DrawTo::Front;
  gpu::clear(GAME_PALETTE_BLACK);

  s_game_state = GameState::Loading;

  flush_kb_buffer();
}

const MenuScreen *play_menu_update(u32 dt) {
  // Handle loading and game over on the slower paths.
  if (s_game_state != GameState::Playing) {
    switch (s_game_state) {
      case GameState::Playing:
        break;
      case GameState::Loading:
        return run_loading();
      case GameState::GameOver:
        // TODO
        break;
    }
  }

#if DEBUG_PRINT_FPS
  static i32 s_fps_ticks;
  static i32 s_fps_timer;
  ++s_fps_ticks;
  s_fps_timer += dt;
  if (s_fps_timer > Funcs98::ticks_per_sec()) {
    Funcs98::clear_screen();
    printf("FPS: %u\n", s_fps_ticks);
    s_fps_ticks = 0;
    s_fps_timer = 0;
  }
#endif

#if 1
  static i32 s_spawner;
  s_spawner += dt;
  if (s_spawner >= 0) {
    const u8 angle = (rand() >> 4);
    const u16 x = (rand() >> 4) % PLAY_AREA_WIDTH;
    const u16 y = (rand() >> 4) % PLAY_AREA_HEIGHT;
    if (emit_bullet(x, y, angle, true)) {
      s_spawner = -Funcs98::ticks_per_sec() / 20;
    }
  }
#endif

  const u32 keyboard_state = read_keyboard_state();
  if (keyboard_state & KB_STATE_Q) {
    return &g_main_menu;
  }

  Vec2_8 velocity;
  velocity.i.x = velocity.i.y = 0;
  bool player_moved = false;

  if (keyboard_state & (KB_STATE_W | KB_STATE_UP)) {
    //if (s_player_position.u.y) // TODO: clamp
    velocity.i.y = -SHIP_MOVE_SPEED;
    player_moved = true;
  }
  if (keyboard_state & (KB_STATE_S | KB_STATE_DOWN)) {
    velocity.i.y = SHIP_MOVE_SPEED;
    player_moved = true;
  }
  if (keyboard_state & (KB_STATE_A | KB_STATE_LEFT)) {
    velocity.i.x = -SHIP_MOVE_SPEED;
    player_moved = true;
  }
  if (keyboard_state & (KB_STATE_D | KB_STATE_RIGHT)) {
    velocity.i.x = SHIP_MOVE_SPEED;
    player_moved = true;
  }

  //

  // Move the stars.
  {
    Vec2_16 *positions = s_stars;
    for (u32 i = 0; i < NUM_STARS; i++) {
      Vec2_16 &pos = *positions++;
      const u16 y = PLAY_AREA_BORDER_Y + pos.u.y;
      u16 x = PLAY_AREA_BORDER_X + (pos.u.x >> STAR_SHIFT);

      // TODO: 2 function calls is really excessive for a single pixel
      gpu::undraw_quad(
        x, y,
        x + 1, y + 1
      );

      pos.u.x--;
      if (pos.u.x >= (PLAY_AREA_WIDTH << STAR_SHIFT)) {
        pos.u.x = (PLAY_AREA_WIDTH << STAR_SHIFT) - 1;
      }
      x = PLAY_AREA_BORDER_X + (pos.u.x >> STAR_SHIFT);

      gpu::draw_quad(
        x, y,
        x + 1, y + 1,
        GAME_PALETTE_WHITE
      );
    }
  }

  //

  // Redraw the player first.
  if (player_moved) {
    Vec2_16 pos = s_player_position;
    gpu::undraw_quad(
      PLAY_AREA_BORDER_X + pos.i.x - 1, PLAY_AREA_BORDER_Y + pos.i.y - 1,
      PLAY_AREA_BORDER_X + pos.i.x + 2, PLAY_AREA_BORDER_Y + pos.i.y + 2
    );
    pos.i.x += velocity.i.x;
    pos.i.y += velocity.i.y;
    gpu::draw_quad(
      PLAY_AREA_BORDER_X + pos.i.x - 1, PLAY_AREA_BORDER_Y + pos.i.y - 1,
      PLAY_AREA_BORDER_X + pos.i.x + 2, PLAY_AREA_BORDER_Y + pos.i.y + 2,
      GAME_PALETTE_WHITE
    );
    s_player_position = pos;
  }

  s_since_last_shot++;
  if (keyboard_state & (KB_STATE_ENTER | KB_STATE_E | KB_STATE_SPACE) && s_since_last_shot >= PLAYER_SHOOT_TIMEOUT) {
    emit_bullet(s_player_position.u.x + 2 * SHIP_WIDTH, s_player_position.u.y + SHIP_HEIGHT / 2, 0, false);
    s_since_last_shot = 0;
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
        s_bullet_metadata.erase_at(i);
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
    const u8 *metadatas = s_bullet_metadata.raw;

    for (u32 i = 0; i < count; i++) {
      const Vec2_16 pos = *positions++;
      const u8 metadata = *metadatas++;

      // Draw the thingy.
      gpu::draw_quad(
        PLAY_AREA_BORDER_X + pos.i.x - 1, PLAY_AREA_BORDER_Y + pos.i.y - 1,
        PLAY_AREA_BORDER_X + pos.i.x + 2, PLAY_AREA_BORDER_Y + pos.i.y + 2,
        (metadata & BULLET_METADATA_HURTS_PLAYER) ? GAME_PALETTE_RED : GAME_PALETTE_YELLOW
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
