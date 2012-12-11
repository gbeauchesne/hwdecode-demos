/*
 *  common.c - Common code
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

#define _GNU_SOURCE 1 /* strtof_l() */
#include "sysdeps.h"
#include "common.h"
#include "utils.h"
#include <strings.h> /* strcasecmp() [POSIX.1-2001] */
#include <stddef.h>
#include <stdarg.h>
#include <locale.h>
#include <errno.h>

#ifdef USE_VAAPI
#include "vaapi.h"
#include "vaapi_compat.h"
#endif

#define DEBUG 1
#include "debug.h"

#ifndef USE_FFMPEG
#ifdef USE_VAAPI
#define HWACCEL_DEFAULT HWACCEL_VAAPI
#endif
#ifdef USE_VDPAU
#define HWACCEL_DEFAULT HWACCEL_VDPAU
#endif
#ifdef USE_XVBA
#define HWACCEL_DEFAULT HWACCEL_XVBA
#endif
#endif
#ifndef HWACCEL_DEFAULT
#define HWACCEL_DEFAULT HWACCEL_NONE
#endif

#if !defined(DISPLAY_DEFAULT) && defined(USE_X11)
# define DISPLAY_DEFAULT DISPLAY_X11
#endif
#if !defined(DISPLAY_DEFAULT) && defined(USE_DRM)
# define DISPLAY_DEFAULT DISPLAY_DRM
#endif
#if !defined(DISPLAY_DEFAULT)
# error "Undefined DISPLAY_DEFAULT"
#endif

static CommonContext g_common_context = {
    .hwaccel_type       = HWACCEL_DEFAULT,
    .display_type       = DISPLAY_DEFAULT,
    .window_size.width  = 640,
    .window_size.height = 480,
    .rotation           = ROTATION_NONE,
    .genimage_type      = GENIMAGE_AUTO,
    .getimage_mode      = GETIMAGE_NONE,
    .getimage_format    = 0,
    .putimage_mode      = PUTIMAGE_NONE,
    .putimage_format    = 0,
};

CommonContext *common_get_context(void)
{
    return &g_common_context;
}

static inline void ensure_bounds(Rectangle *r, unsigned int w, unsigned int h)
{
    if (r->x < 0)
        r->x = 0;
    if (r->y < 0)
        r->y = 0;
    if (r->width > w - r->x)
        r->width = w - r->x;
    if (r->height > h - r->y)
        r->height = h - r->y;
}

int common_init_decoder(unsigned int picture_width, unsigned int picture_height)
{
    CommonContext * const common = common_get_context();

    D(bug("Decoded surface size: %ux%u\n", picture_width, picture_height));

    if (common->putimage_size.width == 0 ||
        common->putimage_size.height == 0) {
        common->putimage_size.width  = picture_width;
        common->putimage_size.height = picture_height;
    }

#if USE_VAAPI
    /* vaPutSurface() source region */
    if (common->use_vaapi_putsurface_source_rect)
        ensure_bounds(&common->vaapi_putsurface_source_rect,
                      picture_width,
                      picture_height);

    /* vaAssociateSubpicture() source region */
    if (common->use_vaapi_subpicture_source_rect)
        ensure_bounds(&common->vaapi_subpicture_source_rect,
                      common->putimage_size.width,
                      common->putimage_size.height);

    /* vaAssociateSubpicture() target region */
    if (common->use_vaapi_subpicture_target_rect) {
        if (common->use_vaapi_subpicture_flags &&
            (common->vaapi_subpicture_flags & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD))
            ensure_bounds(&common->vaapi_subpicture_target_rect,
                          common->window_size.width,
                          common->window_size.height);
        else
            ensure_bounds(&common->vaapi_subpicture_target_rect,
                          picture_width,
                          picture_height);
    }
#endif
    return 0;
}

int common_display(void)
{
    printf("press any key to exit\n");
    getchar();
    return 0;
}

enum HWAccelType hwaccel_type(void)
{
    return common_get_context()->hwaccel_type;
}

enum DisplayType display_type(void)
{
    return common_get_context()->display_type;
}

enum GetImageMode getimage_mode(void)
{
    return common_get_context()->getimage_mode;
}

uint32_t getimage_format(void)
{
    return common_get_context()->getimage_format;
}

enum PutImageMode putimage_mode(void)
{
    return common_get_context()->putimage_mode;
}

uint32_t putimage_format(void)
{
    return common_get_context()->putimage_format;
}

typedef struct {
    unsigned int value;
    const char  *str;
} map_t;

static const map_t map_hwaccel_types[] = {
    { HWACCEL_NONE,             "none"          },
    { HWACCEL_VAAPI,            "vaapi"         },
    { HWACCEL_VDPAU,            "vdpau"         },
    { HWACCEL_XVBA,             "xvba"          },
    { 0, }
};

