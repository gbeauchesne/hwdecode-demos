/*
 *  vaapi_compat.h - VA API compatibility glue
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

#ifndef VAAPI_COMPAT_H
#define VAAPI_COMPAT_H

/* Compatibility glue with older libVA API 0.29 */
#if USE_OLD_VAAPI
typedef struct _VASliceParameterBufferBase {
    unsigned int slice_data_size;
    unsigned int slice_data_offset;
    unsigned int slice_data_flag;
} VASliceParameterBufferBase;
#endif

#ifndef VA_CHECK_VERSION
#define VA_MAJOR_VERSION 0
#define VA_MINOR_VERSION 29
#define VA_MICRO_VERSION 0
#define VA_SDS_VERSION   0
#define VA_CHECK_VERSION(major,minor,micro) \
        (VA_MAJOR_VERSION > (major) || \
         (VA_MAJOR_VERSION == (major) && VA_MINOR_VERSION > (minor)) || \
         (VA_MAJOR_VERSION == (major) && VA_MINOR_VERSION == (minor) && VA_MICRO_VERSION >= (micro)))
#endif

#ifndef VA_FOURCC
#define VA_FOURCC(ch0, ch1, ch2, ch3)           \
    ((uint32_t)(uint8_t)(ch0) |                 \
     ((uint32_t)(uint8_t)(ch1) << 8) |          \
     ((uint32_t)(uint8_t)(ch2) << 16) |         \
     ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif

#ifndef VA_INVALID_ID
#define VA_INVALID_ID           0xffffffff
#endif
#ifndef VA_INVALID_SURFACE
#define VA_INVALID_SURFACE      VA_INVALID_ID
#endif

/* Compatibility glue with VA API >= 0.31 */
#if VA_CHECK_VERSION(0,31,0)
#define vaSyncSurface(dpy, context, surface) (vaSyncSurface)((dpy), (surface))
#define vaPutImage2             vaPutImage
#define vaAssociateSubpicture2  vaAssociateSubpicture
#endif

/* Used in codec implementation to set up the right bit-fields */
#ifdef USE_OLD_VAAPI
# define BFV(a, b)              a
# define BFM(a, b, c)           c
# define BFMP(p, a, b, c)       p##_##c
# define NEW(x)                 /* nothing */
#else
# define BFV(a, b)              a.b
# define BFM(a, b, c)           a.b.c
# define BFMP(p, a, b, c)       a.b.c
# define NEW(x)                 x
#endif

/* Check for VA/SDS version */
#ifndef VA_CHECK_VERSION_SDS
#define VA_CHECK_VERSION_SDS(major, minor, micro, sds)                  \
    (VA_CHECK_VERSION(major, minor, (micro)+1) ||                       \
     (VA_CHECK_VERSION(major, minor, micro) && VA_SDS_VERSION >= (sds)))
#endif

/* Defined to 1 if VA/GLX 'bind' API is available */
#define HAS_VAAPI_GLX_BIND                                \
    (VA_MAJOR_VERSION == 0 &&                             \
     ((VA_MINOR_VERSION == 30 &&                          \
       VA_MICRO_VERSION == 4 && VA_SDS_VERSION >= 5) ||   \
      (VA_MINOR_VERSION == 31 &&                          \
       VA_MICRO_VERSION == 0 && VA_SDS_VERSION < 5)))

#endif /* VAAPI_COMPAT_H */
