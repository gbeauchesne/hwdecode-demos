/*
 *  vdpau_h264.c - H.264 decoding through VDPAU
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
#include "h264.h"

static void vdpau_h264_init_reference_frame(VdpReferenceFrameH264 *rf)
{
    rf->surface                 = VDP_INVALID_HANDLE;
    rf->is_long_term            = VDP_FALSE;
    rf->top_is_reference        = VDP_FALSE;
    rf->bottom_is_reference     = VDP_FALSE;
    rf->field_order_cnt[0]      = 0;
    rf->field_order_cnt[1]      = 0;
    rf->frame_idx               = 0;
}

int decode(void)
{
    static const uint8_t start_code_prefix_one_3bytes[] = { 0x00, 0x00, 0x01 };
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpPictureInfoH264 *pic_info;
    int i, j;

    H264PictureInfo h264_pic_info;
    H264SliceInfo h264_slice_info;
    H264IQMatrix h264_iq_matrix;
    const uint8_t *h264_slice_data;
    unsigned int h264_slice_data_size;

    if (!vdpau)
        return -1;

    h264_get_picture_info(&h264_pic_info);
    h264_get_slice_info(&h264_slice_info);
    h264_get_iq_matrix(&h264_iq_matrix);
    h264_get_slice_data(&h264_slice_data, &h264_slice_data_size);

    if (vdpau_init_decoder(VDP_DECODER_PROFILE_H264_HIGH,
                           h264_pic_info.width, h264_pic_info.height) < 0)
        return -1;

    if ((pic_info = vdpau_alloc_picture()) == NULL)
        return -1;
    pic_info->slice_count                               = 1;
    pic_info->field_order_cnt[0]                        = 0;
    pic_info->field_order_cnt[1]                        = 0;
    pic_info->is_reference                              = h264_pic_info.pic_fields.bits.reference_pic_flag;
    pic_info->frame_num                                 = 0;
    pic_info->field_pic_flag                            = h264_pic_info.pic_fields.bits.field_pic_flag;
    pic_info->bottom_field_flag                         = h264_pic_info.pic_fields.bits.bottom_field_flag;
    pic_info->num_ref_frames                            = h264_pic_info.num_ref_frames;
    pic_info->mb_adaptive_frame_field_flag              = h264_pic_info.seq_fields.bits.mb_adaptive_frame_field_flag && !pic_info->field_pic_flag;
    pic_info->constrained_intra_pred_flag               = h264_pic_info.pic_fields.bits.constrained_intra_pred_flag;
    pic_info->weighted_pred_flag                        = h264_pic_info.pic_fields.bits.weighted_pred_flag;
    pic_info->weighted_bipred_idc                       = h264_pic_info.pic_fields.bits.weighted_bipred_idc;
    pic_info->frame_mbs_only_flag                       = h264_pic_info.seq_fields.bits.frame_mbs_only_flag;
    pic_info->transform_8x8_mode_flag                   = h264_pic_info.pic_fields.bits.transform_8x8_mode_flag;
    pic_info->chroma_qp_index_offset                    = h264_pic_info.chroma_qp_index_offset;
    pic_info->second_chroma_qp_index_offset             = h264_pic_info.second_chroma_qp_index_offset;
    pic_info->pic_init_qp_minus26                       = h264_pic_info.pic_init_qp_minus26;
    pic_info->num_ref_idx_l0_active_minus1              = h264_slice_info.num_ref_idx_l0_active_minus1;
    pic_info->num_ref_idx_l1_active_minus1              = h264_slice_info.num_ref_idx_l1_active_minus1;
    pic_info->log2_max_frame_num_minus4                 = h264_pic_info.seq_fields.bits.log2_max_frame_num_minus4;
    pic_info->pic_order_cnt_type                        = h264_pic_info.seq_fields.bits.pic_order_cnt_type;
    pic_info->log2_max_pic_order_cnt_lsb_minus4         = h264_pic_info.seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4;
    pic_info->delta_pic_order_always_zero_flag          = h264_pic_info.seq_fields.bits.delta_pic_order_always_zero_flag;
    pic_info->direct_8x8_inference_flag                 = h264_pic_info.seq_fields.bits.direct_8x8_inference_flag;
    pic_info->entropy_coding_mode_flag                  = h264_pic_info.pic_fields.bits.entropy_coding_mode_flag;
    pic_info->pic_order_present_flag                    = h264_pic_info.pic_fields.bits.pic_order_present_flag;
    pic_info->deblocking_filter_control_present_flag    = h264_pic_info.pic_fields.bits.deblocking_filter_control_present_flag;
    pic_info->redundant_pic_cnt_present_flag            = h264_pic_info.pic_fields.bits.redundant_pic_cnt_present_flag;

    for (j = 0; j < 6; j++) {
        for (i = 0; i < 16; i++)
            pic_info->scaling_lists_4x4[j][i] = h264_iq_matrix.ScalingList4x4[j][i];
    }
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 64; i++)
            pic_info->scaling_lists_8x8[j][i] = h264_iq_matrix.ScalingList8x8[j][i];
    }

    for (i = 0; i < ARRAY_ELEMS(pic_info->referenceFrames); i++)
        vdpau_h264_init_reference_frame(&pic_info->referenceFrames[i]);

    if (memcmp(h264_slice_data, start_code_prefix_one_3bytes, 3) != 0) {
        if (vdpau_append_slice_data(start_code_prefix_one_3bytes, 3) < 0)
            return -1;
    }
    if (vdpau_append_slice_data(h264_slice_data, h264_slice_data_size) < 0)
        return -1;

    return vdpau_decode();
}
