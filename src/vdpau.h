/*
 *  vdpau.h - VDPAU common code
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

#ifndef VDPAU_H
#define VDPAU_H

#include <stdint.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

typedef struct _VDPAUContext VDPAUContext;
typedef struct _VDPAUSurface VDPAUSurface;

struct _VDPAUSurface {
    uint32_t                    vdp_surface;
    unsigned int                width;
    unsigned int                height;
};

struct _VDPAUContext {
    VdpDevice                   device;
    VdpGetProcAddress          *get_proc_address;
    union {
        VdpPictureInfoMPEG1Or2  mpeg2;
#if HAVE_VDPAU_MPEG4
        VdpPictureInfoMPEG4Part2 mpeg4;
#endif
        VdpPictureInfoH264      h264;
        VdpPictureInfoVC1       vc1;
    }                           picture_info;
    unsigned int                picture_width;
    unsigned int                picture_height;
    VdpDecoder                  decoder;
    VdpVideoMixer               video_mixer;
    VDPAUSurface                video_surface;
    VDPAUSurface                output_surface;
    VDPAUSurface                bitmap_surface;
    VDPAUSurface                subpic_surface;
    VdpPresentationQueue        flip_queue;
    VdpPresentationQueueTarget  flip_target;
    VdpBitstreamBuffer         *bitstream_buffers;
    unsigned int                bitstream_buffers_count;
    unsigned int                bitstream_buffers_count_max;
    unsigned int                use_vdpau_glx_interop;
    void                       *glx_surface;
};

VDPAUContext *vdpau_get_context(void);

int vdpau_check_status(VdpStatus status, const char *msg);

void *vdpau_alloc_picture(void);
int vdpau_append_slice_data(const uint8_t *buf, unsigned int buf_size);

int vdpau_init_decoder(VdpDecoderProfile profile,
                       unsigned int      picture_width,
                       unsigned int      picture_height);

int vdpau_decode(void);

int vdpau_glx_create_surface(unsigned int target, unsigned int texture);
void vdpau_glx_destroy_surface(void);
int vdpau_glx_begin_render_surface(void);
int vdpau_glx_end_render_surface(void);

#endif /* VDPAU_H */
