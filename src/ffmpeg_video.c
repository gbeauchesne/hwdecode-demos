/*
 *  ffmpeg_video.c - FFmpeg video-specific code
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

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
# include <libavformat/avformat.h>
#endif
#ifdef HAVE_FFMPEG_AVFORMAT_H
# include <ffmpeg/avformat.h>
#endif
#if LIBAVFORMAT_VERSION_INT < ((52<<16)+(3<<8)+0) /* 52.3.0 */
# define av_close_input_stream av_close_input_file
#endif
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(64<<8)+0) /* 52.64.0 */
# define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
#endif

#if USE_H264
#include "h264.h"
#define codec_get_video_data h264_get_video_data
#endif

#if USE_VC1
#include "vc1.h"
#define FORCE_VIDEO_FORMAT  "vc1"
#define codec_get_video_data vc1_get_video_data
#endif

#if USE_MPEG2
#include "mpeg2.h"
#define codec_get_video_data mpeg2_get_video_data
#endif

#if USE_MPEG4
#include "mpeg4.h"
#define codec_get_video_data mpeg4_get_video_data
#endif

#ifndef FORCE_VIDEO_FORMAT
#define FORCE_VIDEO_FORMAT NULL
#endif

int decode(void)
{
    AVProbeData pd;
    ByteIOContext ioctx;
    AVInputFormat *format = NULL;
    AVFormatContext *ic = NULL;
    AVCodec *codec;
    AVCodecContext *avctx = NULL;
    AVPacket packet;
    AVStream *video_stream;
    int i, got_picture, error = -1;

    const uint8_t *video_data;
    unsigned int video_data_size;

    av_register_all();
    av_init_packet(&packet);
    codec_get_video_data(&video_data, &video_data_size);

    pd.filename = "";
    pd.buf      = (uint8_t *)video_data;
    pd.buf_size = MIN(video_data_size, 32*1024);
    if (FORCE_VIDEO_FORMAT)
        format = av_find_input_format(FORCE_VIDEO_FORMAT);
    if (!format && (format = av_probe_input_format(&pd, 1)) == NULL)
        goto end;

    if (init_put_byte(&ioctx, (uint8_t *)video_data, video_data_size, 0, NULL, NULL, NULL, NULL) < 0)
        goto end;

    if (av_open_input_stream(&ic, &ioctx, "", format, NULL) < 0)
        goto end;

    if (av_find_stream_info(ic) < 0)
        goto end;
    dump_format(ic, 0, "", 0);

    video_stream = NULL;
    for (i = 0; i < ic->nb_streams; i++) {
        if (ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && !video_stream)
            video_stream = ic->streams[i];
        else
            ic->streams[i]->discard = AVDISCARD_ALL;
    }
    if (!video_stream)
        goto end;
    avctx = video_stream->codec;
    if (ffmpeg_init_context(avctx) < 0)
        goto end;

    if ((codec = avcodec_find_decoder(avctx->codec_id)) == NULL)
        goto end;
    if (avcodec_open(avctx, codec) < 0)
        goto end;

    got_picture = 0;
    while (av_read_frame(ic, &packet) == 0) {
        if (packet.stream_index != video_stream->index)
            continue;
        if ((got_picture = ffmpeg_decode(avctx, packet.data, packet.size)) < 0)
            goto end;
        /* read only one frame */
        if (got_picture) {
            error = 0;
            break;
        }
    }
    if (!got_picture) {
        if ((got_picture = ffmpeg_decode(avctx, NULL, 0)) < 0)
            goto end;
        error = 0;
    }

end:
    av_free_packet(&packet);
    if (avctx)
        avcodec_close(avctx);
    if (ic)
        av_close_input_stream(ic);
    return error;
}
