/*
 *  vaapi_jpeg.c - JPEG decoding through VA API
 *
 *  Copyright (C) 2013 Intel Corporation
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
#include "jpeg.h"

int decode(void)
{
    VAAPIContext * const vaapi = vaapi_get_context();
    VAPictureParameterBufferJPEGBaseline *pic_param;
    VASliceParameterBufferJPEGBaseline *slice_param;
    VAIQMatrixBufferJPEGBaseline *iq_matrix;
    VAHuffmanTableBufferJPEGBaseline *huf_table;
    int i, j, slice_count;

    JPEGPictureInfo jpeg_pic_info;
    JPEGSliceInfo jpeg_slice_info;
    JPEGIQMatrix jpeg_iq_matrix;
    JPEGHuffmanTable jpeg_huf_table;
    const uint8_t *jpeg_slice_data;
    unsigned int jpeg_slice_data_size;

    if (!vaapi)
        return -1;

    jpeg_get_picture_info(&jpeg_pic_info);
    jpeg_get_iq_matrix(&jpeg_iq_matrix);
    jpeg_get_huf_table(&jpeg_huf_table);

    if (vaapi_init_decoder(VAProfileJPEGBaseline, VAEntrypointVLD,
                           jpeg_pic_info.width, jpeg_pic_info.height) < 0)
        return -1;

    if ((pic_param = vaapi_alloc_picture(sizeof(*pic_param))) == NULL)
        return -1;

#define COPY(field) \
    pic_param->field = jpeg_pic_info.field
    pic_param->picture_width = jpeg_pic_info.width;
    pic_param->picture_height = jpeg_pic_info.height;
    COPY(num_components);
    for (i = 0; i < pic_param->num_components; i++) {
        COPY(components[i].component_id);
        COPY(components[i].h_sampling_factor);
        COPY(components[i].v_sampling_factor);
        COPY(components[i].quantiser_table_selector);
    }
#undef COPY

    if ((iq_matrix = vaapi_alloc_iq_matrix(sizeof(*iq_matrix))) == NULL)
        return -1;

    memcpy(iq_matrix->load_quantiser_table, jpeg_iq_matrix.load_quantiser_table,
           sizeof(iq_matrix->load_quantiser_table));
    memcpy(iq_matrix->quantiser_table, jpeg_iq_matrix.quantiser_table,
           sizeof(iq_matrix->quantiser_table));

    if ((huf_table = vaapi_alloc_huf_table(sizeof(*huf_table))) == NULL)
        return -1;

    memcpy(huf_table->load_huffman_table, jpeg_huf_table.load_huffman_table,
           sizeof(huf_table->load_huffman_table));
    for (i = 0; i < ARRAY_ELEMS(huf_table->huffman_table); i++) {
        memcpy(huf_table->huffman_table[i].num_dc_codes,
               jpeg_huf_table.huffman_table[i].num_dc_codes,
               sizeof(huf_table->huffman_table[i].num_dc_codes));
        memcpy(huf_table->huffman_table[i].dc_values,
               jpeg_huf_table.huffman_table[i].dc_values,
               sizeof(huf_table->huffman_table[i].dc_values));
        memcpy(huf_table->huffman_table[i].num_ac_codes,
               jpeg_huf_table.huffman_table[i].num_ac_codes,
               sizeof(huf_table->huffman_table[i].num_ac_codes));
        memcpy(huf_table->huffman_table[i].ac_values,
               jpeg_huf_table.huffman_table[i].ac_values,
               sizeof(huf_table->huffman_table[i].ac_values));
    }

    slice_count = jpeg_get_slice_count();
    for (i = 0; i < slice_count; i++) {
        if (jpeg_get_slice_info(i, &jpeg_slice_info) < 0)
            return -1;
        if (jpeg_get_slice_data(i, &jpeg_slice_data, &jpeg_slice_data_size) < 0)
            return -1;
        if (jpeg_slice_data_size != jpeg_slice_info.slice_data_size)
            return -1;
        if ((slice_param = vaapi_alloc_slice(sizeof(*slice_param),
                                             jpeg_slice_data,
                                             jpeg_slice_data_size)) == NULL)
            return -1;

#define COPY(field) \
        slice_param->field = jpeg_slice_info.field
        COPY(slice_data_size);
        COPY(slice_horizontal_position);
        COPY(slice_vertical_position);
        COPY(num_components);
        for (j = 0; j < slice_param->num_components; j++) {
            COPY(components[j].component_selector);
            COPY(components[j].dc_table_selector);
            COPY(components[j].ac_table_selector);
        }
        COPY(restart_interval);
        COPY(num_mcus);
#undef COPY
    }

    return vaapi_decode();
}
