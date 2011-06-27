/*
 *  vdpau_mpeg2.c - MPEG-2 decoding through VDPAU
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
#include "mpeg2.h"

static const uint8_t ff_identity[64] = {
    0,   1,  2,  3,  4,  5,  6,  7,
    8,   9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};

static const uint8_t ff_zigzag_direct[64] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

static const uint8_t ff_mpeg1_default_intra_matrix[64] = {
    8,  16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

static const uint8_t ff_mpeg1_default_non_intra_matrix[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};

int decode(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpPictureInfoMPEG1Or2 *pic_info;
    int i;
    const uint8_t *intra_matrix, *intra_matrix_lookup;
    const uint8_t *inter_matrix, *inter_matrix_lookup;

    MPEG2PictureInfo mpeg2_pic_info;
    MPEG2IQMatrix mpeg2_iq_matrix;
    const uint8_t *mpeg2_slice_data;
    unsigned int mpeg2_slice_data_size;

    if (!vdpau)
        return -1;

    mpeg2_get_picture_info(&mpeg2_pic_info);
    mpeg2_get_iq_matrix(&mpeg2_iq_matrix);

    if (vdpau_init_decoder(VDP_DECODER_PROFILE_MPEG2_MAIN,
                           mpeg2_pic_info.width, mpeg2_pic_info.height) < 0)
        return -1;

    if ((pic_info = vdpau_alloc_picture()) == NULL)
        return -1;
    pic_info->forward_reference                 = VDP_INVALID_HANDLE;
    pic_info->backward_reference                = VDP_INVALID_HANDLE;
    pic_info->slice_count                       = mpeg2_get_slice_count();
    pic_info->picture_structure                 = mpeg2_pic_info.picture_coding_extension.bits.picture_structure;
    pic_info->picture_coding_type               = mpeg2_pic_info.picture_coding_type;
    pic_info->intra_dc_precision                = mpeg2_pic_info.picture_coding_extension.bits.intra_dc_precision;
    pic_info->frame_pred_frame_dct              = mpeg2_pic_info.picture_coding_extension.bits.frame_pred_frame_dct;
    pic_info->concealment_motion_vectors        = mpeg2_pic_info.picture_coding_extension.bits.concealment_motion_vectors;
    pic_info->intra_vlc_format                  = mpeg2_pic_info.picture_coding_extension.bits.intra_vlc_format;
    pic_info->alternate_scan                    = mpeg2_pic_info.picture_coding_extension.bits.alternate_scan;
    pic_info->q_scale_type                      = mpeg2_pic_info.picture_coding_extension.bits.q_scale_type;
    pic_info->top_field_first                   = mpeg2_pic_info.picture_coding_extension.bits.top_field_first;
    pic_info->full_pel_forward_vector           = 0;
    pic_info->full_pel_backward_vector          = 0;
    pic_info->f_code[0][0]                      = (mpeg2_pic_info.f_code >> 12) & 0xf;
    pic_info->f_code[0][1]                      = (mpeg2_pic_info.f_code >>  8) & 0xf;
    pic_info->f_code[1][0]                      = (mpeg2_pic_info.f_code >>  4) & 0xf;
    pic_info->f_code[1][1]                      = mpeg2_pic_info.f_code & 0xf;

    if (mpeg2_iq_matrix.load_intra_quantiser_matrix) {
	intra_matrix = mpeg2_iq_matrix.intra_quantiser_matrix;
	intra_matrix_lookup = ff_zigzag_direct;
    }
    else {
	intra_matrix = ff_mpeg1_default_intra_matrix;
	intra_matrix_lookup = ff_identity;
    }
    if (mpeg2_iq_matrix.load_non_intra_quantiser_matrix) {
	inter_matrix = mpeg2_iq_matrix.non_intra_quantiser_matrix;
	inter_matrix_lookup = ff_zigzag_direct;
    }
    else {
	inter_matrix = ff_mpeg1_default_non_intra_matrix;
	inter_matrix_lookup = ff_identity;
    }
    for (i = 0; i < 64; i++) {
	pic_info->intra_quantizer_matrix[intra_matrix_lookup[i]] =
	    intra_matrix[i];
	pic_info->non_intra_quantizer_matrix[inter_matrix_lookup[i]] =
            inter_matrix[i];
    }

    for (i = 0; i < pic_info->slice_count; i++) {
        if (mpeg2_get_slice_data(i, &mpeg2_slice_data, &mpeg2_slice_data_size) < 0)
            return -1;
        if (vdpau_append_slice_data(mpeg2_slice_data, mpeg2_slice_data_size) < 0)
            return -1;
    }

    return vdpau_decode();
}
