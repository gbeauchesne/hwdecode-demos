/*
 *  vdpau.c - VDPAU common code
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
#include "vdpau.h"
#include "vdpau_gate.h"
#include "common.h"
#include "utils.h"
#include "x11.h"

#if USE_GLX
#include "glx.h"
#endif

#define DEBUG 1
#include "debug.h"

static VDPAUContext *vdpau_context;

/* Define wait delay (in microseconds) between two
   VdpPresentationQueueQuerySurfaceStatus() calls */
#define VDPAU_SYNC_DELAY 10

#define VDPAU_MAX_PARAMS        10
#define VDPAU_MAX_ATTRS         10
#define VDPAU_MAX_FEATURES      20

static const struct {
    uint32_t format;
    uint32_t vdp_format;
}
image_formats[] = {
    { IMAGE_YV12,       VDP_YCBCR_FORMAT_YV12           },
    { IMAGE_NV12,       VDP_YCBCR_FORMAT_NV12           },
    { IMAGE_RGB32,      VDP_RGBA_FORMAT_B8G8R8A8        },
    { 0, VDP_INVALID_HANDLE }
};

/** Converts Image format to VDPAU format */
static uint32_t vdpau_get_format(uint32_t format)
{
    unsigned int i;
    for (i = 0; image_formats[i].format != 0; i++) {
        if (image_formats[i].format == format)
            return image_formats[i].vdp_format;
    }
    return VDP_INVALID_HANDLE;
}

/** Converts VDPAU format to Image format */
static uint32_t image_get_yuv_format(uint32_t vdp_format)
{
    unsigned int i;
    for (i = 0; image_formats[i].format != 0; i++) {
        if (image_formats[i].vdp_format == vdp_format &&
            !IS_RGB_IMAGE_FORMAT(image_formats[i].format))
            return image_formats[i].format;
    }
    return 0;
}

/** Converts VDPAU format to Image format */
static uint32_t image_get_rgb_format(uint32_t vdp_format)
{
    unsigned int i;
    for (i = 0; image_formats[i].format != 0; i++) {
        if (image_formats[i].vdp_format == vdp_format &&
            IS_RGB_IMAGE_FORMAT(image_formats[i].format))
            return image_formats[i].format;
    }
    return 0;
}

/** Checks whether the VDPAU implementation supports the specified profile */
static VdpBool
vdpau_is_supported_profile(VdpDevice device, VdpDecoderProfile profile)
{
    VdpBool is_supported = VDP_FALSE;
    uint32_t max_level, max_references, max_width, max_height;
    VdpStatus status;

    status = vdpau_decoder_query_capabilities(
        device,
        profile,
        &is_supported,
        &max_level,
        &max_references,
        &max_width,
        &max_height
    );
    return (vdpau_check_status(status, "VdpDecoderQueryCapabilities()") &&
            is_supported);
}

/** Checks whether the VDPAU implementation supports the specified YUV format */
static VdpBool
vdpau_is_supported_ycbcr_format(VdpDevice device, VdpYCbCrFormat format)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus status;

    status = vdpau_video_surface_query_ycbcr_caps(
        device,
        VDP_CHROMA_TYPE_420,
        format,
        &is_supported
    );
    return (vdpau_check_status(status, "VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities()") &&
            is_supported);
}

/** Checks whether the VDPAU implementation supports the specified RGBA bitmap format */
static VdpBool
vdpau_is_supported_bitmap_format(VdpDevice device, VdpRGBAFormat format)
{
    VdpBool is_supported = VDP_FALSE;
    uint32_t max_width, max_height;
    VdpStatus status;

    status = vdpau_bitmap_surface_query_capabilities(
        device,
        format,
        &is_supported,
        &max_width,
        &max_height
    );
    return (vdpau_check_status(status, "VdpBitmapSurfaceQueryCapabilities()") &&
            is_supported);
}

/** Returns the maximum dimensions supported by the VDPAU implementation for that profile */
static VdpBool
vdpau_get_surface_size_max(
    VdpDevice           device,
    VdpDecoderProfile   profile,
    uint32_t           *pmax_width,
    uint32_t           *pmax_height
)
{
    VdpBool is_supported = VDP_FALSE;
    uint32_t max_level, max_references, max_width, max_height;
    VdpStatus status;

    status = vdpau_decoder_query_capabilities(
        device,
        profile,
        &is_supported,
        &max_level,
        &max_references,
        &max_width,
        &max_height
    );

    if (pmax_width)
        *pmax_width = 0;
    if (pmax_height)
        *pmax_height = 0;

    if (!vdpau_check_status(status, "VdpDecoderQueryCapabilities()") ||
        !is_supported)
        return VDP_FALSE;

    if (pmax_width)
        *pmax_width = max_width;
    if (max_height)
        *pmax_height = max_height;

    return VDP_TRUE;
}

/** Checks wether video mixer supports a specific feature */
static VdpBool
vdpau_video_mixer_has_feature(VdpDevice device, VdpVideoMixerFeature feature)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus status;

    status = vdpau_video_mixer_query_feature_support(
        device,
        feature,
        &is_supported
    );
    return (vdpau_check_status(status, "VdpVideoMixerQueryFeatureSupport()") &&
            is_supported);
}

