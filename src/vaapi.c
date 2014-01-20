/*
 *  vaapi.c - VA API common code
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
#include "vaapi.h"
#include "vaapi_compat.h"
#include "common.h"
#include "utils.h"

#if USE_X11
# include "x11.h"
#endif

#if USE_DRM
# include "vo_drm.h"
#endif
#if USE_VAAPI_DRM
# include <va/va_drm.h>
#endif

#if USE_GLX
# include "glx.h"
#endif
#if USE_VAAPI_GLX
# include <va/va_glx.h>
#endif

#define DEBUG 1
#include "debug.h"


static VAAPIContext *vaapi_context;

static inline const char *string_of_VAImageFormat(VAImageFormat *imgfmt)
{
    return string_of_FOURCC(imgfmt->fourcc);
}

static const char *string_of_VAProfile(VAProfile profile)
{
    switch (profile) {
#define PROFILE(profile) \
        case VAProfile##profile: return "VAProfile" #profile
        PROFILE(MPEG2Simple);
        PROFILE(MPEG2Main);
        PROFILE(MPEG4Simple);
        PROFILE(MPEG4AdvancedSimple);
        PROFILE(MPEG4Main);
#if VA_CHECK_VERSION(0,32,0)
        PROFILE(JPEGBaseline);
        PROFILE(H263Baseline);
        PROFILE(H264ConstrainedBaseline);
#endif
        PROFILE(H264Baseline);
        PROFILE(H264Main);
        PROFILE(H264High);
        PROFILE(VC1Simple);
        PROFILE(VC1Main);
        PROFILE(VC1Advanced);
#undef PROFILE
    default: break;
    }
    return "<unknown>";
}

static const char *string_of_VAEntrypoint(VAEntrypoint entrypoint)
{
    switch (entrypoint) {
#define ENTRYPOINT(entrypoint) \
        case VAEntrypoint##entrypoint: return "VAEntrypoint" #entrypoint
        ENTRYPOINT(VLD);
        ENTRYPOINT(IZZ);
        ENTRYPOINT(IDCT);
        ENTRYPOINT(MoComp);
        ENTRYPOINT(Deblocking);
#if VA_CHECK_VERSION(0,32,0)
        ENTRYPOINT(EncSlice);
        ENTRYPOINT(EncPicture);
#endif
#undef ENTRYPOINT
    default: break;
    }
    return "<unknown>";
}

static const char *string_of_VADisplayAttribType(VADisplayAttribType type)
{
    switch (type) {
#define TYPE(type) \
        case VADisplayAttrib##type: return "VADisplayAttrib" #type
        TYPE(Brightness);
        TYPE(Contrast);
        TYPE(Hue);
        TYPE(Saturation);
        TYPE(BackgroundColor);
#if !VA_CHECK_VERSION(0,34,0)
        TYPE(DirectSurface);
#endif
#if VA_CHECK_VERSION(0,32,0)
        TYPE(Rotation);
#endif
#undef TYPE
    default: break;
    }
    return "<unknown>";
}

static void destroy_buffers(VADisplay display, VABufferID *buffers, unsigned int n_buffers)
{
    unsigned int i;
    for (i = 0; i < n_buffers; i++) {
        if (buffers[i] != VA_INVALID_ID) {
            vaDestroyBuffer(display, buffers[i]);
            buffers[i] = VA_INVALID_ID;
        }
    }
}

static bool
has_display_attribute(VADisplayAttribType type)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    int i;

    if (vaapi->display_attrs) {
        for (i = 0; i < vaapi->n_display_attrs; i++) {
            if (vaapi->display_attrs[i].type == type)
                return true;
        }
    }
    return false;
}

#if 0
static bool
get_display_attribute(VADisplayAttribType type, int *pvalue)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VADisplayAttribute attr;
    VAStatus status;

    attr.type  = type;
    attr.flags = VA_DISPLAY_ATTRIB_GETTABLE;
    status = vaGetDisplayAttributes(vaapi->display, &attr, 1);
    if (!vaapi_check_status(status, "vaGetDisplayAttributes()"))
        return false;

    if (pvalue)
        *pvalue = attr.value;
    return true;
}
#endif

static bool
set_display_attribute(VADisplayAttribType type, int value)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VADisplayAttribute attr;
    VAStatus status;

    attr.type  = type;
    attr.value = value;
    attr.flags = VA_DISPLAY_ATTRIB_SETTABLE;
    status = vaSetDisplayAttributes(vaapi->display, &attr, 1);
    if (!vaapi_check_status(status, "vaSetDisplayAttributes()"))
        return false;
    return true;
}

int vaapi_init(VADisplay display)
{
    CommonContext * common = common_get_context();
    VAAPIContext *vaapi;
    int major_version, minor_version;
    int i, num_display_attrs, max_display_attrs;
    VADisplayAttribute *display_attrs = NULL;
    VAStatus status;

    if (vaapi_context)
        return 0;

    if (!display)
        goto error;
    D(bug("VA display %p\n", display));

    status = vaInitialize(display, &major_version, &minor_version);
    if (!vaapi_check_status(status, "vaInitialize()"))
        goto error;
    D(bug("VA API version %d.%d\n", major_version, minor_version));

    max_display_attrs = vaMaxNumDisplayAttributes(display);
    display_attrs = malloc(max_display_attrs * sizeof(display_attrs[0]));
    if (!display_attrs)
        goto error;

    num_display_attrs = 0; /* XXX: workaround old GMA500 bug */
    status = vaQueryDisplayAttributes(display, display_attrs, &num_display_attrs);
    if (!vaapi_check_status(status, "vaQueryDisplayAttributes()"))
        goto error;
    D(bug("%d display attributes available\n", num_display_attrs));
    for (i = 0; i < num_display_attrs; i++) {
        VADisplayAttribute * const display_attr = &display_attrs[i];
        D(bug("  %-32s (%s/%s) min %d max %d value 0x%x\n",
              string_of_VADisplayAttribType(display_attr->type),
              (display_attr->flags & VA_DISPLAY_ATTRIB_GETTABLE) ? "get" : "---",
              (display_attr->flags & VA_DISPLAY_ATTRIB_SETTABLE) ? "set" : "---",
              display_attr->min_value,
              display_attr->max_value,
              display_attr->value));
    }

    if (common->use_vaapi_background_color) {
        VADisplayAttribute attr;
        attr.type  = VADisplayAttribBackgroundColor;
        attr.value = common->vaapi_background_color;
        status = vaSetDisplayAttributes(display, &attr, 1);
        if (!vaapi_check_status(status, "vaSetDisplayAttributes()"))
            goto error;
    }

    if ((vaapi = calloc(1, sizeof(*vaapi))) == NULL)
        goto error;
    vaapi->display               = display;
    vaapi->config_id             = VA_INVALID_ID;
    vaapi->context_id            = VA_INVALID_ID;
    vaapi->surface_id            = VA_INVALID_ID;
    vaapi->subpic_image.image_id = VA_INVALID_ID;
    for (i = 0; i < ARRAY_ELEMS(vaapi->subpic_ids); i++)
        vaapi->subpic_ids[i]     = VA_INVALID_ID;
    vaapi->pic_param_buf_id      = VA_INVALID_ID;
    vaapi->iq_matrix_buf_id      = VA_INVALID_ID;
    vaapi->bitplane_buf_id       = VA_INVALID_ID;
    vaapi->huf_table_buf_id      = VA_INVALID_ID;
    vaapi->display_attrs         = display_attrs;
    vaapi->n_display_attrs       = num_display_attrs;

    vaapi_context = vaapi;

    if (common->rotation != ROTATION_NONE) {
        if (!has_display_attribute(VADisplayAttribRotation))
            printf("VAAPI: display rotation attribute is not supported\n");
        else {
            int rotation;
            switch (common->rotation) {
            case ROTATION_NONE: rotation = VA_ROTATION_NONE; break;
            case ROTATION_90:   rotation = VA_ROTATION_90;   break;
            case ROTATION_180:  rotation = VA_ROTATION_180;  break;
            case ROTATION_270:  rotation = VA_ROTATION_270;  break;
            default:            ASSERT(0 && "unsupported rotation mode");
            }
            if (!set_display_attribute(VADisplayAttribRotation, rotation))
                return -1;
        }
    }
    return 0;