static const map_t map_display_types[] = {
#if USE_X11
    { DISPLAY_X11,              "x11"           },
#endif
#if USE_GLX
    { DISPLAY_GLX,              "glx"           },
#endif
#if USE_DRM
    { DISPLAY_DRM,              "drm"           },
#endif
    { 0, }
};

static const map_t map_rotation_modes[] = {
    { ROTATION_NONE,            "none"          },
    { ROTATION_90,              "90"            },
    { ROTATION_180,             "180"           },
    { ROTATION_270,             "270"           },
    { 0, }
};

static const map_t map_genimage_types[] = {
    { GENIMAGE_AUTO,            "auto"          },
    { GENIMAGE_RECTS,           "rects"         },
    { GENIMAGE_RGB_RECTS,       "rgb-rects"     },
    { GENIMAGE_FLOWERS,         "flowers"       },
    { 0, }
};

static const map_t map_getimage_modes[] = {
    { GETIMAGE_NONE,            "none"          },
    { GETIMAGE_FROM_VIDEO,      "video"         },
    { GETIMAGE_FROM_OUTPUT,     "output"        },
    { GETIMAGE_FROM_PIXMAP,     "pixmap"        },
    { 0, }
};

static const map_t map_putimage_modes[] = {
    { PUTIMAGE_NONE,            "none"          },
    { PUTIMAGE_OVERRIDE,        "override"      },
    { PUTIMAGE_BLEND,           "blend"         },
    { 0, }
};

static const map_t map_image_formats[] = {
    { IMAGE_NV12,               "nv12"          },
    { IMAGE_YV12,               "yv12"          },
    { IMAGE_IYUV,               "iyuv"          },
    { IMAGE_I420,               "i420"          },
    { IMAGE_AYUV,               "ayuv"          },
    { IMAGE_UYVY,               "uyvy"          },
    { IMAGE_YUY2,               "yuy2"          },
    { IMAGE_YUYV,               "yuyv"          },
    { IMAGE_RGB32,              "rgb32"         },
    { IMAGE_ARGB,               "argb"          },
    { IMAGE_BGRA,               "bgra"          },
    { IMAGE_RGBA,               "rgba"          },
    { IMAGE_ABGR,               "abgr"          },
    { 0, }
};

static const map_t map_texture_formats[] = {
    { IMAGE_RGBA,               "rgba"          },
    { IMAGE_BGRA,               "bgra"          },
    { 0, }
};

static const map_t map_texture_targets[] = {
    { TEXTURE_TARGET_2D,        "2d"            },
    { TEXTURE_TARGET_RECT,      "rect"          },
    { 0, }
};

static const map_t map_vaapi_putsurface_flags[] = {
#ifdef USE_VAAPI
    { VA_FRAME_PICTURE,                 "frame"                 },
    { VA_TOP_FIELD,                     "top-field"             },
    { VA_BOTTOM_FIELD,                  "bottom-field"          },
    { VA_CLEAR_DRAWABLE,                "clear-drawable"        },
#ifdef VA_SRC_BT601
    { VA_SRC_BT601,                     "bt.601"                },
#endif
#ifdef VA_SRC_BT709
    { VA_SRC_BT709,                     "bt.709"                },
#endif
#ifdef VA_SRC_SMPTE_240
    { VA_SRC_SMPTE_240,                 "smpte.240"             },
#endif
#ifdef VA_FILTER_SCALING_DEFAULT
    { VA_FILTER_SCALING_DEFAULT,        "scaling_default"       },
#endif
#ifdef VA_FILTER_SCALING_FAST
    { VA_FILTER_SCALING_FAST,           "scaling_fast"          },
#endif
#ifdef VA_FILTER_SCALING_HQ
    { VA_FILTER_SCALING_HQ,             "scaling_hq"            },
#endif
#ifdef VA_FILTER_SCALING_NL_ANAMORPHIC
    { VA_FILTER_SCALING_NL_ANAMORPHIC,  "scaling_nla"           },
#endif
#endif
    { 0, }
};

static const map_t map_vaapi_subpicture_flags[] = {
#ifdef USE_VAAPI
    { VA_SUBPICTURE_CHROMA_KEYING,                  "chroma-key"        },
    { VA_SUBPICTURE_GLOBAL_ALPHA,                   "global-alpha"      },
#ifdef VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD
    { VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD,    "screen-coords"     },
#endif
#endif
    { 0, }
};

static const char *map_get_string(const map_t m[], uint32_t value)
{
    unsigned int i;
    for (i = 0; m[i].str; i++) {
        if (m[i].value == value)
            return m[i].str;
    }
    return "<unknown>";
}

static int map_get_value(const map_t m[], const char *str, uint32_t *value)
{
    unsigned int i;
    for (i = 0; m[i].str; i++) {
        if (strcasecmp(m[i].str, str) == 0) {
            *value = m[i].value;
            return 1;
        }
    }
    return 0;
}