/** Checks wheter video mixer supports a specific attribute */
static VdpBool
vdpau_video_mixer_has_attribute(VdpDevice device, VdpVideoMixerAttribute attribute)
{
    VdpBool is_supported = VDP_FALSE;
    VdpStatus status;

    status = vdpau_video_mixer_query_attribute_support(
        device,
        attribute,
        &is_supported
    );
    return (vdpau_check_status(status, "VdpVideoMixerQueryAttributeSupport()") &&
            is_supported);
}

/** Gets video mixer parameter value */
static VdpBool
vdpau_get_video_mixer_param_range(
    VdpDevice               device,
    VdpVideoMixerParameter  param,
    uint32_t               *pmin_value,
    uint32_t               *pmax_value
)
{
    VdpBool is_supported = VDP_FALSE;
    uint32_t min_value, max_value;
    VdpStatus status;

    status = vdpau_video_mixer_query_parameter_support(
        device,
        param,
        &is_supported
    );
    if (!vdpau_check_status(status, "VdpVideoMixerQueryParameterSupport()") ||
        !is_supported)
        return VDP_FALSE;

    status = vdpau_video_mixer_query_parameter_value_range(
        device,
        param,
        &min_value,
        &max_value
    );
    if (!vdpau_check_status(status, "VdpVideoMixerQueryParameterValueRange()"))
        return VDP_FALSE;

    if (pmin_value)
        *pmin_value = min_value;
    if (pmax_value)
        *pmax_value = max_value;
    return VDP_TRUE;
}

static void destroy_bitstream_buffers(VDPAUContext *vdpau)
{
    if (vdpau->bitstream_buffers) {
        free(vdpau->bitstream_buffers);
        vdpau->bitstream_buffers = NULL;
        vdpau->bitstream_buffers_count = 0;
        vdpau->bitstream_buffers_count_max = 0;
    }
}

static VdpBitstreamBuffer *create_bitstream_buffer(VDPAUContext *vdpau)
{
    VdpBitstreamBuffer *bitstream_buffers;

    if (vdpau->bitstream_buffers_count + 1 > vdpau->bitstream_buffers_count_max) {
        vdpau->bitstream_buffers_count_max += 16;
        bitstream_buffers = realloc(
            vdpau->bitstream_buffers,
            (vdpau->bitstream_buffers_count_max *
             sizeof(bitstream_buffers[0]))
        );
        if (!bitstream_buffers)
            return NULL;
        vdpau->bitstream_buffers = bitstream_buffers;
    }
    return &vdpau->bitstream_buffers[vdpau->bitstream_buffers_count++];
}

static void destroy_flip_queue(VDPAUContext *vdpau)
{
    if (vdpau->flip_queue != VDP_INVALID_HANDLE) {
        vdpau_presentation_queue_destroy(vdpau->flip_queue);
        vdpau->flip_queue = VDP_INVALID_HANDLE;
    }

    if (vdpau->flip_target != VDP_INVALID_HANDLE) {
        vdpau_presentation_queue_target_destroy(vdpau->flip_target);
        vdpau->flip_target = VDP_INVALID_HANDLE;
    }
}

static int create_flip_queue(VDPAUContext *vdpau, Drawable drawable)
{
    VdpPresentationQueue flip_queue;
    VdpPresentationQueueTarget flip_target;
    VdpStatus status;

    status = vdpau_presentation_queue_target_create_x11(
        vdpau->device,
        drawable,
        &flip_target
    );
    if (!vdpau_check_status(status, "VdpPresentationQueueTargetCreateX11()"))
        return -1;

    status = vdpau_presentation_queue_create(
        vdpau->device,
        flip_target,
        &flip_queue
    );
    if (!vdpau_check_status(status, "VdpPresentationQueueCreate()"))
        return -1;

    vdpau->flip_queue   = flip_queue;
    vdpau->flip_target  = flip_target;
    return 0;
}

static void destroy_output_surface(VDPAUContext *vdpau, VDPAUSurface *surface)
{
    if (surface->vdp_surface != VDP_INVALID_HANDLE) {
        vdpau_output_surface_destroy(surface->vdp_surface);
        surface->vdp_surface = VDP_INVALID_HANDLE;
        surface->width       = 0;
        surface->height      = 0;
    }
}

static int create_output_surface(
    VDPAUContext *vdpau,
    VDPAUSurface *surface,
    unsigned int  width,
    unsigned int  height
)
{
    VdpOutputSurface output_surface;
    VdpStatus status;

    status = vdpau_output_surface_create(
        vdpau->device,
        VDP_RGBA_FORMAT_B8G8R8A8,
        width, height,
        &output_surface
    );
    if (!vdpau_check_status(status, "VdpOutputSurfaceCreate()"))
        return -1;

    surface->vdp_surface = output_surface;
    surface->width       = width;
    surface->height      = height;
    return 0;
}

static int sync_output_surface(VDPAUContext *vdpau)
{
    VdpPresentationQueueStatus queue_status;
    VdpTime dummy_time;
    VdpStatus status;

    do {
        status = vdpau_presentation_queue_query_surface_status(
            vdpau->flip_queue,
            vdpau->output_surface.vdp_surface,
            &queue_status,
            &dummy_time
        );
        if (!vdpau_check_status(status, "VdpPresentationQueueQuerySurfaceStatus()"))
            return -1;

        delay_usec(VDPAU_SYNC_DELAY);
    } while (queue_status != VDP_PRESENTATION_QUEUE_STATUS_VISIBLE);
    return 0;
}