error:
    free(display_attrs);
    return -1;
}

int vaapi_exit(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    unsigned int i;

    if (!vaapi)
        return 0;

#if USE_GLX
    if (display_type() == DISPLAY_GLX)
        vaapi_glx_destroy_surface();
#endif

    destroy_buffers(vaapi->display, &vaapi->pic_param_buf_id, 1);
    destroy_buffers(vaapi->display, &vaapi->iq_matrix_buf_id, 1);
    destroy_buffers(vaapi->display, &vaapi->bitplane_buf_id, 1);
    destroy_buffers(vaapi->display, &vaapi->huf_table_buf_id, 1);
    destroy_buffers(vaapi->display, vaapi->slice_buf_ids, vaapi->n_slice_buf_ids);

    if (vaapi->subpic_flags) {
        free(vaapi->subpic_flags);
        vaapi->subpic_flags = NULL;
    }

    if (vaapi->subpic_formats) {
        free(vaapi->subpic_formats);
        vaapi->subpic_formats = NULL;
        vaapi->n_subpic_formats = 0;
    }

    if (vaapi->image_formats) {
        free(vaapi->image_formats);
        vaapi->image_formats = NULL;
        vaapi->n_image_formats = 0;
    }

    if (vaapi->entrypoints) {
        free(vaapi->entrypoints);
        vaapi->entrypoints = NULL;
        vaapi->n_entrypoints = 0;
    }

    if (vaapi->profiles) {
        free(vaapi->profiles);
        vaapi->profiles = NULL;
        vaapi->n_profiles = 0;
    }

    if (vaapi->slice_params) {
        free(vaapi->slice_params);
        vaapi->slice_params = NULL;
        vaapi->slice_params_alloc = 0;
        vaapi->n_slice_params = 0;
    }

    if (vaapi->slice_buf_ids) {
        free(vaapi->slice_buf_ids);
        vaapi->slice_buf_ids = NULL;
        vaapi->n_slice_buf_ids = 0;
    }

    if (vaapi->subpic_image.image_id != VA_INVALID_ID) {
        vaDestroyImage(vaapi->display, vaapi->subpic_image.image_id);
        vaapi->subpic_image.image_id = VA_INVALID_ID;
    }

    for (i = 0; i < ARRAY_ELEMS(vaapi->subpic_ids); i++) {
        if (vaapi->subpic_ids[i] != VA_INVALID_ID) {
            vaDestroySubpicture(vaapi->display, vaapi->subpic_ids[i]);
            vaapi->subpic_ids[i] = VA_INVALID_ID;
        }
    }

    if (vaapi->context_id != VA_INVALID_ID) {
        vaDestroyContext(vaapi->display, vaapi->context_id);
        vaapi->context_id = VA_INVALID_ID;
    }

    if (vaapi->surface_id != VA_INVALID_ID) {
        vaDestroySurfaces(vaapi->display, &vaapi->surface_id, 1);
        vaapi->surface_id = VA_INVALID_ID;
    }

    if (vaapi->config_id != VA_INVALID_ID) {
        vaDestroyConfig(vaapi->display, vaapi->config_id);
        vaapi->config_id = VA_INVALID_ID;
    }

    if (vaapi->display_attrs) {
        free(vaapi->display_attrs);
        vaapi->display_attrs = NULL;
        vaapi->n_display_attrs = 0;
    }

    if (vaapi->display) {
        vaTerminate(vaapi->display);
        vaapi->display = NULL;
    }

    free(vaapi_context);
    return 0;
}

VAAPIContext *vaapi_get_context(void)
{
    return vaapi_context;
}

int vaapi_check_status(VAStatus status, const char *msg)
{
    if (status != VA_STATUS_SUCCESS) {
        fprintf(stderr, "[%s] %s: %s\n", PACKAGE_NAME, msg, vaErrorStr(status));
        return 0;
    }
    return 1;
}

static void *alloc_buffer(VAAPIContext *vaapi, int type, unsigned int size, VABufferID *buf_id)
{
    VAStatus status;
    void *data = NULL;

    *buf_id = VA_INVALID_ID;
    status = vaCreateBuffer(vaapi->display, vaapi->context_id,
                            type, size, 1, NULL, buf_id);
    if (!vaapi_check_status(status, "vaCreateBuffer()"))
        return NULL;

    status = vaMapBuffer(vaapi->display, *buf_id, &data);
    if (!vaapi_check_status(status, "vaMapBuffer()"))
        return NULL;

    return data;
}

void *vaapi_alloc_picture(unsigned int size)
{
    VAAPIContext *vaapi = vaapi_get_context();
    if (!vaapi)
        return NULL;
    return alloc_buffer(vaapi, VAPictureParameterBufferType, size, &vaapi->pic_param_buf_id);
}

void *vaapi_alloc_iq_matrix(unsigned int size)
{
    VAAPIContext *vaapi = vaapi_get_context();
    if (!vaapi)
        return NULL;
    return alloc_buffer(vaapi, VAIQMatrixBufferType, size, &vaapi->iq_matrix_buf_id);
}

void *vaapi_alloc_bitplane(unsigned int size)
{
    VAAPIContext *vaapi = vaapi_get_context();
    if (!vaapi)
        return NULL;
    return alloc_buffer(vaapi, VABitPlaneBufferType, size, &vaapi->bitplane_buf_id);
}

void *vaapi_alloc_huf_table(unsigned int size)
{
    VAAPIContext *vaapi = vaapi_get_context();
    if (!vaapi)
        return NULL;
#if VA_CHECK_VERSION(0,32,1)
    return alloc_buffer(vaapi, VAHuffmanTableBufferType, size,
        &vaapi->huf_table_buf_id);
#else
    return NULL;
#endif
}

