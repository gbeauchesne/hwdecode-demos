/*
 *  image.h - Utilities for image manipulation
 *
 *  hwdecode-demos (C) 2009-2010 Splitted-Desktop Systems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include "config.h"

#define MAX_IMAGE_PLANES 3

#define IMAGE_FOURCC(ch0, ch1, ch2, ch3)        \
    ((uint32_t)(uint8_t) (ch0) |                \
     ((uint32_t)(uint8_t) (ch1) << 8) |         \
     ((uint32_t)(uint8_t) (ch2) << 16) |        \
     ((uint32_t)(uint8_t) (ch3) << 24 ))

#define IS_RGB_IMAGE_FORMAT(FORMAT)                      \
    ((FORMAT) == IMAGE_ARGB || (FORMAT) == IMAGE_BGRA || \
     (FORMAT) == IMAGE_RGBA || (FORMAT) == IMAGE_ABGR)

#ifdef WORDS_BIGENDIAN
# define IMAGE_FORMAT_NE(be, le) IMAGE_##be
#else
# define IMAGE_FORMAT_NE(be, le) IMAGE_##le
#endif

// Planar YUV 4:2:0, 12-bit, 1 plane for Y and 1 plane for UV
#define IMAGE_NV12   IMAGE_FOURCC('N','V','1','2')
// Planar YUV 4:2:0, 12-bit, 3 planes for Y V U
#define IMAGE_YV12   IMAGE_FOURCC('Y','V','1','2')
// Planar YUV 4:2:0, 12-bit, 3 planes for Y U V
#define IMAGE_IYUV   IMAGE_FOURCC('I','Y','U','V')
#define IMAGE_I420   IMAGE_FOURCC('I','4','2','0')
// Packed YUV 4:4:4, 32-bit, A Y U V
#define IMAGE_AYUV   IMAGE_FOURCC('A','Y','U','V')
// Packed YUV 4:2:2, 16-bit, Cb Y0 Cr Y1
#define IMAGE_UYVY   IMAGE_FOURCC('U','Y','V','Y')
// Packed YUV 4:2:2, 16-bit, Y0 Cb Y1 Cr
#define IMAGE_YUY2   IMAGE_FOURCC('Y','U','Y','2')
#define IMAGE_YUYV   IMAGE_FOURCC('Y','U','Y','V')
// Packed RGB 8:8:8, 32-bit, A R G B, native endian byte-order
#define IMAGE_RGB32  IMAGE_FORMAT_NE(RGBA, BGRA)
// Packed RGB 8:8:8, 32-bit, A R G B
#define IMAGE_ARGB   IMAGE_FOURCC('A','R','G','B')
// Packed RGB 8:8:8, 32-bit, B G R A
#define IMAGE_BGRA   IMAGE_FOURCC('B','G','R','A')
// Packed RGB 8:8:8, 32-bit, R G B A
  #define IMAGE_RGBA   IMAGE_FOURCC('R','G','B','A')
// Packed RGB 8:8:8, 32-bit, A B G R
#define IMAGE_ABGR   IMAGE_FOURCC('A','B','G','R')

typedef struct _Image Image;

struct _Image {
    uint32_t            format;
    unsigned int        width;
    unsigned int        height;
    uint8_t            *data;
    unsigned int        data_size;
    unsigned int        num_planes;
    uint8_t            *pixels[MAX_IMAGE_PLANES];
    unsigned int        offsets[MAX_IMAGE_PLANES];
    unsigned int        pitches[MAX_IMAGE_PLANES];
};

Image *image_create(unsigned int width, unsigned int height, uint32_t format);
void image_destroy(Image *img);

// Generate a random image, in RGB32 format
Image *image_generate(unsigned int width, unsigned int height);

// Convert images, applying scaling and color-space conversion, if required
int image_convert(Image *dst_img, Image *src_img);

uint32_t image_rgba_format(
    int          bits_per_pixel,
    int          is_msb_first,
    unsigned int red_mask,
    unsigned int green_mask,
    unsigned int blue_mask,
    unsigned int alpha_mask
);

void image_draw_rectangle(Image *img, int x, int y, int w, int h, uint32_t c);

// Write image to file (PPM format)
int image_write(Image *img, FILE *fp);

#endif /* IMAGE_H */