static int destroy_video_surface(VDPAUContext *vdpau, VDPAUSurface *surface)
{
    VdpStatus status;

    if (surface->vdp_surface == VDP_INVALID_HANDLE)
        return 0;

    status = vdpau_video_surface_destroy(surface->vdp_surface);
    surface->vdp_surface = VDP_INVALID_HANDLE;
    surface->width       = 0;
    surface->height      = 0;
    if (!vdpau_check_status(status, "VdpVideoSurfaceDestroy()"))
        return -1;
    return 0;
}

static int create_video_surface(
    VDPAUContext *vdpau,
    VDPAUSurface *surface,
    unsigned int  width,
    unsigned int  height,
    VdpChromaType chroma_type
)
{
    VdpVideoSurface video_surface;
    VdpStatus status;

    status = vdpau_video_surface_create(
        vdpau->device,
        chroma_type,
        width,
        height,
        &video_surface
    );
    if (!vdpau_check_status(status, "VdpVideoSurfaceCreate()"))
        return -1;

    surface->vdp_surface = video_surface;
    surface->width       = width;
    surface->height      = height;
    return 0;
}

static void destroy_bitmap_surface(VDPAUContext *vdpau, VDPAUSurface *surface)
{
    if (surface->vdp_surface != VDP_INVALID_HANDLE) {
        vdpau_bitmap_surface_destroy(surface->vdp_surface);
        surface->vdp_surface = VDP_INVALID_HANDLE;
        surface->width       = 0;
        surface->height      = 0;
    }
}

static int create_bitmap_surface(
    VDPAUContext *vdpau,
    VDPAUSurface *surface,
    unsigned int  width,
    unsigned int  height
)
{
    VdpBitmapSurface bitmap_surface;
    VdpStatus status;

    status = vdpau_bitmap_surface_create(
        vdpau->device,
        VDP_RGBA_FORMAT_B8G8R8A8,
        width, height,
        VDP_FALSE,
        &bitmap_surface
    );
    if (!vdpau_check_status(status, "VdpBitmapSurfaceCreate()"))
        return -1;

    surface->vdp_surface = bitmap_surface;
    surface->width       = width;
    surface->height      = height;
    return 0;
}

static int vdpau_init(void)
{
    CommonContext * const common = common_get_context();
    X11Context *x11_context;
    VdpDevice device;
    VdpGetProcAddress *get_proc_address;

    if (vdpau_context)
        return 0;

    if ((x11_context = x11_get_context()) == NULL)
        return -1;

    if (vdp_device_create_x11(x11_context->display, x11_context->screen,
                              &device, &get_proc_address) != VDP_STATUS_OK)
        return -1;

    if ((vdpau_context = calloc(1, sizeof(*vdpau_context))) == NULL)
        return -1;
    vdpau_context->device                     = device;
    vdpau_context->get_proc_address           = get_proc_address;
    vdpau_context->decoder                    = VDP_INVALID_HANDLE;
    vdpau_context->video_mixer                = VDP_INVALID_HANDLE;
    vdpau_context->video_surface.vdp_surface  = VDP_INVALID_HANDLE;
    vdpau_context->output_surface.vdp_surface = VDP_INVALID_HANDLE;
    vdpau_context->bitmap_surface.vdp_surface = VDP_INVALID_HANDLE;
    vdpau_context->subpic_surface.vdp_surface = VDP_INVALID_HANDLE;
    vdpau_context->flip_queue                 = VDP_INVALID_HANDLE;
    vdpau_context->flip_target                = VDP_INVALID_HANDLE;
    vdpau_context->use_vdpau_glx_interop      = (
        common->vdpau_glx_video_surface ||
        common->vdpau_glx_output_surface
    );
    return 0;
}

static int vdpau_exit(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau)
        return 0;

#if USE_GLX
    if (display_type() == DISPLAY_GLX)
        vdpau_glx_destroy_surface();
#endif

    destroy_output_surface(vdpau, &vdpau->output_surface);
    destroy_bitmap_surface(vdpau, &vdpau->bitmap_surface);
    destroy_video_surface(vdpau, &vdpau->video_surface);
    destroy_flip_queue(vdpau);

    if (vdpau->video_mixer != VDP_INVALID_HANDLE) {
        vdpau_video_mixer_destroy(vdpau->video_mixer);
        vdpau->video_mixer = VDP_INVALID_HANDLE;
    }

    destroy_bitstream_buffers(vdpau);

    if (vdpau->device != VDP_INVALID_HANDLE) {
        vdpau_device_destroy(vdpau->device);
        vdpau->device = VDP_INVALID_HANDLE;
    }

    free(vdpau_context);
    return 0;
}

VDPAUContext *vdpau_get_context(void)
{
    return vdpau_context;
}

int vdpau_check_status(VdpStatus status, const char *msg)
{
    if (status != VDP_STATUS_OK) {
        fprintf(stderr, "[%s] %s: %s\n", PACKAGE_NAME, msg, vdpau_get_error_string(status));
        return 0;
    }
    return 1;
}

