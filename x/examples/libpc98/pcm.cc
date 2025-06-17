#include "pcm.h"
#include "funcs.h"

#include <cstdio>
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#include <sys/segments.h>

// Hardware ID (read) and OPN mask (read/write):
//   7:4 - ID - see device_name()
//   3:2 - ???
//   1:0 - OPN mask - see reset_fifo()
#define PORT_SOUND_HW_ID_OPN_MASK 0xA460
// FIFO status (read):
//   7:7 - 0=full, 1=not full
//   6:6 - 0=not empty, 1=empty
//   5:5 - 0=not overflowed, 1=overflowed
//   0:0 - flips on sampling rate clock
// Volume (writing):
//   7:5 - volume destination
//   3:0 - 1111=min, 0000=max
#define PORT_FIFO_STATUS 0xA466
// FIFO control:
//   7:7 - 1=on, 0=off
//   6:6 - 1=record, 0=playback
//   5:5 - 1=interrupt enabled, 0=disabled
//   4:4 - interrupt flag set (read), reset (write)
//   3:3 - 1=initialised, 0=not
//   2:0 - rate - see reset_fifo()
#define PORT_FIFO_CONTROL 0xA468
// Bitrate and panning (only when FIFO is off):
//   6:6 - 1=16bit, 0=8bit
//   5:5 - left pan enabled
//   4:4 - right pan enabled
//   2:0 - 010
#define PORT_BITRATE_PAN 0xA46A
// Mute control.
//   0:0 - 1=muted, 0=not
#define PORT_MUTE 0xA66E
// 32KB FIFO buffer. signed char.
#define PORT_PCM_DATA 0xA46C
#define PCM_FIFO_BYTES 0x8000

// This doesn't work properly and would cause a hitch because it's only
// triggered when it's completely empty rather than before.
#define USE_INTERRUPTS 0

namespace pcm {

// Active FIFO buffer. User writes and interrupt reads, entire block at once.
#if USE_INTERRUPTS
// https://www.delorie.com/djgpp/v2faq/faq18_9.html
// https://www.delorie.com/djgpp/doc/ug/interrupts/irqs.html
extern "C" const signed char *g_pcm_fifo_buffer;
extern "C" unsigned short g_pcm_fifo_size;
extern "C" void g_pcm_int_handler();
//extern "C" __dpmi_paddr g_pcm_old_handler;
static _go32_dpmi_seginfo g_pcm_old_handler;

// HACK: lock memory properly
//#include <crt0.h>
//int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;

#else // USE_INTERRUPTS
namespace {

const i8 *g_pcm_fifo_buffer;
u16 g_pcm_fifo_size;
void (*s_pcm_refill_data)(void);

// Keep track of how far behind we are and request top ups as we go.
#define PCM_TICK_SCALE 256
#define PCM_TICK_SHIFT 8
uclock_t s_pcm_ticks_per_sample;
uclock_t s_pcm_last_check;
uclock_t s_pcm_lag_ticks;

} // namespace
#endif // USE_INTERRUPTS

namespace {

// System inited.
bool s_inited;
// Saved FIFO state.
u8 s_old_fifo_status;
u8 s_old_fifo_control;
u8 s_old_bitrate_pan;
u8 s_old_mute;

// Get the audio device name.
const char * device_name(u8 hw_id) {
  switch (hw_id) {
    case 0: return "PC-98DO+";
    case 1: return "PC-98GS";
    case 2: return "PC-9801-73";
    case 3: return "PC-9801-73/76";
    case 4: return "PC-9821/PC-9801-86"; // expecting to see this
    case 5: return "PC-9801-86";
    case 6: return "PC-9821Np";
    case 7: return "PC-9821X";
    case 8: return "PC-9821Canbe";
    case 15: return "No device";
  }
  return "Unknown device";
}

#if USE_INTERRUPTS
u16 get_interrupt_port_w(u8 hw_id) {
  return (hw_id == 3 || hw_id == 5) ? 0x0288 : 0x0188;
}

u8 get_interrupt(u8 hw_id) {
  // Find the ports we need to use. The read port is the next address up.
  const u16 interrupt_port_w = get_interrupt_port_w(hw_id);
  const u16 interrupt_port_r = interrupt_port_w + 2;
  // Write to it and read back the result.
  // TODO: document these ports better
  outportb(interrupt_port_w, 0x0E);
  const u8 result = inportb(interrupt_port_r);
  // Map that to an interrupt.
  switch (result >> 6) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 4;
    case 3: return 5;
  }
  // Not reachable.
  return 0;
}
#endif // USE_INTERRUPTS

void reset_fifo(SamplingRate::E rate, SampleSize::E size) {
  // Always stereo playback. The callback will duplicate if format is mono.
  const Panning::E panning = Panning::pan_stereo;

  // Disable FIFO.
  u8 fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl &= 0x1f; // 00011111
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Reset FIFO.
  fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl |= 0x08; // 00001000
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);
  fifo_ctrl &= 0xF7; // 11110111
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Playback, no interrupts, set rate.
  //fifo_ctrl &= 0xF0; // 11110000
  fifo_ctrl &= 0xB0; //10110000
  fifo_ctrl |= static_cast<u8>(rate);
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Set other options.
  u8 fifo_bitrate = 0x82; // 10000010
  fifo_bitrate |= static_cast<u8>(size);
  fifo_bitrate |= static_cast<u8>(panning);
  outportb(PORT_BITRATE_PAN, fifo_bitrate);

