/*
 *  crystalhd_video.c - Crystal HD video-specific code
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
#include "crystalhd.h"
#include "buffer.h"
#include "common.h"

#if USE_MPEG2
#include "mpeg2.h"
#define CODEC                   BC_VID_ALGO_MPEG2
#define PictureInfo             MPEG2PictureInfo
#define codec_get_picture_info  mpeg2_get_picture_info

static int
codec_get_video_data(uint8_t **buf, unsigned int *buf_size, int *alloc)
{
    const uint8_t *video_data;
    unsigned int video_data_size;

    mpeg2_get_video_data(&video_data, &video_data_size);

    *alloc    = 0;
    *buf      = (uint8_t *)video_data;
    *buf_size = video_data_size;
    return 0;
}
#endif

#if USE_VC1
#include "vc1.h"
#define CODEC                   BC_VID_ALGO_VC1
#define PictureInfo             VC1PictureInfo
#define codec_get_picture_info  vc1_get_picture_info

static int
codec_get_video_data(uint8_t **buf, unsigned int *buf_size, int *alloc)
{
    Buffer *buffer = NULL;
    const uint8_t *video_data;
    unsigned int video_data_size;
    int ret = -1;

    /* End-of-Sequence start code */
    static const uint8_t eos_start_code[4] = { 0x00, 0x00, 0x01, 0x0a };

    vc1_get_video_data(&video_data, &video_data_size);

    if (common_get_context()->crystalhd_flush) {
        *alloc    = 0;
        *buf      = (uint8_t *)video_data;
        *buf_size = video_data_size;
    }

    buffer = buffer_create(video_data_size + 4);
    if (!buffer)
        goto end;
    if (buffer_append(buffer, video_data, video_data_size) < 0)
        goto end;
    if (buffer_append(buffer, eos_start_code, sizeof(eos_start_code)) < 0)
        goto end;

    *alloc = 1;
    buffer_steal(buffer, buf, buf_size);
    ret = 0;

end:
    if (buffer)
        buffer_destroy(buffer);
    return ret;
}
#endif

#if USE_H264
#include "h264.h"
#define CODEC                   BC_VID_ALGO_H264
#define PictureInfo             H264PictureInfo
#define codec_get_picture_info  h264_get_picture_info

static inline uint32_t get16(const uint8_t *ptr)
{
    uint32_t v;
    v  = ((uint32_t)*ptr++) << 8;
    v |= *ptr;
    return v;
}

static inline uint32_t get16p(const uint8_t **ptr)
{
    uint32_t v = get16(*ptr);
    *ptr += 2;
    return v;
}

static inline uint32_t get32(const uint8_t *ptr)
{
    uint32_t v;
    v  = ((uint32_t)*ptr++) << 24;
    v |= ((uint32_t)*ptr++) << 16;
    v |= ((uint32_t)*ptr++) <<  8;
    v |= *ptr;
    return v;
}

static inline uint32_t get32p(const uint8_t **ptr)
{
    uint32_t v = get32(*ptr);
    *ptr += 4;
    return v;
}

#define MP4_FOURCC(ch1, ch2, ch3, ch4) \
    ((((uint32_t)ch1) << 24) | \
     (((uint32_t)ch2) << 16) | \
     (((uint32_t)ch3) <<  8) | \
      ((uint32_t)ch4))

static int get_atom(const uint8_t **pptr, unsigned int size, uint32_t atom)
{
    const uint8_t *ptr = *pptr;
    const uint8_t *ptr_end = ptr + size;
    int i, found = 0;

    static const struct {
        uint32_t atom;
        char terminator;
    }
    atoms[] = {
        { MP4_FOURCC('m','o','o','v'), 0 },
        { MP4_FOURCC('m','v','h','d'), 1 },
        { MP4_FOURCC('i','o','d','s'), 1 },
        { MP4_FOURCC('t','r','a','k'), 0 },
        { MP4_FOURCC('t','k','h','d'), 1 },
        { MP4_FOURCC('t','r','e','f'), 1 },
        { MP4_FOURCC('e','d','t','s'), 0 },
        { MP4_FOURCC('e','l','s','t'), 1 },
        { MP4_FOURCC('m','d','i','a'), 0 },
        { MP4_FOURCC('m','d','h','d'), 1 },
        { MP4_FOURCC('h','d','l','r'), 1 },
        { MP4_FOURCC('m','i','n','f'), 0 },
        { MP4_FOURCC('v','m','h','d'), 1 },
        { MP4_FOURCC('s','m','h','d'), 1 },
        { MP4_FOURCC('h','m','h','d'), 1 },
        { MP4_FOURCC('d','i','n','f'), 0 },
        { MP4_FOURCC('d','r','e','f'), 1 },
        { MP4_FOURCC('s','t','b','l'), 0 },
        { MP4_FOURCC('s','t','t','s'), 1 },
        { MP4_FOURCC('c','t','t','s'), 1 },
        { MP4_FOURCC('s','t','s','s'), 1 },
        { MP4_FOURCC('s','t','s','d'), 1 },
        { MP4_FOURCC('s','t','s','z'), 1 },
        { MP4_FOURCC('s','t','s','c'), 1 },
        { MP4_FOURCC('s','t','c','o'), 1 },
        { MP4_FOURCC('c','o','6','4'), 1 },
        { MP4_FOURCC('s','t','s','h'), 1 },
        { MP4_FOURCC('s','t','d','p'), 1 },
        { MP4_FOURCC('m','d','a','t'), 1 },
        { MP4_FOURCC('f','r','e','e'), 1 },
        { MP4_FOURCC('s','k','i','p'), 1 },
        { MP4_FOURCC('u','d','t','a'), 1 },
        { 0, -1 }
    };

    do {
        size  = get32(ptr);
        found = get32(ptr+4) == atom;
        if (!found) {
            for (i = 0; atoms[i].atom; i++) {
                if (get32(ptr+4) == atoms[i].atom)
                    break;
            }
            if (!atoms[i].atom)
                break;
            if (atoms[i].terminator)
                ptr += size;
            else
                ptr += 8;
            if (ptr >= ptr_end)
                break;
        }
    } while (!found);

    *pptr = ptr;
    if (!found)
        return -1;
    return 0;
}

