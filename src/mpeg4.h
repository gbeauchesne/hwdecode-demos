/*
 *  mpeg4.h - MPEG-4 video dump
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

#ifndef MPEG4_H
#define MPEG4_H

typedef struct _MPEG4PictureInfo MPEG4PictureInfo;
typedef struct _MPEG4IQMatrix    MPEG4IQMatrix;
typedef struct _MPEG4SliceInfo   MPEG4SliceInfo;

struct _MPEG4PictureInfo
{
    unsigned short      width;
    unsigned short      height;
    union {
        struct {
            unsigned int short_video_header                     : 1; 
            unsigned int chroma_format                          : 2; 
            unsigned int interlaced                             : 1; 
            unsigned int obmc_disable                           : 1; 
            unsigned int sprite_enable                          : 2; 
            unsigned int sprite_warping_accuracy                : 2; 
            unsigned int quant_type                             : 1; 
            unsigned int quarter_sample                         : 1; 
            unsigned int data_partitioned                       : 1; 
            unsigned int reversible_vlc                         : 1; 
            unsigned int resync_marker_disable                  : 1; 
        } bits;
        unsigned int value;
    } vol_fields;
    unsigned char       no_of_sprite_warping_points;
    short               sprite_trajectory_du[3];
    short               sprite_trajectory_dv[3];
    unsigned char       quant_precision;
    union {
        struct {
            unsigned int vop_coding_type                        : 2; 
            unsigned int backward_reference_vop_coding_type     : 2; 
            unsigned int vop_rounding_type                      : 1; 
            unsigned int intra_dc_vlc_thr                       : 3; 
            unsigned int top_field_first                        : 1; 
            unsigned int alternate_vertical_scan_flag           : 1; 
        } bits;
        unsigned int value;
    } vop_fields;
    unsigned char       vop_fcode_forward;
    unsigned char       vop_fcode_backward;
    unsigned short      vop_time_increment_resolution;
    unsigned char       num_gobs_in_vop;
    unsigned char       num_macroblocks_in_gob;
    short               TRB;
    short               TRD;
};

struct _MPEG4IQMatrix {
    unsigned char       load_intra_quant_mat;
    unsigned char       load_non_intra_quant_mat;
    unsigned char       intra_quant_mat[64];
    unsigned char       non_intra_quant_mat[64];
};

struct _MPEG4SliceInfo {
    unsigned int        slice_data_size;
    unsigned int        slice_data_offset;
    unsigned int        macroblock_offset;
    unsigned int        macroblock_number;
    int                 quant_scale;
};

void mpeg4_get_video_data(const uint8_t **data, unsigned int *size);
void mpeg4_get_picture_info(MPEG4PictureInfo *pic_info);
void mpeg4_get_iq_matrix(MPEG4IQMatrix *iq_matrix);
int mpeg4_get_slice_count(void);
int mpeg4_get_slice_info(int slice, MPEG4SliceInfo *slice_info);
int mpeg4_get_slice_data(int slice, const uint8_t **data, unsigned int *size);

#endif /* MPEG4_H */
