/*
 *  vc1.h - VC-1 video dump
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

#ifndef VC1_H
#define VC1_H

typedef struct _VC1PictureInfo VC1PictureInfo;
typedef struct _VC1SliceInfo   VC1SliceInfo;

struct _VC1PictureInfo {
    unsigned char       profile;
    unsigned char       level;
    unsigned short      width;
    unsigned short      height;
    union {
        struct {
            unsigned int pulldown                       : 1;
            unsigned int interlace                      : 1;
            unsigned int tfcntrflag                     : 1;
            unsigned int finterpflag                    : 1;
            unsigned int psf                            : 1;
            unsigned int multires                       : 1;
            unsigned int overlap                        : 1;
            unsigned int syncmarker                     : 1;
            unsigned int rangered                       : 1;
            unsigned int max_b_frames                   : 3;
        } bits;
        unsigned int value;
    } sequence_fields;
    union {
        struct {
            unsigned int broken_link                    : 1;
            unsigned int closed_entry                   : 1;
            unsigned int panscan_flag                   : 1;
            unsigned int loopfilter                     : 1;
        } bits;
        unsigned int value;
    } entrypoint_fields;
    unsigned char       conditional_overlap_flag;
    unsigned char       fast_uvmc_flag;
    union {
        struct {
            unsigned int luma_flag                      : 1;
            unsigned int luma                           : 3;
            unsigned int chroma_flag                    : 1;
            unsigned int chroma                         : 3;
        } bits;
        unsigned int value;
    } range_mapping_fields;
    unsigned char       b_picture_fraction;
    unsigned char       cbp_table;
    unsigned char       mb_mode_table;
    unsigned char       range_reduction_frame;
    unsigned char       rounding_control;
    unsigned char       post_processing;
    unsigned char       picture_resolution_index;
    unsigned char       luma_scale;
    unsigned char       luma_shift;
    union {
        struct {
            unsigned int picture_type                   : 3;
            unsigned int frame_coding_mode              : 3;
            unsigned int top_field_first                : 1;
            unsigned int is_first_field                 : 1;
            unsigned int intensity_compensation         : 1;
        } bits;
        unsigned int value;
    } picture_fields;
    union {
        struct {
            unsigned int mv_type_mb                     : 1;
            unsigned int direct_mb                      : 1;
            unsigned int skip_mb                        : 1;
            unsigned int field_tx                       : 1;
            unsigned int forward_mb                     : 1;
            unsigned int ac_pred                        : 1;
            unsigned int overflags                      : 1;
        } flags;
        unsigned int value;
    } raw_coding;
    union {
        struct {
            unsigned int bp_mv_type_mb                  : 1;
            unsigned int bp_direct_mb                   : 1;
            unsigned int bp_skip_mb                     : 1;
            unsigned int bp_field_tx                    : 1;
            unsigned int bp_forward_mb                  : 1;
            unsigned int bp_ac_pred                     : 1;
            unsigned int bp_overflags                   : 1;
        } flags;
        unsigned int value;
    } bitplane_present;
    union {
        struct {
            unsigned int reference_distance_flag        : 1;
            unsigned int reference_distance             : 5;
            unsigned int num_reference_pictures         : 1;
            unsigned int reference_field_pic_indicator  : 1;
        } bits;
        unsigned int value;
    } reference_fields;
    union {
        struct {
            unsigned int mv_mode                        : 3;
            unsigned int mv_mode2                       : 3;
            unsigned int mv_table                       : 3;
            unsigned int two_mv_block_pattern_table     : 2;
            unsigned int four_mv_switch                 : 1;
            unsigned int four_mv_block_pattern_table    : 2;
            unsigned int extended_mv_flag               : 1;
            unsigned int extended_mv_range              : 2;
            unsigned int extended_dmv_flag              : 1;
            unsigned int extended_dmv_range             : 2;
        } bits;
        unsigned int value;
    } mv_fields;
    union {
        struct {
            unsigned int dquant                         : 2;
            unsigned int quantizer                      : 2;
            unsigned int half_qp                        : 1;
            unsigned int pic_quantizer_scale            : 5;
            unsigned int pic_quantizer_type             : 1;
            unsigned int dq_frame                       : 1;
            unsigned int dq_profile                     : 2;
            unsigned int dq_sb_edge                     : 2;
            unsigned int dq_db_edge                     : 2;
            unsigned int dq_binary_level                : 1;
            unsigned int alt_pic_quantizer              : 5;
        } bits;
        unsigned int value;
    } pic_quantizer_fields;
    union {
        struct {
            unsigned int variable_sized_transform_flag  : 1;
            unsigned int mb_level_transform_type_flag   : 1;
            unsigned int frame_level_transform_type     : 2;
            unsigned int transform_ac_codingset_idx1    : 2;
            unsigned int transform_ac_codingset_idx2    : 2;
            unsigned int intra_transform_dc_table       : 1;
        } bits;
        unsigned int value;
    } transform_fields;
};

struct _VC1SliceInfo {
    unsigned int        slice_data_size;
    unsigned int        slice_data_offset;
    unsigned int        macroblock_offset;
    int                 slice_vertical_position;
};

void vc1_get_video_data(const uint8_t **data, unsigned int *size);
void vc1_get_picture_info(VC1PictureInfo *pic_info);
void vc1_get_bitplane(const uint8_t **data, unsigned int *size);
int vc1_get_slice_count(void);
int vc1_get_slice_info(int slice, VC1SliceInfo *slice_info);
int vc1_get_slice_data(int slice, const uint8_t **data, unsigned int *size);

#endif /* VC1_H */