static int commit_slices(VAAPIContext *vaapi)
{
    VAStatus status;
    VABufferID *slice_buf_ids;
    VABufferID slice_param_buf_id, slice_data_buf_id;

    if (vaapi->n_slice_params == 0)
        return 0;

    slice_buf_ids =
        fast_realloc(vaapi->slice_buf_ids,
                     &vaapi->slice_buf_ids_alloc,
                     (vaapi->n_slice_buf_ids + 2) * sizeof(slice_buf_ids[0]));
    if (!slice_buf_ids)
        return -1;
    vaapi->slice_buf_ids = slice_buf_ids;

    slice_param_buf_id = VA_INVALID_ID;
    status = vaCreateBuffer(vaapi->display, vaapi->context_id,
                            VASliceParameterBufferType,
                            vaapi->slice_param_size,
                            vaapi->n_slice_params, vaapi->slice_params,
                            &slice_param_buf_id);
    if (!vaapi_check_status(status, "vaCreateBuffer() for slice params"))
        return -1;
    vaapi->n_slice_params = 0;

    slice_data_buf_id = VA_INVALID_ID;
    status = vaCreateBuffer(vaapi->display, vaapi->context_id,
                            VASliceDataBufferType,
                            vaapi->slice_data_size,
                            1, (void *)vaapi->slice_data,
                            &slice_data_buf_id);
    if (!vaapi_check_status(status, "vaCreateBuffer() for slice data"))
        return -1;
    vaapi->slice_data = NULL;
    vaapi->slice_data_size = 0;

    slice_buf_ids[vaapi->n_slice_buf_ids++] = slice_param_buf_id;
    slice_buf_ids[vaapi->n_slice_buf_ids++] = slice_data_buf_id;
    return 0;
}

void *vaapi_alloc_slice(unsigned int size, const uint8_t *buf, unsigned int buf_size)
{
    VAAPIContext *vaapi = vaapi_get_context();
    uint8_t *slice_params;
    VASliceParameterBufferBase *slice_param;

    if (!vaapi)
        return NULL;

    if (vaapi->slice_param_size == 0)
        vaapi->slice_param_size = size;
    else if (vaapi->slice_param_size != size)
        return NULL;

    if (!vaapi->slice_data)
        vaapi->slice_data = buf;
    if (vaapi->slice_data + vaapi->slice_data_size != buf) {
        if (commit_slices(vaapi) < 0)
            return NULL;
        vaapi->slice_data = buf;
    }

    slice_params =
        fast_realloc(vaapi->slice_params,
                     &vaapi->slice_params_alloc,
                     (vaapi->n_slice_params + 1) * vaapi->slice_param_size);
    if (!slice_params)
        return NULL;
    vaapi->slice_params = slice_params;

    slice_param = (VASliceParameterBufferBase *)(slice_params + vaapi->n_slice_params * vaapi->slice_param_size);
    slice_param->slice_data_size   = buf_size;
    slice_param->slice_data_offset = vaapi->slice_data_size;
    slice_param->slice_data_flag   = VA_SLICE_DATA_FLAG_ALL;

    vaapi->n_slice_params++;
    vaapi->slice_data_size += buf_size;
    return slice_param;
}

static int has_profile(VAAPIContext *vaapi, VAProfile profile)
{
    VAStatus status;
    int i;

    if (!vaapi->profiles || vaapi->n_profiles == 0) {
        vaapi->profiles = calloc(vaMaxNumProfiles(vaapi->display), sizeof(vaapi->profiles[0]));

        status = vaQueryConfigProfiles(vaapi->display,
                                       vaapi->profiles,
                                       &vaapi->n_profiles);
        if (!vaapi_check_status(status, "vaQueryConfigProfiles()"))
            return 0;

        D(bug("%d profiles available\n", vaapi->n_profiles));
        for (i = 0; i < vaapi->n_profiles; i++)
            D(bug("  %s\n", string_of_VAProfile(vaapi->profiles[i])));
    }

    for (i = 0; i < vaapi->n_profiles; i++) {
        if (vaapi->profiles[i] == profile)
            return 1;
    }
    return 0;
}

static int has_entrypoint(VAAPIContext *vaapi, VAProfile profile, VAEntrypoint entrypoint)
{
    VAStatus status;
    int i;

    if (!vaapi->entrypoints || vaapi->n_entrypoints == 0) {
        vaapi->entrypoints = calloc(vaMaxNumEntrypoints(vaapi->display), sizeof(vaapi->entrypoints[0]));

        status = vaQueryConfigEntrypoints(vaapi->display, profile,
                                          vaapi->entrypoints,
                                          &vaapi->n_entrypoints);
        if (!vaapi_check_status(status, "vaQueryConfigEntrypoints()"))
            return 0;

        D(bug("%d entrypoints available for %s\n", vaapi->n_entrypoints,
              string_of_VAProfile(profile)));
        for (i = 0; i < vaapi->n_entrypoints; i++)
            D(bug("  %s\n", string_of_VAEntrypoint(vaapi->entrypoints[i])));
    }

    for (i = 0; i < vaapi->n_entrypoints; i++) {
        if (vaapi->entrypoints[i] == entrypoint)
            return 1;
    }
    return 0;
}

int vaapi_init_decoder(VAProfile    profile,
                       VAEntrypoint entrypoint,
                       unsigned int picture_width,
                       unsigned int picture_height)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAConfigAttrib attrib;
    VAConfigID config_id = VA_INVALID_ID;
    VAContextID context_id = VA_INVALID_ID;
    VASurfaceID surface_id = VA_INVALID_ID;
    VAStatus status;

    if (!vaapi)
        return -1;

    if (common_init_decoder(picture_width, picture_height) < 0)
        return -1;

    if (!has_profile(vaapi, profile))
        return -1;
    if (!has_entrypoint(vaapi, profile, entrypoint))
        return -1;

#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        GLXContext * const glx = glx_get_context();

        if (!glx)
            return -1;

        if (glx_init_texture(picture_width, picture_height) < 0)
            return -1;

        if (vaapi_glx_create_surface(glx->texture_target, glx->texture) < 0)
            return -1;
    }
#endif

    if (vaapi->profile != profile || vaapi->entrypoint != entrypoint) {
        if (vaapi->config_id != VA_INVALID_ID)
            vaDestroyConfig(vaapi->display, vaapi->config_id);

        attrib.type = VAConfigAttribRTFormat;
        status = vaGetConfigAttributes(vaapi->display, profile, entrypoint,
                                       &attrib, 1);
        if (!vaapi_check_status(status, "vaGetConfigAttributes()"))
            return -1;
        if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
            return -1;

        status = vaCreateConfig(vaapi->display, profile, entrypoint,
                                &attrib, 1, &config_id);
        if (!vaapi_check_status(status, "vaCreateConfig()"))
            return -1;
    }
    else
        config_id = vaapi->config_id;

    if (vaapi->picture_width != picture_width || vaapi->picture_height != picture_height) {
        if (vaapi->surface_id != VA_INVALID_ID)
            vaDestroySurfaces(vaapi->display, &vaapi->surface_id, 1);

        status = vaCreateSurfaces(vaapi->display, picture_width, picture_height,
                                  VA_RT_FORMAT_YUV420, 1, &surface_id);
        if (!vaapi_check_status(status, "vaCreateSurfaces()"))
            return -1;

        if (vaapi->context_id != VA_INVALID_ID)
            vaDestroyContext(vaapi->display, vaapi->context_id);

        status = vaCreateContext(vaapi->display, config_id,
                                 picture_width, picture_height,
                                 VA_PROGRESSIVE,
                                 &surface_id, 1,
                                 &context_id);
        if (!vaapi_check_status(status, "vaCreateContext()"))
            return -1;
    }
    else {
        context_id = vaapi->context_id;
        surface_id = vaapi->surface_id;
    }

    vaapi->config_id      = config_id;
    vaapi->context_id     = context_id;
    vaapi->surface_id     = surface_id;
    vaapi->profile        = profile;
    vaapi->entrypoint     = entrypoint;
    vaapi->picture_width  = picture_width;
    vaapi->picture_height = picture_height;
    return 0;
}