  // Unmute.
  u8 muted = inportb(PORT_MUTE);
  muted &= 0xFE; // 11111110
  outportb(PORT_MUTE, muted);

  // Set volume, 50% or it Hertz.
  set_volume(Volume::vol_half);

#if 0 // not sure this helps?
  // Clean out FIFO.
  for (int i = 0; i < PCM_FIFO_BYTES; i++) {
    outportb(PORT_PCM_DATA, 0);
  }
#endif
}

void enable_playback() {
  // Enable FIFO.
  u8 fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl |= 0x80; // 1000000
#if USE_INTERRUPTS
  fifo_ctrl |= 0x30; // 00110000
#endif // USE_INTERRUPTS
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

#if !USE_INTERRUPTS && 0
  // Wait for FIFO to drain.
  while (true) {
    const u8 status = inportb(PORT_FIFO_STATUS);
    if (status & 0x40) {
      break;
    }
  }
#endif
}

void refill_data_stereo() {
  // Read and reset the size.
  const u16 size = g_pcm_fifo_size;
  g_pcm_fifo_size = 0;

  // Copy data over.
  const i8 *buffer = g_pcm_fifo_buffer;
  for (int i = 0; i < size / 4; i++) {
    outportb(PORT_PCM_DATA, *buffer++);
    outportb(PORT_PCM_DATA, *buffer++);
    outportb(PORT_PCM_DATA, *buffer++);
    outportb(PORT_PCM_DATA, *buffer++);
  }
  switch (size & 3) {
    case 3: outportb(PORT_PCM_DATA, *buffer++);
    case 2: outportb(PORT_PCM_DATA, *buffer++);
    case 1: outportb(PORT_PCM_DATA, *buffer++);
    case 0: break;
  }

#if USE_INTERRUPTS
  // Inform the user that we need more data.
  detail::g_pcm_buffer_empty = true;
#endif // USE_INTERRUPTS
}

void refill_data_mono() {
  // Read and reset the size.
  const u16 size = g_pcm_fifo_size;
  g_pcm_fifo_size = 0;

  // Copy data over.
  const i8 *buffer = g_pcm_fifo_buffer;
  for (int i = 0; i < size / 2; i++) {
    const i8 v0 = *buffer++;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
    const i8 v1 = *buffer++;
    outportb(PORT_PCM_DATA, v1);
    outportb(PORT_PCM_DATA, v1);
  }
  if (size & 1) {
    const i8 v0 = *buffer++;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
  }

#if USE_INTERRUPTS
  // Inform the user that we need more data.
  detail::g_pcm_buffer_empty = true;
#endif // USE_INTERRUPTS
}

