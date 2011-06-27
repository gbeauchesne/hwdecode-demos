/*
 *  vdpau_gate.h - VDPAU hooks
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

#ifndef VDPAU_GATE_H
#define VDPAU_GATE_H

#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

// VdpGetApiVersion
VdpStatus
vdpau_get_api_version(uint32_t *api_version);

// VdpGetInformationString
VdpStatus
vdpau_get_information_string(const char **info_string);

// VdpGetErrorString
const char *
vdpau_get_error_string(VdpStatus vdp_status);

// VdpDeviceDestroy
VdpStatus
vdpau_device_destroy(VdpDevice device);

// VdpVideoSurfaceCreate
VdpStatus
vdpau_video_surface_create(
    VdpDevice            device,
    VdpChromaType        chroma_type,
    uint32_t             width,
    uint32_t             height,
    VdpVideoSurface     *surface
);

// VdpVideoSurfaceDestroy
VdpStatus
vdpau_video_surface_destroy(VdpVideoSurface surface);

// VdpVideoSurfaceGetBitsYCbCr
VdpStatus
vdpau_video_surface_get_bits_ycbcr(
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **dest,
    uint32_t            *stride
);

// VdpVideoSurfacePutBitsYCbCr
VdpStatus
vdpau_video_surface_put_bits_ycbcr(
    VdpVideoSurface      surface,
    VdpYCbCrFormat       format,
    uint8_t            **src,
    uint32_t            *stride
);

// VdpOutputSurfaceCreate
VdpStatus
vdpau_output_surface_create(
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpOutputSurface    *surface
);

// VdpOutputSurfaceDestroy
VdpStatus
vdpau_output_surface_destroy(VdpOutputSurface surface);

// VdpOutputSurfaceGetBitsNative
VdpStatus
vdpau_output_surface_get_bits_native(
    VdpOutputSurface     surface,
    const VdpRect       *source_rect,
    uint8_t            **dst,
    uint32_t            *stride
);

// VdpOutputSurfacePutBitsNative
VdpStatus
vdpau_output_surface_put_bits_native(
    VdpOutputSurface     surface,
    uint8_t            **src,
    const uint32_t      *stride,
    const VdpRect       *dest_rect
);

// VdpOutputSurfaceRenderBitmapSurface
VdpStatus
vdpau_output_surface_render_bitmap_surface(
    VdpOutputSurface     destination_surface,
    const VdpRect       *destination_rect,
    VdpBitmapSurface     source_surface,
    const VdpRect       *source_rect,
    const VdpColor      *colors,
    const VdpOutputSurfaceRenderBlendState *blend_state,
    uint32_t             flags
);

// VdpOutputSurfaceRenderOutputSurface
VdpStatus
vdpau_output_surface_render_output_surface(
    VdpOutputSurface    destination_surface,
    const VdpRect      *destination_rect,
    VdpOutputSurface    source_surface,
    const VdpRect      *source_rect,
    const VdpColor     *colors,
    const VdpOutputSurfaceRenderBlendState *blend_state,
    uint32_t            flags
);

// VdpBitmapSurfaceQueryCapabilities
VdpStatus
vdpau_bitmap_surface_query_capabilities(
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    VdpBool             *is_supported,
    uint32_t            *max_width,
    uint32_t            *max_height
);

// VdpBitmapSurfaceCreate
VdpStatus
vdpau_bitmap_surface_create(
    VdpDevice            device,
    VdpRGBAFormat        rgba_format,
    uint32_t             width,
    uint32_t             height,
    VdpBool              frequently_accessed,
    VdpBitmapSurface    *surface
);

// VdpBitmapSurfaceDestroy
VdpStatus
vdpau_bitmap_surface_destroy(VdpBitmapSurface surface);

// VdpBitmapSurfacePutBitsNative
VdpStatus
vdpau_bitmap_surface_put_bits_native(
    VdpBitmapSurface     surface,
    uint8_t            **src,
    const uint32_t      *stride,
    const VdpRect       *dest_rect
);

// VdpVideoMixerQueryFeatureSupport
VdpStatus
vdpau_video_mixer_query_feature_support(
    VdpDevice                   device,
    VdpVideoMixerFeature        feature,
    VdpBool                    *is_supported
);

// VdpVideoMixerQueryAttributeSupport
VdpStatus
vdpau_video_mixer_query_attribute_support(
    VdpDevice                   device,
    VdpVideoMixerAttribute      attribute,
    VdpBool                    *is_supported
);

// VdpVideoMixerSetFeatureEnables
VdpStatus
vdpau_video_mixer_set_feature_enables(
    VdpVideoMixer               mixer,
    uint32_t                    feature_count,
    const VdpVideoMixerFeature *features,
    const VdpBool              *feature_enables
);

// VdpVideoMixerSetAttributeValues
VdpStatus
vdpau_video_mixer_set_attribute_values(
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    const void                  **attribute_values
);

// VdpVideoMixerQueryParameterSupport
VdpStatus
vdpau_video_mixer_query_parameter_support(
    VdpDevice                   device,
    VdpVideoMixerParameter      parameter,
    VdpBool                    *is_supported
);

// VdpVideoMixerQueryParameterValueRange
VdpStatus
vdpau_video_mixer_query_parameter_value_range(
    VdpDevice                   device,
    VdpVideoMixerParameter      parameter,
    void                       *min_value,
    void                       *max_value
);

// VdpVideoMixerCreate
VdpStatus
vdpau_video_mixer_create(
    VdpDevice                     device,
    uint32_t                      feature_count,
    VdpVideoMixerFeature const   *features,
    uint32_t                      parameter_count,
    VdpVideoMixerParameter const *parameters,
    const void                   *parameter_values,
    VdpVideoMixer                *mixer
);

// VdpVideoMixerDestroy
VdpStatus
vdpau_video_mixer_destroy(VdpVideoMixer mixer);

// VdpVideoMixerRender
VdpStatus
vdpau_video_mixer_render(
    VdpVideoMixer                 mixer,
    VdpOutputSurface              background_surface,
    const VdpRect                *background_source_rect,
    VdpVideoMixerPictureStructure current_picture_structure,
    uint32_t                      video_surface_past_count,
    const VdpVideoSurface        *video_surface_past,
    VdpVideoSurface               video_surface_current,
    uint32_t                      video_surface_future_count,
    const VdpVideoSurface        *video_surface_future,
    const VdpRect                *video_source_rect,
    VdpOutputSurface              destination_surface,
    const VdpRect                *destination_rect,
    const VdpRect                *destination_video_rect,
    uint32_t                      layer_count,
    const VdpLayer               *layers
);

// VdpVideoMixerGetAttributeValues
VdpStatus
vdpau_video_mixer_get_attribute_values(
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    void                        **attribute_values
);

// VdpVideoMixerSetAttributeValues
VdpStatus
vdpau_video_mixer_set_attribute_values(
    VdpVideoMixer                 mixer,
    uint32_t                      attribute_count,
    const VdpVideoMixerAttribute *attributes,
    const void                  **attribute_values
);

// VdpPresentationQueueCreate
VdpStatus
vdpau_presentation_queue_create(
    VdpDevice                  device,
    VdpPresentationQueueTarget presentation_queue_target,
    VdpPresentationQueue      *presentation_queue
);

// VdpPresentationQueueDestroy
VdpStatus
vdpau_presentation_queue_destroy(VdpPresentationQueue presentation_queue);

// VdpPresentationQueueDisplay
VdpStatus
vdpau_presentation_queue_display(
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    uint32_t             clip_width,
    uint32_t             clip_height,
    VdpTime              earliest_presentation_time
);

// VdpPresentationQueueBlockUntilSurfaceIdle
VdpStatus
vdpau_presentation_queue_block_until_surface_idle(
    VdpPresentationQueue presentation_queue,
    VdpOutputSurface     surface,
    VdpTime             *first_presentation_time
);

// VdpPresentationQueueQuerySurfaceStatus
VdpStatus
vdpau_presentation_queue_query_surface_status(
    VdpPresentationQueue        presentation_queue,
    VdpOutputSurface            surface,
    VdpPresentationQueueStatus *status,
    VdpTime                    *first_presentation_time
);

// VdpPresentationQueueTargetCreateX11
VdpStatus
vdpau_presentation_queue_target_create_x11(
    VdpDevice                   device,
    Drawable                    drawable,
    VdpPresentationQueueTarget *target
);

// VdpPresentationQueueTargetDestroy
VdpStatus
vdpau_presentation_queue_target_destroy(
    VdpPresentationQueueTarget presentation_queue_target
);

// VdpDecoderCreate
VdpStatus
vdpau_decoder_create(
    VdpDevice            device,
    VdpDecoderProfile    profile,
    uint32_t             width,
    uint32_t             height,
    uint32_t             max_references,
    VdpDecoder          *decoder
);

// VdpDecoderDestroy
VdpStatus
vdpau_decoder_destroy(VdpDecoder decoder);

// VdpDecoderRender
VdpStatus
vdpau_decoder_render(
    VdpDecoder                decoder,
    VdpVideoSurface           target,
    VdpPictureInfo const     *picture_info,
    uint32_t                  bitstream_buffers_count,
    VdpBitstreamBuffer const *bitstream_buffers
);

// VdpDecoderQueryCapabilities
VdpStatus
vdpau_decoder_query_capabilities(
    VdpDevice            device,
    VdpDecoderProfile    profile,
    VdpBool             *is_supported,
    uint32_t            *max_level,
    uint32_t            *max_references,
    uint32_t            *max_width,
    uint32_t            *max_height
);

// VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities
VdpStatus
vdpau_video_surface_query_ycbcr_caps(
    VdpDevice            device,
    VdpChromaType        surface_chroma_type,
    VdpYCbCrFormat       bits_ycbcr_format,
    VdpBool             *is_supported
);

// VdpOutputSurfaceQueryGetPutBitsNativeCapabilities
VdpStatus
vdpau_output_surface_query_rgba_caps(
    VdpDevice            device,
    VdpRGBAFormat        surface_rgba_format,
    VdpBool             *is_supported
);

#endif /* VDPAU_GATE_H */