static int
get_image_format(
    VAAPIContext   *vaapi,
    uint32_t        fourcc,
    VAImageFormat **image_format
)
{
    VAStatus status;
    int i;

    if (image_format)
        *image_format = NULL;

    if (!vaapi->image_formats || vaapi->n_image_formats == 0) {
        vaapi->image_formats = calloc(vaMaxNumImageFormats(vaapi->display),
                                      sizeof(vaapi->image_formats[0]));
        if (!vaapi->image_formats)
            return 0;

        status = vaQueryImageFormats(vaapi->display,
                                     vaapi->image_formats,
                                     &vaapi->n_image_formats);
        if (!vaapi_check_status(status, "vaQueryImageFormats()"))
            return 0;

        D(bug("%d image formats\n", vaapi->n_image_formats));
        for (i = 0; i < vaapi->n_image_formats; i++)
            D(bug("  %s\n", string_of_VAImageFormat(&vaapi->image_formats[i])));
    }

    for (i = 0; i < vaapi->n_image_formats; i++) {
        if (vaapi->image_formats[i].fourcc == fourcc) {
            if (image_format)
                *image_format = &vaapi->image_formats[i];
            return 1;
        }
    }
    return 0;
}

/** Checks whether VAAPI format is RGB */
static int is_vaapi_rgb_format(const VAImageFormat *image_format)
{
    switch (image_format->fourcc) {
    case VA_FOURCC('A','R','G','B'):
    case VA_FOURCC('A','B','G','R'):
    case VA_FOURCC('B','G','R','A'):
    case VA_FOURCC('R','G','B','A'):
        return 1;
    }
    return 0;
}

static int bind_image(VAImage *va_image, Image *image)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAImageFormat * const va_format = &va_image->format;
    VAStatus status;
    void *va_image_data;
    unsigned int i;

    if (va_image->num_planes > MAX_IMAGE_PLANES)
        return -1;

    status = vaMapBuffer(vaapi->display, va_image->buf, &va_image_data);
    if (!vaapi_check_status(status, "vaMapBuffer()"))
        return -1;

    memset(image, 0, sizeof(*image));
    image->format = va_format->fourcc;
    if (is_vaapi_rgb_format(va_format)) {
        image->format = image_rgba_format(
            va_format->bits_per_pixel,
            va_format->byte_order == VA_MSB_FIRST,
            va_format->red_mask,
            va_format->green_mask,
            va_format->blue_mask,
            va_format->alpha_mask
        );
        if (!image->format)
            return -1;
    }

    image->width      = va_image->width;
    image->height     = va_image->height;
    image->num_planes = va_image->num_planes;
    for (i = 0; i < va_image->num_planes; i++) {
        image->pixels[i]  = (uint8_t *)va_image_data + va_image->offsets[i];
        image->pitches[i] = va_image->pitches[i];
    }
    return 0;
}

static int release_image(VAImage *va_image)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAStatus status;

    status = vaUnmapBuffer(vaapi->display, va_image->buf);
    if (!vaapi_check_status(status, "vaUnmapBuffer()"))
        return -1;
    return 0;
}

static const uint32_t image_formats[] = {
    VA_FOURCC('Y','V','1','2'),
    VA_FOURCC('N','V','1','2'),
    VA_FOURCC('U','Y','V','Y'),
    VA_FOURCC('Y','U','Y','V'),
    VA_FOURCC('A','R','G','B'),
    VA_FOURCC('A','B','G','R'),
    VA_FOURCC('B','G','R','A'),
    VA_FOURCC('R','G','B','A'),
    0
};

/** Converts Image format to VAAPI format */
static uint32_t get_vaapi_format(uint32_t format)
{
    uint32_t fourcc;

    /* Only translate image formats we support */
    switch (format) {
    case IMAGE_NV12:
    case IMAGE_YV12:
    case IMAGE_IYUV:
    case IMAGE_I420:
    case IMAGE_AYUV:
    case IMAGE_UYVY:
    case IMAGE_YUY2:
    case IMAGE_YUYV:
    case IMAGE_ARGB:
    case IMAGE_BGRA:
    case IMAGE_RGBA:
    case IMAGE_ABGR:
        fourcc = format;
        break;
    default:
        fourcc = 0;
        break;
    }
    return fourcc;
}

static int get_image(VASurfaceID surface, Image *dst_img)
{
    CommonContext * const common = common_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();
    VAImage image;
    VAImageFormat *image_format = NULL;
    VAStatus status;
    Image bound_image;
    int i, is_bound_image = 0, is_derived_image = 0, error = -1;

    image.image_id = VA_INVALID_ID;
    image.buf      = VA_INVALID_ID;

    if (common->getimage_format) {
        uint32_t fourcc = get_vaapi_format(common->getimage_format);
        if (!fourcc || !get_image_format(vaapi, fourcc, &image_format))
            goto end;
    }

    if (!image_format && common->vaapi_derive_image) {
        status = vaDeriveImage(vaapi->display, surface, &image);
        if (vaapi_check_status(status, "vaDeriveImage()")) {
            if (image.image_id != VA_INVALID_ID && image.buf != VA_INVALID_ID) {
                D(bug("using vaDeriveImage()\n"));
                is_derived_image = 1;
                image_format = &image.format;
            }
            else {
                D(bug("vaDeriveImage() returned success but VA image is invalid. Trying vaGetImage()\n"));
            }
        }
    }

    if (!image_format) {
        for (i = 0; image_formats[i] != 0; i++) {
            if (get_image_format(vaapi, image_formats[i], &image_format))
                break;
        }
    }

    if (!image_format)
        goto end;
    D(bug("selected %s image format for getimage\n",
          string_of_VAImageFormat(image_format)));

    if (!is_derived_image) {
        status = vaCreateImage(vaapi->display, image_format,
                               vaapi->picture_width, vaapi->picture_height,
                               &image);
        if (!vaapi_check_status(status, "vaCreateImage()"))
            goto end;
        D(bug("created image with id 0x%08x and buffer id 0x%08x\n",
              image.image_id, image.buf));

        VARectangle src_rect;
        if (common->use_getimage_rect) {
            Rectangle * const img_rect = &common->getimage_rect;
            if (img_rect->x < 0 || img_rect->y >= vaapi->picture_width)
                goto end;
            if (img_rect->y < 0 || img_rect->y >= vaapi->picture_height)
                goto end;
            if (img_rect->x + img_rect->width > vaapi->picture_width)
                goto end;
            if (img_rect->y + img_rect->height > vaapi->picture_height)
                goto end;
            src_rect.x      = img_rect->x;
            src_rect.y      = img_rect->y;
            src_rect.width  = img_rect->width;
            src_rect.height = img_rect->height;
        }
        else {
            src_rect.x      = 0;
            src_rect.y      = 0;
            src_rect.width  = vaapi->picture_width;
            src_rect.height = vaapi->picture_height;
        }
        D(bug("src rect (%d,%d):%ux%u\n",
              src_rect.x, src_rect.y, src_rect.width, src_rect.height));

        status = vaGetImage(
            vaapi->display, vaapi->surface_id,
            src_rect.x, src_rect.y, src_rect.width, src_rect.height,
            image.image_id
        );
        if (!vaapi_check_status(status, "vaGetImage()")) {
            vaDestroyImage(vaapi->display, image.image_id);
            goto end;
        }
    }

    if (bind_image(&image, &bound_image) < 0)
        goto end;
    is_bound_image = 1;

    if (image_convert(dst_img, &bound_image) < 0)
        goto end;

    error = 0;
end:
    if (is_bound_image) {
        if (release_image(&image) < 0)
            error = -1;
    }

    if (image.image_id != VA_INVALID_ID) {
        status = vaDestroyImage(vaapi->display, image.image_id);
        if (!vaapi_check_status(status, "vaDestroyImage()"))
            error = -1;
    }
    return error;
}

