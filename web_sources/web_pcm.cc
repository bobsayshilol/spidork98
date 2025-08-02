#include "pcm.h"
#include "logs.h"

#include "web_common.h"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_init.h>

#include <memory>

namespace pcm {

namespace {

struct SDLDeleter {
  void operator()(SDL_AudioStream *a) {
    SDL_DestroyAudioStream(a);
  }
};

std::unique_ptr<SDL_AudioStream, SDLDeleter> s_stream;

const i8 *s_buffer;

} // namespace

FASTCALL bool init(SamplingRate::E rate, Format::E format, const i8 *buffer) {
  if (SDL_WasInit(SDL_INIT_AUDIO)) {
    logging::print(logging::Level::Warning, "Audio already setup");
    return false;
  }

  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    logging::print(logging::Level::Error, "Failed to init audio: %s", SDL_GetError());
    return false;
  }

  if (rate != SamplingRate::kHz_16_5 || format != Format::fmt_mono) {
    logging::print(logging::Level::Error, "Unsupported PCM rate/format requested");
    return false;
  }

  SDL_AudioSpec spec{};
  spec.format = SDL_AUDIO_S8;
  spec.channels = 1;
  spec.freq = 16500;
  s_stream.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr,nullptr));
  if (!s_stream) {
    logging::print(logging::Level::Error, "Failed to open SDL audio device");
    return false;
  }

  SDL_ResumeAudioStreamDevice(s_stream.get());

  // Match the PC98 impl.
  set_volume(Volume::vol_half);

  s_buffer = buffer;
  return true;
}

FASTCALL void shutdown() {
  if (!SDL_WasInit(SDL_INIT_AUDIO)) return;

  s_stream.reset();
  s_buffer = nullptr;

  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

FASTCALL int to_hz(SamplingRate::E rate) {
  switch (rate) {
    case SamplingRate::kHz_44_1: return 44100;
    case SamplingRate::kHz_33:   return 33080;
    case SamplingRate::kHz_22:   return 22050;
    case SamplingRate::kHz_16_5: return 16540;
    case SamplingRate::kHz_11:   return 11030;
    case SamplingRate::kHz_8_3:  return 8270;
    case SamplingRate::kHz_5_5:  return 5520;
    case SamplingRate::kHz_4_1:  return 4130;
  }
  return -1;
}

FASTCALL void set_volume(Volume::E volume) {
  if (!s_stream) return;

  float gain = 0;
  switch (volume) {
    case Volume::vol_min:       gain = 0; break;
    case Volume::vol_1_quater:  gain = 0.25; break;
    case Volume::vol_half:      gain = 0.5; break;
    case Volume::vol_3_quater:  gain = 0.75; break;
    case Volume::vol_max:       gain = 1; break;
  }
  SDL_SetAudioStreamGain(s_stream.get(), gain * gain);
}

FASTCALL bool is_empty() {
  if (!s_stream) return false;
  // Bodge number, ~100ms.
  return SDL_GetAudioStreamAvailable(s_stream.get()) < 1600;
}

FASTCALL void filled(u16 buffer_elems) {
  if (!s_stream) return;
  SDL_PutAudioStreamData(s_stream.get(), s_buffer, buffer_elems);
}

} // namespace pcm
