/*
 *  image.c - Utilities for image manipulation
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

#include "sysdeps.h"
#include "image.h"
#include "utils.h"
#include "common.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if HAVE_AVUTIL
# ifdef HAVE_LIBAVUTIL_PIXFMT_H
#  include <libavutil/pixfmt.h>
# endif
# ifdef HAVE_FFMPEG_AVUTIL_H
#  include <ffmpeg/avutil.h>
# endif
#endif

#if HAVE_SWSCALE
# ifdef HAVE_LIBSWSCALE_SWSCALE_H
#  include <libswscale/swscale.h>
# endif
# ifdef HAVE_FFMPEG_SWSCALE_H
#  include <ffmpeg/swscale.h>
# endif
#endif

#if HAVE_CAIRO
#include <cairo.h>
#endif

#define DEBUG 1
#include "debug.h"

#undef  FOURCC
#define FOURCC IMAGE_FOURCC

Image *image_create(unsigned int width, unsigned int height, uint32_t format)
{
    unsigned int i, width2, height2, size2, size;
    Image *img = NULL;

    img = calloc(1, sizeof(*img));
    if (!img)
        goto error;

    img->format         = format;
    img->width          = width;
    img->height         = height;
    size                = width * height;
    width2              = (width  + 1) / 2;
    height2             = (height + 1) / 2;
    size2               = width2 * height2;
    switch (format) {
    case IMAGE_ARGB:
    case IMAGE_BGRA:
    case IMAGE_RGBA:
    case IMAGE_ABGR:
        img->pitches[0] = width * 4;
        break;
    case IMAGE_NV12:
        img->num_planes = 2;
        img->pitches[0] = width;
        img->offsets[0] = 0;
        img->pitches[1] = width;
        img->offsets[1] = size;
        img->data_size  = size + 2 * size2;
        break;
    case IMAGE_YV12:
    case IMAGE_IYUV:
    case IMAGE_I420:
        img->num_planes = 3;
        img->pitches[0] = width;
        img->offsets[0] = 0;
        img->pitches[1] = width2;
        img->offsets[1] = size;
        img->pitches[2] = width2;
        img->offsets[2] = size + size2;
        img->data_size  = size + 2 * size2;
        break;
    default:
        goto error;
    }
    if (IS_RGB_IMAGE_FORMAT(format)) {
        img->num_planes = 1;
        img->offsets[0] = 0;
        img->data_size  = img->pitches[0] * height;
    }
    if (!img->data_size)
        goto error;

    img->data = malloc(img->data_size);
    if (!img->data)
        goto error;

    for (i = 0; i < img->num_planes; i++)
        img->pixels[i] = img->data + img->offsets[i];
    return img;

error:
    free(img);
    return NULL;
}

#if HAVE_CAIRO
static const int PETAL_MIN = 5;
static const int PETAL_VAR = 8;

static void draw_flower(cairo_t *cr, int x, int y, int petal_size)
{
    /* This comes from Clutter/Cairo examples */
    int i, j;
    int size;
    int n_groups; // num groups of petals 1-3
    int n_petals; // num of petals 4 - 8
    int pm1, pm2;
    int idx, last_idx = -1;

    static const double colors[] = {
        0.71, 0.81, 0.83, 
        1.00, 0.78, 0.57,
        0.64, 0.30, 0.35,
        0.73, 0.40, 0.39, 
        0.91, 0.56, 0.64, 
        0.70, 0.47, 0.45,  
        0.92, 0.75, 0.60,  
        0.82, 0.86, 0.85,  
        0.51, 0.56, 0.67, 
        1.00, 0.79, 0.58,  
    };

    size = petal_size * 8;
    n_groups = gen_random_int_range(1, 4);

    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_set_tolerance(cr, 0.1);

    for (i = 0; i < n_groups; i++) {
        n_petals = gen_random_int_range(4, 9);
        cairo_save(cr);

        cairo_rotate(cr, gen_random_int_range(0, 6));

        do {
            idx = gen_random_int_range(0, ((sizeof(colors) / sizeof(double) / 3)) * 3);
        } while (idx == last_idx);

        cairo_set_source_rgba(cr,
                              colors[idx],
                              colors[idx + 1],
                              colors[idx + 2],
                              0.5);

        last_idx = idx;

        /* Some bezier randomness */
        pm1 = gen_random_int_range(0, 20);
        pm2 = gen_random_int_range(0, 4);

        for (j = 1; j < n_petals + 1; j++) {
            cairo_save(cr);
            cairo_rotate(cr, ((2 * M_PI) / n_petals) * j);

            /* Petals are made up beziers */
            cairo_new_path(cr);
            cairo_move_to(cr, 0, 0);
            cairo_rel_curve_to(cr,
                               petal_size, petal_size,
                               (pm2 + 2) * petal_size, petal_size,
                               (2 * petal_size) + pm1, 0);
            cairo_rel_curve_to(cr,
                               0 + (pm2 * petal_size), -petal_size,
                               -petal_size, -petal_size,
                               -((2 * petal_size) + pm1), 0);
            cairo_close_path(cr);
            cairo_fill(cr);
            cairo_restore(cr);
	}

        petal_size -= gen_random_int_range(0, size / 8);
        cairo_restore(cr);
    }

    /* Finally draw flower center */
    do {
        idx = gen_random_int_range(0, ((sizeof(colors) / sizeof(double) / 3)) * 3);
    } while (idx == last_idx);

    if (petal_size < 0)
        petal_size = gen_random_int_range(0, 10);

    cairo_set_source_rgba(cr,
                          colors[idx],
                          colors[idx + 1],
                          colors[idx + 2],
                          0.5);

    cairo_arc(cr, 0, 0, petal_size, 0, M_PI * 2);
    cairo_fill(cr);
    cairo_restore(cr);
}
#endif