static inline int vaapi_decode_to_image(void)
{
    CommonContext * const common = common_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();

    return get_image(vaapi->surface_id, common->image);
}

static int put_image(VASurfaceID surface, Image *img)
{
    CommonContext * const common = common_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();
    VAImageFormat *va_image_format = NULL;
    VAImage va_image;
    VAStatus status;
    Image bound_image;
    int i, w, h, is_bound_image = 0, is_derived_image = 0, error = -1;

    va_image.image_id = VA_INVALID_ID;
    va_image.buf      = VA_INVALID_ID;

    if (common->putimage_format) {
        uint32_t fourcc = get_vaapi_format(common->putimage_format);
        if (!fourcc || !get_image_format(vaapi, fourcc, &va_image_format))
            goto end;
    }

    if (!va_image_format && common->vaapi_derive_image) {
        status = vaDeriveImage(vaapi->display, surface, &va_image);
        if (vaapi_check_status(status, "vaDeriveImage()")) {
            if (va_image.image_id != VA_INVALID_ID && va_image.buf != VA_INVALID_ID) {
                D(bug("using vaDeriveImage()\n"));
                is_derived_image = 1;
                va_image_format = &va_image.format;
            }
            else {
                D(bug("vaDeriveImage() returned success but VA image is invalid. Trying vaPutImage()\n"));
            }
        }
    }

    if (!va_image_format) {
        for (i = 0; image_formats[i] != 0; i++) {
            if (get_image_format(vaapi, image_formats[i], &va_image_format))
                break;
        }
    }

    if (!va_image_format)
        goto end;
    D(bug("selected %s image format for putimage in override mode\n",
          string_of_VAImageFormat(va_image_format)));

    if (!is_derived_image) {
        if (common->vaapi_putimage_scaled) {
            /* Let vaPutImage() scale the image, though only a very few
               drivers support that */
            w = img->width;
            h = img->height;
        }
        else {
            w = vaapi->picture_width;
            h = vaapi->picture_height;
        }

        status = vaCreateImage(vaapi->display, va_image_format, w, h, &va_image);
        if (!vaapi_check_status(status, "vaCreateImage()"))
            goto end;
        D(bug("created image with id 0x%08x and buffer id 0x%08x\n",
              va_image.image_id, va_image.buf));
    }

    if (bind_image(&va_image, &bound_image) < 0)
        goto end;
    is_bound_image = 1;
    if (image_convert(&bound_image, img) < 0)
        goto end;
    is_bound_image = 0;
    if (release_image(&va_image) < 0)
        goto end;

    if (!is_derived_image) {
        status = vaPutImage2(vaapi->display, surface, va_image.image_id,
                             0, 0, va_image.width, va_image.height,
                             0, 0, vaapi->picture_width, vaapi->picture_height);
        if (!vaapi_check_status(status, "vaPutImage()"))
            goto end;
    }
    error = 0;
end:
    if (is_bound_image) {
        if (release_image(&va_image) < 0)
            error = -1;
    }

    if (va_image.image_id != VA_INVALID_ID) {
        status = vaDestroyImage(vaapi->display, va_image.image_id);
        if (!vaapi_check_status(status, "vaDestroyImage()"))
            error = -1;
    }
    return error;
}

static const uint32_t subpic_formats[] = {
    VA_FOURCC('A','R','G','B'),
    VA_FOURCC('A','B','G','R'),
    VA_FOURCC('B','G','R','A'),
    VA_FOURCC('R','G','B','A'),
    0
};

static int
get_subpic_format(
    VAAPIContext   *vaapi,
    uint32_t        fourcc,
    VAImageFormat **format,
    unsigned int   *flags
)
{
    VAStatus status;
    unsigned int i;

    if (format)
        *format = NULL;

    if (flags)
        *flags = 0;

    if (!vaapi->subpic_formats) {
        vaapi->n_subpic_formats = vaMaxNumSubpictureFormats(vaapi->display);

        vaapi->subpic_formats = calloc(vaapi->n_subpic_formats,
                                       sizeof(vaapi->subpic_formats[0]));
        if (!vaapi->subpic_formats)
            return 0;

        vaapi->subpic_flags = calloc(vaapi->n_subpic_formats,
                                     sizeof(vaapi->subpic_flags[0]));
        if (!vaapi->subpic_flags)
            return 0;

        status = vaQuerySubpictureFormats(vaapi->display,
                                          vaapi->subpic_formats,
                                          vaapi->subpic_flags,
                                          &vaapi->n_subpic_formats);
        if (!vaapi_check_status(status, "vaQuerySubpictureFormats()"))
            return 0;

        D(bug("%d subpicture formats\n", vaapi->n_subpic_formats));
        for (i = 0; i < vaapi->n_subpic_formats; i++) {
            VAImageFormat *const image_format = &vaapi->subpic_formats[i];
            D(bug("  %s, flags 0x%x\n",
                  string_of_VAImageFormat(image_format),
                  vaapi->subpic_flags[i]));
            if (is_vaapi_rgb_format(image_format))
                D(bug("    byte-order %s, %d-bit, depth %d, r/g/b/a 0x%08x/0x%08x/0x%08x/0x%08x\n",
                      image_format->byte_order == VA_MSB_FIRST ? "MSB" : "LSB",
                      image_format->bits_per_pixel,
                      image_format->depth,
                      image_format->red_mask,
                      image_format->green_mask,
                      image_format->blue_mask,
                      image_format->alpha_mask));
        }
    }

    for (i = 0; i < vaapi->n_subpic_formats; i++) {
        if (vaapi->subpic_formats[i].fourcc == fourcc) {
            if (format)
                *format = &vaapi->subpic_formats[i];
            if (flags)
                *flags = vaapi->subpic_flags[i];
            return 1;
        }
    }
    return 0;
}

