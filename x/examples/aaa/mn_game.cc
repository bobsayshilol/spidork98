#include "game.h"
#include "menus.h"
#include "pal_fade.h"
#include "unordvec.h"
#include "loading.h"

#include "funcs.h"
#include "gpuscrn.h"
#include "images.h"
#include "keyboard.h"
#include "logs.h"
#include "maths.h"
#include "utils.h"

#include <conio.h>
#include <cstdio>
#include <dpmi.h>
#include <go32.h>

#define DEBUG_PRINT_FPS 1

namespace game {
namespace menus {

namespace {

struct GameState { enum E { Loading, LoadingDone, Playing, GameOver, }; };
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
#define GAME_PALETTE_ENEMY_BULLET (64 + 4)
#define GAME_PALETTE_PLAYER_BULLET (64 + 5)
#define GAME_PALETTE_GREY (64 + 6)
#define GAME_PALETTE_DAMAGED (64 + 7)
#define GAME_PALETTE_PLAYER_HEALTH_0 (64 + 8)
#define GAME_PALETTE_PLAYER_HEALTH_1 (64 + 9)
#define GAME_PALETTE_PLAYER_HEALTH_2 (64 + 10)
#define GAME_PALETTE_PLAYER_HEALTH_3 (64 + 11)

// TODO: pc beeper as fallback
#define VOICE_HANDLE_GAME_BGM 0
#define VOICE_HANDLE_GAME_OOF 1
#define VOICE_HANDLE_GAME_PICKUP 2
#define VOICE_HANDLE_GAME_BOOM 3

// TODO: I should have added swappable audio voices/packed data
#define do_beep_shoot()
#define do_beep_hit()

//

#define NUM_STARS 16
Vec2_16 s_stars[NUM_STARS];

#define STAR_SHIFT 1

//

#define BULLET_MOVE_SPEED 4
#define BULLET_SIZE 2 // width and height

// 12 per ring x 6 rings x 2 launchers = 144, 1/3 off screen
// From testing ~50 is max from the player with no modifiers.
#define MAX_BULLETS 128
StaticUnorderedVector<Vec2_16, MAX_BULLETS> s_bullet_positions;
StaticUnorderedVector<Vec2_8, MAX_BULLETS> s_bullet_velocities;
StaticUnorderedVector<u8, MAX_BULLETS> s_bullet_metadata;

#define BULLET_METADATA_HURTS_PLAYER (1 << 0)

//

#define MAX_HEALTH 4
i8 s_player_health;
#define PLAYER_IFRAMES 150 // ~2.5s @ 60fps
u8 s_player_iframes;

#define SHIP_WIDTH 32
#define SHIP_HEIGHT 16
images::ImageData s_ship_sprite;
images::ImageData s_ship_mask;

#define SHIP_MOVE_SPEED 2
Vec2_16 s_player_position; // top left position

#define PLAYER_SHOOT_TIMEOUT 10 // in frames
u32 s_since_last_shot;

//

#define BUCKO_SPRITE_SIZE 16
images::ImageData s_bucko_sprite;
images::ImageData s_bucko_mask;

#define MAX_BUCKOS 10
#define SHOW_BUCKOS_FOR (10 * 60) // in frames
struct BuckoState { Vec2_16 pos; i16 frames_left; };
StaticUnorderedVector<BuckoState, MAX_BUCKOS> s_bucko_states;

//#define NUM_BUCKOS_THIS_LEVEL (5 + g_level_selected * 5)
u16 s_num_buckos_collected;
u16 s_num_buckos_spawned;

//

#define ENEMY_STARTING_HEALTH (g_level_selected > 0 ? (g_level_selected >= 2) ? 12 : 8 : 5)

#define ENEMY_SPRITE_SIZE 32
images::ImageData s_enemy_sprite;
images::ImageData s_enemy_mask;

#define MAX_ENEMIES 2
struct EnemyState { Vec2_16 pos; Vec2_8 vel; u8 stage; u8 health; u8 angle; u8 last_shot; u16 meta; };
StaticUnorderedVector<EnemyState, MAX_ENEMIES> s_enemy_states;

//

int s_loader_tick;
void tick_loader(LoadingProgress::E progress) {
  switch (progress) {
    case LoadingProgress::BarStart: {
      s_loader_tick = 0;

      _go32_dpmi_meminfo info;
      _go32_dpmi_get_free_memory_information(&info);
      logging::print(logging::Level::Info, "RAM available: %liB (%li pages)", info.available_memory, info.available_physical_pages);
    } break;
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
      logging::print(logging::Level::Info, "FadeOutStart: %i", s_loader_tick);
      s_loader_tick = 0;

      // Disable all the old audio.
      for (int i = 0; i < MAX_SOUNDS; i++) {
        soundsystem::free_handle(i);
      }
      break;
    case LoadingProgress::FadeOutTick:
      ++s_loader_tick;
      switch (s_loader_tick) {
        // Load background.
        case 5:
          gpu::g_draw_to = gpu::DrawTo::Back;
          {
            images::ImageData background;
            if (background.load(GAME_DATA_PATH("game_bg.img"))) {
              soundsystem::update();
              images::draw_image(0, 0, background);
            } else {
              logging::print(logging::Level::Error, "Failed to load game background");
              gpu::clear(GAME_PALETTE_BLACK);
            }
          }
          gpu::g_draw_to = gpu::DrawTo::Front;
          break;

        // Load border images
        case 10:
          // TODO: actual images
          gpu::g_draw_to = gpu::DrawTo::Back;
          gpu::draw_quad(0,                                    0,                                     PLAY_AREA_BORDER_X,                   PLAY_AREA_BORDER_Y,                    GAME_PALETTE_DAMAGED);
          gpu::draw_quad(PLAY_AREA_BORDER_X,                   0,                                     PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, PLAY_AREA_BORDER_Y,                    GAME_PALETTE_GREY);
          gpu::draw_quad(PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, 0,                                     GPU_WIDTH,                            PLAY_AREA_BORDER_Y,                    GAME_PALETTE_DAMAGED);
          gpu::draw_quad(0,                                    PLAY_AREA_BORDER_Y,                    PLAY_AREA_BORDER_X,                   PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GAME_PALETTE_GREY);
          gpu::draw_quad(PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, PLAY_AREA_BORDER_Y,                    GPU_WIDTH,                            PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GAME_PALETTE_GREY);
          gpu::draw_quad(0,                                    PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, PLAY_AREA_BORDER_X,                   GPU_HEIGHT,                            GAME_PALETTE_DAMAGED);
          gpu::draw_quad(PLAY_AREA_BORDER_X,                   PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, GPU_HEIGHT,                            GAME_PALETTE_GREY);
          gpu::draw_quad(PLAY_AREA_BORDER_X + PLAY_AREA_WIDTH, PLAY_AREA_BORDER_Y + PLAY_AREA_HEIGHT, GPU_WIDTH,                            GPU_HEIGHT,                            GAME_PALETTE_DAMAGED);

          gpu::draw_quad(PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 2 * PLAY_AREA_HEIGHT / 8, 3 * PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 3 * PLAY_AREA_HEIGHT / 8, GAME_PALETTE_PLAYER_HEALTH_3);
          gpu::draw_quad(PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 3 * PLAY_AREA_HEIGHT / 8, 3 * PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 4 * PLAY_AREA_HEIGHT / 8, GAME_PALETTE_PLAYER_HEALTH_2);
          gpu::draw_quad(PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 4 * PLAY_AREA_HEIGHT / 8, 3 * PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 5 * PLAY_AREA_HEIGHT / 8, GAME_PALETTE_PLAYER_HEALTH_1);
          gpu::draw_quad(PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 5 * PLAY_AREA_HEIGHT / 8, 3 * PLAY_AREA_BORDER_X / 4, PLAY_AREA_BORDER_Y + 6 * PLAY_AREA_HEIGHT / 8, GAME_PALETTE_PLAYER_HEALTH_0);
          gpu::g_draw_to = gpu::DrawTo::Front;
          break;

        // Bucko sprite.
        case 15:
        if (!s_bucko_sprite.load(GAME_DATA_PATH("bucko.img"))) {
          logging::print(logging::Level::Error, "Failed to load bucko sprite");
        }
        break;
        case 20:
        if (!s_bucko_mask.load(GAME_DATA_PATH("bucko_m.img"))) {
          logging::print(logging::Level::Error, "Failed to load bucko mask");
        }
        if (s_bucko_mask.m_width != BUCKO_SPRITE_SIZE || s_bucko_mask.m_height != BUCKO_SPRITE_SIZE ||
            s_bucko_sprite.m_width != BUCKO_SPRITE_SIZE || s_bucko_sprite.m_height != BUCKO_SPRITE_SIZE)
        {
          s_bucko_mask.clear();
          s_bucko_sprite.clear();
          logging::print(logging::Level::Error, "Bad bucko sprite or mask size");
          g_had_error = true;
        }
        break;

        // Ship sprite.
        case 25:
        if (!s_ship_sprite.load(GAME_DATA_PATH("ship.img"))) {
          logging::print(logging::Level::Error, "Failed to load ship sprite");
        }
        break;
        case 30:
        if (!s_ship_mask.load(GAME_DATA_PATH("ship_m.img"))) {
          logging::print(logging::Level::Error, "Failed to load ship mask");
        }
        if (s_ship_mask.m_width != SHIP_WIDTH || s_ship_mask.m_height != SHIP_HEIGHT ||
            s_ship_sprite.m_width != SHIP_WIDTH || s_ship_sprite.m_height != SHIP_HEIGHT)
        {
          s_ship_mask.clear();
          s_ship_sprite.clear();
          logging::print(logging::Level::Error, "Bad ship sprite or mask size");
          g_had_error = true;
        }
        break;

        // Enemy sprite
        case 35: {
          const char *sprite_name = "";
          switch (g_level_selected) {
            case 0: sprite_name = GAME_DATA_PATH("train.img"); break;
            case 1: sprite_name = GAME_DATA_PATH("alien.img"); break;
            case 2: case 3: sprite_name = GAME_DATA_PATH("hawc.img"); break;
          }
          if (!s_enemy_sprite.load(sprite_name)) {
            logging::print(logging::Level::Error, "Failed to load enemy sprite");
          }
        } break;
        case 40: {
          const char *sprite_mask = "";
          switch (g_level_selected) {
            case 0: sprite_mask = GAME_DATA_PATH("train_m.img"); break;
            case 1: sprite_mask = GAME_DATA_PATH("alien_m.img"); break;
            case 2: case 3: sprite_mask = GAME_DATA_PATH("hawc_m.img"); break;
          }
          if (!s_enemy_mask.load(sprite_mask)) {
            logging::print(logging::Level::Error, "Failed to load enemy mask");
          }
          if (s_enemy_mask.m_width != ENEMY_SPRITE_SIZE || s_enemy_mask.m_height != ENEMY_SPRITE_SIZE ||
              s_enemy_sprite.m_width != ENEMY_SPRITE_SIZE || s_enemy_sprite.m_height != ENEMY_SPRITE_SIZE)
          {
            s_enemy_mask.clear();
            s_enemy_sprite.clear();
            logging::print(logging::Level::Error, "Bad enemy sprite or mask size");
            g_had_error = true;
          }
        } break;
      }
      break;

    case LoadingProgress::FadeInStart:
      logging::print(logging::Level::Info, "FadeInStart: %i", s_loader_tick);
      s_loader_tick = 0;

      // Copy backbuffer to front now that it's ready.
      soundsystem::update();
      gpu::wait_for_vsync();
      gpu::undraw_quad(0, 0, GPU_WIDTH, GPU_HEIGHT);
      break;
    case LoadingProgress::FadeInTick:
      ++s_loader_tick;
      switch (s_loader_tick) {
        // Load noises.
        case 5:
          if (!soundsystem::load_sound(VOICE_HANDLE_GAME_OOF, GAME_DATA_PATH("ow.pcm"), false)) {
            logging::print(logging::Level::Warning, "Missing game oof");
          }
        break;

        // Moar sounds.
        case 10:
          if (!soundsystem::load_sound(VOICE_HANDLE_GAME_PICKUP, GAME_DATA_PATH("gotcha.pcm"), false)) {
            logging::print(logging::Level::Warning, "Missing pickup sound");
          }
          break;

        // Load BGM.
        case 15:
          if (!soundsystem::load_sound(VOICE_HANDLE_GAME_BGM, GAME_DATA_PATH("song2.pcm"), true)) {
            logging::print(logging::Level::Warning, "Missing game bgm");
          }
          break;
      }
      break;

    case LoadingProgress::Done: {
      logging::print(logging::Level::Info, "Done: %i", s_loader_tick);

      // HACK: just restart the audio system to fix the desync issue.
      if (g_sound_enabled) {
        toggle_audio();
        toggle_audio();
      }
      // Kick off the bgm.
      soundsystem::play(VOICE_HANDLE_GAME_BGM);

      _go32_dpmi_meminfo info;
      _go32_dpmi_get_free_memory_information(&info);
      logging::print(logging::Level::Info, "RAM left: %liB (%li pages)", info.available_memory, info.available_physical_pages);
    } break;
  }

  // Make sure to tick the audio system since we're blocking in here.
  soundsystem::update();
}

void run_loading() {
  // Note: this is blocking!
  loading_screen(tick_loader);

  // We'll use this for health and goals.
  gpu::enable_text_layer(true);
  Funcs98::clear_screen();
}

//

void do_game_over(bool winner) {
  Funcs98::clear_screen();

  // Show a game over message.
  const u8 num_cols = ScreenCols_98(); // 80
  const u8 num_rows = ScreenRows_98(); // 24
  const char *text = winner ? "\xA2 M I S S I O N   S U C C E S S F U L \xA3" : "\xA2 G A M E   O V E R \xA3";
  int y = num_rows / 2 - 1;
  int x = (num_cols - strlen(text)) / 2;
  ScreenPutString_98(text, COLOUR_GREEN, x, y);

  if (winner) {
    // Unlock the next level.
    if ((g_unlocked_levels == g_level_selected) && ((g_unlocked_levels + 1) < NUM_LEVELS)) {
      g_unlocked_levels++;
    }

    char temp[64];
    sprintf(temp, "You rescued %u out of %u buckos!", s_num_buckos_collected, s_num_buckos_spawned);
    text = temp;
    y += 2;
    x = (num_cols - strlen(text)) / 2;
    ScreenPutString_98(text, COLOUR_GREEN, x, y);
  }

  text = "(Press Q or ESCAPE to return)";
  y += 2;
  x = (num_cols - strlen(text)) / 2;
  ScreenPutString_98(text, COLOUR_GREEN, x, y);

  s_game_state = GameState::GameOver;
  flush_kb_buffer();
}

const MenuScreen *run_game_over() {
  if (kbhit_98()) {
    const int ch = getch();
    if (ch == 'q' || ch == KEY_ESCAPE) {
      return &g_main_menu;
    }
  }
  return &g_playing_menu;
}

//

void update_health_palette() {
  STATIC_ASSERT(MAX_HEALTH == 4);
  for (int i = 0; i < MAX_HEALTH; i++) {
    const bool red = s_player_health <= i;
    gpu::set_palette_colour(GAME_PALETTE_PLAYER_HEALTH_0 + i, red ? 255 : 0, red ? 0 : 255, 0);
  }
}

bool emit_bullet(u16 x, u16 y, u8 angle, bool hurts_player) {
  Vec2_16 *pos = s_bullet_positions.try_add();
  if (!pos) {
    return false;
  }
  pos->u.x = x;
  pos->u.y = y;

  Vec2_8 &vel = s_bullet_velocities.add();
  STATIC_ASSERT(BULLET_MOVE_SPEED == 4); // 1 bit sign + 2 bit move
  vel.i.x = maths::cos(angle) / (1 << 5);
  vel.i.y = maths::sin(angle) / (1 << 5);

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
  s_player_iframes = 0;
  s_player_position.i.x = PLAY_AREA_WIDTH / 8;
  s_player_position.i.y = PLAY_AREA_HEIGHT / 2;
  s_bullet_positions.clear();
  s_bullet_velocities.clear();
  s_bullet_metadata.clear();
  s_bucko_states.clear();
  s_num_buckos_collected = 0;
  s_num_buckos_spawned = 0;
  s_enemy_states.clear();

  {
    STATIC_ASSERT(NUM_LEVELS == 4);
    EnemyState & enemy = s_enemy_states.add();
    enemy.pos.u.x = PLAY_AREA_WIDTH * 3 / 4;
    enemy.pos.u.y = PLAY_AREA_HEIGHT * 1 / 2;
    enemy.vel.i.x = 0;
    enemy.vel.i.y = 0;
    enemy.stage = (g_level_selected << 1) + 1;
    enemy.health = ENEMY_STARTING_HEALTH;
    enemy.angle = 127; // facing left
    enemy.last_shot = 0;
    enemy.meta = (rand() >> 3);
    switch (g_level_selected) {
      case 0: {
        enemy.pos.u.y = PLAY_AREA_HEIGHT * 1 / 3;
        enemy.vel.i.y = 1;
      } break;

      case 1: case 2: {
        // TODO: randomise
        enemy.pos.u.x = PLAY_AREA_WIDTH * 3 / 4;
        enemy.pos.u.y = PLAY_AREA_HEIGHT * 1 / 2;
      } break;

      case 3: {
        enemy.stage = (2 << 1) + 1;
        EnemyState & enemy2 = s_enemy_states.add();
        enemy2 = enemy;
        enemy.pos.u.y = PLAY_AREA_HEIGHT * 1 / 3;
        enemy2.pos.u.y = PLAY_AREA_HEIGHT * 2 / 3;
      } break;
    }
  }

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
  gpu::set_palette_colour(GAME_PALETTE_ENEMY_BULLET, 255, 255, 0);
  gpu::set_palette_colour(GAME_PALETTE_PLAYER_BULLET, 255, 0, 255);
  gpu::set_palette_colour(GAME_PALETTE_GREY, 127, 127, 127);
  gpu::set_palette_colour(GAME_PALETTE_DAMAGED, 127, 127, 127);
  update_health_palette();

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
        run_loading();
        s_game_state = GameState::LoadingDone;
        return g_had_error ? NULL : &g_playing_menu;
      case GameState::LoadingDone: // hacky state so that dt isn't huge on first call
        s_game_state = GameState::Playing;
        return &g_playing_menu;
      case GameState::GameOver:
        return run_game_over();
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

#if 0
  static u32 s_x;
  static u32 s_y;
  s_x = (s_x + 2) & 255;
  s_y = (s_y + 1) & 127;
  images::draw_sprite(s_x, s_y, s_bucko_sprite, s_bucko_mask);
#endif

  //

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
#if 0
  // Normalise movement vector.
  if (velocity.u.x && velocity.u.y) {
    velocity.i.x >>= 1;
    velocity.i.y >>= 1;
  }
#endif

  //

  // Display damage.
  if (s_player_iframes > 0) {
    unsigned t = --s_player_iframes;
    t = ((PLAYER_IFRAMES / 30) * 256 * (PLAYER_IFRAMES - t)) / PLAYER_IFRAMES;
    const i8 s = maths::sin(t);
    gpu::set_palette_colour(GAME_PALETTE_DAMAGED, 127 + s, 127 - s, 127 - s);
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
#if 1
    images::draw_sprite(PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y, s_ship_sprite, s_ship_mask);
#else
    gpu::draw_quad(
      PLAY_AREA_BORDER_X + pos.i.x, PLAY_AREA_BORDER_Y + pos.i.y,
      PLAY_AREA_BORDER_X + pos.i.x + SHIP_WIDTH, PLAY_AREA_BORDER_Y + pos.i.y + SHIP_HEIGHT,
      GAME_PALETTE_WHITE
    );
#endif
  }

  s_since_last_shot++;
  if (keyboard_state & (KB_STATE_ENTER | KB_STATE_E | KB_STATE_SPACE) && s_since_last_shot >= PLAYER_SHOOT_TIMEOUT) {
    if (emit_bullet(s_player_position.u.x + SHIP_WIDTH + 3, s_player_position.u.y + SHIP_HEIGHT / 2, 0, false)) {
      do_beep_shoot();
      s_since_last_shot = 0;
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
    const u8 *metadatas = s_bullet_metadata.raw;
    const Vec2_16 player_pos = s_player_position;
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

      // Collision with the player.
      const u8 metadata = *metadatas;
      if (metadata & BULLET_METADATA_HURTS_PLAYER) {
        const bool collided =
          (player_pos.i.x <= pos.i.x) & (pos.i.x <= player_pos.i.x + SHIP_WIDTH) &
          (player_pos.i.y <= pos.i.y) & (pos.i.y <= player_pos.i.y + SHIP_HEIGHT);
        if (collided) {
          s_bullet_velocities.erase_at(i);
          s_bullet_positions.erase_at(i);
          s_bullet_metadata.erase_at(i);
          --count;
          --i;

          // Do some damage if not in an iframe.
          if (s_player_iframes == 0) {
            soundsystem::play(VOICE_HANDLE_GAME_OOF);
            if (!g_invincible) {
              --s_player_health;
            }
            update_health_palette();
            if (s_player_health <= 0) {
              do_game_over(false);
              break;
            }
            s_player_iframes = PLAYER_IFRAMES;
          }
          continue;
        }

      } else {
        // Collisions with enemies.
        u32 num_enemies = s_enemy_states.count;
        EnemyState *enemy_states = s_enemy_states.raw;
        bool collided = false;
        for (u32 e = 0; e < num_enemies; e++) {
          const Vec2_16 enemy_pos = enemy_states->pos;
          collided =
            (enemy_pos.i.x <= pos.i.x) & (pos.i.x <= enemy_pos.i.x + ENEMY_SPRITE_SIZE) &
            (enemy_pos.i.y <= pos.i.y) & (pos.i.y <= enemy_pos.i.y + ENEMY_SPRITE_SIZE);
          if (!collided) {
            // We kept this one, go to next.
            enemy_states++;
            continue;
          }

          do_beep_hit();
          if (--(enemy_states->health) == 0) {
            enemy_states->health = ENEMY_STARTING_HEALTH;

            // Spawn buckos around the enemy.
            const int dd[5] = { ENEMY_SPRITE_SIZE, 0, -ENEMY_SPRITE_SIZE, 0, ENEMY_SPRITE_SIZE };
            for (int b = 0; b < 4; b++) {
              BuckoState *bucko_state = s_bucko_states.try_add();
              if (bucko_state) {
                s_num_buckos_spawned++;
                bucko_state->frames_left = SHOW_BUCKOS_FOR + 4 - (b << 1);
                bucko_state->pos.i.x = enemy_pos.i.x + dd[b];
                bucko_state->pos.i.y = enemy_pos.i.y + dd[b + 1];
              }
            }

            // Fire off some death shots, 8 dirs.
            for (int bul = 0; bul < 256; bul += 256 / 8) {
              emit_bullet(enemy_pos.u.x, enemy_pos.u.y, bul, true);
            }

            if (--(enemy_states->stage) == 0) {
              soundsystem::play(VOICE_HANDLE_GAME_BOOM);

              // Erase and remove the enemy.
              gpu::undraw_quad(
                PLAY_AREA_BORDER_X + enemy_pos.i.x, PLAY_AREA_BORDER_Y + enemy_pos.i.y,
                PLAY_AREA_BORDER_X + ENEMY_SPRITE_SIZE + enemy_pos.i.x, PLAY_AREA_BORDER_Y + ENEMY_SPRITE_SIZE + enemy_pos.i.y
              );
              s_enemy_states.erase_at(e);
              --num_enemies;
              --e;
            }
          }

          // We collided, don't try and collide again.
          break;
        }

        // Remove bullet.
        if (collided) {
          s_bullet_velocities.erase_at(i);
          s_bullet_positions.erase_at(i);
          s_bullet_metadata.erase_at(i);
          --count;
          --i;
          continue;
        }
      }

      // We kept this one, go to next.
      positions++;
      metadatas++;
    }
  }

  // Tick enemy logic and render them.
  {
    const u32 count = s_enemy_states.count;
    EnemyState *states = s_enemy_states.raw;
    for (u32 i = 0; i < count; i++) {
      EnemyState state = *states;

      u8 shoot_after = 6; // in frames
      u8 angle_delta = 64 + (16 / 4); // 4 directions + slowly rotating.

      // Per-level logic.
      const u8 level = g_level_selected;
      switch (level) {
        // Tutorial.
        case 0: {
          // Shoot slower, no angles.
          shoot_after = 30;
          angle_delta = 0;

          // Level one bounces up and down.
          if ((state.pos.u.y < PLAY_AREA_HEIGHT / 4) | (state.pos.u.y > PLAY_AREA_HEIGHT * 3 / 4)) {
            state.vel.i.y = -state.vel.i.y;
          }
        } break;

        case 1:
          // Just 4 directions.
          angle_delta = 64;
        case 2: case 3: {
          // Get faster with each level.
          shoot_after = 20 - (level << 2);

          // Change movement direction about once a second.
          //    64 - no of frames before movement change
          //  1024 - no of changes before burst attack
          //  2048 - "do burst attack" flag
          ++state.meta;
          if (state.meta & 64) {
            state.meta += 64 + ((rand() >> 2) & 15);
            state.meta &= ~2048U;

            // Random walk looks bad.
            //const u8 v_angle = (rand() >> 1);
            //state.vel.i.x = maths::sin(v_angle) / (1 << 6);
            //state.vel.i.y = maths::cos(v_angle) / (1 << 6);

            // Pick another point and try and work out the rough direction.
            // Bodge at the end is so that the enemy doesn't move outside of the bounds.
            const int x = PLAY_AREA_WIDTH / 3 + ( (rand() >> 1) % (PLAY_AREA_WIDTH * 2 / 3 - 2 * ENEMY_SPRITE_SIZE - 5) );
            const int y = PLAY_AREA_HEIGHT / 8 + ( (rand() >> 1) % (PLAY_AREA_HEIGHT * 7 / 8 - 2 * ENEMY_SPRITE_SIZE - 5) );
            int dx = x - state.pos.i.x;
            int dy = y - state.pos.i.y;

            // Normalise it.
            if (dx < 0) { dx = -2; } else if (dx > 0) dx = 2;
            if (dy < 0) { dy = -2; } else if (dy > 0) dy = 2;
            if (dx && dy) {
              dx >>= 1;
              dy >>= 1;
            }
            state.vel.i.x = dx;
            state.vel.i.y = dy;
          }
          // Burst attack every few seconds.
          if (state.meta & 1024) {
            state.meta ^= (1024 | 2048);
            state.vel.i.x = 0;
            state.vel.i.y = 0;
          }
          if ((level != 1) && (state.meta & 2048)) {
            angle_delta = ~16;
            shoot_after >>= 2;
          }
        } break;
      }
      
      // Movement.
      if (state.vel.u.x | state.vel.u.y) {
        // Do the usual dance of undraw, move, draw.
        // TODO: is this a big perf hit?
        gpu::undraw_quad(
          PLAY_AREA_BORDER_X + state.pos.i.x, PLAY_AREA_BORDER_Y + state.pos.i.y,
          PLAY_AREA_BORDER_X + ENEMY_SPRITE_SIZE + state.pos.i.x, PLAY_AREA_BORDER_Y + ENEMY_SPRITE_SIZE + state.pos.i.y
        );
        //state.pos.i.x = utils::clamp<i16>(state.pos.i.x + state.vel.i.x, 0, PLAY_AREA_WIDTH - ENEMY_SPRITE_SIZE);
        //state.pos.i.y = utils::clamp<i16>(state.pos.i.y + state.vel.i.y, 0, PLAY_AREA_HEIGHT - ENEMY_SPRITE_SIZE);
        state.pos.i.x += state.vel.i.x;
        state.pos.i.y += state.vel.i.y;
      }
#if 1
      images::draw_sprite(PLAY_AREA_BORDER_X + state.pos.i.x, PLAY_AREA_BORDER_Y + state.pos.i.y, s_enemy_sprite, s_enemy_mask);
#else
      gpu::draw_quad(
        PLAY_AREA_BORDER_X + state.pos.i.x, PLAY_AREA_BORDER_Y + state.pos.i.y,
        PLAY_AREA_BORDER_X + state.pos.i.x + ENEMY_SPRITE_SIZE, PLAY_AREA_BORDER_Y + state.pos.i.y + ENEMY_SPRITE_SIZE,
        GAME_PALETTE_GREY
      );
#endif

      // Shooting.
      if (++state.last_shot >= shoot_after) {
        if (emit_bullet(state.pos.u.x + ENEMY_SPRITE_SIZE / 2, state.pos.u.y + ENEMY_SPRITE_SIZE / 2, state.angle, true)) {
          state.angle -= angle_delta;
          state.last_shot = 0;
        }
      }

      *states = state;
      states++;
    }
  }

  //

  // Render bullets.
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
        (metadata & BULLET_METADATA_HURTS_PLAYER) ? GAME_PALETTE_ENEMY_BULLET : GAME_PALETTE_PLAYER_BULLET
      );
    }
  }

