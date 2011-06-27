/*
 *  vaapi.h - VA API common code
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

#ifndef VAAPI_H
#define VAAPI_H

#ifdef USE_OLD_VAAPI
#include <va_x11.h>
#else
#include <va/va.h>
#include <va/va_x11.h>
#endif

typedef struct _VAAPIContext VAAPIContext;

struct _VAAPIContext {
    VADisplay           display;
    VAConfigID          config_id;
    VAContextID         context_id;
    VASurfaceID         surface_id;
    VASubpictureID      subpic_ids[5];
    VAImage             subpic_image;
    VAProfile           profile;
    VAProfile          *profiles;
    int                 n_profiles;
    VAEntrypoint        entrypoint;
    VAEntrypoint       *entrypoints;
    int                 n_entrypoints;
    VAImageFormat      *image_formats;
    int                 n_image_formats;
    VAImageFormat      *subpic_formats;
    unsigned int       *subpic_flags;
    unsigned int        n_subpic_formats;
    unsigned int        picture_width;
    unsigned int        picture_height;
    VABufferID          pic_param_buf_id;
    VABufferID          iq_matrix_buf_id;
    VABufferID          bitplane_buf_id;
    VABufferID         *slice_buf_ids;
    unsigned int        n_slice_buf_ids;
    unsigned int        slice_buf_ids_alloc;
    void               *slice_params;
    unsigned int        slice_param_size;
    unsigned int        n_slice_params;
    unsigned int        slice_params_alloc;
    const uint8_t      *slice_data;
    unsigned int        slice_data_size;
    int                 use_glx_copy;
    void               *glx_surface;
};

int vaapi_init(VADisplay display);
int vaapi_exit(void);
int vaapi_display(void);

VAAPIContext *vaapi_get_context(void);

int vaapi_check_status(VAStatus status, const char *msg);

void *vaapi_alloc_picture(unsigned int size);
void *vaapi_alloc_iq_matrix(unsigned int size);
void *vaapi_alloc_bitplane(unsigned int size);
void *vaapi_alloc_slice(unsigned int size, const uint8_t *buf, unsigned int buf_size);

int vaapi_init_decoder(VAProfile        profile,
                       VAEntrypoint     entrypoint,
                       unsigned int     picture_width,
                       unsigned int     picture_height);

int vaapi_decode(void);

int vaapi_glx_create_surface(unsigned int target, unsigned int texture);
void vaapi_glx_destroy_surface(void);
int vaapi_glx_begin_render_surface(void);
int vaapi_glx_end_render_surface(void);

#endif /* VAAPI_H */