static int blend_image(VASurfaceID surface, Image *img)
{
    CommonContext * const common = common_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();
    unsigned int subpic_count, subpic_count_x, subpic_count_y, i, j;
    unsigned int subpic_flags = 0;
    VAImageFormat *subpic_format = NULL;
    VAStatus status;
    Image bound_image;
    int is_bound_image = 0, error = -1;
    Rectangle src_rect, dst_rect, srect, drect;

    vaapi->subpic_image.image_id = VA_INVALID_ID;
    vaapi->subpic_image.buf      = VA_INVALID_ID;

    if (common->putimage_format) {
        uint32_t fourcc = get_vaapi_format(common->putimage_format);
        if (!fourcc || !get_subpic_format(vaapi, fourcc, &subpic_format, &subpic_flags))
            goto end;
    }

    if (!subpic_format) {
        for (i = 0; subpic_formats[i] != 0; i++) {
            if (get_subpic_format(vaapi, subpic_formats[i], &subpic_format, &subpic_flags))
                break;
        }
    }
    if (!subpic_format)
        goto end;

    /* Check HW supports the expected subpicture features */
    if (common->use_vaapi_subpicture_flags) {
        static const struct {
            unsigned int flags;
            const char *name;
        } g_flags[] = {
            { VA_SUBPICTURE_GLOBAL_ALPHA,
              "VA_SUBPICTURE_GLOBAL_ALPHA" },
#ifdef VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD
            { VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD,
              "VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD" },
#endif
            { 0, }
        };

        const unsigned int missing_flags =
            common->vaapi_subpicture_flags & ~subpic_flags;

        for (i = 0; g_flags[i].flags != 0; i++) {
            if (missing_flags & g_flags[i].flags) {
                D(bug("driver does not support %s subpicture flag\n",
                      g_flags[i].name));
                goto end;
            }
        }
    }

    D(bug("selected %s subpicture format for putimage in blend mode\n",
          string_of_VAImageFormat(subpic_format)));

    status = vaCreateImage(vaapi->display, subpic_format,
                           img->width, img->height, &vaapi->subpic_image);
    if (!vaapi_check_status(status, "vaCreateImage()"))
        goto end;
    D(bug("created image with id 0x%08x and buffer id 0x%08x\n",
          vaapi->subpic_image.image_id, vaapi->subpic_image.buf));

    if (bind_image(&vaapi->subpic_image, &bound_image) < 0)
        goto end;
    is_bound_image = 1;
    if (image_convert(&bound_image, img) < 0)
        goto end;
    is_bound_image = 0;
    if (release_image(&vaapi->subpic_image) < 0)
        goto end;

    subpic_count = 1;
    if (common->vaapi_multi_subpictures)
        subpic_count = ARRAY_ELEMS(vaapi->subpic_ids);

    for (i = 0; i < subpic_count; i++) {
        status = vaCreateSubpicture(
            vaapi->display,
            vaapi->subpic_image.image_id,
            &vaapi->subpic_ids[i]
        );
        if (!vaapi_check_status(status, "vaCreateSubpicture()"))
            goto end;
        D(bug("created subpicture with id 0x%08x\n", vaapi->subpic_ids[i]));

        if (common->use_vaapi_subpicture_flags) {

            if (common->vaapi_subpicture_flags & VA_SUBPICTURE_GLOBAL_ALPHA) {
                status = vaSetSubpictureGlobalAlpha(
                    vaapi->display,
                    vaapi->subpic_ids[i],
                    common->vaapi_subpicture_alpha
                );
                if (!vaapi_check_status(status, "vaSetSubpictureGlobalAlpha()"))
                    goto end;
            }
        }
    }

    if (common->use_vaapi_subpicture_source_rect)
        src_rect = common->vaapi_subpicture_source_rect;
    else {
        src_rect.x      = 0;
        src_rect.y      = 0;
        src_rect.width  = img->width;
        src_rect.height = img->height;
    }

    if (common->use_vaapi_subpicture_target_rect)
        dst_rect = common->vaapi_subpicture_target_rect;
    else {
        dst_rect.x      = 0;
        dst_rect.y      = 0;
        dst_rect.width  = vaapi->picture_width;
        dst_rect.height = vaapi->picture_height;
    }

    D(bug("render %d subpicture%s from (%d,%d):%ux%u to (%d,%d):%ux%u\n",
          subpic_count, subpic_count > 1 ? "s" : "",
          src_rect.x, src_rect.y, src_rect.width, src_rect.height,
          dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height));

    subpic_count_y = subpic_count / 2;
    if (subpic_count_y == 0)
        subpic_count_y = 1;

    subpic_flags = 0;
    if (common->use_vaapi_subpicture_flags)
        subpic_flags = common->vaapi_subpicture_flags;

    for (j = 0; j < subpic_count_y; j++) {
        subpic_count_x = subpic_count / subpic_count_y;
        if (j == subpic_count_y - 1)
            subpic_count_x += subpic_count % subpic_count_y; /* 0/1 */

        for (i = 0; i < subpic_count_x; i++) {
            srect.x      = src_rect.x + (i * src_rect.width) / subpic_count_x;
            srect.y      = src_rect.y + (j * src_rect.height) / subpic_count_y;
            srect.width  = src_rect.width / subpic_count_x;
            srect.height = src_rect.height / subpic_count_y;
            drect.x      = dst_rect.x + (i * dst_rect.width) / subpic_count_x;
            drect.y      = dst_rect.y + (j * dst_rect.height) / subpic_count_y;
            drect.width  = dst_rect.width / subpic_count_x;
            drect.height = dst_rect.height / subpic_count_y;

            if (i == subpic_count_x - 1) {
                srect.width  = (src_rect.x + src_rect.width) - srect.x;
                drect.width  = (dst_rect.x + dst_rect.width) - drect.x;
            }

            if (j == subpic_count_y - 1) {
                srect.height = (src_rect.y + src_rect.height) - srect.y;
                drect.height = (dst_rect.y + dst_rect.height) - drect.y;
            }

            if (subpic_count > 1)
                D(bug("  id 0x%08x: src (%d,%d):%ux%u, dst (%d,%d):%ux%u\n",
                      vaapi->subpic_ids[j * subpic_count_y + i],
                      srect.x, srect.y, srect.width, srect.height,
                      drect.x, drect.y, drect.width, drect.height));

            status = vaAssociateSubpicture2(
                vaapi->display,
                vaapi->subpic_ids[j * subpic_count_y + i],
                &surface, 1,
                srect.x, srect.y, srect.width, srect.height,
                drect.x, drect.y, drect.width, drect.height,
                subpic_flags
            );
            if (!vaapi_check_status(status, "vaAssociateSubpicture()"))
                goto end;
        }
    }
    error = 0;
end:
    if (is_bound_image) {
        if (release_image(&vaapi->subpic_image) < 0)
            error = -1;
    }
    return error;
}

