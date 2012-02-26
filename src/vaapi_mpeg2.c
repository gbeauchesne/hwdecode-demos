/*
 *  vaapi_mpeg2.c - MPEG-2 decoding through VA API
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
#include "mpeg2.h"

int decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAPictureParameterBufferMPEG2 *pic_param;
    VASliceParameterBufferMPEG2 *slice_param;
    VAIQMatrixBufferMPEG2 *iq_matrix;
    int i, slice_count;

    MPEG2PictureInfo mpeg2_pic_info;
    MPEG2SliceInfo mpeg2_slice_info;
    MPEG2IQMatrix mpeg2_iq_matrix;
    const uint8_t *mpeg2_slice_data;
    unsigned int mpeg2_slice_data_size;

    if (!vaapi)
        return -1;

    mpeg2_get_picture_info(&mpeg2_pic_info);

    if (vaapi_init_decoder(VAProfileMPEG2Main, VAEntrypointVLD,
                           mpeg2_pic_info.width, mpeg2_pic_info.height) < 0)
        return -1;

    if ((pic_param = vaapi_alloc_picture(sizeof(*pic_param))) == NULL)
        return -1;

#define COPY(field) \
    pic_param->field = mpeg2_pic_info.field
#define COPY_BFM(a,b,c) \
    pic_param->BFM(a,b,c) = mpeg2_pic_info.a.b.c
    pic_param->horizontal_size = mpeg2_pic_info.width;
    pic_param->vertical_size = mpeg2_pic_info.height;
    pic_param->forward_reference_picture = 0xffffffff;
    pic_param->backward_reference_picture = 0xffffffff;
    COPY(picture_coding_type);
    COPY(f_code);
    pic_param->BFV(picture_coding_extension, value) = 0; /* reset all bits */
    COPY_BFM(picture_coding_extension, bits, intra_dc_precision);
    COPY_BFM(picture_coding_extension, bits, picture_structure);
    COPY_BFM(picture_coding_extension, bits, top_field_first);
    COPY_BFM(picture_coding_extension, bits, frame_pred_frame_dct);
    COPY_BFM(picture_coding_extension, bits, concealment_motion_vectors);
    COPY_BFM(picture_coding_extension, bits, q_scale_type);
    COPY_BFM(picture_coding_extension, bits, intra_vlc_format);
    COPY_BFM(picture_coding_extension, bits, alternate_scan);
    COPY_BFM(picture_coding_extension, bits, repeat_first_field);
    COPY_BFM(picture_coding_extension, bits, progressive_frame);
    COPY_BFM(picture_coding_extension, bits, is_first_field);
#undef COPY_BFM
#undef COPY

    if ((iq_matrix = vaapi_alloc_iq_matrix(sizeof(*iq_matrix))) == NULL)
        return -1;
    mpeg2_get_iq_matrix(&mpeg2_iq_matrix);

#define COPY(field) iq_matrix->field = mpeg2_iq_matrix.field
    COPY(load_intra_quantiser_matrix);
    COPY(load_non_intra_quantiser_matrix);
    COPY(load_chroma_intra_quantiser_matrix);
    COPY(load_chroma_non_intra_quantiser_matrix);
    for (i = 0; i < 64; i++) {
        COPY(intra_quantiser_matrix[i]);
        COPY(non_intra_quantiser_matrix[i]);
        COPY(chroma_intra_quantiser_matrix[i]);
        COPY(chroma_non_intra_quantiser_matrix[i]);
    }
#undef COPY

    slice_count = mpeg2_get_slice_count();
    for (i = 0; i < slice_count; i++) {
        if (mpeg2_get_slice_info(i, &mpeg2_slice_info) < 0)
            return -1;
        if (mpeg2_get_slice_data(i, &mpeg2_slice_data, &mpeg2_slice_data_size) < 0)
            return -1;
        if (mpeg2_slice_data_size != mpeg2_slice_info.slice_data_size)
            return -1;
        if ((slice_param = vaapi_alloc_slice(sizeof(*slice_param),
                                             mpeg2_slice_data,
                                             mpeg2_slice_data_size)) == NULL)
            return -1;

#define COPY(field) slice_param->field = mpeg2_slice_info.field
        COPY(macroblock_offset);
#if VA_CHECK_VERSION(0,32,0)
        COPY(slice_horizontal_position);
#endif
        COPY(slice_vertical_position);
        COPY(quantiser_scale_code);
        COPY(intra_slice_flag);
#undef COPY
    }

    return vaapi_decode();
}
