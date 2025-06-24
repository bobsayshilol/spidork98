#include "bench.h"
#include "macros.h"
#include "types.h"

#include <cstring>

namespace {

#define PORT_PCM_DATA 0xA46C

FORCEINLINE void refill_data_mono_1byte_1unroll(const u16 size, const i8 *buffer) {
  for (int i = 0; i < size; i++) {
    const i8 v0 = *buffer++;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
  }
}

FORCEINLINE void refill_data_mono_1byte_2unroll(const u16 size, const i8 *buffer) {
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
}

FORCEINLINE void refill_data_mono_1byte_4unroll(const u16 size, const i8 *buffer) {
  for (int i = 0; i < size / 4; i++) {
    const i8 v0 = *buffer++;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
    const i8 v1 = *buffer++;
    outportb(PORT_PCM_DATA, v1);
    outportb(PORT_PCM_DATA, v1);
    const i8 v2 = *buffer++;
    outportb(PORT_PCM_DATA, v2);
    outportb(PORT_PCM_DATA, v2);
    const i8 v3 = *buffer++;
    outportb(PORT_PCM_DATA, v3);
    outportb(PORT_PCM_DATA, v3);
  }
  switch (size & 3) {
    case 3: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 2: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 1: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 0: break;
  }
}

FORCEINLINE void refill_data_mono_2byte_2unroll(const u16 size, const i8 *buffer) {
  const u16 *buffer16 = reinterpret_cast<const u16*>(buffer);
  for (int i = 0; i < size / 2; i++) {
    const u16 v16 = *buffer16++;
    const i8 v0 = (v16 >> 0) & 0xFF;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
    const i8 v1 = (v16 >> 8) & 0xFF;
    outportb(PORT_PCM_DATA, v1);
    outportb(PORT_PCM_DATA, v1);
  }
  buffer += size / 2;
  if (size & 1) {
    const i8 v0 = *buffer++;
    outportb(PORT_PCM_DATA, v0);
    outportb(PORT_PCM_DATA, v0);
  }
}

FORCEINLINE void refill_data_mono_2byte_4unroll(const u16 size, const i8 *buffer) {
  const u16 *buffer16 = reinterpret_cast<const u16*>(buffer);
  for (int i = 0; i < size / 4; i++) {
    {
      const u16 v16 = *buffer16++;
      const i8 v0 = (v16 >> 0) & 0xFF;
      outportb(PORT_PCM_DATA, v0);
      outportb(PORT_PCM_DATA, v0);
      const i8 v1 = (v16 >> 8) & 0xFF;
      outportb(PORT_PCM_DATA, v1);
      outportb(PORT_PCM_DATA, v1);
    }
    {
      const u16 v16 = *buffer16++;
      const i8 v0 = (v16 >> 0) & 0xFF;
      outportb(PORT_PCM_DATA, v0);
      outportb(PORT_PCM_DATA, v0);
      const i8 v1 = (v16 >> 8) & 0xFF;
      outportb(PORT_PCM_DATA, v1);
      outportb(PORT_PCM_DATA, v1);
    }
  }
  buffer += size / 4;
  switch (size & 3) {
    case 3: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 2: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 1: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 0: break;
  }
}

FORCEINLINE void refill_data_mono_4byte_4unroll(const u16 size, const i8 *buffer) {
  const u32 *buffer32 = reinterpret_cast<const u32*>(buffer);
  for (int i = 0; i < size / 4; i++) {
      const u32 v32 = *buffer32++;
      const i8 v0 = (v32 >> 0) & 0xFF;
      outportb(PORT_PCM_DATA, v0);
      outportb(PORT_PCM_DATA, v0);
      const i8 v1 = (v32 >> 8) & 0xFF;
      outportb(PORT_PCM_DATA, v1);
      outportb(PORT_PCM_DATA, v1);
      const i8 v2 = (v32 >> 16) & 0xFF;
      outportb(PORT_PCM_DATA, v2);
      outportb(PORT_PCM_DATA, v2);
      const i8 v3 = (v32 >> 24) & 0xFF;
      outportb(PORT_PCM_DATA, v3);
      outportb(PORT_PCM_DATA, v3);
  }
  buffer += size / 4;
  switch (size & 3) {
    case 3: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 2: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 1: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 0: break;
  }
}

FORCEINLINE void refill_data_mono_4byte_4unroll_asm(const u16 size, const i8 *buffer) {
  const u32 *buffer32 = reinterpret_cast<const u32*>(buffer);
  for (int i = 0; i < size / 4; i++) {
      const u32 v32 = *buffer32++;
      __asm__ __volatile__ (
        "mov %1, %%eax\n"
        "outb %%al, %0\n"
        "outb %%al, %0\n"
        "shr $0x8, %%eax\n"
        "outb %%al, %0\n"
        "outb %%al, %0\n"
        "shr $0x8, %%eax\n"
        "outb %%al, %0\n"
        "outb %%al, %0\n"
        "shr $0x8, %%eax\n"
        "outb %%al, %0\n"
        "outb %%al, %0\n"
        : // no outputs
        : "dN" (static_cast<u16>(PORT_PCM_DATA)), "r" (v32)
        : "eax"
      );
  }
  buffer += size / 4;
  switch (size & 3) {
    case 3: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 2: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 1: { const u8 v = *buffer++; outportb(PORT_PCM_DATA, v); outportb(PORT_PCM_DATA, v); }
    case 0: break;
  }
}

} // namespace


BENCHMARK_FUNC(compare_pcm_copy) {
  const int num_iterations = 1000;
  ALIGNAS(4) i8 big_buffer[512];
  memset(big_buffer, 0, 512);

  BENCHMARK_RUN(mono_1byte_1unroll, num_iterations) {
    refill_data_mono_1byte_1unroll(512, big_buffer);
  }
  BENCHMARK_RUN(mono_1byte_2unroll, num_iterations) {
    refill_data_mono_1byte_2unroll(512, big_buffer);
  }
  BENCHMARK_RUN(mono_1byte_4unroll, num_iterations) {
    refill_data_mono_1byte_4unroll(512, big_buffer);
  }

  BENCHMARK_RUN(mono_2byte_2unroll, num_iterations) {
    refill_data_mono_2byte_2unroll(512, big_buffer);
  }
  BENCHMARK_RUN(mono_2byte_4unroll, num_iterations) {
    refill_data_mono_2byte_4unroll(512, big_buffer);
  }

  BENCHMARK_RUN(mono_4byte_4unroll, num_iterations) {
    refill_data_mono_4byte_4unroll(512, big_buffer);
  }

  BENCHMARK_RUN(mono_4byte_4unroll_asm, num_iterations) {
    refill_data_mono_4byte_4unroll_asm(512, big_buffer);
  }
}
