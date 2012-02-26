/*
 *  mpeg2.h - MPEG-2 video dump
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

#ifndef MPEG2_H
#define MPEG2_H

typedef struct _MPEG2PictureInfo MPEG2PictureInfo;
typedef struct _MPEG2IQMatrix    MPEG2IQMatrix;
typedef struct _MPEG2SliceInfo   MPEG2SliceInfo;

struct _MPEG2PictureInfo {
    unsigned short      width;
    unsigned short      height;
    unsigned int        picture_coding_type;
    unsigned int        f_code;
    union {
        struct {
            unsigned int intra_dc_precision             : 2; 
            unsigned int picture_structure              : 2; 
            unsigned int top_field_first                : 1; 
            unsigned int frame_pred_frame_dct           : 1; 
            unsigned int concealment_motion_vectors     : 1;
            unsigned int q_scale_type                   : 1;
            unsigned int intra_vlc_format               : 1;
            unsigned int alternate_scan                 : 1;
            unsigned int repeat_first_field             : 1;
            unsigned int progressive_frame              : 1;
            unsigned int is_first_field                 : 1;
        } bits;
        unsigned int value;
    } picture_coding_extension;
};

struct _MPEG2IQMatrix {
    unsigned char       load_intra_quantiser_matrix;
    unsigned char       load_non_intra_quantiser_matrix;
    unsigned char       load_chroma_intra_quantiser_matrix;
    unsigned char       load_chroma_non_intra_quantiser_matrix;
    unsigned char       intra_quantiser_matrix[64];
    unsigned char       non_intra_quantiser_matrix[64];
    unsigned char       chroma_intra_quantiser_matrix[64];
    unsigned char       chroma_non_intra_quantiser_matrix[64];
};

struct _MPEG2SliceInfo {
    unsigned int        slice_data_size;
    unsigned int        slice_data_offset;
    unsigned int        macroblock_offset;
    unsigned int        slice_horizontal_position;
    unsigned int        slice_vertical_position;
    int                 quantiser_scale_code;
    int                 intra_slice_flag;
};

void mpeg2_get_video_data(const uint8_t **data, unsigned int *size);
void mpeg2_get_picture_info(MPEG2PictureInfo *pic_info);
void mpeg2_get_iq_matrix(MPEG2IQMatrix *iq_matrix);
int mpeg2_get_slice_count(void);
int mpeg2_get_slice_info(int slice, MPEG2SliceInfo *slice_info);
int mpeg2_get_slice_data(int slice, const uint8_t **data, unsigned int *size);

#endif /* MPEG2_H */
