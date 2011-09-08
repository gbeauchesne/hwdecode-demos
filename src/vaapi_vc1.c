/*
 *  vaapi_vc1.c - VC-1 decoding through VA API
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
#include "vc1.h"

#ifdef USE_OLD_VAAPI
#define V_raw_coding       raw_coding_flag
#define M_raw_coding       raw_coding
#define V_bitplane_present bitplane_present_flag
#define M_bitplane_present bitplane_present
#else
#define V_raw_coding       raw_coding
#define M_raw_coding       raw_coding
#define V_bitplane_present bitplane_present
#define M_bitplane_present bitplane_present
#endif

int decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAPictureParameterBufferVC1 *pic_param;
    VASliceParameterBufferVC1 *slice_param;
    uint8_t *bitplane;
    int i, slice_count;

    VC1PictureInfo vc1_pic_info;
    VC1SliceInfo vc1_slice_info;
    const uint8_t *vc1_bitplane;
    unsigned int vc1_bitplane_size;
    const uint8_t *vc1_slice_data;
    unsigned int vc1_slice_data_size;

    if (!vaapi)
        return -1;

    vc1_get_picture_info(&vc1_pic_info);

    if (vaapi_init_decoder(VAProfileVC1Advanced, VAEntrypointVLD,
                           vc1_pic_info.width, vc1_pic_info.height) < 0)
        return -1;

    if ((pic_param = vaapi_alloc_picture(sizeof(*pic_param))) == NULL)
        return -1;

#define COPY(field) \
    pic_param->field = vc1_pic_info.field
#define COPY_BFM(a,b,c) \
    pic_param->BFM(a,b,c) = vc1_pic_info.a.b.c
#define COPY_BFMP(p,a,b,c) \
    pic_param->BFMP(p,a,b,c) = vc1_pic_info.a.b.c
    pic_param->forward_reference_picture = 0xffffffff;
    pic_param->backward_reference_picture = 0xffffffff;
    pic_param->inloop_decoded_picture = 0xffffffff;
    pic_param->BFV(sequence_fields, value) = 0; /* reset all bits */
    COPY_BFM(sequence_fields, bits, interlace);
    COPY_BFM(sequence_fields, bits, syncmarker);
    COPY_BFM(sequence_fields, bits, overlap);
    NEW(COPY_BFM(sequence_fields, bits, pulldown));
    NEW(COPY_BFM(sequence_fields, bits, tfcntrflag));
    NEW(COPY_BFM(sequence_fields, bits, finterpflag));
    NEW(COPY_BFM(sequence_fields, bits, psf));
    NEW(COPY_BFM(sequence_fields, bits, multires));
    NEW(COPY_BFM(sequence_fields, bits, rangered));
    NEW(COPY_BFM(sequence_fields, bits, max_b_frames));
#if VA_CHECK_VERSION(0,32,0)
    pic_param->BFM(sequence_fields, bits, profile) = vc1_pic_info.profile;