void *vdpau_alloc_picture(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau)
        return NULL;

    memset(&vdpau->picture_info, 0, sizeof(vdpau->picture_info));
    return &vdpau->picture_info;
}

int vdpau_append_slice_data(const uint8_t *buf, unsigned int buf_size)
{
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpBitstreamBuffer *bitstream_buffer;

    if (!vdpau)
        return -1;

    if ((bitstream_buffer = create_bitstream_buffer(vdpau)) == NULL)
        return -1;

    bitstream_buffer->struct_version    = VDP_BITSTREAM_BUFFER_VERSION;
    bitstream_buffer->bitstream         = buf;
    bitstream_buffer->bitstream_bytes   = buf_size;
    return 0;
}

int vdpau_init_decoder(VdpDecoderProfile profile,
                       unsigned int      picture_width,
                       unsigned int      picture_height)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    uint32_t max_width, max_height;
    VdpChromaType chroma_type = VDP_CHROMA_TYPE_420;
    VdpDecoder decoder;
    VdpVideoMixer video_mixer;
    VdpVideoMixerParameter params[VDPAU_MAX_PARAMS];
    const void *param_values[VDPAU_MAX_PARAMS];
    unsigned int n_params = 0;
    unsigned int features[VDPAU_MAX_FEATURES];
    VdpBool features_enable[VDPAU_MAX_FEATURES];
    unsigned int n_features = 0;
    VdpVideoMixerAttribute attrs[VDPAU_MAX_ATTRS];
    const void *attr_values[VDPAU_MAX_ATTRS];
    unsigned int n_attrs = 0;
    unsigned int i;
    VdpStatus status;

    typedef struct {
        VdpVideoMixerFeature feature;
        const char *name;
    } VdpVideoMixerFeaturesMap;

    static const VdpVideoMixerFeaturesMap features_map[] = {
        { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL,
          "deinterlace-temporal" },
        { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL,
          "deinterlace-temporal-spatial" },
        { VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE,
          "inverse-telecine" },
        { VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION,
          "noise-reduction" },
        { VDP_VIDEO_MIXER_FEATURE_SHARPNESS,
          "sharpness" },
        { VDP_VIDEO_MIXER_FEATURE_LUMA_KEY,
          "luma-key" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1,
          "high-quality-scaling-L1" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2,
          "high-quality-scaling-L2" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3,
          "high-quality-scaling-L3" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4,
          "high-quality-scaling-L4" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5,
          "high-quality-scaling-L5" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6,
          "high-quality-scaling-L6" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7,
          "high-quality-scaling-L7" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8,
          "high-quality-scaling-L8" },
        { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9,
          "high-quality-scaling-L9" }
    };

    typedef struct {
        VdpVideoMixerAttribute attribute;
        const char *name;
    } VdpVideoMixerAttributesMap;

    static const VdpVideoMixerAttributesMap attributes_map[] = {
        { VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR,
          "background-color" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX,
          "csc-matrix" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL,
          "noise-reduction-level" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL,
          "sharpness-level" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA,
          "luma-key-min-luma" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA,
          "luma-key-max-luma" },
        { VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE,
          "skip-chroma-deinterlace" }
    };

    if (!vdpau)
        return -1;

    if (common_init_decoder(picture_width, picture_height) < 0)
        return -1;

    if (!vdpau_is_supported_profile(vdpau->device, profile))
        return -1;
    if (!vdpau_get_surface_size_max(vdpau->device, profile, &max_width, &max_height))
        return -1;
    if (picture_width > max_width || picture_height > max_height)
        return -1;

#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        if (glx_init_texture(picture_width, picture_height) < 0)
            return -1;
    }
#endif

    if (create_video_surface(vdpau, &vdpau->video_surface,
                             picture_width, picture_height, chroma_type) < 0)
        return -1;

    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH;
    param_values[n_params++] = &picture_width;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT;
    param_values[n_params++] = &picture_height;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE;
    param_values[n_params++] = &chroma_type;

    if (common->vdpau_layers) {
        uint32_t min_layers, max_layers;
        if (!vdpau_get_video_mixer_param_range(vdpau->device,
                                               VDP_VIDEO_MIXER_PARAMETER_LAYERS,
                                               &min_layers, &max_layers))
            return -1;
        D(bug("video mixer supports up to %d layers\n", max_layers));
        params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_LAYERS;
        param_values[n_params++] = &max_layers;
    }

    D(bug("supported video mixer features:\n"));
    for (i = 0; i < ARRAY_ELEMS(features_map); i++) {
        const VdpVideoMixerFeaturesMap * const m = &features_map[i];
        if (vdpau_video_mixer_has_feature(vdpau->device, m->feature))
            D(bug("  %s\n", m->name));
    }

    D(bug("supported video mixer attributes:\n"));
    for (i = 0; i < ARRAY_ELEMS(attributes_map); i++) {
        const VdpVideoMixerAttributesMap * const m = &attributes_map[i];
        if (vdpau_video_mixer_has_attribute(vdpau->device, m->attribute))
            D(bug("  %s\n", m->name));
    }

    if (common->vdpau_hqscaling > 0)
        features[n_features++] =
            VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 +
            common->vdpau_hqscaling - 1;

    status = vdpau_video_mixer_create(
        vdpau->device,
        n_features, features,
        n_params, params, param_values,
        &video_mixer
    );
    if (!vdpau_check_status(status, "VdpVideoMixerCreate()"))
        return -1;

    if (n_features > 0) {
        for (i = 0; i < n_features; i++)
            features_enable[i] = VDP_TRUE;

        status = vdpau_video_mixer_set_feature_enables(
            video_mixer,
            n_features,
            features,
            features_enable
        );
        if (!vdpau_check_status(status, "VdpVideoMixerSetFeatureEnables()"))
            return -1;
    }