static void error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(1);
}

static void append_cliprect(int x, int y, unsigned int w, unsigned int h)
{
    CommonContext * const common = common_get_context();
    Rectangle *r;

    r = fast_realloc(common->cliprects,
                     &common->cliprects_size,
                     sizeof(r[0]) * (1 + common->cliprects_count));
    if (!r)
        return;

    common->cliprects    = r;
    common->use_clipping = 1;

    r = &r[common->cliprects_count++];
    r->x      = x;
    r->y      = y;
    r->width  = w;
    r->height = h;
}

typedef enum {
    OPT_TYPE_UINT = 1,
    OPT_TYPE_FLOAT,
    OPT_TYPE_ENUM,
    OPT_TYPE_STRUCT,
    OPT_TYPE_STRING,
    OPT_TYPE_FLAGS, /* separator is ':' */
} opt_type_t;

enum {
    OPT_FLAG_NEGATE           = 1 << 0, /* accept --no- form args */
    OPT_FLAG_OPTIONAL_SUBARG  = 1 << 1, /* option sub-argument is optional */
    OPT_FLAG_DEPRECATED       = 1 << 2, /* deprecated option */
};

typedef struct opt opt_t;

typedef int (*opt_subparse_func_t)(const char *arg, const opt_t *opt);

struct opt {
    const char         *name;
    const char         *desc;
    opt_type_t          type;
    unsigned int        flags;
    void               *var;
    unsigned int       *varflag;
    unsigned int        value;
    const map_t        *enum_map;
    opt_subparse_func_t subparse_func;
};

static int get_size(const char *arg, Size *s, unsigned int *pflag)
{
    unsigned int w, h;

    if (sscanf(arg, "%ux%u", &w, &h) == 2) {
        s->width  = w;
        s->height = h;
        if (pflag)
            *pflag = 1;
        return 0;
    }
    return -1;
}

static int get_rect(const char *arg, Rectangle *r, unsigned int *pflag)
{
    int x, y;
    unsigned int w, h;
    Size size;

    if (get_size(arg, &size, NULL) == 0) {
        r->x      = 0;
        r->y      = 0;
        r->width  = size.width;
        r->height = size.height;
        if (pflag)
            *pflag = 1;
        return 0;
    }
    if (sscanf(arg, "(%d,%d):%ux%u", &x, &y, &w, &h) == 4 ||
        sscanf(arg, "%d,%d:%ux%u",   &x, &y, &w, &h) == 4) {
        r->x      = x;
        r->y      = y;
        r->width  = w;
        r->height = h;
        if (pflag)
            *pflag = 1;
        return 0;
    }
    return -1;
}

static int get_color(const char *arg, unsigned int *pcolor, unsigned int *pflag)
{
    char *end;
    unsigned long v;

    v = strtoul(arg, &end, 16);
    if (*arg != '\0' && end && *end == '\0') {
        *pcolor = v;
        if (pflag)
            *pflag = 1;
        return 0;
    }
    return -1;
}

static int opt_subparse_float(const char *arg, const opt_t *opt)
{
    float * const pval = opt->var;
    float v;
    char *end_ptr;

    static locale_t C_locale = NULL;

    if (!C_locale) {
        C_locale = newlocale(LC_ALL_MASK, "C", NULL);
        if (!C_locale)
            return -1;
    }

    assert(pval);

    errno = 0;
    v = strtof_l(arg, &end_ptr, C_locale);
    if (end_ptr == arg || errno == ERANGE)
        return -1;
    *pval = v;
    return 0;
}

static int opt_subparse_size(const char *arg, const opt_t *opt)
{
    return get_size(arg, opt->var, opt->varflag);
}

static int opt_subparse_rect(const char *arg, const opt_t *opt)
{
    return get_rect(arg, opt->var, opt->varflag);
}

static int opt_subparse_color(const char *arg, const opt_t *opt)
{
    return get_color(arg, opt->var, opt->varflag);
}

static int opt_subparse_cliprect(const char *arg, const opt_t *opt)
{
    Rectangle r;

    if (get_rect(arg, &r, NULL) < 0)
        return -1;

    append_cliprect(r.x, r.y, r.width, r.height);
    return 0;
}

static int opt_subparse_enum(const char *arg, const opt_t *opt)
{
    uint32_t v;
    int * const pval = opt->var;

    assert(pval);
    assert(opt->enum_map);
    if (map_get_value(opt->enum_map, arg, &v)) {
        *pval = v;
        return 0;
    }
    return -1;
}