static const unsigned int RECT_MIN = 10;
static const unsigned int RECT_VAR = 50;

static void draw_rectangle(Image *img, int x, int y, int w, int h, uint32_t c)
{
    uint32_t *pixels = (uint32_t *)(img->pixels[0] + y * img->pitches[0] + x * 4);
    unsigned int i, j;

    if (x >= img->width || y >= img->height)
        return;

    /* XXX: no need to optimize this */
    for (j = 0; j < h && (y + j) < img->height; j++, pixels += img->width) {
        for (i = 0; i < w && (x + i) < img->width; i++)
            pixels[i] = c;
    }
}

void image_draw_rectangle(Image *img, int x, int y, int w, int h, uint32_t c)
{
    assert(img->format == IMAGE_RGB32);
    draw_rectangle(img, x, y, w, h, c);
}

static int image_generate_flowers(Image *img)
{
#if HAVE_CAIRO
    cairo_surface_t *surface;
    cairo_t *cr;
    int i, x, y, w;

    surface = cairo_image_surface_create_for_data(img->pixels[0],
                                                  CAIRO_FORMAT_RGB24,
                                                  img->width,
                                                  img->height,
                                                  img->pitches[0]);
    if (!surface)
        return 0;

    cr = cairo_create(surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    for (i = 0; i < 10; i++) {
        x = gen_random_int_range(0, img->width - (PETAL_MIN + PETAL_VAR) * 2);
        y = gen_random_int_range(0, img->height);
        w = gen_random_int_range(PETAL_MIN, PETAL_MIN + PETAL_VAR);
        draw_flower(cr, x, y, w);
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return 1;
#endif
    return 0;
}

static int image_generate_rects(Image *img)
{
    int i, x, y, w, h, c;

    /* Simply draw random rectangles */
    memset(img->data, 0, img->data_size);
    for (i = 0; i < 10; i++) {
        x = gen_random_int_range(0, img->width - (RECT_MIN + RECT_VAR));
        y = gen_random_int_range(0, img->height);
        w = gen_random_int_range(RECT_MIN, RECT_MIN + RECT_VAR);
        h = gen_random_int_range(RECT_MIN, RECT_MIN + RECT_VAR);
        c = 0xff000000 | gen_random_int_range(0, 1U << 24);
        draw_rectangle(img, x, y, w, h, c);
    }
    return 1;
}

static int image_generate_rgb_rects(Image *img)
{
    const unsigned int w = img->width;
    const unsigned int h = img->height;

    /* Draw R/G/B rectangles */
    memset(img->data, 0, img->data_size);
    draw_rectangle(img, 0,   0, w/2, h/2, 0xffff0000);
    draw_rectangle(img, w/2, 0, w/2, h/2, 0xff00ff00);
    draw_rectangle(img, 0, h/2, w/2, h/2, 0xff0000ff);
    return 1;
}

Image *image_generate(unsigned int width, unsigned int height)
{
    Image *img = image_create(width, height, IMAGE_RGB32);
    if (!img)
        return NULL;

    int ok;
    const enum GenImageType genimage_type = common_get_context()->genimage_type;
    switch (genimage_type) {
    case GENIMAGE_AUTO:
    case GENIMAGE_FLOWERS:
        ok = image_generate_flowers(img);
        if (ok || genimage_type == GENIMAGE_FLOWERS)
            break;
        /* fall-through */
    case GENIMAGE_RECTS:
        ok = image_generate_rects(img);
        break;
    case GENIMAGE_RGB_RECTS:
        ok = image_generate_rgb_rects(img);
        break;
    default:
        ok = 0;
        break;
    }

    if (!ok) {
        image_destroy(img);
        return NULL;
    }
    return img;
}

void image_destroy(Image *img)
{
    if (!img)
        return;

    if (img->data) {
        free(img->data);
        img->data = NULL;
    }
    free(img);
}

uint32_t image_rgba_format(
    int          bits_per_pixel,
    int          is_msb_first,
    unsigned int red_mask,
    unsigned int green_mask,
    unsigned int blue_mask,
    unsigned int alpha_mask
)
{
    static const struct {
        int             fourcc;
        unsigned char   bits_per_pixel;
        unsigned char   is_msb_first;
        unsigned int    red_mask;
        unsigned int    green_mask;
        unsigned int    blue_mask;
        unsigned int    alpha_mask;
    }
#define BE 1
#define LE 0
#ifdef WORDS_BIGENDIAN
#define NE BE
#define OE LE
#else
#define NE LE
#define OE BE
#endif
    pixfmts[] = {
        { IMAGE_FORMAT_NE(ABGR, RGBA),
          32, NE, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
        { IMAGE_FORMAT_NE(ABGR, RGBA),
          32, OE, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
        { IMAGE_FORMAT_NE(ARGB, BGRA),
          32, NE, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
        { IMAGE_FORMAT_NE(ARGB, BGRA),
          32, OE, 0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff },
        { IMAGE_FORMAT_NE(RGBA, ABGR),
          32, NE, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff },
        { IMAGE_FORMAT_NE(RGBA, ABGR),
          32, OE, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
    };
    int i;
    for (i = 0; i < ARRAY_ELEMS(pixfmts); i++) {
        if (pixfmts[i].bits_per_pixel == bits_per_pixel &&
            pixfmts[i].is_msb_first   == is_msb_first   &&
            pixfmts[i].red_mask       == red_mask       &&
            pixfmts[i].green_mask     == green_mask     &&
            pixfmts[i].blue_mask      == blue_mask      &&
            pixfmts[i].alpha_mask     == alpha_mask)
            return pixfmts[i].fourcc;
    }
    return 0;
}

#if HAVE_SWSCALE
static enum PixelFormat get_pixel_format(uint32_t fourcc)
{
    switch (fourcc) {
    case FOURCC('N','V','1','2'): return PIX_FMT_NV12;
    case FOURCC('I','Y','U','V'): /* duplicate of */
    case FOURCC('I','4','2','0'): return PIX_FMT_YUV420P;
    case FOURCC('U','Y','V','Y'): return PIX_FMT_UYVY422;
    case FOURCC('Y','U','Y','2'): /* duplicate of */
    case FOURCC('Y','U','Y','V'): return PIX_FMT_YUYV422;
    case IMAGE_ARGB: return PIX_FMT_ARGB;
    case IMAGE_BGRA: return PIX_FMT_BGRA;
    case IMAGE_RGBA: return PIX_FMT_RGBA;
    case IMAGE_ABGR: return PIX_FMT_ABGR;
    }
    return PIX_FMT_NONE;
}
#endif

#if HAVE_SWSCALE
static int image_convert_libswscale(
    uint8_t     *arg_src[MAX_IMAGE_PLANES],
    int          arg_src_stride[MAX_IMAGE_PLANES],
    unsigned int src_width,
    unsigned int src_height,
    uint32_t     src_fourcc,
    uint8_t     *arg_dst[MAX_IMAGE_PLANES],
    int          arg_dst_stride[MAX_IMAGE_PLANES],
    unsigned int dst_width,
    unsigned int dst_height,
    uint32_t     dst_fourcc
)
{
    int error = -1;
    struct SwsContext *sws = NULL;
    enum PixelFormat src_pix_fmt, dst_pix_fmt;
    uint8_t *src[MAX_IMAGE_PLANES];
    uint8_t *dst[MAX_IMAGE_PLANES];
    int src_stride[MAX_IMAGE_PLANES];
    int dst_stride[MAX_IMAGE_PLANES];
    uint8_t *tmp_src[MAX_IMAGE_PLANES] = { NULL, };
    uint8_t *tmp_dst[MAX_IMAGE_PLANES] = { NULL, };
    int tmp_src_stride[MAX_IMAGE_PLANES];
    int tmp_dst_stride[MAX_IMAGE_PLANES];
    int i, j, stride;

    /* XXX: libswscale does not support AYUV formats yet */
    switch (src_fourcc) {
    case IMAGE_AYUV:
        src_pix_fmt = PIX_FMT_YUV444P;
        for (i = 0; i < 3; i++) {
            tmp_src[i] = malloc(src_width * src_height);
            if (!tmp_src[i])
                goto end;
            tmp_src_stride[i] = src_width;
            src[i] = tmp_src[i];
            src_stride[i] = tmp_src_stride[i];
        }
        stride = arg_src_stride[0];
        for (j = 0; j < src_height; j++) {
            for (i = 0; i < src_width; i++) {
                const int src_offset = j * arg_src_stride[0] + 4 * i;
                const int dst_offset = j * src_width + i;
#ifdef WORDS_BIGENDIAN
                tmp_src[0][dst_offset] = arg_src[0][src_offset + 1];
                tmp_src[1][dst_offset] = arg_src[0][src_offset + 2];
                tmp_src[2][dst_offset] = arg_src[0][src_offset + 3];
#else
                tmp_src[0][dst_offset] = arg_src[0][src_offset + 2];
                tmp_src[1][dst_offset] = arg_src[0][src_offset + 1];
                tmp_src[2][dst_offset] = arg_src[0][src_offset + 0];
#endif
            }
        }
        break;
    case IMAGE_YV12:
        src_pix_fmt = PIX_FMT_YUV420P;
        src[0] = arg_src[0];
        src_stride[0] = arg_src_stride[0];
        src[1] = arg_src[2];
        src_stride[1] = arg_src_stride[2];
        src[2] = arg_src[1];
        src_stride[2] = arg_src_stride[1];
        break;
    default:
        src_pix_fmt = get_pixel_format(src_fourcc);
        for (i = 0; i < MAX_IMAGE_PLANES; i++) {
            src[i] = arg_src[i];
            src_stride[i] = arg_src_stride[i];
        }
        break;
    }

    /* XXX: libswscale does not support AYUV formats yet */
    switch (dst_fourcc) {
    case IMAGE_AYUV:
        dst_pix_fmt = PIX_FMT_YUV444P;
        for (i = 0; i < 3; i++) {
            tmp_dst[i] = malloc(dst_width * dst_height);
            if (!tmp_dst[i])
                goto end;
            tmp_dst_stride[i] = dst_width;
            dst[i] = tmp_dst[i];
            dst_stride[i] = tmp_dst_stride[i];
        }
        break;
    case IMAGE_YV12:
        dst_pix_fmt = PIX_FMT_YUV420P;
        dst[0] = arg_dst[0];
        dst_stride[0] = arg_dst_stride[0];
        dst[1] = arg_dst[2];
        dst_stride[1] = arg_dst_stride[2];
        dst[2] = arg_dst[1];
        dst_stride[2] = arg_dst_stride[1];
        break;
    default:
        dst_pix_fmt = get_pixel_format(dst_fourcc);
        for (i = 0; i < MAX_IMAGE_PLANES; i++) {
            dst[i] = arg_dst[i];
            dst_stride[i] = arg_dst_stride[i];
        }
        break;
    }

    if (src_pix_fmt == PIX_FMT_NONE || dst_pix_fmt == PIX_FMT_NONE)
        goto end;

    sws = sws_getContext(src_width, src_height, src_pix_fmt,
                         dst_width, dst_height, dst_pix_fmt,
                         SWS_BICUBIC, NULL, NULL, NULL);
    if (!sws)
        goto end;

    sws_scale(sws, src, src_stride, 0, src_height, dst, dst_stride);
    sws_freeContext(sws);

    /* XXX: libswscale does not support AYUV formats yet */
    switch (dst_fourcc) {
    case IMAGE_AYUV:
        stride = arg_dst_stride[0];
        for (j = 0; j < dst_height; j++) {
            for (i = 0; i < dst_width; i++) {
                const int src_offset = j * dst_width + i;
                const int dst_offset = j * arg_dst_stride[0] + 4 * i;
#ifdef WORDS_BIGENDIAN
                arg_dst[0][dst_offset + 0] = 0xff;
                arg_dst[0][dst_offset + 1] = tmp_dst[0][src_offset];
                arg_dst[0][dst_offset + 2] = tmp_dst[1][src_offset];
                arg_dst[0][dst_offset + 3] = tmp_dst[2][src_offset];
#else
                arg_dst[0][dst_offset + 0] = tmp_dst[2][src_offset];
                arg_dst[0][dst_offset + 1] = tmp_dst[1][src_offset];
                arg_dst[0][dst_offset + 2] = tmp_dst[0][src_offset];
                arg_dst[0][dst_offset + 3] = 0xff;
#endif
            }
        }
        break;
    }

    error = 0;
end:
    for (i = 0; i < MAX_IMAGE_PLANES; i++) {
        free(tmp_src[i]);
        free(tmp_dst[i]);
    }
    return error;
}
#endif

static int image_copy_NV12(
    uint8_t     *src[MAX_IMAGE_PLANES],
    int          src_stride[MAX_IMAGE_PLANES],
    uint8_t     *dst[MAX_IMAGE_PLANES],
    int          dst_stride[MAX_IMAGE_PLANES],
    unsigned int width,
    unsigned int height
)
{
    uint8_t *s, *d;
    unsigned int y;

    // Copy Y
    s = src[0];
    d = dst[0];
    for (y = 0; y < height; y++, s += src_stride[0], d += dst_stride[0])
        memcpy(d, s, width);

    // Copy UV
    s = src[1];
    d = dst[1];
    for (y = 0; y < height/2; y++, s += src_stride[1], d += dst_stride[1])
        memcpy(d, s, width);
    return 0;
}

static int image_copy_RGB32(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height
)
{
    const unsigned int bytes_per_line = 4 * width;
    unsigned int y;

    for (y = 0; y < height; y++, src += src_stride, dst += dst_stride)
        memcpy(dst, src, bytes_per_line);
    return 0;
}

static inline int image_convert_RGB32(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height,
    unsigned int ridx,
    unsigned int gidx,
    unsigned int bidx,
    unsigned int aidx
)
{
    unsigned int x, y;

    for (y = 0; y < height; y++, src += src_stride, dst += dst_stride)
        for (x = 0; x < width; x++) {
#ifdef WORDS_BIGENDIAN
            /* ARGB */
            dst[x*4 + ridx] = src[x*4 + 1];
            dst[x*4 + gidx] = src[x*4 + 2];
            dst[x*4 + bidx] = src[x*4 + 3];
            dst[x*4 + aidx] = src[x*4 + 0];
#else
            /* BGRA */
            dst[x*4 + ridx] = src[x*4 + 2];
            dst[x*4 + gidx] = src[x*4 + 1];
            dst[x*4 + bidx] = src[x*4 + 0];
            dst[x*4 + aidx] = src[x*4 + 3];
#endif
        }
    return 0;
}

static int image_convert_RGB32_to_ARGB(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height
)
{
    return image_convert_RGB32(
        src, src_stride,
        dst, dst_stride,
        width, height,
        1, 2, 3, 0
    );
}

static int image_convert_RGB32_to_BGRA(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height
)
{
    return image_convert_RGB32(
        src, src_stride,
        dst, dst_stride,
        width, height,
        2, 1, 0, 3
    );
}

static int image_convert_RGB32_to_RGBA(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height
)
{
    return image_convert_RGB32(
        src, src_stride,
        dst, dst_stride,
        width, height,
        0, 1, 2, 3
    );
}

static int image_convert_RGB32_to_ABGR(
    uint8_t     *src,
    unsigned int src_stride,
    uint8_t     *dst,
    unsigned int dst_stride,
    unsigned int width,
    unsigned int height
)
{
    return image_convert_RGB32(
        src, src_stride,
        dst, dst_stride,
        width, height,
        3, 2, 1, 0
    );
}

static int image_convert_1(
    uint8_t     *arg_src[MAX_IMAGE_PLANES],
    int          arg_src_stride[MAX_IMAGE_PLANES],
    unsigned int src_width,
    unsigned int src_height,
    uint32_t     src_fourcc,
    uint8_t     *arg_dst[MAX_IMAGE_PLANES],
    int          arg_dst_stride[MAX_IMAGE_PLANES],
    unsigned int dst_width,
    unsigned int dst_height,
    uint32_t     dst_fourcc
)
{
    if (src_fourcc == dst_fourcc &&
        src_width  == dst_width  &&
        src_height == dst_height) {
        switch (src_fourcc) {
        case IMAGE_NV12:
            return image_copy_NV12(
                arg_src, arg_src_stride,
                arg_dst, arg_dst_stride,
                src_width, src_height
            );
        case IMAGE_ARGB:
        case IMAGE_BGRA:
        case IMAGE_RGBA:
        case IMAGE_ABGR:
            return image_copy_RGB32(
                arg_src[0], arg_src_stride[0],
                arg_dst[0], arg_dst_stride[0],
                src_width, src_height
            );
        }
    }

    /* libswscale < 0.10.0 has bugs in ARGB -> RGBA conversion */
    if (src_fourcc == IMAGE_RGB32 &&
        src_width  == dst_width   &&
        src_height == dst_height) {
        switch (dst_fourcc) {
        case IMAGE_ARGB:
            return image_convert_RGB32_to_ARGB(
                arg_src[0], arg_src_stride[0],
                arg_dst[0], arg_dst_stride[0],
                src_width, src_height
            );
        case IMAGE_BGRA:
            return image_convert_RGB32_to_BGRA(
                arg_src[0], arg_src_stride[0],
                arg_dst[0], arg_dst_stride[0],
                src_width, src_height
            );
        case IMAGE_RGBA:
            return image_convert_RGB32_to_RGBA(
                arg_src[0], arg_src_stride[0],
                arg_dst[0], arg_dst_stride[0],
                src_width, src_height
            );
        case IMAGE_ABGR:
            return image_convert_RGB32_to_ABGR(
                arg_src[0], arg_src_stride[0],
                arg_dst[0], arg_dst_stride[0],
                src_width, src_height
            );
        }
    }

#if HAVE_SWSCALE
    return image_convert_libswscale(
        arg_src, arg_src_stride,
        src_width, src_height, src_fourcc,
        arg_dst, arg_dst_stride,
        dst_width, dst_height, dst_fourcc
    );
#endif
    return -1;
}

static inline int
image_get_parts(
    Image   *img,
    uint8_t *pixels[MAX_IMAGE_PLANES],
    int      stride[MAX_IMAGE_PLANES]
)
{
    unsigned int i;

    if (img->data && img->data_size) {
        /* Genuine image created with image_create() */
        for (i = 0; i < img->num_planes; i++) {
            pixels[i] = img->data + img->offsets[i];
            stride[i] = img->pitches[i];
        }
    }
    else {
        /* Image is probably "bound" from another structure */
        for (i = 0; i < img->num_planes; i++) {
            pixels[i] = img->pixels[i];
            stride[i] = img->pitches[i];
        }
    }

    for (; i < MAX_IMAGE_PLANES; i++) {
        pixels[i] = NULL;
        stride[i] = 0;
    }
    return 0;
}

int image_convert(Image *dst_img, Image *src_img)
{
    uint8_t *src[MAX_IMAGE_PLANES];
    int src_stride[MAX_IMAGE_PLANES];
    uint8_t *dst[MAX_IMAGE_PLANES];
    int dst_stride[MAX_IMAGE_PLANES];

    D(bug("convert %s:%ux%u to %s:%ux%u\n",
          string_of_FOURCC(src_img->format), src_img->width, src_img->height,
          string_of_FOURCC(dst_img->format), dst_img->width, dst_img->height));

    if (image_get_parts(src_img, src, src_stride) < 0)
        return -1;

    if (image_get_parts(dst_img, dst, dst_stride) < 0)
        return -1;

    return image_convert_1(src,
                           src_stride,
                           src_img->width,
                           src_img->height,
                           src_img->format,
                           dst,
                           dst_stride,
                           dst_img->width,
                           dst_img->height,
                           dst_img->format);
}

int image_write(Image *img, FILE *fp)
{
    unsigned int x, y, src_stride;
    const uint32_t *src;
    uint8_t rgb_pixel[3];

    D(bug("write %s:%ux%u\n",
          string_of_FOURCC(img->format), img->width, img->height));

    if (img->format != IMAGE_RGB32)
        return -1;

    src = (uint32_t *)img->pixels[0];
    src_stride = img->pitches[0] / 4;

    /* PPM format */
    fprintf(fp, "P6\n%u %u\n255\n", img->width, img->height);
    for (y = 0; y < img->height; y++) {
        for (x = 0; x < img->width; x++) {
            const uint32_t color = src[x];
            rgb_pixel[0] = color >> 16;
            rgb_pixel[1] = color >> 8;
            rgb_pixel[2] = color & 0xff;
            if (fwrite(rgb_pixel, sizeof(rgb_pixel), 1, fp) != 1)
                return -1;
        }
        src += src_stride;
    }
    return 0;
}
