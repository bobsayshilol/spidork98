#include "sound.h"
#include "logs.h"
#include "macros.h"
#include "memory.h"
#include "utils.h"

#include <cstdio>
#include <cstring>

// Keep voices around after shutdown so that we don't have to reload them.
#define VOICES_ARE_PRESERVED_AFTER_SHUTDOWN 1

namespace soundsystem {

namespace {

struct Sound {
  bool playing : 1;
  bool taken : 1;
  bool loop : 1;
  const i8 *data;
  u32 length;
  u32 index;
};

Sound s_sounds[MAX_SOUNDS];

bool s_active;
pcm::SamplingRate::E s_pcm_rate;
i8 *s_buffer;
int s_buffer_size;

} // namespace

FASTCALL bool init(pcm::SamplingRate::E pcm_rate) {
  if (s_active) {
    logging::print(logging::Level::Error, "Sound system already initialised");
    return false;
  }

  if (pcm_rate != pcm::SamplingRate::kHz_8_3 && pcm_rate != pcm::SamplingRate::kHz_16_5) {
    logging::print(logging::Level::Error, "Unsupported sampling rate: %i", pcm::to_hz(pcm_rate));
    return false;
  }

  // ~120ms buffer size seems good since that works out to <10fps and only uses ~10% total performance.
  s_buffer_size = pcm_rate == pcm::SamplingRate::kHz_16_5 ? 2048 : 1024;
  s_buffer = memory::alloc4<i8>(s_buffer_size);
  if (!s_buffer) {
    logging::print(logging::Level::Error, "Failed to allocate sound buffer");
    return false;
  }

  // Setup the PCM subsystem.
  if (!pcm::init(pcm_rate, pcm::Format::fmt_mono, s_buffer)) {
    logging::print(logging::Level::Error, "Failed to init PCM subsystem");
    memory::free4(s_buffer);
    return false;
  }

#if !VOICES_ARE_PRESERVED_AFTER_SHUTDOWN
  for (Handle h = 0; h < MAX_SOUNDS; h++) {
    Sound &snd = s_sounds[h];
    snd.playing = false;
    snd.taken = false;
    snd.data = 0;
  }
#endif

  logging::print(logging::Level::Info, "Sound system initialised @ %iHz", pcm::to_hz(pcm_rate));
  s_pcm_rate = pcm_rate;
  s_active = true;
  return true;
}

FASTCALL void shutdown() {
  if (!s_active) return;
  s_active = false;

  pcm::shutdown();
  memory::free4(s_buffer);

#if !VOICES_ARE_PRESERVED_AFTER_SHUTDOWN
  for (Handle h = 0; h < MAX_SOUNDS; h++) {
    free_handle(h);
  }
#endif

  logging::print(logging::Level::Info, "Sound system shut down");
}

FASTCALL void update() {
  if (!s_active) return;
  if (!pcm::is_empty()) return;

  // TODO: this should be broken up over multiple frames

  const u32 buffer_size = s_buffer_size;
  i8 *const buffer = s_buffer;

  // Mix the voices together.
  int playing_voices = 0;
  for (Handle h = 0; h < MAX_SOUNDS; h++) {
    Sound &snd = s_sounds[h];
    if (!snd.playing) continue;

play_looper:
    const u32 index = snd.index;
    const i8 *input = snd.data + index;
    const u32 length = snd.length;
    const u32 remaining = length - index;

    const u32 bytes_to_copy = utils::min(remaining, buffer_size);
    i8 *output = buffer;

    if (playing_voices == 0) {
      // If this is the first voice then we can simply memcpy.
      memcpy(output, input, bytes_to_copy);
    } else {
      // Note that the converter does volume scaling for us so we just need to add here.
      // TODO: SWAR the adds
      for (u32 i = 0; i < bytes_to_copy / 4; i++) { // TODO: check optimisation from unrolling
        *output++ += *input++;
        *output++ += *input++;
        *output++ += *input++;
        *output++ += *input++;
      }
    }
    playing_voices++;

    // If this is the end then either loop or stop it.
    snd.index += bytes_to_copy;
    if (snd.index >= length) {
      if (snd.loop) {
        snd.index = 0;
        goto play_looper;
      } else {
        snd.playing = false;
      }
    }
  }

  // Play silence.
  if (playing_voices == 0) {
    memset(buffer, 0, s_buffer_size);
  }

  // Feed the new data to the PCM layer.
  pcm::filled(buffer_size);
}

//

FASTCALL bool load_sound(Handle handle, const char *path, bool loop) {
  if (handle < 0 || handle >= MAX_SOUNDS) return false;
  Sound &snd = s_sounds[handle];
  if (snd.taken) {
    logging::print(logging::Level::Warning, "Voice handle %u already taken", handle);
    return false;
  }

  // Load the data.
  FILE * input = fopen(path, "rb");
  if (!input) {
    logging::print(logging::Level::Error, "Failed to open %s", path);
    return false;
  }
  DEFER(FILE *, p, input, fclose(p));

  // Check magic.
  u32 magic = 0;
  if (fread(&magic, 4, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  } else if (magic != FOURCC('S', 'D', '9', '8')) {
    logging::print(logging::Level::Error, "File isn't a sound: %s", path);
    return false;
  }

  // Read off rate and frame count.
  u8 pcm_rate8 = 0;
  u32 frame_count = 0;
  if (fread(&pcm_rate8, 1, 1, input) != 1 || fread(&frame_count, 4, 1, input) != 1) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    return false;
  }

  if (pcm_rate8 != static_cast<u8>(s_pcm_rate)) {
    logging::print(logging::Level::Error, "File rate (%u) doesn't match sound system rate (%u)"
      , pcm::to_hz(static_cast<pcm::SamplingRate::E>(pcm_rate8))
      , pcm::to_hz(s_pcm_rate)
    );
    return false;
  }

  // Read in the data;
  i8 *data = memory::alloc4<i8>(frame_count);
  if (!data) {
    logging::print(logging::Level::Error, "Failed to allocate voice buffer");
    return false;
  }
  if (fread(data, 1, frame_count, input) != frame_count) {
    logging::print(logging::Level::Error, "Failed to read %s", path);
    memory::free4(data);
    return false;
  }

  // Sound object is ready.
  snd.playing = false;
  snd.taken = true;
  snd.loop = loop;
  snd.data = data;
  snd.length = frame_count;
  snd.index = 0;
  logging::print(logging::Level::Info, "Loaded %s into voice %u", path, handle);
  return true;
}

FASTCALL void free_handle(Handle handle) {
  if (handle < 0 || handle >= MAX_SOUNDS) return;
  Sound &snd = s_sounds[handle];
  if (!snd.taken) return;

  snd.playing = false;
  memory::free4(const_cast<i8*>(snd.data));
  snd.taken = false;
  logging::print(logging::Level::Info, "Unloaded voice %u", handle);
}

//

FASTCALL void play(Handle handle) {
  if (handle < 0 || handle >= MAX_SOUNDS) return;
  Sound &snd = s_sounds[handle];
  if (!snd.taken) return;

  snd.index = 0;
  snd.playing = true;
}

FASTCALL void stop(Handle handle) {
  if (handle < 0 || handle >= MAX_SOUNDS) return;
  Sound &snd = s_sounds[handle];
  if (!snd.taken) return;

  snd.playing = false;
}

} // namespace soundsystem