static int opt_subparse_flags(const char *arg, const opt_t *opt)
{
    unsigned int * const pval = opt->var;
    const char *str;
    unsigned int flags = 0;
    char flag_str[256];
    uint32_t v;

    assert(pval);
    assert(opt->enum_map);
    do {
        str = strchr(arg, ':');
        if (str) {
            const int len = str - arg;
            if (len >= sizeof(flag_str))
                return -1;
            strncpy(flag_str, arg, len);
            flag_str[len] = '\0';
            ++str;
        }
        else
            strcpy(flag_str, arg);
        if (!map_get_value(opt->enum_map, flag_str, &v))
            return -1;
        flags |= v;
        arg = str;
    } while (arg && *arg);
    if (pval)
        *pval = flags;
    return 0;
}

#define UINT_VALUE(VAR, VALUE)                  \
    .type          = OPT_TYPE_UINT,             \
    .var           = &g_common_context.VAR,     \
    .value         = VALUE

#define BOOL_VALUE(VAR)                         \
    UINT_VALUE(VAR, 1),                         \
    .flags         = OPT_FLAG_NEGATE

#define FLOAT_VALUE(VAR, VALUE)                 \
    .type          = OPT_TYPE_FLOAT,            \
    .var           = &g_common_context.VAR,     \
    .value         = VALUE

#define ENUM_VALUE(VAR, MAP, VALUE)             \
    .type          = OPT_TYPE_ENUM,             \
    .var           = &g_common_context.VAR,     \
    .value         = VALUE,                     \
    .enum_map      = map_##MAP

#define FLAGS_VALUE(VAR, MAP, VALUE)            \
    .type          = OPT_TYPE_FLAGS,            \
    .var           = &g_common_context.VAR,     \
    .value         = VALUE,                     \
    .enum_map      = map_##MAP,                 \
    .varflag       = &g_common_context.use_##VAR

#define STRUCT_VALUE_RAW(FUNC)                  \
    .type          = OPT_TYPE_STRUCT,           \
    .subparse_func = opt_subparse_##FUNC

#define STRUCT_VALUE(FUNC, VAR)                 \
    STRUCT_VALUE_RAW(FUNC),                     \
    .var           = &g_common_context.VAR

#define STRUCT_VALUE_WITH_FLAG(FUNC, VAR)       \
    STRUCT_VALUE(FUNC, VAR),                    \
    .varflag       = &g_common_context.use_##VAR

#define STRING_VALUE(VAR)                       \
    .type          = OPT_TYPE_STRING,           \
    .var           = &g_common_context.VAR

