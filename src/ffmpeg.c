/*
 *  ffmpeg.c - FFmpeg common code
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
#include "ffmpeg.h"
#include "common.h"
#include "utils.h"

#if USE_X11
# include "x11.h"
#endif

#ifdef USE_VAAPI
# include "vaapi.h"
# ifdef HAVE_LIBAVCODEC_VAAPI_H
#  include <libavcodec/vaapi.h>
#  define USE_FFMPEG_VAAPI 1
# endif
#endif

#define DEBUG 1
#include "debug.h"

static FFmpegContext *ffmpeg_context;
struct vaapi_context *vaapi_context;

static int ffmpeg_init(void)
{
    FFmpegContext *ffmpeg;

    if (ffmpeg_context)
        return 0;
    
    if ((ffmpeg = calloc(1, sizeof(*ffmpeg))) == NULL)
        return -1;

    if ((ffmpeg->frame = avcodec_alloc_frame()) == NULL) {
        free(ffmpeg);
        return -1;
    }

    avcodec_init();
    avcodec_register_all();

    ffmpeg_context = ffmpeg;
    return 0;
}

static int ffmpeg_exit(void)
{
    FFmpegContext * const ffmpeg = ffmpeg_get_context();

    if (!ffmpeg)
        return 0;

    av_freep(&ffmpeg->frame);

    free(ffmpeg_context);
    ffmpeg_context = NULL;
    return 0;
}

#ifdef USE_FFMPEG_VAAPI
static int ffmpeg_vaapi_init(void)
{
    VADisplay dpy;

    switch (display_type()) {
#if USE_X11
    case DISPLAY_X11:
        dpy = vaGetDisplay(x11_get_context()->display);
        break;
#endif
    default:
        fprintf(stderr, "ERROR: unsupported display type\n");
        return -1;
    }
    if (vaapi_init(dpy) < 0)
        return -1;
    if ((vaapi_context = calloc(1, sizeof(*vaapi_context))) == NULL)
        return -1;
    vaapi_context->display = vaapi_get_context()->display;
    return 0;
}

static int ffmpeg_vaapi_exit(void)
{
    if (vaapi_exit() < 0)
        return -1;
    free(vaapi_context);
    return 0;
}
#endif

FFmpegContext *ffmpeg_get_context(void)
{
    return ffmpeg_context;
}

#ifdef USE_FFMPEG_VAAPI
static enum PixelFormat get_format(struct AVCodecContext *avctx,
                                   const enum PixelFormat *fmt)
{
    int i, profile;

    for (i = 0; fmt[i] != PIX_FMT_NONE; i++) {
        if (fmt[i] != PIX_FMT_VAAPI_VLD)
            continue;
        switch (avctx->codec_id) {
        case CODEC_ID_MPEG2VIDEO:
            profile = VAProfileMPEG2Main;
            break;
        case CODEC_ID_MPEG4:
        case CODEC_ID_H263:
            profile = VAProfileMPEG4AdvancedSimple;
            break;
        case CODEC_ID_H264:
            profile = VAProfileH264High;
            break;
        case CODEC_ID_WMV3:
            profile = VAProfileVC1Main;
            break;
        case CODEC_ID_VC1:
            profile = VAProfileVC1Advanced;
            break;
        default:
            profile = -1;
            break;
        }
        if (profile >= 0) {
            if (vaapi_init_decoder(profile, VAEntrypointVLD, avctx->width, avctx->height) == 0) {
                VAAPIContext * const vaapi = vaapi_get_context();
                vaapi_context->config_id   = vaapi->config_id;
                vaapi_context->context_id  = vaapi->context_id;
                avctx->hwaccel_context     = vaapi_context;
                return fmt[i];
            }
        }
    }
    return PIX_FMT_NONE;
}

static int get_buffer(struct AVCodecContext *avctx, AVFrame *pic)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    void *surface = (void *)(uintptr_t)vaapi->surface_id;

    pic->type           = FF_BUFFER_TYPE_USER;
    pic->age            = 1;
    pic->data[0]        = surface;
    pic->data[1]        = NULL;
    pic->data[2]        = NULL;
    pic->data[3]        = surface;
    pic->linesize[0]    = 0;
    pic->linesize[1]    = 0;
    pic->linesize[2]    = 0;
    pic->linesize[3]    = 0;
    return 0;
}

static void release_buffer(struct AVCodecContext *avctx, AVFrame *pic)
{
    pic->data[0]        = NULL;
    pic->data[1]        = NULL;
    pic->data[2]        = NULL;
    pic->data[3]        = NULL;
}
#endif

int ffmpeg_init_context(AVCodecContext *avctx)
{
    switch (hwaccel_type()) {
#ifdef USE_FFMPEG_VAAPI
    case HWACCEL_VAAPI:
        avctx->thread_count    = 1;
        avctx->get_format      = get_format;
        avctx->get_buffer      = get_buffer;
        avctx->reget_buffer    = get_buffer;
        avctx->release_buffer  = release_buffer;
        avctx->draw_horiz_band = NULL;
        avctx->slice_flags     = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
        break;
#endif
    default:
        break;
    }
    return 0;
}

int ffmpeg_decode(AVCodecContext *avctx, const uint8_t *buf, unsigned int buf_size)
{
    CommonContext * const common = common_get_context();
    FFmpegContext * const ffmpeg = ffmpeg_get_context();
    int got_picture;
    AVPacket pkt;

    av_init_packet(&pkt);
    pkt.data = buf;
    pkt.size = buf_size;

    got_picture = 0;
    if (avcodec_decode_video2(avctx, ffmpeg->frame, &got_picture, &pkt) < 0)
        return -1;

    if (got_picture && hwaccel_type() == HWACCEL_NONE) {
        if (avctx->width == 0 || avctx->height == 0)
            return -1;

        Image image;
        unsigned int i;
        memset(&image, 0, sizeof(image));

        switch (avctx->pix_fmt) {
        case PIX_FMT_YUV420P:
            image.format = IMAGE_I420;
            image.num_planes = 3;
            break;
        default:
            image.format = 0;
            break;
        }
        if (!image.format)
            return -1;

        image.width  = avctx->width;
        image.height = avctx->height;
        for (i = 0; i < image.num_planes; i++) {
            image.pixels[i]  = ffmpeg->frame->data[i];
            image.pitches[i] = ffmpeg->frame->linesize[i];
        }
        if (image_convert(common->image, &image) < 0)
            return -1;
    }
    return got_picture;
}

static int ffmpeg_display(void)
{
    switch (display_type()) {
#if USE_X11
    case DISPLAY_X11:
        if (x11_display() < 0)
            return -1;
        break;
#endif
    default:
        /* no display server needed */
        break;
    }
    return 0;
}

