/*
 *  h264.h - H.264 video dump
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

#ifndef H264_H
#define H264_H

typedef struct _H264PictureInfo H264PictureInfo;
typedef struct _H264IQMatrix    H264IQMatrix;
typedef struct _H264SliceInfo   H264SliceInfo;

struct _H264PictureInfo {
    unsigned char       profile_idc;
    unsigned char       level_idc;
    unsigned short      width;
    unsigned short      height;
    unsigned short      picture_width_in_mbs_minus1;
    unsigned short      picture_height_in_mbs_minus1;
    unsigned char       bit_depth_luma_minus8;
    unsigned char       bit_depth_chroma_minus8;
    unsigned char       num_ref_frames;
    union {
        struct {
            unsigned int chroma_format_idc                      : 2;
            unsigned int residual_colour_transform_flag         : 1; 
            unsigned int gaps_in_frame_num_value_allowed_flag   : 1; 
            unsigned int frame_mbs_only_flag                    : 1; 
            unsigned int mb_adaptive_frame_field_flag           : 1; 
            unsigned int direct_8x8_inference_flag              : 1; 
            unsigned int MinLumaBiPredSize8x8                   : 1;
            unsigned int log2_max_frame_num_minus4              : 4;
            unsigned int pic_order_cnt_type                     : 2;
            unsigned int log2_max_pic_order_cnt_lsb_minus4      : 4;
            unsigned int delta_pic_order_always_zero_flag       : 1;
        } bits;
        unsigned int value;
    } seq_fields;
    unsigned char       num_slice_groups_minus1;
    unsigned char       slice_group_map_type;
    unsigned short      slice_group_change_rate_minus1;
    signed char         pic_init_qp_minus26;
    signed char         pic_init_qs_minus26;
    signed char         chroma_qp_index_offset;
    signed char         second_chroma_qp_index_offset;
    union {
        struct {
            unsigned int entropy_coding_mode_flag               : 1;
            unsigned int weighted_pred_flag                     : 1;
            unsigned int weighted_bipred_idc                    : 2;
            unsigned int transform_8x8_mode_flag                : 1;
            unsigned int field_pic_flag                         : 1;
            unsigned int bottom_field_flag                      : 1;
            unsigned int constrained_intra_pred_flag            : 1;
            unsigned int pic_order_present_flag                 : 1;
            unsigned int deblocking_filter_control_present_flag : 1;
            unsigned int redundant_pic_cnt_present_flag         : 1;
            unsigned int reference_pic_flag                     : 1;
        } bits;
        unsigned int value;
    } pic_fields;
};

struct _H264IQMatrix {
    unsigned char       ScalingList4x4[6][16];
    unsigned char       ScalingList8x8[2][64];
};

struct _H264SliceInfo {
    unsigned short      macroblock_offset;
    unsigned short      first_mb_in_slice;
    unsigned char       slice_type;
    unsigned char       direct_spatial_mv_pred_flag;
    unsigned char       num_ref_idx_l0_active_minus1;
    unsigned char       num_ref_idx_l1_active_minus1;
    unsigned char       cabac_init_idc;
    char                slice_qp_delta;
    unsigned char       disable_deblocking_filter_idc;
    char                slice_alpha_c0_offset_div2;
    char                slice_beta_offset_div2;
    unsigned char       luma_log2_weight_denom;
    unsigned char       chroma_log2_weight_denom;
    unsigned char       luma_weight_l0_flag;
    short               luma_weight_l0[32];
    short               luma_offset_l0[32];
    unsigned char       chroma_weight_l0_flag;
    short               chroma_weight_l0[32][2];
    short               chroma_offset_l0[32][2];
    unsigned char       luma_weight_l1_flag;
    short               luma_weight_l1[32];
    short               luma_offset_l1[32];
    unsigned char       chroma_weight_l1_flag;
    short               chroma_weight_l1[32][2];
    short               chroma_offset_l1[32][2];
};

void h264_get_video_data(const uint8_t **data, unsigned int *size);
void h264_get_picture_info(H264PictureInfo *pic_info);
void h264_get_iq_matrix(H264IQMatrix *iq_matrix);
void h264_get_slice_info(H264SliceInfo *slice_info);
void h264_get_slice_data(const uint8_t **data, unsigned int *size);

#endif /* H264_H */
