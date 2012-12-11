/*
 *  vo_drm.h - DRM common code
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

#ifndef VO_DRM_H
#define VO_DRM_H

typedef struct _DRMContext DRMContext;

struct _DRMContext {
    char               *device_path_default;
    char               *device_path;
    int                 drm_device;
};

DRMContext *drm_get_context(void);

int drm_init(void);
int drm_exit(void);
int drm_display(void);

#endif /* VO_DRM_H */