#if USE_GLX
    if (display_type() == DISPLAY_GLX && common->vdpau_glx_output_surface) {
        /* Make sure background color is black with alpha channel set to 0xff */
        static const VdpColor bgcolor = { 0.0f, 0.0f, 0.0f, 1.0f };
        attrs[n_attrs] = VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR;
        attr_values[n_attrs++] = &bgcolor;
    }
#endif

    if (n_attrs > 0) {
        status = vdpau_video_mixer_set_attribute_values(
            video_mixer,
            n_attrs,
            attrs,
            attr_values
        );
        if (!vdpau_check_status(status, "VdpVideoMixerSetAttributeValues()"))
            return -1;
    }

    status = vdpau_decoder_create(
        vdpau->device,
        profile,
        picture_width, picture_height,
        1,
        &decoder
    );
    if (!vdpau_check_status(status, "VdpDecoderCreate()"))
        return -1;

    vdpau->picture_width             = picture_width;
    vdpau->picture_height            = picture_height;
    vdpau->decoder                   = decoder;
    vdpau->video_mixer               = video_mixer;
    return 0;
}

static const VdpYCbCrFormat ycbcr_formats[] = {
    VDP_YCBCR_FORMAT_YV12,
    VDP_YCBCR_FORMAT_NV12,
    VDP_INVALID_HANDLE
};

static int vdpau_decode_to_image(void)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();

    VdpYCbCrFormat ycbcr_format = VDP_INVALID_HANDLE;
    VdpStatus status;
    Image *image = NULL;
    uint32_t image_format;
    int i, error = -1;

    if (!common)
        return -1;
    if (getimage_mode() != GETIMAGE_FROM_VIDEO)
        return 0;

    if (getimage_format()) {
        ycbcr_format = vdpau_get_format(getimage_format());
        if (ycbcr_format == VDP_INVALID_HANDLE ||
            !vdpau_is_supported_ycbcr_format(vdpau->device, ycbcr_format))
            goto end;
    }
    else {
        for (i = 0; ycbcr_formats[i] != VDP_INVALID_HANDLE; i++) {
            const VdpYCbCrFormat format = ycbcr_formats[i];
            if (vdpau_is_supported_ycbcr_format(vdpau->device, format)) {
                ycbcr_format = format;
                break;
            }
        }
    }
    if (ycbcr_format == VDP_INVALID_HANDLE)
        goto end;

    image_format = image_get_yuv_format(ycbcr_format);
    if (!image_format)
        goto end;
    D(bug("selected %s image format for getimage\n",
          string_of_FOURCC(image_format)));

    image = image_create(vdpau->picture_width, vdpau->picture_height, image_format);
    if (!image)
        goto end;

    status = vdpau_video_surface_get_bits_ycbcr(
        vdpau->video_surface.vdp_surface,
        ycbcr_format,
        image->pixels,
        image->pitches
    );
    if (!vdpau_check_status(status, "VdpVideoSurfaceGetBitsYCbCr()"))
        goto end;

    if (image_convert(common->image, image) < 0)
        goto end;

    error = 0;
end:
    if (image)
        image_destroy(image);
    return error;
}

int vdpau_decode(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpStatus status;

    if (!vdpau || vdpau->decoder == VDP_INVALID_HANDLE)
        return -1;

    status = vdpau_decoder_render(
        vdpau->decoder,
        vdpau->video_surface.vdp_surface,
        (VdpPictureInfo)&vdpau->picture_info,
        vdpau->bitstream_buffers_count,
        vdpau->bitstream_buffers
    );
    if (!vdpau_check_status(status, "VdpDecoderRender()"))
        return -1;

    return vdpau_decode_to_image();
}

static int put_image(Image *src_img, VDPAUSurface *surface)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpYCbCrFormat ycbcr_format = VDP_INVALID_HANDLE;
    VdpStatus status;
    Image *image = NULL;
    uint32_t image_format;
    int i, error = -1;

    if (!common)
        return -1;

    if (common->putimage_format) {
        ycbcr_format = vdpau_get_format(common->putimage_format);
        if (ycbcr_format == VDP_INVALID_HANDLE ||
            !vdpau_is_supported_ycbcr_format(vdpau->device, ycbcr_format))
            goto end;
    }
    else {
        for (i = 0; ycbcr_formats[i] != VDP_INVALID_HANDLE; i++) {
            const VdpYCbCrFormat format = ycbcr_formats[i];
            if (vdpau_is_supported_ycbcr_format(vdpau->device, format)) {
                ycbcr_format = format;
                break;
            }
        }
    }
    if (ycbcr_format == VDP_INVALID_HANDLE)
        goto end;

    image_format = image_get_yuv_format(ycbcr_format);
    if (!image_format)
        goto end;
    D(bug("selected %s image format for putimage in override mode\n",
          string_of_FOURCC(image_format)));

    image = image_create(surface->width, surface->height, image_format);
    if (!image)
        goto end;

    if (image_convert(image, src_img) < 0)
        goto end;

    status = vdpau_video_surface_put_bits_ycbcr(
        surface->vdp_surface,
        ycbcr_format,
        image->pixels,
        image->pitches
    );
    if (!vdpau_check_status(status, "VdpVideoSurfacePutBitsYCbCr()"))
        goto end;

    error = 0;
