// gcc -x c++ snd_conv.cc -o snd_conv.exe

#include "macros.h"
#include "pcm.h"
#include "progress.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>

namespace {

struct PCMInfo {
  pcm::SamplingRate::E sampling_rate;
  u32 frame_count;
};

u32 fourcc(u32 a, u32 b, u32 c, u32 d) {
  return (a) | (b << 8) | (c << 16) | (d << 24);
}

bool read_header(FILE * input, PCMInfo & info) {
  printf("Parsing header...\n");

  u32 chunk_name = 0;
  u32 chunk_size = 0;

  // RIFF chunk.
  if (fread(&chunk_name, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (chunk_name != fourcc('R', 'I', 'F', 'F')) {
    printf("Bad RIFF chunk: 0x%x\n", chunk_name);
    return false;
  }

  // Ignore RIFF size.
  if (fread(&chunk_size, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  }

  // WAVE format ID thing.
  if (fread(&chunk_name, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (chunk_name != fourcc('W', 'A', 'V', 'E')) {
    printf("Bad WAVE ID: 0x%x\n", chunk_name);
    return false;
  }

  // fmt chunk.
  if (fread(&chunk_name, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (chunk_name != fourcc('f', 'm', 't', ' ')) {
    printf("Bad fmt chunk: 0x%x\n", chunk_name);
    return false;
  }

  // Read off fmt info.
  struct {
    u16 format;
    u16 channels;
    u32 sampling_rate;
    u32 byte_rate;
    u16 block_align;
    u16 bits_per_sample;
  } __attribute__((packed)) fmt;
  STATIC_ASSERT(sizeof(fmt) == 16);

  // fmt chunk size.
  if (fread(&chunk_size, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (chunk_size < sizeof(fmt)) {
    printf("Bad fmt chunk size: %u\n", chunk_size);
    return false;
  }

  // fmt chunk.
  if (fread(&fmt, 1, sizeof(fmt), input) != sizeof(fmt)) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (fmt.format != 1) {
    printf("Unsupported format: %i (should be uncompressed)\n", fmt.format);
    return false;
  } else if (fmt.channels != 1) {
    printf("Unsupported channel count: %i (should be mono)\n", fmt.channels);
    return false;
  } else if (fmt.sampling_rate != 11025 && fmt.sampling_rate != 8000) {
    printf("Unsupported sampling rate: %i (should be 11025 or 8000)\n", fmt.sampling_rate);
    return false;
  } else if (fmt.bits_per_sample != 16) {
    printf("Unsupported bytes/sample: %i (should be 16 bit signed/short)\n", fmt.bits_per_sample);
    return false;
  }

  // Skip the rest of the fmt chunk.
  if (fseek(input, chunk_size - sizeof(fmt), SEEK_CUR) != 0) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  }

  // data chunk.
  if (fread(&chunk_name, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (chunk_name != fourcc('d', 'a', 't', 'a')) {
    printf("Bad data chunk: 0x%x\n", chunk_name);
    return false;
  }

  // data size.
  if (fread(&chunk_size, 1, 4, input) != 4) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  }

  // Done.
  info.sampling_rate = fmt.sampling_rate == 11025 ? pcm::SamplingRate::kHz_11 : pcm::SamplingRate::kHz_8_3;
  info.frame_count = chunk_size / 2;
  return true;
}

bool convert(FILE * input, FILE * output, const PCMInfo & info) {
  printf("Converting sound data...\n");
  Progress progress(info.frame_count, 10000);

  // Write the magic.
  const u32 magic = fourcc('S', 'D', '9', '8');
  if (fwrite(&magic, 4, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Write sampling rate.
  const u8 sampling_rate = info.sampling_rate;
  if (fwrite(&sampling_rate, 1, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Write the data.
  for (u32 frame = 0; frame < info.frame_count; frame++) {
    i16 sample16;
    if (fread(&sample16, 1, 2, input) != 2) {
      printf("File ended early (%i) (%u)\n", __LINE__, frame);
      return false;
    }

    // Scale 16bit to 8bit.
    const i8 sample8 = sample16 / 256;
    if (fwrite(&sample8, 1, 1, output) != 1) {
      printf("Failed to write to output\n");
      return false;
    }

    progress.increment();
  }

  return true;
}

} // namespace

int main(int argc, const char ** argv) {
  if (argc != 3) {
    printf(
      "Usage:\n"
      "  %s <in> <out>\n"
      "<in> is the input WAV file.\n"
      "     No resampling is performed.\n"
      "<out> is the PCM file that'll be written.\n"
      , argv[0]
    );
    return EXIT_FAILURE;
  }

  // Read off file names.
  const char * const in_name = argv[1];
  const char * const out_name = argv[2];

  // Open files.
  FILE * input = fopen(in_name, "rb");
  if (!input) {
    printf("Failed to open %s\n", in_name);
    return EXIT_FAILURE;
  }
  FILE * output = fopen(out_name, "wb");
  if (!output) {
    printf("Failed to open %s\n", out_name);
    return EXIT_FAILURE;
  }

  // Read off and validate the header.
  PCMInfo info;
  if (!read_header(input, info)) {
    printf("Failed to parse header\n");
    return EXIT_FAILURE;
  }

  // Do the conversion.
  if (!convert(input, output, info)) {
    printf("Failed to convert data\n");
    return EXIT_FAILURE;
  }

  // Done.
  printf("PCM file written to %s\n", out_name);
  fclose(output);
  fclose(input);
  return EXIT_SUCCESS;
}