static const opt_t g_options[] = {
    { /* Specify the size of the toplevel window */
      "size",
      "Specify the size of the toplevel window",
      STRUCT_VALUE(size, window_size),
    },
#if USE_DRM
    { /* Use DRM backend */
      "drm",
      "Use DRM backend",
      UINT_VALUE(display_type, DISPLAY_DRM),
    },
#endif
#if USE_X11
    { /* Use X11 windowing system */
      "x11",
      "Use X11 windowing system",
      UINT_VALUE(display_type, DISPLAY_X11),
    },
#endif
#if USE_GLX
    { /* Use OpenGL/X11 rendering */
      "glx",
      "Use OpenGL/X11 rendering",
      UINT_VALUE(display_type, DISPLAY_GLX),
    },
#endif
    { /* Select the windowing system: "x11", "glx" */
      "display",
      "Select the windowing system",
      ENUM_VALUE(display_type, display_types, 0),
    },
    { /* Specify the output file name */
      "output",
      "Specify the output file name",
      STRING_VALUE(output_filename),
    },
    { /* Allow rendering of the decoded video frame into a child window */
      "subwindow",
      "Allow rendering of the decoded video frame into a child window",
      BOOL_VALUE(use_subwindow_rect),
    },
    { /* Specify the location and size of the child window */
      "subwindow-rect",
      "Specify the location and size of the child window",
      STRUCT_VALUE_WITH_FLAG(rect, subwindow_rect),
    },
    { /* Select the display rotation mode: 0 ("none"), 90, 180, 270 */
      "rotation",
      "Select the rotation mode",
      ENUM_VALUE(rotation, rotation_modes, ROTATION_NONE),
    },
    { /* Select type of generated image: "rects", "rgb-rects", "flowers" */
      "genimage",
      "Select type of generated image",
      ENUM_VALUE(genimage_type, genimage_types, 0),
    },
    { /* Download the decoded frame from an HW video surface */
      "getimage",
      "Download the decoded frame from an HW video surface",
      ENUM_VALUE(getimage_mode, getimage_modes, GETIMAGE_FROM_VIDEO),
      .flags = OPT_FLAG_NEGATE | OPT_FLAG_OPTIONAL_SUBARG,
    },
    { /* Specify the target image format used for "GetImage" demos */
      "getimage-format",
      "Specify the target image format used for \"GetImage\" demos",
      ENUM_VALUE(getimage_format, image_formats, 0),
    },
    { /* Allow vaGetImage() from a specific region */
      "getimage-rect",
      "Allow vaGetImage() from a specific region",
      STRUCT_VALUE_WITH_FLAG(rect, getimage_rect),
    },
    { /* Upload a generated image to a HW video surface */
      "putimage",
      "Upload a generated image to a HW video surface",
      ENUM_VALUE(putimage_mode, putimage_modes, PUTIMAGE_OVERRIDE),
      .flags = OPT_FLAG_NEGATE | OPT_FLAG_OPTIONAL_SUBARG,
    },
    { /* Specify the source image format used for "PutImage" demos */
      "putimage-format",
      "Specify the source image format used for \"PutImage\" demos",
      ENUM_VALUE(putimage_format, image_formats, 0),
    },
    { /* Specify the source image size used for "PutImage" demos */
      "putimage-size",
      "Specify the source image size used for \"PutImage\" demos",
      STRUCT_VALUE(size, putimage_size),
    },
#if USE_FFMPEG
    { /* Select the HW acceleration API. e.g. for FFmpeg demos */
      "hwaccel",
      "Select the HW acceleration API. e.g. for FFmpeg demos",
      ENUM_VALUE(hwaccel_type, hwaccel_types, HWACCEL_DEFAULT),
      .flags = OPT_FLAG_NEGATE,
    },
#endif
    { /* Enable clipping the video surface by several predefined rectangles */
      "clipping",
      "Enable clipping the video surface by several predefined rectangles",
      BOOL_VALUE(use_clipping),
    },
    { /* Specify an additional clip rectangle of the form X,Y:WxH */
      "cliprect",
      "Specify an additional clip rectangle",
      STRUCT_VALUE_RAW(cliprect),
    },
    { /* Render a single surface to multiple locations within the same window */
      "multi-rendering",
      "Render a single surface to multiple locations within the same window",
      BOOL_VALUE(multi_rendering),
    },
#if USE_VAAPI
    { /* Allow use of vaDeriveImage() in GetImage or PutImage tests */
      "vaapi-derive-image",
      "Allow use of vaDeriveImage() in GetInmage or PutImage tests (default)",
      BOOL_VALUE(vaapi_derive_image),
    },
    { /* Specify vaPutSurface() source rectangle */
      "vaapi-putsurface-source-rect",
      "Specify vaPutSurface() source rectangle",
      STRUCT_VALUE_WITH_FLAG(rect, vaapi_putsurface_source_rect),
    },
    { /* Specify vaPutSurface() target rectangle */
      "vaapi-putsurface-target-rect",
      "Specify vaPutSurface() target rectangle",
      STRUCT_VALUE_WITH_FLAG(rect, vaapi_putsurface_target_rect),
    },
    { /* Specify vaPutSurface() flags, separated by ':' */
      "vaapi-putsurface-flags",
      "Specify vaPutSurface() flags",
      FLAGS_VALUE(vaapi_putsurface_flags, vaapi_putsurface_flags, 0),
    },
    { /* Specify vaAssociateSubpicture() source rectangle [--putimage blend] */
      "vaapi-subpicture-source-rect",
      "Specify vaAssociateSubpicture() source rectangle",
      STRUCT_VALUE_WITH_FLAG(rect, vaapi_subpicture_source_rect),
    },
    { /* Specify vaAssociateSubpicture() target rectangle [--putimage blend] */
      "vaapi-subpicture-target-rect",
      "Specify vaAssociateSubpicture() target rectangle",
      STRUCT_VALUE_WITH_FLAG(rect, vaapi_subpicture_target_rect),
    },
    { /* Specify vaAssociateSubpicture() flags, separated by ':' */
      "vaapi-subpicture-flags",
      "Specify vaAssociateSubpicture() flags",
      FLAGS_VALUE(vaapi_subpicture_flags, vaapi_subpicture_flags, 0),
    },
    { /* Specify vaSetSubpictureGlobalAlpha() value */
      "vaapi-subpicture-alpha",
      "Specify vaSetSubpictureGlobalAlpha() value",
      FLOAT_VALUE(vaapi_subpicture_alpha, 1.0),
    },
    { /* Render a single surface to multiple locations within the same window */
      /* DEPRECATED: use --multi-rendering */
      "vaapi-multirendering",
      "Render a single surface to multiple locations within the same window",
      BOOL_VALUE(multi_rendering) | OPT_FLAG_DEPRECATED,
    },
    { /* Let vaPutImage() scale the source image into the destination surface */
      "vaapi-putimage-scaled",
      "Allow vaPutImage() to scale the source image into the destination surface",
      BOOL_VALUE(vaapi_putimage_scaled),
    },
    { /* Set background color via a VA-API display attribute */
      "vaapi-background-color",
      "Set background color via a VA-API display attribute",
      STRUCT_VALUE_WITH_FLAG(color, vaapi_background_color),
    },
    { /* Use multiple (5) subpictures to render --putimage blend scene */
      "vaapi-multi-subpictures",
      "Use multiple (5) subpictures to render --putimage blend scene",
      BOOL_VALUE(vaapi_multi_subpictures),
    },
#if USE_GLX
    { /* Use vaCopySurfaceGLX() to transfer surface to a GL texture (default) */
      "vaapi-glx-use-copy",
      "Use vaCopySurfaceGLX() to transfer surface to a GL texture (default)",
      BOOL_VALUE(vaapi_glx_use_copy),
    },
#endif
#endif
#if USE_VDPAU
    { /* Use VDPAU layers for subpictures [--putimage blend] */
      "vdpau-layers",
      "Use VDPAU layers for subpictures",
      BOOL_VALUE(vdpau_layers),
    },
    { /* Enable high-quality scaling */
      "vdpau-hqscaling",
      "Enable high-quality scaling",
      BOOL_VALUE(vdpau_hqscaling),
    },
#if USE_GLX
    { /* Enable VDPAU/GL interop through a VdpVideoSurface */
      "vdpau-glx-video-surface",
      "Enable VDPAU/GL interop through a VdpVideoSurface",
      BOOL_VALUE(vdpau_glx_video_surface),
    },
    { /* Enable VDPAU/GL interop through a VdpOutputSurface */
      "vdpau-glx-output-surface",
      "Enable VDPAU/GL interop through a VdpOutputSurface",
      BOOL_VALUE(vdpau_glx_output_surface),
    },
#endif
#endif
#if USE_GLX
    { /* Specify the GLX texture target to use for rendering */
      "glx-texture-target",
      "Specify the GLX texture target to use for rendering",
      ENUM_VALUE(glx_texture_target, texture_targets, 0),
    },
    { /* Specify the GLX texture format to use for rendering */
      "glx-texture-format",
      "Specify the GLX texture format to use for rendering",
      ENUM_VALUE(glx_texture_format, texture_formats, 0),
    },
    { /* Specify the GLX texture size to use for rendering */
      "glx-texture-size",
      "Specify the GLX texture size to use for rendering",
      STRUCT_VALUE_WITH_FLAG(size, glx_texture_size),
    },
    { /* Render the surface to an FBO prior to rendering to screen */
      "glx-use-fbo",
      "Render the surface to an FBO prior to rendering to screen",
      BOOL_VALUE(glx_use_fbo),
    },
    { /* Render the surface with a reflection effect */
      "glx-use-reflection",
      "Render the surface with a reflection effect",
      BOOL_VALUE(glx_use_reflection),
    },
#endif
#if USE_CRYSTALHD
    { /* Use DtsProcOutputNoCopy() to get the surface from Crystal HD */
      "crystalhd-output-nocopy",
      "Use DtsProcOutputNoCopy() to get the surface from Crystal HD",
      BOOL_VALUE(crystalhd_output_nocopy),
    },
    { /* Use DtsFlushInput() to commit encoded or decoded frames */
      "crystalhd-flush",
      "Use DtsFlushInput() to commit encoded or decoded frames",
      BOOL_VALUE(crystalhd_flush),
    },
#endif
    { NULL, }
};