end:
    if (image)
        image_destroy(image);
    return error;
}

static const VdpRGBAFormat bitmap_formats[] = {
    VDP_RGBA_FORMAT_B8G8R8A8,
    VDP_INVALID_HANDLE
};

static int blend_image(Image *src_img)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpRGBAFormat bitmap_format = VDP_INVALID_HANDLE;
    VdpStatus status;
    Image *image = NULL;
    uint32_t image_format;
    int i, error = -1;

    if (!common)
        return -1;

    if (src_img->format != IMAGE_RGB32)
        goto end;

    if (common->putimage_format) {
        /* XXX: VDPAU only supports RGB formats for subpictures */
        if (IS_RGB_IMAGE_FORMAT(common->putimage_format)) {
            bitmap_format = vdpau_get_format(common->putimage_format);
            if (bitmap_format == VDP_INVALID_HANDLE ||
                !vdpau_is_supported_bitmap_format(vdpau->device, bitmap_format))
                goto end;
        }
    }
    else {
        for (i = 0; bitmap_formats[i] != VDP_INVALID_HANDLE; i++) {
            const VdpRGBAFormat format = bitmap_formats[i];
            if (vdpau_is_supported_bitmap_format(vdpau->device, format)) {
                bitmap_format = format;
                break;
            }
        }
    }
    if (bitmap_format == VDP_INVALID_HANDLE)
        goto end;

    image_format = image_get_rgb_format(bitmap_format);
    if (!image_format)
        goto end;
    D(bug("selected %s image format for putimage in blend mode\n",
          string_of_FOURCC(image_format)));

    image = image_create(src_img->width, src_img->height, image_format);
    if (!image)
        goto end;

    if (image_convert(image, src_img) < 0)
        goto end;

    if (common->vdpau_layers) {
        if (create_output_surface(vdpau, &vdpau->subpic_surface,
                                  src_img->width, src_img->height) < 0)
            goto end;

        status = vdpau_output_surface_put_bits_native(
            vdpau->subpic_surface.vdp_surface,
            image->pixels,
            image->pitches,
            NULL
        );
        if (!vdpau_check_status(status, "VdpOutputSurfacePutBitsNative()"))
            goto end;
    }
    else {
        if (create_bitmap_surface(vdpau, &vdpau->bitmap_surface,
                                  src_img->width, src_img->height) < 0)
            goto end;

        status = vdpau_bitmap_surface_put_bits_native(
            vdpau->bitmap_surface.vdp_surface,
            image->pixels,
            image->pitches,
            NULL
        );
        if (!vdpau_check_status(status, "VdpBitmapSurfacePutBitsNative()"))
            goto end;
    }

    error = 0;
end:
    if (error) {
        destroy_output_surface(vdpau, &vdpau->subpic_surface);
        destroy_bitmap_surface(vdpau, &vdpau->bitmap_surface);
    }
    if (image)
        image_destroy(image);
    return error;
}

static int render_video_surface(
    unsigned int render_width,
    unsigned int render_height
)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpLayer layers[1], *layer;
    unsigned int num_layers = 0;
    VdpRect source_rect, output_rect, display_rect;
    VdpStatus status;

    if (vdpau->video_surface.vdp_surface == VDP_INVALID_HANDLE)
        return -1;

    source_rect.x0  = 0;
    source_rect.y0  = 0;
    source_rect.x1  = vdpau->picture_width;
    source_rect.y1  = vdpau->picture_height;
    output_rect.x0  = 0;
    output_rect.y0  = 0;
    output_rect.x1  = render_width;
    output_rect.y1  = render_height;
    display_rect.x0 = 0;
    display_rect.y0 = 0;
    display_rect.x1 = render_width;
    display_rect.y1 = render_height;

    if (common->vdpau_layers &&
        vdpau->subpic_surface.vdp_surface != VDP_INVALID_HANDLE) {
        layer = &layers[num_layers++];
        layer->struct_version   = VDP_LAYER_VERSION;
        layer->source_surface   = vdpau->subpic_surface.vdp_surface;
        layer->source_rect      = NULL;
        layer->destination_rect = &display_rect;
    }

    status = vdpau_video_mixer_render(
        vdpau->video_mixer,
        VDP_INVALID_HANDLE, NULL,
        VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
        0, NULL,
        vdpau->video_surface.vdp_surface,
        0, NULL,
        &source_rect,
        vdpau->output_surface.vdp_surface,
        &output_rect,
        &display_rect,
        num_layers, layers
    );
    if (!vdpau_check_status(status, "VdpVideoMixerRender()"))
        return -1;
    return 0;
}