int vaapi_decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VABufferID va_buffers[4];
    unsigned int n_va_buffers = 0;
    VAStatus status;

    if (!vaapi)
        return -1;
    if (vaapi->context_id == VA_INVALID_ID)
        return -1;
    if (vaapi->surface_id == VA_INVALID_ID)
        return -1;

    if (commit_slices(vaapi) < 0)
        return -1;

    vaUnmapBuffer(vaapi->display, vaapi->pic_param_buf_id);
    va_buffers[n_va_buffers++] = vaapi->pic_param_buf_id;

    if (vaapi->iq_matrix_buf_id != VA_INVALID_ID) {
        vaUnmapBuffer(vaapi->display, vaapi->iq_matrix_buf_id);
        va_buffers[n_va_buffers++] = vaapi->iq_matrix_buf_id;
    }

    if (vaapi->bitplane_buf_id != VA_INVALID_ID) {
        vaUnmapBuffer(vaapi->display, vaapi->bitplane_buf_id);
        va_buffers[n_va_buffers++] = vaapi->bitplane_buf_id;
    }

    if (vaapi->huf_table_buf_id != VA_INVALID_ID) {
        vaUnmapBuffer(vaapi->display, vaapi->huf_table_buf_id);
        va_buffers[n_va_buffers++] = vaapi->huf_table_buf_id;
    }

    status = vaBeginPicture(vaapi->display, vaapi->context_id,
                            vaapi->surface_id);
    if (!vaapi_check_status(status, "vaBeginPicture()"))
        return -1;

    status = vaRenderPicture(vaapi->display, vaapi->context_id,
                             va_buffers, n_va_buffers);
    if (!vaapi_check_status(status, "vaRenderPicture()"))
        return -1;

    status = vaRenderPicture(vaapi->display, vaapi->context_id,
                             vaapi->slice_buf_ids,
                             vaapi->n_slice_buf_ids);
    if (!vaapi_check_status(status, "vaRenderPicture()"))
        return -1;

    status = vaEndPicture(vaapi->display, vaapi->context_id);
    if (!vaapi_check_status(status, "vaEndPicture()"))
        return -1;

    if (getimage_mode() == GETIMAGE_FROM_VIDEO)
        return vaapi_decode_to_image();

    return 0;
}

#if USE_X11
static int vaapi_display_cliprects(void)
{
    CommonContext * const common = common_get_context();
    X11Context * const x11 = x11_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();
    VASurfaceID clip_surface_id = VA_INVALID_ID;
    VARectangle *cliprects = NULL;
    VAStatus status;
    int i, error = 1;

    if (!common->use_clipping)
        return 0;

    status = vaCreateSurfaces(vaapi->display,
                              vaapi->picture_width,
                              vaapi->picture_height,
                              VA_RT_FORMAT_YUV420, 1, &clip_surface_id);
    if (!vaapi_check_status(status, "vaCreateSurfaces()"))
        goto end;

    if (put_image(clip_surface_id, common->cliprects_image) < 0)
        goto end;

    cliprects = malloc(common->cliprects_count * sizeof(cliprects[0]));
    if (!cliprects)
        goto end;
    for (i = 0; i < common->cliprects_count; i++) {
        cliprects[i].x      = common->cliprects[i].x;
        cliprects[i].y      = common->cliprects[i].y;
        cliprects[i].width  = common->cliprects[i].width;
        cliprects[i].height = common->cliprects[i].height;
    }

    status = vaPutSurface(vaapi->display, clip_surface_id, x11->window,
                          0, 0, vaapi->picture_width, vaapi->picture_height,
                          0, 0, x11->window_width, x11->window_height,
                          cliprects, common->cliprects_count,
                          VA_FRAME_PICTURE);
    if (!vaapi_check_status(status, "vaPutSurface()"))
        goto end;

    error = 0;
end:
    if (cliprects)
        free(cliprects);
    if (clip_surface_id != VA_INVALID_ID)
        vaDestroySurfaces(vaapi->display, &clip_surface_id, 1);
    if (error)
        return -1;
    return 0;
}
#endif

int vaapi_display(void)
{
    CommonContext * const common = common_get_context();
    VAAPIContext * const vaapi = vaapi_get_context();
#if USE_X11
    X11Context * const x11 = x11_get_context();
    unsigned int vaPutSurface_count = 0;
    unsigned int flags = VA_FRAME_PICTURE;
    VAStatus status;
    Drawable drawable;
#endif

    if (!common || !vaapi)
        return -1;

    if (putimage_mode() != PUTIMAGE_NONE) {
        Image *img;
        img = image_generate(
            common->putimage_size.width,
            common->putimage_size.height
        );
        if (img) {
            switch (putimage_mode()) {
            case PUTIMAGE_OVERRIDE:
                if (put_image(vaapi->surface_id, img) < 0)
                    return -1;
                break;
            case PUTIMAGE_BLEND:
                if (blend_image(vaapi->surface_id, img) < 0)
                    return -1;
                break;
            default:
                break;
            }
            image_destroy(img);
        }
    }

    /* XXX: video and output surfaces are the same for VA API */
    if (getimage_mode() == GETIMAGE_FROM_OUTPUT) {
        if (vaapi_decode_to_image() < 0)
            return -1;
        return 0;
    }

    if (getimage_mode() == GETIMAGE_FROM_VIDEO)
        return 0;

    if (display_type() == DISPLAY_DRM)
        return 0;

#if USE_X11
    if (getimage_mode() == GETIMAGE_FROM_PIXMAP)
        drawable = x11->pixmap;
    else
        drawable = x11->window;

    if (common->use_vaapi_putsurface_flags)
        flags = common->vaapi_putsurface_flags;

#if USE_VAAPI_GLX
    if (display_type() == DISPLAY_GLX) {
        vaapi->use_glx_copy = common->vaapi_glx_use_copy;
        if (vaapi->use_glx_copy) {
            status = vaCopySurfaceGLX(vaapi->display,
                                      vaapi->glx_surface,
                                      vaapi->surface_id,
                                      flags);

            if (status == VA_STATUS_ERROR_UNIMPLEMENTED) {
                printf("VAAPI: vaCopySurfaceGLX() is not implemented\n");
                vaapi->use_glx_copy = 0;
            }
            else {
                printf("VAAPI: use vaCopySurfaceGLX()\n");
                if (!vaapi_check_status(status, "vaCopySurfaceGLX()"))
                    return -1;
            }
        }
    }
    else
#endif
    {
        Rectangle src_rect, dst_rect;

        if (common->use_vaapi_putsurface_source_rect)
            src_rect = common->vaapi_putsurface_source_rect;
        else {
            src_rect.x      = 0;
            src_rect.y      = 0;
            src_rect.width  = vaapi->picture_width;
            src_rect.height = vaapi->picture_height;
        }

        if (common->use_vaapi_putsurface_target_rect)
            dst_rect = common->vaapi_putsurface_target_rect;
        else {
            dst_rect.x      = 0;
            dst_rect.y      = 0;
            dst_rect.width  = x11->window_width;
            dst_rect.height = x11->window_height;
        }

        if (common->use_vaapi_background_color)
            flags |= VA_CLEAR_DRAWABLE;

        status = vaPutSurface(vaapi->display, vaapi->surface_id, drawable,
                              src_rect.x, src_rect.y,
                              src_rect.width, src_rect.height,
                              dst_rect.x, dst_rect.y,
                              dst_rect.width, dst_rect.height,
                              NULL, 0, flags);
        if (!vaapi_check_status(status, "vaPutSurface()"))
            return -1;
        ++vaPutSurface_count;

        /* Clear the drawable only the first time */
        if (common->use_vaapi_background_color && !common->use_vaapi_putsurface_flags)
            flags &= ~VA_CLEAR_DRAWABLE;

        if (common->multi_rendering) {
            status = vaPutSurface(vaapi->display, vaapi->surface_id, drawable,
                                  0, 0,
                                  vaapi->picture_width, vaapi->picture_height,
                                  x11->window_width/2, 0,
                                  x11->window_width/2, x11->window_height/2,
                                  NULL, 0, flags);
            if (!vaapi_check_status(status, "vaPutSurface() for multirendering"))
                return -1;
            ++vaPutSurface_count;

            status = vaPutSurface(vaapi->display, vaapi->surface_id, drawable,
                                  0, 0,
                                  vaapi->picture_width, vaapi->picture_height,
                                  0, x11->window_height/2,
                                  x11->window_width/2, x11->window_height/2,
                                  NULL, 0, flags);
            if (!vaapi_check_status(status, "vaPutSurface() for multirendering"))
                return -1;
            ++vaPutSurface_count;
        }
    }

    if (vaapi_display_cliprects() < 0)
        return -1;

    if (common->use_subwindow_rect) {
        status = vaPutSurface(vaapi->display, vaapi->surface_id,
                              x11->subwindow,
                              0, 0, vaapi->picture_width, vaapi->picture_height,
                              0, 0,
                              common->subwindow_rect.width,
                              common->subwindow_rect.height,
                              NULL, 0, 0);
        if (!vaapi_check_status(status, "vaPutSurface()"))
            return -1;
        ++vaPutSurface_count;
    }

    if (vaPutSurface_count > 1) {
        /* We don't have to call vaSyncSurface() explicitly. However,
           if we use multiple vaPutSurface() and subwindows, we probably
           want the surfaces to be presented at the same time */
        status = vaSyncSurface(vaapi->display, vaapi->context_id,
                               vaapi->surface_id);
        if (!vaapi_check_status(status, "vaSyncSurface() for display"))
            return -1;
    }
#endif
    return 0;
}

