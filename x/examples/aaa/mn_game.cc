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

#define PLAY_AREA_BORDER_X 48
#define PLAY_AREA_BORDER_Y 32
#define PLAY_AREA_WIDTH (GPU_WIDTH - PLAY_AREA_BORDER_X * 2)
#define PLAY_AREA_HEIGHT (GPU_HEIGHT - PLAY_AREA_BORDER_Y * 2)

// First 64 are reserved for background.
#define GAME_PALETTE_BLACK (64 + 1)
#define GAME_PALETTE_WHITE (64 + 2)
#define GAME_PALETTE_DEBUG (64 + 3)
#define GAME_PALETTE_RED (64 + 4)
#define GAME_PALETTE_YELLOW (64 + 5)
#define GAME_PALETTE_GREY (64 + 6)

//

#define NUM_STARS 16
Vec2_16 s_stars[NUM_STARS];

#define STAR_SHIFT 1

//

#define BULLET_MOVE_SPEED 2
#define BULLET_SIZE 2 // width and height

// 12 per ring x 6 rings x 2 launchers = 144, 1/3 off screen
// From testing ~50 is max from the player with no modifiers.
#define MAX_BULLETS 128
StaticUnorderedVector<Vec2_16, MAX_BULLETS> s_bullet_positions;
StaticUnorderedVector<Vec2_8, MAX_BULLETS> s_bullet_velocities;
StaticUnorderedVector<u8, MAX_BULLETS> s_bullet_metadata;

#define BULLET_METADATA_HURTS_PLAYER (1 << 0)

//

#define MAX_HEALTH 3
i8 s_player_health;

#define SHIP_WIDTH 16
#define SHIP_HEIGHT 16

#define SHIP_MOVE_SPEED 2
Vec2_16 s_player_position; // top left position

#define PLAYER_SHOOT_TIMEOUT 10 // in frames
u32 s_since_last_shot;

//