int pre(void)
{
    /* XXX: we need the XImage */
    if (hwaccel_type() == HWACCEL_NONE)
        common_get_context()->getimage_mode = GETIMAGE_FROM_VIDEO;

    switch (display_type()) {
#if USE_X11
    case DISPLAY_X11:
        if (x11_init() < 0)
            return -1;
        break;
#endif
    default:
        /* no display server needed */
        break;
    }

    if (ffmpeg_init() < 0)
        return -1;

    switch (hwaccel_type()) {
    case HWACCEL_NONE:
        return 0;
#ifdef USE_FFMPEG_VAAPI
    case HWACCEL_VAAPI:
        return ffmpeg_vaapi_init();
#endif
    default:
        return -1;
    }
    return 0;
}

int post(void)
{
    switch (hwaccel_type()) {
#ifdef USE_FFMPEG_VAAPI
    case HWACCEL_VAAPI:
        if (ffmpeg_vaapi_exit() < 0)
            return -1;
        break;
#endif
    default:
        break;
    }

    if (ffmpeg_exit() < 0)
        return -1;

    switch (display_type()) {
#if USE_X11
    case DISPLAY_X11:
        if (x11_exit() < 0)
            return -1;
        break;
#endif
    default:
        /* no display server needed */
        break;
    }
    return 0;
}

int display(void)
{
    switch (hwaccel_type()) {
#ifdef USE_FFMPEG_VAAPI
    case HWACCEL_VAAPI:
        if (vaapi_display() < 0)
            return -1;
        break;
#endif
    default:
        break;
    }
    return ffmpeg_display();
}