  //

  // Collide with pickups.
  {
    u32 count = s_bucko_states.count;
    BuckoState *bucko_states = s_bucko_states.raw;
    const Vec2_16 player_pos = s_player_position;
    for (u32 i = 0; i < count; i++) {
      --(bucko_states->frames_left);
      const BuckoState bucko_state = *bucko_states;

      // Did we hit it?
      // +--+
      // | +--+
      // +--+ |
      //   +--+
      const bool collided =
        (player_pos.i.x + SHIP_WIDTH  >= bucko_state.pos.i.x) & (bucko_state.pos.i.x + BUCKO_SPRITE_SIZE >= player_pos.i.x) &
        (player_pos.i.y + SHIP_HEIGHT >= bucko_state.pos.i.y) & (bucko_state.pos.i.y + BUCKO_SPRITE_SIZE >= player_pos.i.y);
      if (collided) {
        soundsystem::play(VOICE_HANDLE_GAME_PICKUP);
        ++s_num_buckos_collected;

        // Give back some health.
        if (s_player_health < MAX_HEALTH) {
          s_player_health++;
          update_health_palette();
        }
      }

      // Took too long, remove it.
      if (collided || bucko_state.frames_left <= 0) {
        gpu::undraw_quad(
          PLAY_AREA_BORDER_X + bucko_state.pos.i.x, PLAY_AREA_BORDER_Y + bucko_state.pos.i.y,
          PLAY_AREA_BORDER_X + bucko_state.pos.i.x + BUCKO_SPRITE_SIZE, PLAY_AREA_BORDER_Y + bucko_state.pos.i.y + BUCKO_SPRITE_SIZE
        );

        s_bucko_states.erase_at(i);
        count--;
        i--;
        continue;
      }

      // Chaos mode.
      STATIC_ASSERT(SHOW_BUCKOS_FOR == 600);
      const u16 t = bucko_state.frames_left >> 1;
      const u16 m = (1 << (3 - (t >> 7))) - 1; // 300 / 2^7 < 3
      const int dx = ((t * 5) & m) - (m >> 1);
      const int dy = ((t * 3) & m) - (m >> 1);

      // Draw it.
      images::draw_sprite(PLAY_AREA_BORDER_X + bucko_state.pos.i.x + dx, PLAY_AREA_BORDER_Y + bucko_state.pos.i.y + dy, s_bucko_sprite, s_bucko_mask);

      // Go to next.
      bucko_states++;
    }
  }

  //

  // Check for game over.
  if (s_enemy_states.count == 0 && s_bucko_states.count == 0) {
    do_game_over(true);
  }

  //

  if (!(keyboard_state & KB_STATE_T)) {
    gpu::wait_for_vsync();
  }
  return &g_playing_menu;
}

void play_menu_leave() {
  logging::print(logging::Level::Info, "Leaving gameplay menu");
#if DEBUG_CHECK_VECTOR
  s_bullet_positions.log("bullets");
#endif

  // Cleanup sprites.
  s_bucko_sprite.clear();
  s_bucko_mask.clear();
  s_ship_sprite.clear();
  s_ship_mask.clear();
  images::free_scratch();

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
