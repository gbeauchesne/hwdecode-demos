/*
 *  vaapi_h264.c - H.264 decoding through VA API
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
#include "h264.h"

static void vaapi_h264_init_picture(VAPictureH264 *va_pic)
{
    va_pic->picture_id          = 0xffffffff;
    va_pic->flags               = VA_PICTURE_H264_INVALID;
    va_pic->TopFieldOrderCnt    = 0;
    va_pic->BottomFieldOrderCnt = 0;
}

int decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAPictureParameterBufferH264 *pic_param;
    VASliceParameterBufferH264 *slice_param;
    VAIQMatrixBufferH264 *iq_matrix;
    int i;

    H264PictureInfo h264_pic_info;
    H264SliceInfo h264_slice_info;
    H264IQMatrix h264_iq_matrix;
    const uint8_t *h264_slice_data;
    unsigned int h264_slice_data_size;

    if (!vaapi)
        return -1;

    h264_get_picture_info(&h264_pic_info);
    h264_get_slice_info(&h264_slice_info);
    h264_get_iq_matrix(&h264_iq_matrix);
    h264_get_slice_data(&h264_slice_data, &h264_slice_data_size);

    if (vaapi_init_decoder(VAProfileH264High, VAEntrypointVLD,
                           h264_pic_info.width, h264_pic_info.height) < 0)
        return -1;

    if ((pic_param = vaapi_alloc_picture(sizeof(*pic_param))) == NULL)
        return -1;

#define COPY(field) \
    pic_param->field = h264_pic_info.field
#define COPY_BFM(a,b,c) \
    pic_param->BFM(a,b,c) = h264_pic_info.a.b.c
    COPY(picture_width_in_mbs_minus1);
    COPY(picture_height_in_mbs_minus1);
    COPY(bit_depth_luma_minus8);
    COPY(bit_depth_chroma_minus8);
    COPY(num_ref_frames);
    pic_param->BFV(seq_fields, value) = 0; /* reset all bits */
    COPY_BFM(seq_fields, bits, chroma_format_idc);
    COPY_BFM(seq_fields, bits, residual_colour_transform_flag);
    COPY_BFM(seq_fields, bits, frame_mbs_only_flag);
    COPY_BFM(seq_fields, bits, mb_adaptive_frame_field_flag);
    COPY_BFM(seq_fields, bits, direct_8x8_inference_flag);
    COPY_BFM(seq_fields, bits, MinLumaBiPredSize8x8);
    NEW(COPY_BFM(seq_fields, bits, log2_max_frame_num_minus4));
    NEW(COPY_BFM(seq_fields, bits, pic_order_cnt_type));
    NEW(COPY_BFM(seq_fields, bits, log2_max_pic_order_cnt_lsb_minus4));
    NEW(COPY_BFM(seq_fields, bits, delta_pic_order_always_zero_flag));
    COPY(num_slice_groups_minus1);
    COPY(slice_group_map_type);
    COPY(pic_init_qp_minus26);
    COPY(chroma_qp_index_offset);
    COPY(second_chroma_qp_index_offset);
    pic_param->BFV(pic_fields, value) = 0; /* reset all bits */
    COPY_BFM(pic_fields, bits, entropy_coding_mode_flag);
    COPY_BFM(pic_fields, bits, weighted_pred_flag);
    COPY_BFM(pic_fields, bits, weighted_bipred_idc);
    COPY_BFM(pic_fields, bits, transform_8x8_mode_flag);
    COPY_BFM(pic_fields, bits, field_pic_flag);
    COPY_BFM(pic_fields, bits, constrained_intra_pred_flag);
    NEW(COPY_BFM(pic_fields, bits, pic_order_present_flag));
    NEW(COPY_BFM(pic_fields, bits, deblocking_filter_control_present_flag));
    NEW(COPY_BFM(pic_fields, bits, redundant_pic_cnt_present_flag));
    NEW(COPY_BFM(pic_fields, bits, reference_pic_flag));
    pic_param->frame_num = 0;
    pic_param->CurrPic.picture_id = vaapi->surface_id;
    pic_param->CurrPic.TopFieldOrderCnt = 0;
    pic_param->CurrPic.BottomFieldOrderCnt = 0;
    for (i = 0; i < 16; i++)
        vaapi_h264_init_picture(&pic_param->ReferenceFrames[i]);
#undef COPY_BFM
#undef COPY

    if ((iq_matrix = vaapi_alloc_iq_matrix(sizeof(*iq_matrix))) == NULL)
        return -1;

    memcpy(iq_matrix->ScalingList4x4, h264_iq_matrix.ScalingList4x4,
           sizeof(iq_matrix->ScalingList4x4));
    memcpy(iq_matrix->ScalingList8x8, h264_iq_matrix.ScalingList8x8,
           sizeof(iq_matrix->ScalingList8x8));

    if ((slice_param = vaapi_alloc_slice(sizeof(*slice_param),
                                         h264_slice_data,
                                         h264_slice_data_size)) == NULL)
        return -1;

#define COPY(field) slice_param->field = h264_slice_info.field
    slice_param->slice_data_bit_offset = h264_slice_info.macroblock_offset;
    COPY(first_mb_in_slice);
    COPY(slice_type);
    COPY(direct_spatial_mv_pred_flag);
    COPY(num_ref_idx_l0_active_minus1);
    COPY(num_ref_idx_l1_active_minus1);
    COPY(cabac_init_idc);
    COPY(slice_qp_delta);
    COPY(disable_deblocking_filter_idc);
    COPY(slice_alpha_c0_offset_div2);
    COPY(slice_beta_offset_div2);
    COPY(luma_log2_weight_denom);
    COPY(chroma_log2_weight_denom);
    COPY(luma_weight_l0_flag);
    COPY(chroma_weight_l0_flag);
    COPY(luma_weight_l1_flag);
    COPY(chroma_weight_l1_flag);
#undef COPY
    for (i = 0; i < 32; i++) {
        vaapi_h264_init_picture(&slice_param->RefPicList0[i]);
        vaapi_h264_init_picture(&slice_param->RefPicList1[i]);
    }

    return vaapi_decode();
}