#if USE_INTERRUPTS
// TODO: big WIP here, none of worked so leaving it all in
_go32_dpmi_registers regs, regs2;
void hook_interrupt() {
  detail::g_pcm_buffer_empty = false;

  u8 const sound_id = inportb(PORT_SOUND_HW_ID_OPN_MASK);
  u8 const hw_id = sound_id >> 4;
  const u8 interrupt = get_interrupt(hw_id);

#if 1
if (_go32_dpmi_get_real_mode_interrupt_vector(interrupt, &g_pcm_old_handler)) printf("failed to get\n");
if (_go32_dpmi_simulate_int(interrupt, &regs2)) printf("failed to int1\n");

_go32_dpmi_seginfo info;
info.pm_selector = 0;//_go32_my_cs();
info.pm_offset = (unsigned long)&g_pcm_int_handler;

if (_go32_dpmi_allocate_real_mode_callback_iret(&info, &regs)) printf("failed to alloc\n");
if (_go32_dpmi_set_real_mode_interrupt_vector(interrupt, &info)) printf("failed to set\n");

if (_go32_dpmi_simulate_int(interrupt, &regs2)) printf("failed to int2\n");

#else
  // Save the old handler.
  __dpmi_get_protected_mode_interrupt_vector(interrupt, &g_pcm_old_handler);
printf("old: %08lx:%04x\n", g_pcm_old_handler.offset32, g_pcm_old_handler.selector);

printf("before: %i\n", g_pcm_fifo_size);

  // Install our handler.
  __dpmi_paddr addr;
  addr.offset32 = (unsigned long)&g_pcm_int_handler;
  addr.selector = _my_cs();
  if (__dpmi_set_protected_mode_interrupt_vector(interrupt, &addr) != 0) {
    printf("Failed to set interrupt vector\n");
  }
#endif

/// something ???
  const u16 interrupt_port_w = get_interrupt_port_w(hw_id);
  outportb(interrupt_port_w, 0x27);
  // wait?
  outportb(0x5F, 0x27);
  outportb(0x5F, 0x27);
  outportb(0x5F, 0x27);
  outportb(0x5F, 0x27);
  // write to read port?
  outportb(interrupt_port_w + 2, 0x30);
}

void unhook_interrupt() {
  const u8 sound_id = inportb(PORT_SOUND_HW_ID_OPN_MASK);
  const u8 hw_id = sound_id >> 4;
  const u8 interrupt = get_interrupt(hw_id);
#if 1
  if (_go32_dpmi_set_real_mode_interrupt_vector(interrupt, &g_pcm_old_handler)) printf("failed to restore\n");
  if (_go32_dpmi_free_real_mode_callback(&info)) printf("failed to free\n");
#else
  if (__dpmi_set_protected_mode_interrupt_vector(interrupt, &g_pcm_old_handler) != 0) {
    printf("Failed to set interrupt vector\n");
  }
#endif

printf("after: %i\n", g_pcm_fifo_size);
}
#endif // USE_INTERRUPTS

} // namespace