int s_loader_tick;
void tick_loader(LoadingProgress::E progress) {
  switch (progress) {
    case LoadingProgress::BarStart:
      s_loader_tick = 0;
      break;
    case LoadingProgress::BarTick:
      ++s_loader_tick;
      // Fake some loading here to make it look like it's doing something.
      if (s_loader_tick & 16) {
        Funcs98::delay_ms(25);
      } else if (s_loader_tick & 8) {
        Funcs98::delay_ms(10);
      }
      break;

    case LoadingProgress::FadeOutStart:
      // Disable all the old audio.
      for (int i = 0; i < MAX_SOUNDS; i++) {
        soundsystem::free_handle(i);
      }
      s_loader_tick = 0;
      break;
    case LoadingProgress::FadeOutTick:
      ++s_loader_tick;
      switch (s_loader_tick) {
        // Load BGM.
        case 2:
          if (!soundsystem::load_sound(VOICE_HANDLE_MENU_BGM, GAME_DATA_PATH("song2.pcm"), true)) {
            logging::print(logging::Level::Warning, "Missing game bgm");
          }
          break;

        // Load background.
        case 4:
          gpu::g_draw_to = gpu::DrawTo::Back;
          {
            images::ImageData background;
            if (background.load(GAME_DATA_PATH("game_bg.img"))) {
              images::draw_image(0, 0, background);
            } else {
              logging::print(logging::Level::Error, "Failed to load game background");
              gpu::clear(GAME_PALETTE_BLACK);
            }
          }
          gpu::g_draw_to = gpu::DrawTo::Front;
          break;

        // Load border images
        case 6:
          // TODO: actual images
          gpu::g_draw_to = gpu::DrawTo::Back;
          gpu::draw_quad(0, 0, GPU_WIDTH, PLAY_AREA_BORDER_Y, GAME_PALETTE_GREY);
          gpu::draw_quad(0, PLAY_AREA_BORDER_Y, PLAY_AREA_BORDER_X, PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GAME_PALETTE_GREY);
          gpu::draw_quad(PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, PLAY_AREA_BORDER_Y, GPU_WIDTH, PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GAME_PALETTE_GREY);
          gpu::draw_quad(0, PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GPU_WIDTH, GPU_HEIGHT, GAME_PALETTE_GREY);
          gpu::g_draw_to = gpu::DrawTo::Front;
          break;
      }
      break;

    case LoadingProgress::FadeInStart:
      // Copy backbuffer to front now that it's ready.
      soundsystem::update();
      gpu::wait_for_vsync();
      gpu::undraw_quad(0, 0, GPU_WIDTH, GPU_HEIGHT);
      s_loader_tick = 0;
      break;
    case LoadingProgress::FadeInTick:
      ++s_loader_tick;
      switch (s_loader_tick) {
        // Load noises.
        case 5:
          // TODO: noises
        break;
      }
      break;

    case LoadingProgress::Done:
      // Kick off the bgm.
      soundsystem::play(VOICE_HANDLE_MENU_BGM);
      break;
  }

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
  gpu::set_palette_colour(GAME_PALETTE_GREY, 127, 127, 127);

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

#if 0
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
    if (s_player_position.u.y > SHIP_MOVE_SPEED) {
      velocity.i.y = -SHIP_MOVE_SPEED;
      player_moved = true;
    }
  }
  if (keyboard_state & (KB_STATE_S | KB_STATE_DOWN)) {
    if (s_player_position.u.y < PLAY_AREA_HEIGHT - SHIP_MOVE_SPEED - SHIP_HEIGHT) {
      velocity.i.y = SHIP_MOVE_SPEED;
      player_moved = true;
    }
  }
  if (keyboard_state & (KB_STATE_A | KB_STATE_LEFT)) {
    if (s_player_position.u.x > SHIP_MOVE_SPEED) {
      velocity.i.x = -SHIP_MOVE_SPEED;
      player_moved = true;
    }
  }
  if (keyboard_state & (KB_STATE_D | KB_STATE_RIGHT)) {
    if (s_player_position.u.x < PLAY_AREA_WIDTH - SHIP_MOVE_SPEED - SHIP_WIDTH) {
      velocity.i.x = SHIP_MOVE_SPEED;
      player_moved = true;
    }
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

  // Erase the old player position.
  if (player_moved) {
    Vec2_16 pos = s_player_position;
    gpu::undraw_quad(
      PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y,
      PLAY_AREA_BORDER_X + pos.i.x + SHIP_WIDTH, PLAY_AREA_BORDER_Y + pos.i.y + SHIP_HEIGHT
    );
    pos.i.x += velocity.i.x;
    pos.i.y += velocity.i.y;
    s_player_position = pos;
  }

  // Always redraw the player so that bullets and other objects don't erase it.
  {
    const Vec2_16 pos = s_player_position;
    gpu::draw_quad(
      PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y,
      PLAY_AREA_BORDER_X + pos.i.x + SHIP_WIDTH, PLAY_AREA_BORDER_Y + pos.i.y + SHIP_HEIGHT,
      GAME_PALETTE_WHITE
    );
  }

  s_since_last_shot++;
  if (keyboard_state & (KB_STATE_ENTER | KB_STATE_E | KB_STATE_SPACE) && s_since_last_shot >= PLAYER_SHOOT_TIMEOUT) {
    emit_bullet(s_player_position.u.x + 3 * SHIP_WIDTH / 2, s_player_position.u.y + SHIP_HEIGHT / 2, 0, false);
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
        PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y,
        PLAY_AREA_BORDER_X + pos.i.x + BULLET_SIZE, PLAY_AREA_BORDER_Y + pos.i.y + BULLET_SIZE
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
        PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y,
        PLAY_AREA_BORDER_X + pos.i.x + BULLET_SIZE, PLAY_AREA_BORDER_Y + pos.i.y + BULLET_SIZE,
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
