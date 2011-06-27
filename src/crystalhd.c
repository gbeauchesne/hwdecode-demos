/*
 *  crystalhd.c - Crystal HD common code
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
#include "crystalhd.h"
#include "common.h"
#include "utils.h"
#include "x11.h"

#define DBEUG 1
#include "debug.h"

/* DtsProcOutput() timeout in milliseconds */
#define DTS_OUTPUT_TIMEOUT 1000

static CrystalHDContext *crystalhd_context;

static const char *string_of_BC_STATUS(BC_STATUS status)
{
    switch (status) {
#define STATUS(sts) case BC_STS_##sts: return "BC_STS_" #sts
    STATUS(SUCCESS);
    STATUS(INV_ARG);
    STATUS(BUSY);
    STATUS(NOT_IMPL);
    STATUS(PGM_QUIT);
    STATUS(NO_ACCESS);
    STATUS(INSUFF_RES);
    STATUS(IO_ERROR);
    STATUS(NO_DATA);
    STATUS(VER_MISMATCH);
    STATUS(TIMEOUT);
    STATUS(FW_CMD_ERR);
    STATUS(DEC_NOT_OPEN);
    STATUS(ERR_USAGE);
    STATUS(IO_USER_ABORT);
    STATUS(IO_XFR_ERROR);
    STATUS(DEC_NOT_STARTED);
    STATUS(FWHEX_NOT_FOUND);
    STATUS(FMT_CHANGE);
    STATUS(HIF_ACCESS);
    STATUS(CMD_CANCELLED);
    STATUS(FW_AUTH_FAILED);
    STATUS(BOOTLOADER_FAILED);
    STATUS(CERT_VERIFY_ERROR);
    STATUS(DEC_EXIST_OPEN);
    STATUS(PENDING);
    STATUS(CLK_NOCHG);
    STATUS(ERROR);
#undef STATUS
    }
    return "<unknown>";
}

static int crystalhd_check_status(BC_STATUS status, const char *msg)
{
    if (status != BC_STS_SUCCESS) {
        fprintf(stderr, "[%s] %s: %s\n", PACKAGE_NAME, msg,
                string_of_BC_STATUS(status));
        return 0;
    }
    return 1;
}

static int crystalhd_init(void)
{
    CrystalHDContext *chd;
    BC_STATUS status;

    if (crystalhd_context)
        return 0;

    chd = malloc(sizeof(*chd));
    if (!chd)
        return -1;

    const uint32_t mode = (DTS_PLAYBACK_MODE |
                           DTS_LOAD_FILE_PLAY_FW |
                           DTS_DFLT_RESOLUTION(vdecRESOLUTION_CUSTOM));
    status = DtsDeviceOpen(&chd->device, mode);
    if (!crystalhd_check_status(status, "DtsDeviceOpen()"))
        return -1;

    chd->picture      = NULL;
    crystalhd_context = chd;
    return 0;
}

static int crystalhd_exit(void)
{
    CrystalHDContext *chd = crystalhd_get_context();

    if (!chd)
        return -1;

    if (chd->device) {
        DtsStopDecoder(chd->device);
        DtsCloseDecoder(chd->device);
        DtsDeviceClose(chd->device);
        chd->device = NULL;
    }

    if (chd->picture) {
        image_destroy(chd->picture);
        chd->picture = NULL;
    }

    free(crystalhd_context);
    crystalhd_context = NULL;
    return 0;
}

CrystalHDContext *crystalhd_get_context(void)
{
    return crystalhd_context;
}

int crystalhd_init_decoder(int codec, unsigned int width, unsigned int height)
{
    CrystalHDContext * const chd = crystalhd_get_context();
    BC_STATUS status;

    if (!chd)
        return -1;

    chd->picture = image_create(width, height, IMAGE_NV12);
    if (!chd->picture)
        return -1;
    chd->picture_width  = width;
    chd->picture_height = height;

    status = DtsOpenDecoder(chd->device, BC_STREAM_TYPE_ES);
    if (!crystalhd_check_status(status, "DtsOpenDecoder()"))
        return -1;

    status = DtsSetVideoParams(chd->device, codec, FALSE, FALSE, TRUE, 0);
    if (!crystalhd_check_status(status, "DtsSetVideoParams()"))
        return -1;

    status = DtsStartDecoder(chd->device);
    if (!crystalhd_check_status(status, "DtsStartDecoder()"))
        return -1;

    status = DtsStartCapture(chd->device);
    if (!crystalhd_check_status(status, "DtsStartCapture()"))
        return -1;
    return 0;
}