#undef UINT_VALUE
#undef FLOAT_VALUE
#undef ENUM_VALUE
#undef STRUCT_VALUE
#undef STRUCT_VALUE_RAW
#undef STRUCT_VALUE_WITH_FLAG

static inline void print_bool(const opt_t *o)
{
    unsigned int v = *(unsigned int *)o->var;

    printf("%s", v ? "true" : "false");
}

static void print_enum(const opt_t *o)
{
    unsigned int v = *(unsigned int *)o->var;

    const char *vs = map_get_string(o->enum_map, v);
    if (!vs)
        vs = "<unknown>";
    printf("\"%s\"", vs);
}

static void print_string(const opt_t *o)
{
    const char * const str = *(char **)o->var;

    printf("\"%s\"", str);
}

static void print_var(const opt_t *o)
{
    switch (o->type) {
    case OPT_TYPE_UINT:
        if (o->flags & OPT_FLAG_NEGATE)
            print_bool(o);
        break;
    case OPT_TYPE_ENUM:
        print_enum(o);
        break;
    case OPT_TYPE_STRING:
        print_string(o);
        break;
    default:
        break;
    }
}

static void show_help(const char *prog)
{
    int i, j;

    printf("%s - version %s\n", PACKAGE, PACKAGE_VERSION);
    printf("\n");
    printf("Usage: %s [<option>]*\n", prog);
    printf("Where <option> is any of the following:\n");
    printf("\n");

    for (i = 0; g_options[i].name; i++) {
        const opt_t * const o = &g_options[i];
        int show_default = 0;
        int show_values  = 0;

        switch (o->type) {
        case OPT_TYPE_UINT:
            if (o->flags & OPT_FLAG_NEGATE)
                show_default = 1;
            break;
        case OPT_TYPE_ENUM:
            if (*(unsigned int *)o->var)
                show_default = 1;
            show_values = 1;
            break;
        case OPT_TYPE_FLAGS:
            show_values = 1;
            break;
        default:
            break;
        }

        printf("--%s", o->name);
        printf(" (");
        if (o->flags & OPT_FLAG_DEPRECATED)
            printf("deprecated ");
        switch (o->type) {
        case OPT_TYPE_UINT:
            if (o->flags & OPT_FLAG_NEGATE)
                printf("bool");
            break;
        case OPT_TYPE_FLOAT:
            printf("float");
            break;
        case OPT_TYPE_ENUM:
            printf("enum");
            break;
        case OPT_TYPE_STRUCT:
            printf("struct");
            break;
        case OPT_TYPE_FLAGS:
            printf("flags");
            break;
        case OPT_TYPE_STRING:
            printf("string");
            break;
        }
        printf(")");
        printf("\n");

        if (o->desc)
            printf("  %s.", o->desc);
        if (show_default) {
            printf(" Default: ");
            print_var(o);
            printf(".");
        }
        printf("\n");

        if (show_values) {
            printf("\n");
            printf("  ");
            switch (o->type) {
            case OPT_TYPE_ENUM:
                printf("Possible values:\n");
                break;
            case OPT_TYPE_FLAGS:
                printf("Flags are a combination of the following values "
                       ", with \":\" as the separator:\n");
                break;
            default:
                ASSERT(o->type == OPT_TYPE_ENUM ||
                       o->type == OPT_TYPE_FLAGS);
                break;
            }

            const map_t * const m = o->enum_map;
            for (j = 0; m[j].str; j++)
                printf("    \"%s\"\n", m[j].str);
        }
        printf("\n");
    }
    exit(0);
}

