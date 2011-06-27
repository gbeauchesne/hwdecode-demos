/*
 *  vaapi_mpeg4.c - MPEG-4 decoding through VA API
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
#include "mpeg4.h"

int decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAPictureParameterBufferMPEG4 *pic_param;
    VASliceParameterBufferMPEG4 *slice_param;
    VAIQMatrixBufferMPEG4 *iq_matrix;
    int i, slice_count;

    MPEG4PictureInfo mpeg4_pic_info;
    MPEG4SliceInfo mpeg4_slice_info;
    MPEG4IQMatrix mpeg4_iq_matrix;
    const uint8_t *mpeg4_slice_data;
    unsigned int mpeg4_slice_data_size;

    if (!vaapi)
        return -1;

    mpeg4_get_picture_info(&mpeg4_pic_info);

    if (vaapi_init_decoder(VAProfileMPEG4AdvancedSimple, VAEntrypointVLD,
                           mpeg4_pic_info.width, mpeg4_pic_info.height) < 0)
        return -1;

    if ((pic_param = vaapi_alloc_picture(sizeof(*pic_param))) == NULL)
        return -1;

#define COPY(field) \
    pic_param->field = mpeg4_pic_info.field
#define COPY_BFM(a,b,c) \
    pic_param->BFM(a,b,c) = mpeg4_pic_info.a.b.c
    pic_param->vop_width = mpeg4_pic_info.width;
    pic_param->vop_height = mpeg4_pic_info.height;
    pic_param->forward_reference_picture = 0xffffffff;
    pic_param->backward_reference_picture = 0xffffffff;
    pic_param->BFV(vol_fields, value) = 0; /* reset all bits */
    COPY_BFM(vol_fields, bits, short_video_header);
    COPY_BFM(vol_fields, bits, chroma_format);
    COPY_BFM(vol_fields, bits, interlaced);
    COPY_BFM(vol_fields, bits, obmc_disable);
    COPY_BFM(vol_fields, bits, sprite_enable);
    COPY_BFM(vol_fields, bits, sprite_warping_accuracy);
    COPY_BFM(vol_fields, bits, quant_type);
    COPY_BFM(vol_fields, bits, quarter_sample);
    COPY_BFM(vol_fields, bits, data_partitioned);
    COPY_BFM(vol_fields, bits, reversible_vlc);
    COPY(no_of_sprite_warping_points);
    for (i = 0; i < 3; i++) {
        COPY(sprite_trajectory_du[i]);
        COPY(sprite_trajectory_dv[i]);
    }
    COPY(quant_precision);
    pic_param->BFV(vop_fields, value) = 0; /* reset all bits */
    COPY_BFM(vop_fields, bits, vop_coding_type);
    COPY_BFM(vop_fields, bits, backward_reference_vop_coding_type);
    COPY_BFM(vop_fields, bits, vop_rounding_type);
    COPY_BFM(vop_fields, bits, intra_dc_vlc_thr);
    COPY_BFM(vop_fields, bits, top_field_first);
    COPY_BFM(vop_fields, bits, alternate_vertical_scan_flag);
    COPY(vop_fcode_forward);
    COPY(vop_fcode_backward);
    COPY(num_gobs_in_vop);
    COPY(num_macroblocks_in_gob);
    COPY(TRB);
    COPY(TRD);
#if (VA_CHECK_VERSION(0,31,1) /* XXX: update when changes are merged */ || \
     (VA_CHECK_VERSION(0,31,0) && VA_SDS_VERSION >= 4))
    COPY(vop_time_increment_resolution);
    COPY_BFM(vol_fields, bits, resync_marker_disable);
#endif
#undef COPY_BFM
#undef COPY

    if (mpeg4_iq_matrix.load_intra_quant_mat ||
        mpeg4_iq_matrix.load_non_intra_quant_mat) {
        if ((iq_matrix = vaapi_alloc_iq_matrix(sizeof(*iq_matrix))) == NULL)
            return -1;
        mpeg4_get_iq_matrix(&mpeg4_iq_matrix);

#define COPY(field) iq_matrix->field = mpeg4_iq_matrix.field
        COPY(load_intra_quant_mat);
        COPY(load_non_intra_quant_mat);
        for (i = 0; i < 64; i++) {
            COPY(intra_quant_mat[i]);
            COPY(non_intra_quant_mat[i]);
        }
#undef COPY
    }

    slice_count = mpeg4_get_slice_count();
    for (i = 0; i < slice_count; i++) {
        if (mpeg4_get_slice_info(i, &mpeg4_slice_info) < 0)
            return -1;
        if (mpeg4_get_slice_data(i, &mpeg4_slice_data, &mpeg4_slice_data_size) < 0)
            return -1;
        if (mpeg4_slice_data_size != mpeg4_slice_info.slice_data_size)
            return -1;
        if ((slice_param = vaapi_alloc_slice(sizeof(*slice_param),
                                             mpeg4_slice_data,
                                             mpeg4_slice_data_size)) == NULL)
            return -1;

#define COPY(field) slice_param->field = mpeg4_slice_info.field
        COPY(macroblock_offset);
        COPY(macroblock_number);
        COPY(quant_scale);
#undef COPY
    }

    return vaapi_decode();
}