static int render_subpicture(
    unsigned int render_width,
    unsigned int render_height
)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpOutputSurfaceRenderBlendState blend_state;
    VdpRect source_rect, output_rect;
    VdpStatus status;

    if (common->vdpau_layers) {
        if (vdpau->subpic_surface.vdp_surface == VDP_INVALID_HANDLE)
            return -1;
        return 0;
    }

    if (vdpau->bitmap_surface.vdp_surface == VDP_INVALID_HANDLE)
        return 0;

    blend_state.struct_version                 = VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION;
    blend_state.blend_factor_source_color      = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE;
    blend_state.blend_factor_source_alpha      = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE;
    blend_state.blend_factor_destination_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.blend_factor_destination_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.blend_equation_color           = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD;
    blend_state.blend_equation_alpha           = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD;

    source_rect.x0 = 0;
    source_rect.y0 = 0;
    source_rect.x1 = vdpau->bitmap_surface.width;
    source_rect.y1 = vdpau->bitmap_surface.height;
    output_rect.x0  = 0;
    output_rect.y0  = 0;
    output_rect.x1  = render_width;
    output_rect.y1  = render_height;
    status = vdpau_output_surface_render_bitmap_surface(
        vdpau->output_surface.vdp_surface,
        &output_rect,
        vdpau->bitmap_surface.vdp_surface,
        &source_rect,
        NULL,
        &blend_state,
        VDP_OUTPUT_SURFACE_RENDER_ROTATE_0
    );
    if (!vdpau_check_status(status, "VdpOutputSurfaceRenderBitmapSurface()"))
        return -1;
    return 0;
}

static int render_cliprects(
    unsigned int render_width,
    unsigned int render_height
)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VDPAUSurface video_surface;
    const VdpChromaType chroma_type = VDP_CHROMA_TYPE_420;
    VdpRect source_rect, output_rect, display_rect;
    VdpVideoMixer video_mixer = VDP_INVALID_HANDLE;
    VdpVideoMixerParameter params[VDPAU_MAX_PARAMS];
    const void *param_values[VDPAU_MAX_PARAMS];
    VdpStatus status;
    unsigned int n_params = 0;
    unsigned int i, error = 1;

    if (!common->use_clipping)
        return 0;

    if (create_video_surface(vdpau, &video_surface, 160, 120, chroma_type) < 0)
        return -1;

    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH;
    param_values[n_params++] = &video_surface.width;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT;
    param_values[n_params++] = &video_surface.height;
    params[n_params]         = VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE;
    param_values[n_params++] = &chroma_type;

    status = vdpau_video_mixer_create(
        vdpau->device,
        0, NULL,
        n_params, params, param_values,
        &video_mixer
    );
    if (!vdpau_check_status(status, "VdpVideoMixerCreate()"))
        goto end;

    if (put_image(common->cliprects_image, &video_surface) < 0)
        goto end;

    for (i = 0; i < common->cliprects_count; i++) {
        Rectangle * const rect = &common->cliprects[i];
        const float sx = (float)video_surface.width  / (float)render_width;
        const float sy = (float)video_surface.height / (float)render_height;
        source_rect.x0  = sx * (float)rect->x;
        source_rect.y0  = sy * (float)rect->y;
        source_rect.x1  = source_rect.x0 + sx * (float)rect->width;
        source_rect.y1  = source_rect.y0 + sy * (float)rect->height;
        output_rect.x0  = rect->x;
        output_rect.y0  = rect->y;
        output_rect.x1  = output_rect.x0 + rect->width;
        output_rect.y1  = output_rect.y0 + rect->height;
        display_rect.x0 = rect->x;
        display_rect.y0 = rect->y;
        display_rect.x1 = display_rect.x0 + rect->width;
        display_rect.y1 = display_rect.y0 + rect->height;
        status = vdpau_video_mixer_render(
            video_mixer,
            VDP_INVALID_HANDLE, NULL,
            VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
            0, NULL,
            video_surface.vdp_surface,
            0, NULL,
            &source_rect,
            vdpau->output_surface.vdp_surface,
            &output_rect,
            &display_rect,
            0, NULL
        );
        if (!vdpau_check_status(status, "VdpVideoMixerRender()"))
            goto end;
    }

    error = 0;
end:
    if (video_mixer != VDP_INVALID_HANDLE) {
        status = vdpau_video_mixer_destroy(video_mixer);
        if (!vdpau_check_status(status, "VdpVideoMixerDestroy()"))
            error = 1;
    }

    if (video_surface.vdp_surface != VDP_INVALID_HANDLE) {
        if (destroy_video_surface(vdpau, &video_surface) < 0)
            error = 1;
    }

    if (error)
        return -1;
    return 0;
}

static int vdpau_display(void)
{
    CommonContext * const common = common_get_context();
    X11Context * const x11 = x11_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpTime dummy_time;
    VdpStatus status;
    int use_pixmap = 0;
    unsigned int render_width, render_height;

    if (!common || !x11 || !vdpau)
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
                if (put_image(img, &vdpau->video_surface) < 0)
                    return -1;
                break;
            case PUTIMAGE_BLEND:
                if (blend_image(img) < 0)
                    return -1;
                break;
            default:
                break;
            }
            image_destroy(img);
        }
    }

    if (getimage_mode() == GETIMAGE_FROM_VIDEO)
        return 0;

    if (getimage_mode() == GETIMAGE_FROM_PIXMAP)
        use_pixmap = 1;