static int options_parse(int argc, char *argv[])
{
    int i, j;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            show_help(argv[0]);
            return -1;
        }

        if (*arg++ != '-' || *arg++ != '-')
            continue;

        int negate = 0;
        if (strncmp(arg, "no-", 3) == 0) {
            arg += 3;
            negate = 1;
        }

        for (j = 0; g_options[j].name; j++) {
            const opt_t *opt = &g_options[j];
            if (strcmp(arg, opt->name) == 0) {
                unsigned int * const pval = opt->var;
                unsigned int * const pvar = opt->varflag;
                if (negate) {
                    assert(pval);
                    *pval = 0;
                }
                else {
                    switch (opt->type) {
                    case OPT_TYPE_UINT:
                        assert(pval);
                        *pval = opt->value;
                        break;
                    case OPT_TYPE_FLOAT:
                        assert(pval);
                        if (++i >= argc || opt_subparse_float(argv[i], opt) < 0)
                            error("could not parse %s argument", arg);
                        break;
                        break;
                    case OPT_TYPE_ENUM:
                        assert(pval);
                        if (i + 1 < argc && opt_subparse_enum(argv[i + 1], opt) == 0)
                            i++;
                        else if (opt->flags & OPT_FLAG_OPTIONAL_SUBARG)
                            *pval = opt->value;
                        else
                            error("unknown %s value '%s'", arg, argv[i + 1]);
                        break;
                    case OPT_TYPE_STRUCT:
                        assert(opt->subparse_func);
                        if (++i >= argc || opt->subparse_func(argv[i], opt) < 0)
                            error("could not parse %s argument", arg);
                        break;
                    case OPT_TYPE_FLAGS:
                        assert(pval);
                        if (pvar)
                            *pvar = 0;
                        if (i + 1 < argc && opt_subparse_flags(argv[i + 1], opt) == 0)
                            i++;
                        else if (opt->flags & OPT_FLAG_OPTIONAL_SUBARG)
                            *pval = opt->value;
                        else
                            error("could not parse %s flags '%s'", arg, argv[i + 1]);
                        if (pvar)
                            *pvar = 1;
                        break;
                    case OPT_TYPE_STRING:
                        assert(pval);
                        if (++i >= argc)
                            error("could not parse %s argument", arg);
                        *(char **)pval = argv[i];
                        break;
                    default:
                        error("invalid option type %d", opt->type);
                        break;
                    }
                }
                break;
            }
        }
        if (!g_options[j].name)
            error("unknown option '%s'", argv[i]);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    CommonContext * const common = common_get_context();
    int i, is_error = 1;

    /* Option defaults */
    common->vaapi_derive_image          = 1;
    common->vaapi_glx_use_copy          = 1;
    common->vaapi_subpicture_alpha      = 1.0;
    common->glx_texture_target          = TEXTURE_TARGET_2D;
    common->glx_texture_format          = IMAGE_BGRA;
    common->glx_use_fbo                 = 0;
    common->glx_use_reflection          = 1;

    if (options_parse(argc, argv) < 0)
        goto end;

    if (common->use_clipping && common->cliprects_count == 0) {
        /* add some default clip rectangles */
        const unsigned int ww = common->window_size.width;
        const unsigned int wh = common->window_size.height;
        const unsigned int gw = ww / 32;
        const unsigned int gh = wh / 32;
        append_cliprect(4*gw, 4*gh, 4*gw, 4*gh);
        append_cliprect(ww - 8*gw, 4*gh, 4*gw, 4*gh);
        append_cliprect(ww/2 - 2*gw, 4*gh, 4*gw, 4*gh);
        append_cliprect(ww/2 - 8*gw, wh - 8*gh, 16*gw, 4*gh);
        append_cliprect(ww/2 - 2*gw, wh - 12*gh, 4*gw, 4*gh);
        append_cliprect(ww/2 - 12*gw, wh - 20*gh, 24*gw, 4*gh);
    }

    if (common->use_subwindow_rect) {
        Rectangle * const r = &common->subwindow_rect;
        if (r->width == 0 || r->height == 0) {
            r->x      = common->window_size.width/2;
            r->y      = common->window_size.height/2;
            r->width  = common->window_size.width/2;
            r->height = common->window_size.height/2;
        }
        ensure_bounds(r, common->window_size.width, common->window_size.height);
    }

    if (common->use_vaapi_putsurface_target_rect)
        ensure_bounds(&common->vaapi_putsurface_target_rect,
                      common->window_size.width,
                      common->window_size.height);

    printf("Display type '%s'\n",
           map_get_string(map_display_types, common->display_type));

    if (common->use_subwindow_rect)
        printf("Render to subwindow at (%d,%d) with size %ux%u\n",
               common->subwindow_rect.x,
               common->subwindow_rect.y,
               common->subwindow_rect.width,
               common->subwindow_rect.height);

    if (common->hwaccel_type != HWACCEL_NONE)
        printf("Hardware accelerator '%s'\n",
               map_get_string(map_hwaccel_types, common->hwaccel_type));

    if (common->getimage_mode != GETIMAGE_NONE)
        printf("Readback decoded pixels from '%s' surface, in %s format\n",
               map_get_string(map_getimage_modes, common->getimage_mode),
               (common->getimage_format
                ? map_get_string(map_image_formats, common->getimage_format)
                : "hardware default"));

    if (common->putimage_mode != PUTIMAGE_NONE)
        printf("Transfer custom pixels to surface in '%s' mode, in %s format\n",
               map_get_string(map_putimage_modes, common->putimage_mode),
               (common->putimage_format
                ? map_get_string(map_image_formats, common->putimage_format)
                : "hardware default"));

    if (common->cliprects_count > 0) {
        printf("%d clip rects\n", common->cliprects_count);
        for (i = 0; i < common->cliprects_count; i++)
            printf("  %d: location (%d,%d), size %ux%u\n",
                   i + 1,
                   common->cliprects[i].x,
                   common->cliprects[i].y,
                   common->cliprects[i].width,
                   common->cliprects[i].height);
    }

    common->cliprects_image = image_create(common->window_size.width,
                                           common->window_size.height,
                                           IMAGE_RGB32);
    if (!common->cliprects_image) {
        fprintf(stderr, "ERROR: cliprects image allocation failed\n");
        goto end;
    }

    image_draw_rectangle(common->cliprects_image,
                         0, 0, common->window_size.width, common->window_size.height,
                         0xffffffff);

    for (i = 0; i < common->cliprects_count; i++)
        image_draw_rectangle(common->cliprects_image,
                             common->cliprects[i].x,
                             common->cliprects[i].y,
                             common->cliprects[i].width,
                             common->cliprects[i].height,
                             0xffff5400);

    common->image = image_create(common->window_size.width,
                                 common->window_size.height,
                                 IMAGE_RGB32);
    if (!common->image) {
        fprintf(stderr, "ERROR: image allocation failed\n");
        goto end;
    }

    if (common->output_filename) {
        common->output_file = fopen(common->output_filename, "wb");
        if (!common->output_file) {
            fprintf(stderr, "ERROR: output file creation failed");
            goto end;
        }
    }

    if (pre() < 0) {
        fprintf(stderr, "ERROR: initialization failed\n");
        goto end;
    }

    if (decode() < 0) {
        fprintf(stderr, "ERROR: decode failed\n");
        goto end;
    }

    if (common->output_file && getimage_mode() == GETIMAGE_FROM_VIDEO) {
        if (image_write(common->image, common->output_file) < 0) {
            fprintf(stderr, "ERROR: image write failed\n");
            goto end;
        }
    }

    if (display() < 0) {
        fprintf(stderr, "ERROR: display failed\n");
        goto end;
    }

    if (post() < 0) {
        fprintf(stderr, "ERROR: deinitialization failed\n");
        goto end;
    }

    is_error = 0;
end:
    if (common->output_file)
        fclose(common->output_file);
    free(common->cliprects);
    image_destroy(common->cliprects_image);
    image_destroy(common->image);
    return is_error;
}