FASTCALL bool init(SamplingRate::E rate, Format::E format, const i8 *buffer) {
  if (s_inited) {
    printf("PCM system already initialised\n");
    return false;
  }

  const SampleSize::E size = SampleSize::bits_8;
  if (size != SampleSize::bits_8) {
    printf("Only 8bit audio is implemented\n");
    return false;
  }

  u8 const sound_id = inportb(PORT_SOUND_HW_ID_OPN_MASK);
  u8 const hw_id = sound_id >> 4;
  printf("Audio device like: %s\n", device_name(hw_id));

  // Check this is supported.
  if (hw_id == 0 || hw_id > 6) {
    printf("Unsupported audio device\n");
    return false;
  } else if (hw_id != 4) {
    printf("WARNING: not tested on this audio device\n");
  }

  // Backup current registers.
  s_old_fifo_status = inportb(PORT_FIFO_STATUS);
  s_old_fifo_control = inportb(PORT_FIFO_CONTROL);
  s_old_bitrate_pan = inportb(PORT_BITRATE_PAN);
  s_old_mute = inportb(PORT_MUTE);

  // Pick the callback.
  s_pcm_refill_data = format == Format::fmt_mono ? refill_data_mono : refill_data_stereo;

  // Reset the buffer.
  g_pcm_fifo_buffer = buffer;
  g_pcm_fifo_size = 0;

  // Reset so we don't get random static.
  reset_fifo(rate, size);

#if USE_INTERRUPTS
  // Install interrupt handler.
  hook_interrupt();
#else // USE_INTERRUPTS
  STATIC_ASSERT(UCLOCKS_PER_SEC * PCM_TICK_SCALE > 44000 * 120); // check we can use clocks for timing
  // TODO: stereo
  s_pcm_ticks_per_sample = (Funcs98::ticks_per_sec() * PCM_TICK_SCALE) / pcm::to_hz(rate);
  s_pcm_lag_ticks = 0;
  s_pcm_last_check = Funcs98::ticks();
#endif // USE_INTERRUPTS

  // PCM start.
  enable_playback();

  // All done.
  s_inited = true;
  return true;
}

FASTCALL void shutdown() {
  if (!s_inited) return;
  s_inited = false;

  // Restore registers to reset audio state.
  outportb(PORT_FIFO_STATUS, s_old_fifo_status);
  outportb(PORT_FIFO_CONTROL, s_old_fifo_control);
  outportb(PORT_BITRATE_PAN, s_old_bitrate_pan);
  outportb(PORT_MUTE, s_old_mute);

#if USE_INTERRUPTS
  // Uninstall interrupt handler.
  unhook_interrupt();
#endif // USE_INTERRUPTS

  // Reset buffer.
  g_pcm_fifo_buffer = NULL;
  g_pcm_fifo_size = 0;
  s_pcm_refill_data = 0;
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
  u8 vol = 0xA8; // 10101000
  vol |= static_cast<u8>(volume); // 00000xxx
  outportb(PORT_FIFO_STATUS, vol);
}

#if !USE_INTERRUPTS
FASTCALL bool is_empty() {
  // See how long since caller last checked.
  const uclock_t now = Funcs98::ticks();
  const uclock_t dt = now - s_pcm_last_check;
  s_pcm_last_check = now;

  // Update audio lag.
  s_pcm_lag_ticks += dt;
  return static_cast<long long>(s_pcm_lag_ticks) > 0;
}
#endif // !USE_INTERRUPTS

FASTCALL void filled(u16 buffer_elems) {
  // Set the size of the buffer so that the interrupt knows that there's more to read.
  g_pcm_fifo_size = buffer_elems;

#if USE_INTERRUPTS
  // TODO: is there a race in here?

  // Reset the user-visible flag.
  detail::g_pcm_buffer_empty = false;

  // TOOD: is this right?
  if (some_condition) {
    // Reader caught up, so poke data to the fifo to re-trigger it.
    outportb(PORT_PCM_DATA, 0); // l
    outportb(PORT_PCM_DATA, 0); // r
  }

#else // USE_INTERRUPTS
  const bool underflowed = inportb(PORT_FIFO_STATUS) & 0x40;

  // Refill and update tick lag.
  s_pcm_refill_data();
  const uclock_t ticks = (buffer_elems * s_pcm_ticks_per_sample) >> PCM_TICK_SHIFT;
  s_pcm_lag_ticks -= ticks;

  if (underflowed) {
    // If we underflowed then add a bit more to try and keep up.
    s_pcm_lag_ticks += Funcs98::ticks_per_sec() / 100; // arbitrary bodge
  }
#endif // USE_INTERRUPTS
}

} // namespace pcm