static int crystalhd_get_output_nocopy(void)
{
    CrystalHDContext * const chd = crystalhd_get_context();
    unsigned int width, height, stride;
    Image image;
    BC_DTS_PROC_OUT output;
    BC_STATUS status;

again:
    memset(&output, 0, sizeof(output));
    status = DtsProcOutputNoCopy(chd->device, DTS_OUTPUT_TIMEOUT, &output);
    switch (status) {
    case BC_STS_SUCCESS:
        if (output.PoutFlags & BC_POUT_FLAGS_PIB_VALID) {
            width  = output.PicInfo.width;
            height = output.PicInfo.height;
            if (width != chd->picture_width || height != chd->picture_height)
                return -1;

            // XXX: those are libcrystalhd hardcoded strides
            if (output.PoutFlags & BC_POUT_FLAGS_STRIDE)
                stride = output.StrideSz;
            else if (width <= 720)
                stride = 720;
            else if (width <= 1280)
                stride = 1280;
            else
                stride = 1920;

            // XXX: libcrystalhd YV12 implementation looks wrong anyway
            if (output.PoutFlags & BC_POUT_FLAGS_YV12)
                return -1;

            memset(&image, 0, sizeof(image));
            image.format     = IMAGE_NV12;
            image.width      = width;
            image.height     = height;
            image.num_planes = 2;
            image.pixels[0]  = output.Ybuff;
            image.pitches[0] = stride;
            image.pixels[1]  = output.UVbuff;
            image.pitches[1] = stride;
            if (image_convert(chd->picture, &image) < 0)
                return -1;
        }

        status = DtsReleaseOutputBuffs(chd->device, NULL, FALSE);
        if (!crystalhd_check_status(status, "DtsReleaseOutputBuffs()"))
            return -1;

        if (!(output.PoutFlags & BC_POUT_FLAGS_PIB_VALID))
            goto again;
        break;

    case BC_STS_FMT_CHANGE:
        goto again;

    default:
        if (!crystalhd_check_status(status, "DtsProcOutputNoCopy()"))
            return -1;
        break;
    }
    return 0;
}

static int crystalhd_get_output(void)
{
    CrystalHDContext * const chd = crystalhd_get_context();
    unsigned int width, height, height2, flags;
    BC_DTS_PROC_OUT output;
    BC_STATUS status;

    width   = chd->picture_width;
    height  = chd->picture_height;
again:
    height2 = (height + 1) / 2;

    memset(&output, 0, sizeof(output));
    output.PoutFlags      = BC_POUT_FLAGS_SIZE;
    output.PicInfo.width  = width;
    output.PicInfo.height = height;
    output.Ybuff          = chd->picture->pixels[0];
    output.YbuffSz        = chd->picture->pitches[0] * height;
    output.UVbuff         = chd->picture->pixels[1];
    output.UVbuffSz       = chd->picture->pitches[1] * height2;

    status = DtsProcOutput(chd->device, DTS_OUTPUT_TIMEOUT, &output);
    switch (status) {
    case BC_STS_SUCCESS:
        if (!(output.PoutFlags & BC_POUT_FLAGS_PIB_VALID))
            goto again;
        break;

    case BC_STS_FMT_CHANGE:
        /* XXX: the rest of PicInfo (PIB data) was filled in */
        flags = BC_POUT_FLAGS_PIB_VALID|BC_POUT_FLAGS_FMT_CHANGE;
        if ((output.PoutFlags & flags) == flags) {
            width  = output.PicInfo.width;
            height = output.PicInfo.height;
        }
        goto again;

    default:
        if (!crystalhd_check_status(status, "DtsProcOutput()"))
            return -1;
        break;
    }
    return 0;
}

int crystalhd_decode(const uint8_t *buf, unsigned int buf_size)
{
    CrystalHDContext * const chd = crystalhd_get_context();
    CommonContext * const common = common_get_context();
    BC_STATUS status;

    if (!chd)
        return -1;

    status = DtsProcInput(chd->device, (uint8_t *)buf, buf_size, 0, FALSE);
    if (!crystalhd_check_status(status, "DtsProcInput()"))
        return -1;

    /* DtsFlushInput() requires that current slices are correctly
       identified. e.g. for H.264, the decoder waits for the next one
       prior to processing the current data. So, we emitted a filler
       data slice in crystalhd_video.c */
    if (common->crystalhd_flush) {
        status = DtsFlushInput(chd->device, 0);
        if (!crystalhd_check_status(status, "DtsFlushInput() encoded"))
            return -1;

        status = DtsFlushInput(chd->device, 1);
        if (!crystalhd_check_status(status, "DtsFlushInput() decoded"))
            return -1;
    }

    if (common->crystalhd_output_nocopy) {
        if (crystalhd_get_output_nocopy() < 0)
            return -1;
    }
    else {
        if (crystalhd_get_output() < 0)
            return -1;
    }
    return image_convert(common->image, chd->picture);
}

static int crystalhd_display(void)
{
    return x11_display();
}

int pre(void)
{
    /* XXX: we need the XImage */
    if (hwaccel_type() == HWACCEL_NONE)
        common_get_context()->getimage_mode = GETIMAGE_FROM_VIDEO;

    if (x11_init() < 0)
        return -1;

    if (crystalhd_init() < 0)
        return -1;
    return 0;
}

int post(void)
{
    if (crystalhd_exit() < 0)
        return -1;

    return x11_exit();
}

int display(void)
{
    return crystalhd_display();
}
