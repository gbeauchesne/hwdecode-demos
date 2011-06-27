/*
 *  vdpau_vc1.c - VC-1 decoding through VDPAU
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
#include "vc1.h"

int decode(void)
{
    VDPAUContext * const vdpau = vdpau_get_context();
    VdpPictureInfoVC1 *pic_info;
    int i, picture_type;

    VC1PictureInfo vc1_pic_info;
    const uint8_t *vc1_slice_data;
    unsigned int vc1_slice_data_size;

    if (!vdpau)
        return -1;

    vc1_get_picture_info(&vc1_pic_info);

    if (vdpau_init_decoder(VDP_DECODER_PROFILE_VC1_ADVANCED,
                           vc1_pic_info.width, vc1_pic_info.height) < 0)
        return -1;

    if ((pic_info = vdpau_alloc_picture()) == NULL)
        return -1;

    switch (vc1_pic_info.picture_fields.bits.picture_type) {
    case 0: picture_type = 0; break; /* I */
    case 1: picture_type = 1; break; /* P */
    case 2: picture_type = 3; break; /* B */
    case 3: picture_type = 4; break; /* BI */
    case 4: picture_type = 1; break; /* P "skipped" */
    default: return -1;
    }

    pic_info->forward_reference         = VDP_INVALID_HANDLE;
    pic_info->backward_reference        = VDP_INVALID_HANDLE;
    pic_info->slice_count               = vc1_get_slice_count();
    pic_info->picture_type              = picture_type;
    pic_info->frame_coding_mode         = vc1_pic_info.picture_fields.bits.frame_coding_mode;
    pic_info->postprocflag              = vc1_pic_info.post_processing != 0;
    pic_info->pulldown                  = vc1_pic_info.sequence_fields.bits.pulldown;
    pic_info->interlace                 = vc1_pic_info.sequence_fields.bits.interlace;
    pic_info->tfcntrflag                = vc1_pic_info.sequence_fields.bits.tfcntrflag;
    pic_info->finterpflag               = vc1_pic_info.sequence_fields.bits.finterpflag;
    pic_info->psf                       = vc1_pic_info.sequence_fields.bits.psf;
    pic_info->dquant                    = vc1_pic_info.pic_quantizer_fields.bits.dquant;
    pic_info->panscan_flag              = vc1_pic_info.entrypoint_fields.bits.panscan_flag;
    pic_info->refdist_flag              = vc1_pic_info.reference_fields.bits.reference_distance_flag;
    pic_info->quantizer                 = vc1_pic_info.pic_quantizer_fields.bits.quantizer;
    pic_info->extended_mv               = vc1_pic_info.mv_fields.bits.extended_mv_flag;
    pic_info->extended_dmv              = vc1_pic_info.mv_fields.bits.extended_dmv_flag;
    pic_info->overlap                   = vc1_pic_info.sequence_fields.bits.overlap;
    pic_info->vstransform               = vc1_pic_info.transform_fields.bits.variable_sized_transform_flag;
    pic_info->loopfilter                = vc1_pic_info.entrypoint_fields.bits.loopfilter;
    pic_info->fastuvmc                  = vc1_pic_info.fast_uvmc_flag;
    pic_info->range_mapy_flag           = vc1_pic_info.range_mapping_fields.bits.luma_flag;
    pic_info->range_mapy                = vc1_pic_info.range_mapping_fields.bits.luma;
    pic_info->range_mapuv_flag          = vc1_pic_info.range_mapping_fields.bits.chroma_flag;
    pic_info->range_mapuv               = vc1_pic_info.range_mapping_fields.bits.chroma;
    pic_info->multires                  = vc1_pic_info.sequence_fields.bits.multires;
    pic_info->syncmarker                = vc1_pic_info.sequence_fields.bits.syncmarker;
    pic_info->rangered                  = vc1_pic_info.sequence_fields.bits.rangered;
#if 0
    if (!vdpau_is_nvidia(driver_data, &major_version, &minor_version) ||
        (major_version > 180 || minor_version >= 35))
        pic_info->rangered             |= vc1_pic_info.range_reduction_frame << 1;
#endif
    pic_info->maxbframes                = vc1_pic_info.sequence_fields.bits.max_b_frames;
    pic_info->deblockEnable             = vc1_pic_info.post_processing != 0;
    pic_info->pquant                    = vc1_pic_info.pic_quantizer_fields.bits.pic_quantizer_scale;

    for (i = 0; i < pic_info->slice_count; i++) {
        if (vc1_get_slice_data(i, &vc1_slice_data, &vc1_slice_data_size) < 0)
            return -1;
        if (vdpau_append_slice_data(vc1_slice_data, vc1_slice_data_size) < 0)
            return -1;
    }

    return vdpau_decode();
}