static int
codec_get_video_data(uint8_t **buf, unsigned int *buf_size, int *alloc)
{
    Buffer *buffer = NULL;
    const uint8_t *video_data;
    unsigned int video_data_size;
    const uint8_t *slice_data;
    unsigned int slice_data_size;
    const uint8_t *ptr;
    unsigned int size;
    uint8_t nal_unit_type;
    int i, is_h264, ret = -1;

    static const uint8_t start_code[3] = { 0x00, 0x00, 0x01 };

    h264_get_video_data(&video_data, &video_data_size);
    ptr = video_data;

    buffer = buffer_create(video_data_size);
    if (!buffer)
        goto end;

    /* Parse mp4 header */
    size = get32p(&ptr);
    if (get32p(&ptr) != MP4_FOURCC('f','t','y','p'))
        goto end;
    if (get32p(&ptr) != MP4_FOURCC('i','s','o','m'))
        goto end;
    get32p(&ptr);                               // minor_version

    is_h264 = 0;
    for (i = 0; i < size/4 - 4; i++) {          // compatible_brands
        if (get32p(&ptr) == MP4_FOURCC('a','v','c','1'))
            is_h264 = 1;
    }
    if (!is_h264)
        goto end;

    /* Find SPS/PPS data */
    if (get_atom(&ptr, video_data_size - (ptr - video_data), MP4_FOURCC('s','t','s','d')) < 0)
        goto end;
    ptr += 12;                                  // FullBox('stsd', 0, 0)
    for (i = get32p(&ptr); i > 0; i--) {
        size = get32(ptr);
        if (get32(ptr+4) == MP4_FOURCC('a','v','c','1'))
            break;
        ptr += size;
    }
    if (i == 0)
        goto end;
    ptr += 8 + 6 + 2 + 8*4 + 2 + 32 + 2 + 2;    // VisualSampleEntry()
    size = get32(ptr);
    if (get32(ptr+4) != MP4_FOURCC('a','v','c','C'))
        goto end;
    ptr += 13;

    i = *ptr++ & 0x1f;                          // number of SPS entries
    while (i-- > 0) {
        size = get16p(&ptr);
        if (buffer_append(buffer, start_code, sizeof(start_code)) < 0)
            goto end;
        if (buffer_append(buffer, ptr, size) < 0)
            goto end;
        ptr += size;
    }

    i = *ptr++;                                 // number of PPS entries
    while (i-- > 0) {
        size = get16p(&ptr);
        if (buffer_append(buffer, start_code, sizeof(start_code)) < 0)
            goto end;
        if (buffer_append(buffer, ptr, size) < 0)
            goto end;
        ptr += size;
    }

    /* Append slice data */
    h264_get_slice_data(&slice_data, &slice_data_size);
    if (buffer_append(buffer, start_code, sizeof(start_code)) < 0)
        goto end;
    if (buffer_append(buffer, slice_data, slice_data_size) < 0)
        goto end;

    /* Append End-of-Sequence */
    if (!common_get_context()->crystalhd_flush) {
        nal_unit_type = 0x0a;
        if (buffer_append(buffer, start_code, sizeof(start_code)) < 0)
            goto end;
        if (buffer_append(buffer, &nal_unit_type, 1) < 0)
            goto end;
    }

    /* Append Filler-data */
    else {
        static const uint8_t filler_data[] = { 0xff, 0x80, 0x00 };
        nal_unit_type = 0x0c;
        if (buffer_append(buffer, start_code, sizeof(start_code)) < 0)
            goto end;
        if (buffer_append(buffer, &nal_unit_type, 1) < 0)
            goto end;
        if (buffer_append(buffer, filler_data, sizeof(filler_data)) < 0)
            goto end;
    }

    *alloc = 1;
    buffer_steal(buffer, buf, buf_size);
    ret = 0;

end:
    if (buffer)
        buffer_destroy(buffer);
    return ret;
}
#endif

int decode(void)
{
    PictureInfo pic_info;
    int video_data_alloc = 0;
    uint8_t *video_data;
    unsigned int video_data_size;
    int ret;

    codec_get_picture_info(&pic_info);
    if (crystalhd_init_decoder(CODEC, pic_info.width, pic_info.height) < 0)
        return -1;

    if (codec_get_video_data(&video_data, &video_data_size, &video_data_alloc) < 0)
        return -1;

    ret = crystalhd_decode(video_data, video_data_size);
    if (video_data_alloc)
        free(video_data);
    return ret;
}
