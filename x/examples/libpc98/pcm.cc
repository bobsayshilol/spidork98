#include "maths.h"
#include "pcm.h"

#include <cstdio>
#include <pc.h>

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

namespace pcm {

namespace {

bool s_inited;
unsigned char s_old_fifo_status;
unsigned char s_old_fifo_control;
unsigned char s_old_bitrate_pan;
unsigned char s_old_mute;

// Get the audio device name.
const char * device_name(unsigned char hw_id) {
  switch (hw_id) {
    case 0: return "PC-98DO+";
    case 1: return "PC-98GS";
    case 2: return "PC-9801-73";
    case 3: return "PC-9801-73/76";
    case 4: return "PC-9821/PC-9801-86";
    case 5: return "PC-9801-86";
    case 6: return "PC-9821Np";
    case 7: return "PC-9821X";
    case 8: return "PC-9821Canbe";
    case 15: return "No device";
  }
  return "Unknown device";
}

unsigned char get_interrupt(unsigned char hw_id) {
  // Find the ports we need to use. The read port is the next address up.
  const unsigned short interrupt_port_w = (hw_id == 3 || hw_id == 5) ? 0x0288 : 0x0188;
  const unsigned short interrupt_port_r = interrupt_port_w + 2;
  // Write to it and read back the result.
  // TODO: document these ports better
  outportb(interrupt_port_w, 0x0E);
  const unsigned char result = inportb(interrupt_port_r);
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

void reset_fifo(SamplingRate::E rate, SampleSize::E size, Panning::E panning) {
  // Disable FIFO.
  unsigned char fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl &= 0x1f; // 00011111
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Reset FIFO.
  fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl |= 0x08; // 00001000
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);
  fifo_ctrl &= 0xf7; // 11110111
  //outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Playback, no interrupts, set rate.
  fifo_ctrl &= 0xB0; // 10110000
  fifo_ctrl |= static_cast<unsigned char>(rate);
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);

  // Set other options.
  unsigned char fifo_bitrate = 0x82; // 10000010
  fifo_bitrate |= static_cast<unsigned char>(size);
  fifo_bitrate |= static_cast<unsigned char>(panning);
  outportb(PORT_BITRATE_PAN, fifo_bitrate);

  // Unmute.
  unsigned char muted = inportb(PORT_MUTE);
  muted &= 0xFE; // 11111110
  outportb(PORT_MUTE, muted);

  // Set volume, 50% or it Hertz.
  const unsigned char volume = 0xA8; // 10101000
  outportb(PORT_FIFO_STATUS, volume);

  // Clean out FIFO.
  for (int i = 0; i < 32768; i++) {
    outportb(PORT_PCM_DATA, 0);
  }
}

void enable_playback() {
  // Enable FIFO.
  unsigned char fifo_ctrl = inportb(PORT_FIFO_CONTROL);
  fifo_ctrl |= 0x80;
  outportb(PORT_FIFO_CONTROL, fifo_ctrl);
}

int to_hz(SamplingRate::E rate) {
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

void generate_audio(SamplingRate::E rate, SampleSize::E size, Panning::E panning) {
  (void)size; // TODO: 16 bit

  // mono is 1 sample, stereo 2 samples.
  const int bytes_per_frame = panning == Panning::pan_stereo ? 2 : 1;
  const int num_samples = 0x8000 / bytes_per_frame;

  const unsigned long sampling_rate = to_hz(rate);
  const unsigned long freq1 = 523; // C
  const unsigned long freq2 = 660; // E
  const unsigned long freq3 = 783; // G
  const unsigned long freq4 = 988; // B
  for (int t = 0; t < num_samples; t++) {
    int val = 0;
    val += maths::sin(t * (freq1 * 256) / sampling_rate);
    val += maths::sin(t * (freq2 * 256) / sampling_rate);
    val += maths::sin(t * (freq3 * 256) / sampling_rate);
    val += maths::sin(t * (freq4 * 256) / sampling_rate);
    val /= 4;
    outportb(PORT_PCM_DATA, val); // l
    outportb(PORT_PCM_DATA, val); // r
  }
}

} // namespace

FASTCALL bool init(SamplingRate::E rate, SampleSize::E size, Panning::E panning) {
  if (size != SampleSize::bits_8) {
    printf("Only 8bit audio is implemented\n");
    return false;
  }

  unsigned char const sound_id = inportb(PORT_SOUND_HW_ID_OPN_MASK);
  unsigned char const hw_id = sound_id >> 4;
  printf("Audio device like: %s\n", device_name(hw_id));

  // Check this is supported.
  if (hw_id == 0 || hw_id > 6) {
    printf("Unsupported audio device\n");
    return false;
  }

  const unsigned char interrupt = get_interrupt(hw_id);
  printf("Interrupt is 0x%x\n", interrupt);

  // Backup current registers.
  s_old_fifo_status = inportb(PORT_FIFO_STATUS);
  s_old_fifo_control = inportb(PORT_FIFO_CONTROL);
  s_old_bitrate_pan = inportb(PORT_BITRATE_PAN);
  s_old_mute = inportb(PORT_MUTE);

  // Reset so we don't get random static.
  reset_fifo(rate, size, panning);

  // Hack up something for now.
  generate_audio(rate, size, panning);
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
}

} // namespace pcm