#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        if (!vdpau->use_vdpau_glx_interop)
            use_pixmap = 1; /* use GLX texture-from-pixmap extension */
        render_width  = glx_get_context()->texture_width;
        render_height = glx_get_context()->texture_height;
    }
    else
#endif
    {
        render_width  = x11->window_width;
        render_height = x11->window_height;
    }

    if (vdpau->output_surface.width != render_width ||
        vdpau->output_surface.height != render_height) {
        destroy_output_surface(vdpau, &vdpau->output_surface);
        if (create_output_surface(vdpau, &vdpau->output_surface, render_width, render_height) < 0)
            return -1;
    }

    if (!vdpau->use_vdpau_glx_interop) {
        if (vdpau->flip_queue == VDP_INVALID_HANDLE) {
            Drawable drawable;
#if USE_GLX
            if (display_type() == DISPLAY_GLX)
                drawable = glx_get_pixmap();
            else
#endif
            if (use_pixmap)
                drawable = x11->pixmap;
            else
                drawable = x11->window;
            if (drawable == None)
                return -1;

            if (create_flip_queue(vdpau, drawable) < 0)
                return -1;
        }

        status = vdpau_presentation_queue_block_until_surface_idle(
            vdpau->flip_queue,
            vdpau->output_surface.vdp_surface,
            &dummy_time
        );
        if (!vdpau_check_status(status, "VdpPresentationQueueBlockUntilSurfaceIdle()"))
            return -1;
    }

    if (render_video_surface(render_width, render_height) < 0)
        return -1;

    if (render_subpicture(render_width, render_height) < 0)
        return -1;

    if (render_cliprects(render_width, render_height) < 0)
        return -1;

    if (getimage_mode() == GETIMAGE_FROM_OUTPUT) {
        VdpRect output_rect;
        output_rect.x0  = 0;
        output_rect.y0  = 0;
        output_rect.x1  = render_width;
        output_rect.y1  = render_height;
        status = vdpau_output_surface_get_bits_native(
            vdpau->output_surface.vdp_surface,
            &output_rect,
            common->image->pixels,
            common->image->pitches
        );
        if (!vdpau_check_status(status, "VdpOutputSurfaceGetBitsNative()"))
            return -1;
    }
#if USE_GLX
    else if (vdpau->use_vdpau_glx_interop) {
        GLXContext * const glx = glx_get_context();
        if (!glx)
            return -1;
        if (vdpau_glx_create_surface(glx->texture_target, glx->texture) < 0)
            return -1;
    }
#endif
    else {
        status = vdpau_presentation_queue_display(
            vdpau->flip_queue,
            vdpau->output_surface.vdp_surface,
            render_width,
            render_height,
            0
        );
        if (!vdpau_check_status(status, "VdpPresentationQueueDisplay()"))
            return -1;
    }

    /* Wait for VDPAU rendering to complete */
    if (use_pixmap) {
        if (sync_output_surface(vdpau) < 0)
            return -1;
    }
    return 0;
}

int pre(void)
{
    if (hwaccel_type() != HWACCEL_VDPAU)
        return -1;

    if (x11_init() < 0)
        return -1;
#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        if (glx_init() < 0)
            return -1;
    }
#endif
    return vdpau_init();
}

int post(void)
{
    if (vdpau_exit() < 0)
        return -1;
#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        if (glx_exit() < 0)
            return -1;
    }
#endif
    return x11_exit();
}

int display(void)
{
    if (vdpau_display() < 0)
        return -1;
#if USE_GLX
    if (display_type() == DISPLAY_GLX)
        return glx_display();
#endif
    return x11_display();
}

#if USE_GLX
int vdpau_glx_create_surface(unsigned int target, unsigned int texture)
{
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau->use_vdpau_glx_interop)
        return 0;

    if (!gl_vdpau_init(vdpau->device, vdpau->get_proc_address))
        return -1;

    /* Create VDPAU/GL surface from VdpVideoSurface */
    if (common->vdpau_glx_video_surface) {
        vdpau->glx_surface = gl_vdpau_create_video_surface(
            target,
            vdpau->video_surface.vdp_surface
        );
        if (!vdpau->glx_surface)
            return -1;
        return 0;
    }

    /* Create VDPAU/GL surface from VdpOutputSurface */
    if (common->vdpau_glx_output_surface) {
        vdpau->glx_surface = gl_vdpau_create_output_surface(
            target,
            vdpau->output_surface.vdp_surface
        );
        if (!vdpau->glx_surface)
            return -1;
        return 0;
    }
    return -1;
}

void vdpau_glx_destroy_surface(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau->use_vdpau_glx_interop)
        return;

    if (vdpau->glx_surface) {
        gl_vdpau_destroy_surface(vdpau->glx_surface);
        vdpau->glx_surface = NULL;
    }
    gl_vdpau_exit();
}

int vdpau_glx_begin_render_surface(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau->use_vdpau_glx_interop)
        return 0;

    printf("GLX: use NV_vdpau_interop extension (VDPAU/GLX)\n");
    if (!gl_vdpau_bind_surface(vdpau->glx_surface))
        return -1;
    return 0;
}

int vdpau_glx_end_render_surface(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();

    if (!vdpau->use_vdpau_glx_interop)
        return 0;

    if (!gl_vdpau_unbind_surface(vdpau->glx_surface))
        return -1;
    return 0;
}
#endif