#ifndef USE_FFMPEG
int pre(void)
{
    VADisplay dpy;

    if (hwaccel_type() != HWACCEL_VAAPI)
        return -1;

    switch (display_type()) {
#if USE_X11
    case DISPLAY_X11:
        if (x11_init() < 0)
            return -1;
        break;
#endif
#if USE_GLX
    case DISPLAY_GLX:
        if (glx_init() < 0)
            return -1;
        break;
#endif
#if USE_DRM
    case DISPLAY_DRM:
        if (drm_init() < 0)
            return -1;
        break;
#endif
    default:
        fprintf(stderr, "ERROR: unsupported display type\n");
        return -1;
    }

    switch (display_type()) {
#if USE_VAAPI_GLX
    case DISPLAY_GLX: {
        X11Context * const x11 = x11_get_context();
        if (!x11)
            return -1;
        dpy = vaGetDisplayGLX(x11->display);
        break;
    }
#endif
#if USE_VAAPI_X11
    case DISPLAY_X11: {
        X11Context * const x11 = x11_get_context();
        if (!x11)
            return -1;
        dpy = vaGetDisplay(x11->display);
        break;
    }
#endif
#if USE_VAAPI_DRM
    case DISPLAY_DRM: {
        DRMContext * const drm = drm_get_context();
        if (!drm)
            return -1;
        dpy = vaGetDisplayDRM(drm->drm_device);
        break;
    }
#endif
    default:
        dpy = NULL;
        break;
    }
    return vaapi_init(dpy);
}

int post(void)
{
    if (vaapi_exit() < 0)
        return -1;

    switch (display_type()) {
#if USE_GLX
    case DISPLAY_GLX:
        if (glx_exit() < 0)
            return -1;
        /* fall-through */
#endif
#if USE_X11
    case DISPLAY_X11:
        if (x11_exit() < 0)
            return -1;
        break;
#endif
#if USE_DRM
    case DISPLAY_DRM:
        if (drm_exit() < 0)
            return -1;
        break;
#endif
    default:
        fprintf(stderr, "ERROR: unsupported display type\n");
        return -1;
    }
    return 0;
}

int display(void)
{
    if (vaapi_display() < 0)
        return -1;

    switch (display_type()) {
#if USE_GLX
    case DISPLAY_GLX:
        return glx_display();
#endif
#if USE_X11
    case DISPLAY_X11:
        return x11_display();
#endif
#if USE_DRM
    case DISPLAY_DRM:
        return drm_display();
#endif
    default:
        fprintf(stderr, "ERROR: unsupported display type\n");
        return -1;
    }
    return 0;
}
#endif

int vaapi_glx_create_surface(unsigned int target, unsigned int texture)
{
#if USE_VAAPI_GLX
    VAAPIContext * const vaapi = vaapi_get_context();
    VAStatus status;

    status = vaCreateSurfaceGLX(vaapi->display, target, texture,
                                &vaapi->glx_surface);
    if (vaapi_check_status(status, "vaCreateSurfaceGLX()"))
        return 0;
#endif
    return -1;
}

void vaapi_glx_destroy_surface(void)
{
#if USE_VAAPI_GLX
    VAAPIContext * const vaapi = vaapi_get_context();
    VAStatus status;

    status = vaDestroySurfaceGLX(vaapi->display, vaapi->glx_surface);
    if (vaapi_check_status(status, "vaDestroySurfaceGLX()"))
        return;
#endif
}

int vaapi_glx_begin_render_surface(void)
{
#if USE_VAAPI_GLX
    VAAPIContext * const vaapi = vaapi_get_context();

    if (vaapi->use_glx_copy)
        return 0;

    printf("VAAPI: use vaBeginRenderSurfaceGLX()\n");

#if HAS_VAAPI_GLX_BIND
    VAStatus status = vaAssociateSurfaceGLX(vaapi->display,
                                            vaapi->glx_surface,
                                            vaapi->surface_id,
                                            VA_FRAME_PICTURE);
    if (!vaapi_check_status(status, "vaAssociateSurfaceGLX()"))
        return -1;

    status = vaBeginRenderSurfaceGLX(vaapi->display, vaapi->glx_surface);
    if (!vaapi_check_status(status, "vaBeginRenderSurfaceGLX()"))
        return -1;
    return 0;
#endif
    fprintf(stderr, "ERROR: not implemented\n");
#endif
    return -1;
}

int vaapi_glx_end_render_surface(void)
{
#if USE_VAAPI_GLX
    VAAPIContext * const vaapi = vaapi_get_context();

    if (vaapi->use_glx_copy)
        return 0;

#if HAS_VAAPI_GLX_BIND
    VAStatus status = vaEndRenderSurfaceGLX(vaapi->display, vaapi->glx_surface);
    if (!vaapi_check_status(status, "vaEndRenderSurfaceGLX()"))
        return -1;

    status = vaDeassociateSurfaceGLX(vaapi->display, vaapi->glx_surface);
    if (!vaapi_check_status(status, "vaDeassociateSurfaceGLX()"))
        return -1;
    return 0;
#endif
    fprintf(stderr, "ERROR: not implemented\n");
#endif
    return -1;
}
