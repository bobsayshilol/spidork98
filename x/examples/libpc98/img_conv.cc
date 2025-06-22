// gcc -x c++ -O3 -c img_conv.cc -o img_conv.o
// gcc img_conv.o images.o gpuscrn.o -o img_conv.exe

#include "macros.h"
#include "images.h"
#include "maths.h"
#include "progress.h"
#include "types.h"

#include <cstdio>
#include <cstdlib>

namespace {

struct BMPInfo {
  u16 width;
  u16 height;
  u32 data_offset;
};

u32 fourcc(u32 a, u32 b, u32 c, u32 d) {
  return (a) | (b << 8) | (c << 16) | (d << 24);
}

u16 colour_distance(i16 r0, i16 g0, i16 b0, i16 r1, i16 g1, i16 b1) {
  // Simple, lazy, easy.
  return abs(r0 - r1) + abs(g0 - g1) + abs(b0 - b1);
}

u8 colour_to_palette(images::Palette const & palette, u8 r, u8 g, u8 b) {
  // TODO: memoise the colours since there's not going to be that many...
  // Go through the palette looking for the closest match.
  u8 idx = 0;
  u16 best_distance = 0xFFFF;
  const u8 *rgb = palette.rgb;
  for (int i = 0; i < palette.num_colours; i++) {
    const u8 d = colour_distance(r, g, b, rgb[0], rgb[1], rgb[2]);
    rgb += 3;
    if (d < best_distance) {
      best_distance = d;
      idx = i;
    }
  }

  return idx;
}

bool read_header(FILE * input, BMPInfo & info) {
  printf("Parsing header...\n");

  // Read off the primary header.
  struct {
    u16 sig;
    u32 file_size;
    u32 reserved;
    u32 data_offset;
  } __attribute__((packed)) primary_header;
  STATIC_ASSERT(sizeof(primary_header) == 14);

  if (fread(&primary_header, sizeof(primary_header), 1, input) != 1) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (primary_header.sig != fourcc('B', 'M', 0, 0) || primary_header.reserved != 0) {
    printf("Bad primary header: 0x%x - 0x%x\n", primary_header.sig, primary_header.reserved);
    return false;
  }

  // Read off the info header.
  struct {
    u32 header_size;
    u32 width;
    u32 height;
    u16 planes;
    u16 bpp;
    u32 compression;
    u32 compressed_size;
    u32 hres;
    u32 vres;
    u32 num_colours;
    u32 important_colours;
  } __attribute__((packed)) info_header;
  STATIC_ASSERT(sizeof(info_header) == 40);

  if (fread(&info_header, sizeof(info_header), 1, input) != 1) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  } else if (info_header.header_size != sizeof(info_header)) {
    printf("Unknown info header size: %u\n", info_header.header_size);
    return false;
  } else if (info_header.width % 16) {
    printf("Unsupported width: %u (must be a multiple of 16)\n", info_header.width);
    return false;
  } else if (info_header.planes != 1) {
    printf("Unsupported number of planes: %u (should be 1)\n", info_header.planes);
    return false;
  } else if (info_header.bpp != 24) {
    printf("Unsupported bits per pixel: %u (should be 24)\n", info_header.bpp);
    return false;
  } else if (info_header.compression != 0) {
    printf("Unsupported compression type: %u (should be uncompressed)\n", info_header.compression);
    return false;
  }

  // Check the size isn't going to break sizes.
  if (info_header.width > 0xFFFF || info_header.height > 0xFFFF) {
    printf("Image size too big: %ux%u\n", info_header.width, info_header.height);
    return false;
  }

  // Skip to the data offset.
  if (fseek(input, info.data_offset, SEEK_SET) != 0) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  }

  // Sanity check that there's enough data.
  const u32 data_length = primary_header.file_size - primary_header.data_offset;
  const u32 scanline_size = maths::pad_to<4>(info_header.width) * 3;
  const u32 bytes_expected = scanline_size * info_header.height;
  if (data_length != bytes_expected) {
    printf("Bad data size: found %u, expected %u\n", data_length, bytes_expected);
    return false;
  }

  // Done.
  info.width = info_header.width;
  info.height = info_header.height;
  info.data_offset = primary_header.data_offset;
  return true;
}

bool convert(FILE * input, FILE * output, const BMPInfo & info, const images::Palette & palette) {
  printf("Converting image data...\n");
  Progress progress(info.width * info.height, 20000);

  // Write the magic.
  const u32 magic = fourcc('I', 'M', '9', '8');
  if (fwrite(&magic, 4, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Write width and height.
  const u16 width = info.width;
  const u16 height = info.height;
  if (fwrite(&width, 2, 1, output) != 1 || fwrite(&height, 2, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Reduce the number of reads and writes to speed up conversion.
  const u32 scanline_size = maths::pad_to<4>(width) * 3;
  u8 *scanline_data = static_cast<u8*>(alloca(scanline_size));

  // Data is upside down so we have to read backwards.
  for (int y = height - 1; y >= 0; y--) {
    if (fseek(input, info.data_offset + y * scanline_size, SEEK_SET) != 0) {
      printf("File ended early (%i)\n", __LINE__);
      return false;
    }

    // Read all the RGB triplets for this line.
    if (fread(scanline_data, 1, scanline_size, input) != scanline_size) {
      printf("File ended early (%i) (%u)\n", __LINE__, y);
      return false;
    }

    // Map it to the palette.
    u8 *bgr = scanline_data;
    for (int x = 0; x < width; x++) {
      const u8 pal_idx = colour_to_palette(palette, bgr[2], bgr[1], bgr[0]);
      bgr += 3;
      scanline_data[x] = pal_idx;
      progress.increment();
    }

    // Write the scanline.
    if (fwrite(scanline_data, 1, width, output) != width) {
      printf("Failed to write to output\n");
      return false;
    }
  }

  return true;
}

} // namespace

int main(int argc, const char ** argv) {
  if (argc != 3) {
    printf(
      "Usage:\n"
      "  %s <in> <out>\n"
      "<in> is the input BMP file.\n"
      "<out> is the output IMG that'll be written.\n"
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
  BMPInfo info;
  if (!read_header(input, info)) {
    printf("Failed to parse header\n");
    return EXIT_FAILURE;
  }

  // For now use the default palette.
  const images::Palette & palette = images::default_palette_16;

  // Do the conversion.
  if (!convert(input, output, info, palette)) {
    printf("Failed to convert data\n");
    return EXIT_FAILURE;
  }

  // Done.
  printf("IMG file written to %s\n", out_name);
  fclose(output);
  fclose(input);
  return EXIT_SUCCESS;
}
