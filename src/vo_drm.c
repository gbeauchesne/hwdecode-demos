/*
 *  vo_drm.c - DRM common code
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libudev.h>
#include <xf86drm.h>
#include "vo_drm.h"

/* Get default device path. Actually, the first match in the DRM subsystem */
static bool
ensure_device_path_default(DRMContext *drm)
{
    const char *syspath, *devpath;
    struct udev *udev = NULL;
    struct udev_device *device, *parent;
    struct udev_enumerate *e = NULL;
    struct udev_list_entry *l;
    bool success = false;
    int fd;

    if (drm->device_path_default)
        return true;

    udev = udev_new();
    if (!udev)
        goto end;

    e = udev_enumerate_new(udev);
    if (!e)
        goto end;

    udev_enumerate_add_match_subsystem(e, "drm");
    udev_enumerate_scan_devices(e);
    udev_list_entry_foreach(l, udev_enumerate_get_list_entry(e)) {
        syspath = udev_list_entry_get_name(l);
        device  = udev_device_new_from_syspath(udev, syspath);
        parent  = udev_device_get_parent(device);
        if (strcmp(udev_device_get_subsystem(parent), "pci") != 0) {
            udev_device_unref(device);
            continue;
        }

        devpath = udev_device_get_devnode(device);
        fd = open(devpath, O_RDWR|O_CLOEXEC);
        if (fd < 0) {
            udev_device_unref(device);
            continue;
        }

        drm->device_path_default = strdup(devpath);
        close(fd);
        udev_device_unref(device);
        success = true;
        break;
    }

end:
    if (e)
        udev_enumerate_unref(e);
    if (udev)
        udev_unref(udev);
    return success;
}

DRMContext *
drm_get_context(void)
{
    static DRMContext drm_context;
    static bool is_initialized = false;

    if (!is_initialized) {
        drm_context.drm_device = -1;
        is_initialized = true;
    }
    return &drm_context;
}

int
drm_init(void)
{
    DRMContext * const drm = drm_get_context();

    if (!ensure_device_path_default(drm))
        return -1;

    if (!drm->device_path) {
        drm->device_path = strdup(drm->device_path_default);
        if (!drm->device_path)
            return -1;
    }

    if (drm->drm_device < 0) {
        drm->drm_device = open(drm->device_path, O_RDWR|O_CLOEXEC);
        if (drm->drm_device < 0)
            return -1;
    }
    return 0;
}

int
drm_exit(void)
{
    DRMContext * const drm = drm_get_context();

    if (drm->drm_device >= 0) {
        close(drm->drm_device);
        drm->drm_device = -1;
    }

    free(drm->device_path_default);
    drm->device_path_default = NULL;
    free(drm->device_path);
    drm->device_path = NULL;
    return 0;
}

int
drm_display(void)
{
    /* Nothing to display */
    return 0;
}
