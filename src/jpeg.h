/*
 *  jpeg.h - JPEG image dump
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

#ifndef JPEG_H
#define JPEG_H

typedef struct _JPEGPictureInfo         JPEGPictureInfo;
typedef struct _JPEGIQMatrix            JPEGIQMatrix;
typedef struct _JPEGHuffmanTable        JPEGHuffmanTable;
typedef struct _JPEGSliceInfo           JPEGSliceInfo;

struct _JPEGPictureInfo {
    unsigned short      width;
    unsigned short      height;
    struct {
        unsigned char   component_id;
        unsigned char   h_sampling_factor;
        unsigned char   v_sampling_factor;
        unsigned char   quantiser_table_selector;
    }                   components[255];
    unsigned char       num_components;
};

struct _JPEGIQMatrix {
    unsigned char       load_quantiser_table[4];
    unsigned char       quantiser_table[4][64];
};

struct _JPEGHuffmanTable {
    unsigned char       load_huffman_table[2];
    struct {
        unsigned char   num_dc_codes[16];
        unsigned char   dc_values[12];
        unsigned char   num_ac_codes[16];
        unsigned char   ac_values[162];
        unsigned char   pad[2];
    }                   huffman_table[2];
};

struct _JPEGSliceInfo {
    unsigned int        slice_data_size;
    unsigned int        slice_data_offset;
    unsigned int        slice_data_flag;
    unsigned int        slice_horizontal_position;
    unsigned int        slice_vertical_position;
    struct {
        unsigned char   component_selector;
        unsigned char   dc_table_selector;
        unsigned char   ac_table_selector;
    }                   components[4];
    unsigned char       num_components;
    unsigned short      restart_interval;
    unsigned int        num_mcus;
};

void jpeg_get_video_data(const uint8_t **data, unsigned int *size);
void jpeg_get_picture_info(JPEGPictureInfo *pic_info);
void jpeg_get_iq_matrix(JPEGIQMatrix *iq_matrix);
void jpeg_get_huf_table(JPEGHuffmanTable *huf_table);
int jpeg_get_slice_count(void);
int jpeg_get_slice_info(int slice, JPEGSliceInfo *slice_info);
int jpeg_get_slice_data(int slice, const uint8_t **data, unsigned int *size);

#endif /* JPEG_H */
