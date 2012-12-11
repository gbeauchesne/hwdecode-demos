/*
 *  common.h - Common code
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

#ifndef HWDECODE_DEMOS_COMMON_H
#define HWDECODE_DEMOS_COMMON_H

#include "image.h"

enum HWAccelType {
    HWACCEL_NONE,
    HWACCEL_VAAPI,
    HWACCEL_VDPAU,
    HWACCEL_XVBA
};

enum DisplayType {
    DISPLAY_X11 = 1,
    DISPLAY_GLX,
    DISPLAY_DRM
};

enum RotationMode {
    ROTATION_NONE = 0,
    ROTATION_90,
    ROTATION_180,
    ROTATION_270
};

enum GenImageType {
    GENIMAGE_AUTO = 0,  /* automatic selection   */
    GENIMAGE_RECTS,     /* random rectangles     */
    GENIMAGE_RGB_RECTS, /* R/G/B rectangles      */
    GENIMAGE_FLOWERS    /* flowers (needs Cairo) */
};

enum GetImageMode {
    GETIMAGE_NONE = 0,
    GETIMAGE_FROM_VIDEO,
    GETIMAGE_FROM_OUTPUT,
    GETIMAGE_FROM_PIXMAP
};

enum PutImageMode {
    PUTIMAGE_NONE = 0,
    PUTIMAGE_OVERRIDE,
    PUTIMAGE_BLEND
};

enum TextureTarget {
    TEXTURE_TARGET_2D = 1,
    TEXTURE_TARGET_RECT
};

typedef struct _Size Size;

struct _Size {
    unsigned int        width;
    unsigned int        height;
};

typedef struct _Rectangle Rectangle;

struct _Rectangle {
    int                 x;
    int                 y;
    unsigned int        width;
    unsigned int        height;
};

typedef struct _CommonContext CommonContext;

struct _CommonContext {
    enum HWAccelType    hwaccel_type;
    enum DisplayType    display_type;
    Size                window_size;
    unsigned int        use_subwindow_rect;
    Rectangle           subwindow_rect;
    enum RotationMode   rotation;

    FILE               *output_file;
    char               *output_filename;

    Image              *image;
    enum GenImageType   genimage_type;
    enum GetImageMode   getimage_mode;
    uint32_t            getimage_format;
    unsigned int        use_getimage_rect;
    Rectangle           getimage_rect;
    enum PutImageMode   putimage_mode;
    uint32_t            putimage_format;
    Size                putimage_size;

    Rectangle          *cliprects;
    unsigned int        cliprects_size;
    unsigned int        cliprects_count;
    Image              *cliprects_image;

    unsigned int        use_clipping;
    unsigned int        multi_rendering;
    unsigned int        vaapi_derive_image;
    unsigned int        use_vaapi_putsurface_source_rect;
    Rectangle           vaapi_putsurface_source_rect;
    unsigned int        use_vaapi_putsurface_target_rect;
    Rectangle           vaapi_putsurface_target_rect;
    unsigned int        use_vaapi_putsurface_flags;
    unsigned int        vaapi_putsurface_flags;
    unsigned int        use_vaapi_subpicture_source_rect;
    Rectangle           vaapi_subpicture_source_rect;
    unsigned int        use_vaapi_subpicture_target_rect;
    Rectangle           vaapi_subpicture_target_rect;
    unsigned int        use_vaapi_subpicture_flags;
    unsigned int        vaapi_subpicture_flags;
    float               vaapi_subpicture_alpha;
    unsigned int        vaapi_putimage_scaled;
    unsigned int        use_vaapi_background_color;
    unsigned int        vaapi_background_color;
    unsigned int        vaapi_multi_subpictures;
    unsigned int        vaapi_glx_use_copy;
    unsigned int        vdpau_layers;
    unsigned int        vdpau_hqscaling;
    unsigned int        vdpau_glx_video_surface;
    unsigned int        vdpau_glx_output_surface;
    enum TextureTarget  glx_texture_target;
    unsigned int        glx_texture_format;
    Size                glx_texture_size;
    unsigned int        use_glx_texture_size;
    unsigned int        glx_use_fbo;
    unsigned int        glx_use_reflection;
    unsigned int        crystalhd_output_nocopy;
    unsigned int        crystalhd_flush;
};

CommonContext *common_get_context(void);

int common_init_decoder(unsigned int picture_width, unsigned int picture_height);
int common_display(void);

int pre(void);
int post(void);
int decode(void);
int display(void);

enum HWAccelType hwaccel_type(void);
enum DisplayType display_type(void);
enum GetImageMode getimage_mode(void);
uint32_t getimage_format(void);
enum PutImageMode putimage_mode(void);
uint32_t putimage_format(void);

#endif /* HWDECODE_DEMOS_COMMON_H */
