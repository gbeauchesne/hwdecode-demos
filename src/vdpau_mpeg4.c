/*
 *  vdpau_mpeg4.c - MPEG-4 decoding through VDPAU
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
#include "mpeg4.h"
#include "put_bits.h"

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

static const uint8_t ff_mpeg4_default_intra_matrix[64] = {
     8, 17, 18, 19, 21, 23, 25, 27,
    17, 18, 19, 21, 23, 25, 27, 28,
    20, 21, 22, 23, 24, 26, 28, 30,
    21, 22, 23, 24, 26, 28, 30, 32,
    22, 23, 24, 26, 28, 30, 32, 35,
    23, 24, 26, 28, 30, 32, 35, 38,
    25, 26, 28, 30, 32, 35, 38, 41,
    27, 28, 30, 32, 35, 38, 41, 45,
};

static const uint8_t ff_mpeg4_default_non_intra_matrix[64] = {
    16, 17, 18, 19, 20, 21, 22, 23,
    17, 18, 19, 20, 21, 22, 23, 24,
    18, 19, 20, 21, 22, 23, 24, 25,
    19, 20, 21, 22, 23, 24, 26, 27,
    20, 21, 22, 23, 25, 26, 27, 28,
    21, 22, 23, 24, 26, 27, 28, 30,
    22, 23, 24, 26, 27, 28, 30, 31,
    23, 24, 25, 27, 28, 30, 31, 33,
};

static inline int ilog2(uint32_t v)
{
    /* From <http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog> */
    uint32_t r, shift;
    r     = (v > 0xffff) << 4; v >>= r;
    shift = (v > 0xff  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xf   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    return r | (v >> 1);
}

#define VOP_STARTCODE 0x01b6

enum {
    VOP_I_TYPE = 0,
    VOP_P_TYPE,
    VOP_B_TYPE,
    VOP_S_TYPE
};

static int append_picture_header(const MPEG4PictureInfo *pic_info)
{
    MPEG4SliceInfo slice_info;
    uint8_t buf[32], buf_size;
    const uint8_t *slice_data;
    unsigned int slice_data_size;
    PutBitContext pb;
    int r;

    if (mpeg4_get_slice_info(0, &slice_info) < 0)
        return -1;

    int time_incr = 1 + ilog2(pic_info->vop_time_increment_resolution - 1);
    if (time_incr < 1)
        time_incr = 1;

    /* Reconstruct the VOP header */
    init_put_bits(&pb, buf, sizeof(buf));
    put_bits(&pb, 16, 0);                               /* vop header */
    put_bits(&pb, 16, VOP_STARTCODE);                   /* vop header */
    put_bits(&pb, 2, pic_info->vop_fields.bits.vop_coding_type);

    put_bits(&pb, 1, 0);
    put_bits(&pb, 1, 1);                                /* marker */
    put_bits(&pb, time_incr, 0);                        /* time increment */
    put_bits(&pb, 1, 1);                                /* marker */
    put_bits(&pb, 1, 1);                                /* vop coded */
    if (pic_info->vop_fields.bits.vop_coding_type == VOP_P_TYPE)
        put_bits(&pb, 1, pic_info->vop_fields.bits.vop_rounding_type);
    put_bits(&pb, 3, pic_info->vop_fields.bits.intra_dc_vlc_thr);
    if (pic_info->vol_fields.bits.interlaced) {
        put_bits(&pb, 1, pic_info->vop_fields.bits.top_field_first);
        put_bits(&pb, 1, pic_info->vop_fields.bits.alternate_vertical_scan_flag);
    }
    put_bits(&pb, pic_info->quant_precision, slice_info.quant_scale);
    if (pic_info->vop_fields.bits.vop_coding_type != VOP_I_TYPE)
        put_bits(&pb, 3, pic_info->vop_fcode_forward);
    if (pic_info->vop_fields.bits.vop_coding_type == VOP_B_TYPE)
        put_bits(&pb, 3, pic_info->vop_fcode_backward);

    /* Merge in bits from the first byte of the slice */
    if ((put_bits_count(&pb) % 8) != slice_info.macroblock_offset)
        return -1;
    if (mpeg4_get_slice_data(0, &slice_data, &slice_data_size) < 0)
        return -1;
    r = 8 - (put_bits_count(&pb) % 8);
    put_bits(&pb, r, slice_data[0] & ((1U << r) - 1));
    flush_put_bits(&pb);

    buf_size = put_bits_count(&pb) / 8;
    return vdpau_append_slice_data(buf, buf_size);
}

