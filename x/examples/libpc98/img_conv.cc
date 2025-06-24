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
    const u16 d = colour_distance(r, g, b, rgb[0], rgb[1], rgb[2]);
    rgb += 3;
    if (d < best_distance) {
      best_distance = d;
      idx = i;
    }
  }

  return idx;
}

bool is_colour_in_palette(images::Palette const & palette, u8 r, u8 g, u8 b) {
  const u8 *rgb = palette.rgb;
  for (int i = 0; i < palette.num_colours; i++) {
    if (r == rgb[0] && g == rgb[1] && b == rgb[2]) {
      return true;
    }
    rgb += 3;
  }
  return false;
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
  } else if (primary_header.sig != FOURCC('B', 'M', 0, 0) || primary_header.reserved != 0) {
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

bool convert(FILE * input, FILE * output, const BMPInfo & info, const images::Palette & palette, u16 flags, u8 fps, u8 num_frames) {
  printf("Converting image data...\n");
  Progress progress(info.width * info.height, 20000);

  if (num_frames > 0) {
    // Write the magic.
    const u32 magic = FOURCC('A', 'N', '9', '8');
    if (fwrite(&magic, 4, 1, output) != 1) {
      printf("Failed to write to output\n");
      return false;
    }

    // Validate frame count.
    if (info.height % num_frames) {
      printf("Height(%u) isn't divisible by frame count(%u)\n", info.height, num_frames);
      return false;
    }

    // Write the fps and frame count.
    if (fwrite(&fps, 1, 1, output) != 1 || fwrite(&num_frames, 1, 1, output) != 1) {
      printf("Failed to write to output\n");
      return false;
    }

  } else {
    // Write the magic.
    const u32 magic = FOURCC('I', 'M', '9', '8');
    if (fwrite(&magic, 4, 1, output) != 1) {
      printf("Failed to write to output\n");
      return false;
    }
  }

  // Write width, height, flags.
  const u16 width = info.width;
  const u16 height = (num_frames > 0) ? info.height / num_frames : info.height;
  if (fwrite(&width, 2, 1, output) != 1 || fwrite(&height, 2, 1, output) != 1 || fwrite(&flags, 2, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Reduce the number of reads and writes to speed up conversion.
  const u32 scanline_size = maths::pad_to<4>(width) * 3;
  u8 *scanline_data = static_cast<u8*>(alloca(scanline_size));

  // Data is upside down so we have to read backwards.
  const u16 num_scanlines = info.height;
  for (int y = num_scanlines - 1; y >= 0; y--) {
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

bool extract(FILE * input, FILE * output, const BMPInfo & info) {
  printf("Extracting palette data...\n");
  Progress progress(info.width * info.height, 100000);

  // Write the magic.
  const u32 magic = FOURCC('P', 'L', '9', '8');
  if (fwrite(&magic, 4, 1, output) != 1) {
    printf("Failed to write to output\n");
    return false;
  }

  // Setup palette.
  images::Palette palette;
  palette.num_colours = 0;

  // Reduce the number of reads and writes to speed up conversion.
  const u32 scanline_size = maths::pad_to<4>(info.width) * 3;
  u8 *scanline_data = static_cast<u8*>(alloca(scanline_size));

  // Seek to data start.
  if (fseek(input, info.data_offset, SEEK_SET) != 0) {
    printf("File ended early (%i)\n", __LINE__);
    return false;
  }

  // Start processing.
  const u16 num_scanlines = info.height;
  u8 *pal = palette.rgb;
  for (int y = 0; y < num_scanlines; y++) {
    // Read all the RGB triplets for this line.
    if (fread(scanline_data, 1, scanline_size, input) != scanline_size) {
      printf("File ended early (%i) (%u)\n", __LINE__, y);
      return false;
    }

    // Add each pixel colour to the palette.
    u8 *bgr = scanline_data;
    for (int x = 0; x < info.width; x++) {
      if (!is_colour_in_palette(palette, bgr[2], bgr[1], bgr[0])) {
        palette.num_colours++;
        if (palette.num_colours > IMAGES_MAX_PALETTE_SIZE) {
          printf("Too many colours in image\n");
          return false;
        }
        pal[0] = bgr[2];
        pal[1] = bgr[1];
        pal[2] = bgr[0];
        pal += 3;
      }
      bgr += 3;
      progress.increment();
    }
  }

  // Write the palette.
  if (fwrite(&palette.num_colours, 1, 1, output) != 1 ||
      fwrite(palette.rgb, 3, palette.num_colours, output) != palette.num_colours) {
    printf("Failed to write to output\n");
    return false;
  }

  return true;
}

int main_convert(int argc, const char ** argv) {
  if (argc != 6 && argc != 8) {
    printf(
      "Usage:\n"
      "  %s c <in> <out> <flags> <pal> [<fps> <frames>]\n"
      "  <in> is the input BMP file.\n"
      "  <out> is the output IMG that'll be written.\n"
      "  <flags> special flags OR'd together (0 to ignore):\n"
      "    1 - ping pong the animation (play backwards on loop)\n"
      "    2 - use default 16-bit palette (<pal> will be ignored)\n"
      "  <pal> is the palette to use (generated by extract).\n"
      "If the image is an animation you must also specify:\n"
      "  <fps> is the frame rate.\n"
      "  <frames> how many frames in the animation.\n"
      , argv[0]
    );
    return EXIT_FAILURE;
  }

  // Read off inputs.
  const char * const in_name = argv[2];
  const char * const out_name = argv[3];
  const int flags = atoi(argv[4]);
  const char * const palette_path = argv[5];
  const int fps = (argc == 6) ? 0 : atoi(argv[6]);
  const int num_frames = (argc == 6) ? 0 : atoi(argv[7]);

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

  // For now use the default palette.
  images::Palette palette;
  if (flags & 2) {
    palette = images::default_palette_16;
  } else if (!images::load_palette(palette, palette_path)) {
    printf("Failed to parse palette\n");
    return EXIT_FAILURE;
  }

  // Read off and validate the header.
  BMPInfo info;
  if (!read_header(input, info)) {
    printf("Failed to parse header\n");
    return EXIT_FAILURE;
  }

  // Do the conversion.
  if (!convert(input, output, info, palette, flags, fps, num_frames)) {
    printf("Failed to convert data\n");
    return EXIT_FAILURE;
  }

  // Done.
  printf("IMG file written to %s\n", out_name);
  fclose(output);
  fclose(input);
  return EXIT_SUCCESS;
}

int main_extract(int argc, const char ** argv) {
  if (argc != 4) {
    printf(
      "Usage:\n"
      "  %s e <in> <out>\n"
      "  <in> is the input BMP file.\n"
      "  <out> is the output PAL that'll be written.\n"
      , argv[0]
    );
    return EXIT_FAILURE;
  }

  // Read off inputs.
  const char * const in_name = argv[2];
  const char * const out_name = argv[3];

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

  // Extract it.
  if (!extract(input, output, info)) {
    printf("Failed to extract palette\n");
    return EXIT_FAILURE;
  }

  // Done.
  printf("PAL file written to %s\n", out_name);
  fclose(output);
  fclose(input);
  return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char ** argv) {
  if (argc >= 2) {
    const char mode = argv[1][0];
    if (mode == 'c') {
      return main_convert(argc, argv);
    } else if (mode == 'e') {
      return main_extract(argc, argv);
    }
  }

  printf(
    "Usage:\n"
    "  %s <mode> ...\n"
    "  <mode> is the mode of operation:\n"
    "    c - convert an image.\n"
    "    e - extract palette from an image.\n"
    , argv[0]
  );
  return EXIT_FAILURE;
}