#endif
    pic_param->coded_width = vc1_pic_info.width;
    pic_param->coded_height = vc1_pic_info.height;
    NEW(pic_param->BFV(entrypoint_fields, value) = 0); /* reset all bits */
    COPY_BFM(entrypoint_fields, bits, broken_link);
    COPY_BFM(entrypoint_fields, bits, closed_entry);
    NEW(COPY_BFM(entrypoint_fields, bits, panscan_flag));
    COPY_BFM(entrypoint_fields, bits, loopfilter);
    COPY(conditional_overlap_flag);
    COPY(fast_uvmc_flag);
    pic_param->BFV(range_mapping_fields, value) = 0; /* reset all bits */
    COPY_BFMP(range_mapping, range_mapping_fields, bits, luma_flag);
    COPY_BFMP(range_mapping, range_mapping_fields, bits, luma);
    COPY_BFMP(range_mapping, range_mapping_fields, bits, chroma_flag);
    COPY_BFMP(range_mapping, range_mapping_fields, bits, chroma);
    COPY(b_picture_fraction);
    COPY(cbp_table);
    COPY(mb_mode_table);
    COPY(range_reduction_frame);
    COPY(rounding_control);
    COPY(post_processing);
    COPY(picture_resolution_index);
    COPY(luma_scale);
    COPY(luma_shift);
    pic_param->BFV(picture_fields, value) = 0; /* reset all bits */
    COPY_BFM(picture_fields, bits, picture_type);
    COPY_BFM(picture_fields, bits, frame_coding_mode);
    COPY_BFM(picture_fields, bits, top_field_first);
    COPY_BFM(picture_fields, bits, is_first_field);
    COPY_BFM(picture_fields, bits, intensity_compensation);
    pic_param->BFV(V_raw_coding, value) = 0; /* reset all bits */
    COPY_BFM(M_raw_coding, flags, mv_type_mb);
    COPY_BFM(M_raw_coding, flags, direct_mb);
    COPY_BFM(M_raw_coding, flags, skip_mb);
    COPY_BFM(M_raw_coding, flags, field_tx);
    COPY_BFM(M_raw_coding, flags, forward_mb);
    COPY_BFM(M_raw_coding, flags, ac_pred);
    COPY_BFM(M_raw_coding, flags, overflags);
    pic_param->BFV(V_bitplane_present, value) = 0; /* reset all bits */
    COPY_BFM(M_bitplane_present, flags, bp_mv_type_mb);
    COPY_BFM(M_bitplane_present, flags, bp_direct_mb);
    COPY_BFM(M_bitplane_present, flags, bp_skip_mb);
    COPY_BFM(M_bitplane_present, flags, bp_field_tx);
    COPY_BFM(M_bitplane_present, flags, bp_forward_mb);
    COPY_BFM(M_bitplane_present, flags, bp_ac_pred);
    COPY_BFM(M_bitplane_present, flags, bp_overflags);
    pic_param->BFV(reference_fields, value) = 0; /* reset all bits */
    COPY_BFM(reference_fields, bits, reference_distance_flag);
    COPY_BFM(reference_fields, bits, reference_distance);
    COPY_BFM(reference_fields, bits, num_reference_pictures);
    COPY_BFM(reference_fields, bits, reference_field_pic_indicator);
    pic_param->BFV(mv_fields, value) = 0; /* reset all bits */
    COPY_BFM(mv_fields, bits, mv_mode);
    COPY_BFM(mv_fields, bits, mv_mode2);
    COPY_BFM(mv_fields, bits, mv_table);
    COPY_BFM(mv_fields, bits, two_mv_block_pattern_table);
    COPY_BFM(mv_fields, bits, four_mv_switch);
    COPY_BFM(mv_fields, bits, four_mv_block_pattern_table);
    COPY_BFM(mv_fields, bits, extended_mv_flag);
    COPY_BFM(mv_fields, bits, extended_mv_range);
    COPY_BFM(mv_fields, bits, extended_dmv_flag);
    COPY_BFM(mv_fields, bits, extended_dmv_range);
    pic_param->BFV(pic_quantizer_fields, value) = 0; /* reset all bits */
    COPY_BFM(pic_quantizer_fields, bits, dquant);
    COPY_BFM(pic_quantizer_fields, bits, quantizer);
    COPY_BFM(pic_quantizer_fields, bits, half_qp);
    COPY_BFM(pic_quantizer_fields, bits, pic_quantizer_scale);
    COPY_BFM(pic_quantizer_fields, bits, pic_quantizer_type);
    COPY_BFM(pic_quantizer_fields, bits, dq_frame);
    COPY_BFM(pic_quantizer_fields, bits, dq_profile);
    COPY_BFM(pic_quantizer_fields, bits, dq_sb_edge);
    COPY_BFM(pic_quantizer_fields, bits, dq_db_edge);
    COPY_BFM(pic_quantizer_fields, bits, dq_binary_level);
    COPY_BFM(pic_quantizer_fields, bits, alt_pic_quantizer);
    pic_param->BFV(transform_fields, value) = 0; /* reset all bits */
    COPY_BFM(transform_fields, bits, variable_sized_transform_flag);
    COPY_BFM(transform_fields, bits, mb_level_transform_type_flag);
    COPY_BFM(transform_fields, bits, frame_level_transform_type);
    COPY_BFM(transform_fields, bits, transform_ac_codingset_idx1);
    COPY_BFM(transform_fields, bits, transform_ac_codingset_idx2);
    COPY_BFM(transform_fields, bits, intra_transform_dc_table);
#undef COPY_BFMP
#undef COPY_BFM
#undef COPY

    if (pic_param->BFV(V_bitplane_present, value)) {
        vc1_get_bitplane(&vc1_bitplane, &vc1_bitplane_size);
        if ((bitplane = vaapi_alloc_bitplane(vc1_bitplane_size)) == NULL)
            return -1;
        memcpy(bitplane, vc1_bitplane, vc1_bitplane_size);
    }

    slice_count = vc1_get_slice_count();
    for (i = 0; i < slice_count; i++) {
        if (vc1_get_slice_info(i, &vc1_slice_info) < 0)
            return -1;
        if (vc1_get_slice_data(i, &vc1_slice_data, &vc1_slice_data_size) < 0)
            return -1;
        if (vc1_slice_data_size != vc1_slice_info.slice_data_size)
            return -1;
        if ((slice_param = vaapi_alloc_slice(sizeof(*slice_param),
                                             vc1_slice_data,
                                             vc1_slice_data_size)) == NULL)
            return -1;

#define COPY(field) slice_param->field = vc1_slice_info.field
        COPY(macroblock_offset);
        COPY(slice_vertical_position);
#undef COPY
    }

    return vaapi_decode();
}