int decode(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpPictureInfoMPEG4Part2 *pic_info;
    int i;
    const uint8_t *intra_matrix, *intra_matrix_lookup;
    const uint8_t *inter_matrix, *inter_matrix_lookup;

    MPEG4PictureInfo mpeg4_pic_info;
    MPEG4IQMatrix mpeg4_iq_matrix;
    const uint8_t *mpeg4_slice_data;
    unsigned int mpeg4_slice_data_size;

    if (!vdpau)
        return -1;

    mpeg4_get_picture_info(&mpeg4_pic_info);
    mpeg4_get_iq_matrix(&mpeg4_iq_matrix);

    if (vdpau_init_decoder(VDP_DECODER_PROFILE_MPEG4_PART2_ASP,
                           mpeg4_pic_info.width, mpeg4_pic_info.height) < 0)
        return -1;

    if ((pic_info = vdpau_alloc_picture()) == NULL)
        return -1;
    pic_info->forward_reference                 = VDP_INVALID_HANDLE;
    pic_info->backward_reference                = VDP_INVALID_HANDLE;
    pic_info->vop_time_increment_resolution     = mpeg4_pic_info.vop_time_increment_resolution;
    pic_info->vop_coding_type                   = mpeg4_pic_info.vop_fields.bits.vop_coding_type;
    pic_info->vop_fcode_forward                 = mpeg4_pic_info.vop_fcode_forward;
    pic_info->vop_fcode_backward                = mpeg4_pic_info.vop_fcode_backward;
    pic_info->resync_marker_disable             = mpeg4_pic_info.vol_fields.bits.resync_marker_disable;
    pic_info->interlaced                        = mpeg4_pic_info.vol_fields.bits.interlaced;
    pic_info->quant_type                        = mpeg4_pic_info.vol_fields.bits.quant_type;
    pic_info->quarter_sample                    = mpeg4_pic_info.vol_fields.bits.quarter_sample;
    pic_info->short_video_header                = mpeg4_pic_info.vol_fields.bits.short_video_header;
    pic_info->rounding_control                  = mpeg4_pic_info.vop_fields.bits.vop_rounding_type;
    pic_info->alternate_vertical_scan_flag      = mpeg4_pic_info.vop_fields.bits.alternate_vertical_scan_flag;
    pic_info->top_field_first                   = mpeg4_pic_info.vop_fields.bits.top_field_first;

    if (mpeg4_iq_matrix.load_intra_quant_mat) {
	intra_matrix = mpeg4_iq_matrix.intra_quant_mat;
	intra_matrix_lookup = ff_zigzag_direct;
    }
    else {
	intra_matrix = ff_mpeg4_default_intra_matrix;
	intra_matrix_lookup = ff_identity;
    }
    if (mpeg4_iq_matrix.load_non_intra_quant_mat) {
	inter_matrix = mpeg4_iq_matrix.non_intra_quant_mat;
	inter_matrix_lookup = ff_zigzag_direct;
    }
    else {
	inter_matrix = ff_mpeg4_default_non_intra_matrix;
	inter_matrix_lookup = ff_identity;
    }
    for (i = 0; i < 64; i++) {
	pic_info->intra_quantizer_matrix[intra_matrix_lookup[i]] =
	    intra_matrix[i];
	pic_info->non_intra_quantizer_matrix[inter_matrix_lookup[i]] =
            inter_matrix[i];
    }

    if (append_picture_header(&mpeg4_pic_info) < 0)
        return -1;

    for (i = 0; i < mpeg4_get_slice_count(); i++) {
        if (mpeg4_get_slice_data(i, &mpeg4_slice_data, &mpeg4_slice_data_size) < 0)
            return -1;
        /* The first byte was merged with the header */
        if (i == 0) {
            ++mpeg4_slice_data;
            --mpeg4_slice_data_size;
        }
        if (vdpau_append_slice_data(mpeg4_slice_data, mpeg4_slice_data_size) < 0)
            return -1;
    }

    return vdpau_decode();
}
