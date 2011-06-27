/*
 *  crystalhd.h - Crystal HD common code
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

#ifndef CRYSTALHD_H
#define CRYSTALHD_H

#include <libcrystalhd_if.h>
#include "image.h"

typedef struct _CrystalHDContext CrystalHDContext;

struct _CrystalHDContext {
    HANDLE              device;
    Image              *picture;
    unsigned int        picture_width;
    unsigned int        picture_height;
};

CrystalHDContext *crystalhd_get_context(void);

int crystalhd_init_decoder(int codec, unsigned int width, unsigned int height);
int crystalhd_decode(const uint8_t *buf, unsigned int buf_size);

#endif /* CRYSTALHD_H */
