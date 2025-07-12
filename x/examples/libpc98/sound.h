#ifndef SOUND_H
#define SOUND_H

#include "pcm.h"
#include "types.h"

namespace soundsystem {

// Total number of sound slots available.
#define MAX_SOUNDS 4

// Handle to a sound slot.
typedef int Handle;


// Initialise the sound system.
// Use this or PCM, not both!
FASTCALL bool init(pcm::SamplingRate::E pcm_rate);

// Shutdown the sound system.
FASTCALL void shutdown();

// Tick the sound system.
// Must be called at least once a frame.
FASTCALL void update();


// Load a sound into a voice slot.
FASTCALL bool load_sound(Handle handle, const char *path, bool loop);

// Free a voice slot.
FASTCALL void free_handle(Handle handle);


// Play the sound in a voice slot.
FASTCALL void play(Handle handle);

// Stop the sound in a voice slot.
FASTCALL void stop(Handle handle);

} // namespace soundsystem

#endif
