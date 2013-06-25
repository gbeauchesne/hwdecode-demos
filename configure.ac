# hwdecode_demos package version number
m4_define([hwdecode_demos_major_version], [0])
m4_define([hwdecode_demos_minor_version], [9])
m4_define([hwdecode_demos_micro_version], [6])
m4_define([hwdecode_demos_pre_version],   [1])
m4_define([hwdecode_demos_version],
          [hwdecode_demos_major_version.hwdecode_demos_minor_version.hwdecode_demos_micro_version])
m4_if(hwdecode_demos_pre_version, [0], [], [
m4_append([hwdecode_demos_version], hwdecode_demos_pre_version, [.pre])
])

AC_PREREQ([2.57])
AC_INIT([hwdecode_demos], [hwdecode_demos_version], [gbeauchesne@splitted-desktop.com], [hwdecode-demos])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE

AM_CONFIG_HEADER([src/config.h])

HWDECODE_DEMOS_MAJOR_VERSION=hwdecode_demos_major_version
HWDECODE_DEMOS_MINOR_VERSION=hwdecode_demos_minor_version
HWDECODE_DEMOS_MICRO_VERSION=hwdecode_demos_micro_version
AC_DEFINE([HWDECODE_DEMOS_MAJOR_VERSION], [hwdecode_demos_major_version], [Major version of the driver])
AC_DEFINE([HWDECODE_DEMOS_MINOR_VERSION], [hwdecode_demos_minor_version], [Minor version of the driver])
AC_DEFINE([HWDECODE_DEMOS_MICRO_VERSION], [hwdecode_demos_micro_version], [Micro version of the driver])
AC_DEFINE([HWDECODE_DEMOS_PRE_VERSION], [hwdecode_demos_pre_version], [Preversion of the driver])

dnl Use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [
  AM_DEFAULT_VERBOSITY=1
  AC_SUBST(AM_DEFAULT_VERBOSITY)
])

AC_ARG_ENABLE(ffmpeg,
              AC_HELP_STRING([--enable-ffmpeg],
                             [enable FFmpeg @<:@default=yes@:>@]),
              [], [enable_ffmpeg="yes"])
AC_ARG_ENABLE(crystalhd,
              AC_HELP_STRING([--enable-crystalhd],
                             [enable Crystal HD @<:@default=yes@:>@]),
              [], [enable_crystalhd="yes"])
AC_ARG_ENABLE(vaapi,
              AC_HELP_STRING([--enable-vaapi],
                             [enable VA-API @<:@default=yes@:>@]),
              [], [enable_vaapi="yes"])
AC_ARG_ENABLE(vdpau,
              AC_HELP_STRING([--enable-vdpau],
                             [enable VDPAU @<:@default=yes@:>@]),
              [], [enable_vdpau="yes"])
AC_ARG_ENABLE(xvba,
              AC_HELP_STRING([--enable-xvba],
                             [enable XvBA @<:@default=yes@:>@]),
              [], [enable_xvba="yes"])

AC_ARG_ENABLE(drm,
              AC_HELP_STRING([--enable-drm],
                             [enable DRM backend @<:@default=yes@:>@]),
              [], [enable_drm="yes"])
AC_ARG_ENABLE(x11,
              AC_HELP_STRING([--enable-x11],
                             [enable X11 backend @<:@default=yes@:>@]),
              [], [enable_x11="yes"])
AC_ARG_ENABLE(glx,
              AC_HELP_STRING([--enable-glx],
                             [enable GLX backend @<:@default=yes@:>@]),
              [], [enable_glx="yes"])

AC_ARG_WITH(libva,
            AC_HELP_STRING([--with-libva=PREFIX],
                           [build against libva in the specified PREFIX]),
            [], [with_libva="pkgconfig"])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AM_PROG_CC_C_O

AC_C_BIGENDIAN
AC_HEADER_STDC
AC_SYS_LARGEFILE

dnl We want pthreads. Try libpthread first, then libc_r (FreeBSD), then PTL.
HAVE_PTHREADS=yes
AC_CHECK_LIB(pthread, pthread_create, , [
    AC_CHECK_LIB(c_r, pthread_create, , [
        AC_CHECK_LIB(PTL, pthread_create, , [
            HAVE_PTHREADS=no
        ])
    ])
])
if test "$HAVE_PTHREADS" = "yes"; then
    AC_DEFINE(HAVE_PTHREADS, 1, [Defined if pthreads are available])
fi

dnl Checks for libraries.
AC_CHECK_LIB(rt, timer_create)

dnl Checks if dl() comes from the C library or -ldl
AC_SEARCH_LIBS([dlopen], [dl])

dnl Checks for library functions.
AC_CHECK_FUNCS(clock_gettime)

dnl Check for FFmpeg
PKG_CHECK_MODULES(LIBAVUTIL, [libavutil],
    [ac_cv_have_avutil="yes"],
    [ac_cv_have_avutil="no"]
)
if test "$ac_cv_have_avutil" = "yes"; then
    AC_DEFINE(HAVE_AVUTIL, 1, [Defined if libavutil is available])
fi
AC_CHECK_HEADERS([libavutil/pixfmt.h ffmpeg/avutil.h])

PKG_CHECK_MODULES(LIBAVFORMAT, [libavformat],
    [ac_cv_have_avformat="yes"],
    [ac_cv_have_avformat="no"]
)
if test "$ac_cv_have_avformat" = "yes"; then
    AC_DEFINE(HAVE_AVFORMAT, 1, [Defined if libavformat is available])
fi
AC_CHECK_HEADERS([libavformat/avformat.h ffmpeg/avformat.h])

PKG_CHECK_MODULES(LIBAVCODEC, [libavcodec],
    [ac_cv_have_avcodec="yes"],
    [ac_cv_have_avcodec="no"]
)
if test "$ac_cv_have_avcodec" = "yes"; then
    AC_DEFINE(HAVE_AVCODEC, 1, [Defined if libavcodec is available])
fi
AC_CHECK_HEADERS([libavcodec/avcodec.h ffmpeg/avcodec.h])
AC_CHECK_HEADERS([libavcodec/vaapi.h])

PKG_CHECK_MODULES(LIBSWSCALE, [libswscale],
    [ac_cv_have_swscale="yes"],
    [ac_cv_have_swscale="no"]
)
if test "$ac_cv_have_swscale" = "yes"; then
    AC_DEFINE(HAVE_SWSCALE, 1, [Defined if libswscale is available])
fi
AC_CHECK_HEADERS([libswscale/swscale.h ffmpeg/swscale.h])

AM_CONDITIONAL(USE_FFMPEG, test "$enable_ffmpeg:$ac_cv_have_avformat:$ac_cv_have_swscale:$ac_cv_have_avcodec" = "yes:yes:yes:yes")

dnl Check for Cairo
PKG_CHECK_MODULES(CAIRO, [cairo],
    [ac_cv_have_cairo="yes"],
    [ac_cv_have_cairo="no"]
)
if test "$ac_cv_have_cairo" = "yes"; then
    AC_DEFINE(HAVE_CAIRO, 1, [Defined if cairo is available])
fi
AM_CONDITIONAL(USE_CAIRO, test "$ac_cv_have_cairo" = "yes")

dnl Check for DRM/libudev
USE_DRM="no"
if test "$enable_drm" = "yes"; then
    PKG_CHECK_MODULES(DRM, [libdrm], [USE_DRM="yes"], [USE_DRM="no"])
    PKG_CHECK_MODULES(UDEV, [libudev], [:], [USE_DRM="no"])

    if test "$USE_DRM" = "yes"; then
        saved_CPPFLAGS="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $DRM_CFLAGS"
        AC_CHECK_HEADERS([drm_fourcc.h], [:], [USE_DRM="no"])
        CPPFLAGS="$saved_CPPFLAGS"
    fi
fi
AM_CONDITIONAL(USE_DRM, [test "$USE_DRM" = "yes"])
if test "$USE_DRM" = "yes"; then
    AC_DEFINE(USE_DRM, 1, [Defined if DRM is enabled])
fi

dnl Check for X11
USE_X11="no"
PKG_CHECK_MODULES(X11_DEPS, [x11], [HAVE_X11="yes"], [HAVE_X11="no"])
if test "$enable_x11" = "yes" -a "$HAVE_X11" = "yes"; then
    USE_X11="yes"
    AC_SUBST(X11_DEPS_CFLAGS)
    AC_SUBST(X11_DEPS_LIBS)
fi
AM_CONDITIONAL(USE_X11, [test "$USE_X11" = "yes"])
if test "$USE_X11" = "yes"; then
    AC_DEFINE(USE_X11, 1, [Defined if X11 is enabled])
else
    enable_glx="no"
    enable_vdpau="no"
    enable_xvba="no"
fi

dnl Check for OpenGL
HAVE_GL="no"
HAVE_GLU="no"
PKG_CHECK_MODULES(GL_DEPS,  [gl],  [HAVE_GL="yes"],  [HAVE_GL="no"])
if test "$HAVE_GL" = "yes"; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $GL_DEPS_CFLAGS"
    AC_CHECK_HEADERS([GL/gl.h], [:], [HAVE_GL="no"])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for GLU
HAVE_GLU="no"
PKG_CHECK_MODULES(GLU_DEPS, [glu], [HAVE_GLU="yes"], [HAVE_GLU="no"])
if test "$HAVE_GLU" = "yes"; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $GLU_DEPS_CFLAGS"
    AC_CHECK_HEADERS([GL/glu.h], [:], [HAVE_GLU="no"])
    CPPFLAGS="$saved_CPPFLAGS"
fi

dnl Check for GLX
USE_GLX="no"
if test "$enable_glx" = "yes" -a "$HAVE_GL:$HAVE_GLU" = "yes:yes"; then
    saved_CPPFLAGS="$CPPFLAGS"
    saved_LIBS="$LIBS"
    CPPFLAGS="$CPPFLAGS $GL_DEPS_CFLAGS $GLU_DEPS_CFLAGS $X11_DEPS_CFLAGS"
    LIBS="$LIBS $GL_DEPS_LIBS $GLU_DEPS_LIBS $X11_DEPS_LIBS"
    USE_GLX="yes"
    AC_CHECK_HEADERS([GL/glx.h], [:], [USE_GLX="no"])
    AC_CHECK_LIB([GL], [glXCreateContext], [:], [USE_GLX="no"])
    AC_CHECK_LIB([GLU], [gluPerspective], [:], [USE_GLX="no"])
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"
fi
AM_CONDITIONAL(USE_GLX, test "$USE_GLX" = "yes")
if test "$USE_GLX" = "yes"; then
    AC_DEFINE(USE_GLX, 1, [Defined if GLX is enabled])
fi

dnl Check for Crystal HD
AC_CACHE_CHECK([for Crystal HD],
    ac_cv_have_crystalhd, [
    for incdir in /usr/include /usr/local/include; do
        CRYSTALHD_CFLAGS="-I${incdir}/libcrystalhd -D__LINUX_USER__"
        CRYSTALHD_LIBS="-lcrystalhd"
        saved_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $CRYSTALHD_CFLAGS"
        saved_LIBS="$LIBS"
        LIBS="$LIBS $CRYSTALHD_LIBS"
        AC_TRY_LINK(
        [#include <libcrystalhd_if.h>],
        [DtsDeviceOpen(0, 0)],
        [ac_cv_have_crystalhd="yes"],
        [ac_cv_have_crystalhd="no"])
        CFLAGS="$saved_CFLAGS"
        LIBS="$saved_LIBS"
        if test "$ac_cv_have_crystalhd" = "yes"; then
            break
        fi
    done
])
USE_CRYSTALHD="no"
if test "$enable_crystalhd:$ac_cv_have_crystalhd" = "yes:yes"; then
    USE_CRYSTALHD="yes"
fi
AC_SUBST(CRYSTALHD_CFLAGS)
AC_SUBST(CRYSTALHD_LIBS)
AM_CONDITIONAL(USE_CRYSTALHD, test "$USE_CRYSTALHD" = "yes")

dnl Check for VA-API
AC_CACHE_CHECK([for old libVA API 0.29],
    ac_cv_have_old_vaapi, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -lva"
    AC_TRY_LINK(
    [#include <va_x11.h>],
    [vaGetDisplay(NULL)],
    [ac_cv_have_old_vaapi="yes"],
    [ac_cv_have_old_vaapi="no"])
    LIBS="$saved_LIBS"
])
USE_OLD_VAAPI="no"
if test "$enable_vaapi:$ac_cv_have_old_vaapi" = "yes:yes"; then
    USE_VAAPI="yes"
    USE_OLD_VAAPI="yes"
    OLD_VAAPI_LIBS="-lva"
fi
AC_SUBST(OLD_VAAPI_CFLAGS)
AC_SUBST(OLD_VAAPI_LIBS)
AM_CONDITIONAL(USE_OLD_VAAPI, test "$USE_OLD_VAAPI" = "yes")

if test "$enable_vaapi:$ac_cv_have_old_vaapi" = "yes:no"; then
    if test "$with_libva" = "pkgconfig"; then
        PKG_CHECK_MODULES(VAAPI_DEPS, [libva],
            [ac_cv_have_vaapi="yes" USE_VAAPI="yes"],
            [ac_cv_have_vaapi="no"]
        )
    else
        AC_CACHE_CHECK([for VA-API (core) in ${with_libva}],
            ac_cv_have_vaapi, [
            saved_CFLAGS="$CFLAGS"
            saved_LIBS="$LIBS"
            for lib in lib lib32 lib64; do
                VAAPI_DEPS_CFLAGS="-I${with_libva}/include"
                VAAPI_DEPS_LIBS="-L${with_libva}/$lib -lva-drm -lva"
                CFLAGS="$saved_CFLAGS $VAAPI_DEPS_CFLAGS"
                LIBS="$saved_LIBS $VAAPI_DEPS_LIBS"
                AC_TRY_LINK(
                    [#include <va/va.h>],
                    [vaInitialize(0,0,0)],
                    [ac_cv_have_vaapi="yes" USE_VAAPI="yes"; break],
                    [ac_cv_have_vaapi="no"]
                )
            done
            CFLAGS="$saved_CFLAGS"
            LIBS="$saved_LIBS"
        ])
    fi
fi

if test "$USE_DRM" = "yes"; then
    if test "$with_libva" = "pkgconfig"; then
        PKG_CHECK_MODULES(VAAPI_DRM_DEPS, [libva-drm],
            [ac_cv_have_vaapi_drm="yes"],
            [ac_cv_have_vaapi_drm="no"]
        )
    else
        AC_CACHE_CHECK([for VA-API (drm) in ${with_libva}],
            ac_cv_have_vaapi_drm, [
            saved_CFLAGS="$CFLAGS"
            saved_LIBS="$LIBS"
            for lib in lib lib32 lib64; do
                VAAPI_DRM_DEPS_CFLAGS="-I${with_libva}/include"
                VAAPI_DRM_DEPS_LIBS="-L${with_libva}/$lib -lva-drm -lva"
                CFLAGS="$saved_CFLAGS $VAAPI_DRM_DEPS_CFLAGS"
                LIBS="$saved_LIBS $VAAPI_DRM_DEPS_LIBS"
                AC_TRY_LINK(
                    [#include <va/va_drm.h>],
                    [vaGetDisplayDRM(0)],
                    [ac_cv_have_vaapi_drm="yes"; break],
                    [ac_cv_have_vaapi_drm="no"]
                )
            done
            CFLAGS="$saved_CFLAGS"
            LIBS="$saved_LIBS"
        ])
    fi
fi
USE_VAAPI_DRM="no"
if test "$USE_DRM:$enable_vaapi:$ac_cv_have_vaapi_drm" = "yes:yes:yes"; then
    USE_VAAPI="yes"
    USE_VAAPI_DRM="yes"
fi
AM_CONDITIONAL(USE_VAAPI_DRM, [test "$USE_VAAPI_DRM" = "yes"])

if test "$USE_X11" = "yes"; then
    if test "$with_libva" = "pkgconfig"; then
        PKG_CHECK_MODULES(VAAPI_X11_DEPS, [libva-x11],
            [ac_cv_have_vaapi_x11="yes"],
            [ac_cv_have_vaapi_x11="no"]
        )
    else
        AC_CACHE_CHECK([for VA-API (x11) in ${with_libva}],
            ac_cv_have_vaapi_x11, [
            saved_CFLAGS="$CFLAGS"
            saved_LIBS="$LIBS"
            for lib in lib lib32 lib64; do
                VAAPI_X11_DEPS_CFLAGS="-I${with_libva}/include"
                VAAPI_X11_DEPS_LIBS="-L${with_libva}/$lib -lva-x11 -lva"
                CFLAGS="$saved_CFLAGS $VAAPI_X11_DEPS_CFLAGS"
                LIBS="$saved_LIBS $VAAPI_X11_DEPS_LIBS"
                AC_TRY_LINK(
                    [#include <va/va_x11.h>],
                    [vaGetDisplay(NULL)],
                    [ac_cv_have_vaapi_x11="yes"; break],
                    [ac_cv_have_vaapi_x11="no"]
                )
            done
            CFLAGS="$saved_CFLAGS"
            LIBS="$saved_LIBS"
        ])
    fi
fi
USE_VAAPI_X11="no"
if test "$USE_X11:$enable_vaapi:$ac_cv_have_vaapi_x11" = "yes:yes:yes"; then
    USE_VAAPI="yes"
    USE_VAAPI_X11="yes"
fi
AM_CONDITIONAL(USE_VAAPI_X11, [test "$USE_VAAPI_X11" = "yes"])

if test "$USE_GLX" = "yes"; then
    if test "$with_libva" = "pkgconfig"; then
        PKG_CHECK_MODULES(VAAPI_GLX_DEPS, [libva-glx],
            [ac_cv_have_vaapi_glx="yes"],
            [ac_cv_have_vaapi_glx="no"]
        )
    else
        AC_CACHE_CHECK([for VA-API (glx) in ${with_libva}],
            ac_cv_have_vaapi_glx, [
            saved_CFLAGS="$CFLAGS"
            saved_LIBS="$LIBS"
            for lib in lib lib32 lib64; do
                VAAPI_GLX_DEPS_CFLAGS="-I${with_libva}/include"
                VAAPI_GLX_DEPS_LIBS="-L${with_libva}/$lib -lva-glx -lva"
                CFLAGS="$saved_CFLAGS $VAAPI_GLX_DEPS_CFLAGS"
                LIBS="$saved_LIBS $VAAPI_GLX_DEPS_LIBS"
                AC_TRY_LINK(
                    [#include <va/va_glx.h>],
                    [vaGetDisplayGLX(NULL)],
                    [ac_cv_have_vaapi_glx="yes"; break],
                    [ac_cv_have_vaapi_glx="no"]
                )
            done
            CFLAGS="$saved_CFLAGS"
            LIBS="$saved_LIBS"
        ])
    fi
fi
USE_VAAPI_GLX="no"
if test "$USE_GLX:$enable_vaapi:$ac_cv_have_vaapi_glx" = "yes:yes:yes"; then
    USE_VAAPI="yes"
    USE_VAAPI_GLX="yes"
fi
AM_CONDITIONAL(USE_VAAPI_GLX, [test "$USE_VAAPI_GLX" = "yes"])

AM_CONDITIONAL(USE_VAAPI, [test "$USE_VAAPI" = "yes"])

dnl Check for JPEG decoding API (0.32.1+)
USE_VAAPI_JPEG=0
AC_CACHE_CHECK([for VA/JPEG decoding API],
    ac_cv_have_vaapi_jpeg_decoding, [
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS $VAAPI_DEPS_CFLAGS"
    saved_LIBS="$LIBS"
    LIBS="$LIBS $VAAPI_DEPS_LIBS"
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[#include <va/va.h>]],
            [[VAPictureParameterBufferJPEGBaseline pic_param;
              VASliceParameterBufferJPEGBaseline slice_param;
              VAHuffmanTableBufferJPEGBaseline huffman_table;
              VAIQMatrixBufferJPEGBaseline iq_matrix;]])],
        [ac_cv_have_vaapi_jpeg_decoding="yes" USE_VAAPI_JPEG=1],
        [ac_cv_have_vaapi_jpeg_decoding="no"]
    )
    CPPFLAGS="$saved_CPPFLAGS"
    LIBS="$saved_LIBS"
])
AM_CONDITIONAL([USE_VAAPI_JPEG], [test $USE_VAAPI_JPEG -eq 1])

dnl Check for VDPAU
AC_CACHE_CHECK([for VDPAU],
    ac_cv_have_vdpau, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -lvdpau"
    AC_TRY_LINK(
    [#include <vdpau/vdpau_x11.h>],
    [vdp_device_create_x11(0,0,0,0)],
    [ac_cv_have_vdpau="yes" VDPAU_DEPS_LIBS="-lvdpau"],
    [ac_cv_have_vdpau="no"])
    LIBS="$saved_LIBS"
])
AC_SUBST(VDPAU_DEPS_CFLAGS)
AC_SUBST(VDPAU_DEPS_LIBS)
AM_CONDITIONAL(USE_VDPAU, test "$enable_vdpau:$ac_cv_have_vdpau" = "yes:yes")

AC_CACHE_CHECK([for VDPAU/MPEG-4 support],
    ac_cv_have_vdpau_mpeg4, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -lvdpau"
    AC_TRY_LINK(
    [#include <vdpau/vdpau.h>],
    [VdpPictureInfoMPEG4Part2 pic_info],
    [ac_cv_have_vdpau_mpeg4="yes" HAVE_VDPAU_MPEG4=1],
    [ac_cv_have_vdpau_mpeg4="no"  HAVE_VDPAU_MPEG4=0])
    LIBS="$saved_LIBS"
])
AC_DEFINE_UNQUOTED(HAVE_VDPAU_MPEG4, [$HAVE_VDPAU_MPEG4], [VDPAU/MPEG-4 support])
AM_CONDITIONAL(USE_VDPAU_MPEG4, test "$ac_cv_have_vdpau_mpeg4" = "yes")

dnl Check for XvBA
AC_CACHE_CHECK([for XvBA],
    ac_cv_have_xvba, [
    saved_LIBS="$LIBS"
    LIBS="$LIBS -lXvBAW"
    LIBS="$LIBS -lGL -ldl" # XXX: help broken XvBA libraries...
    AC_TRY_LINK(
    [
#include <X11/Xlib.h>
#include <amdpcom.h>
#include <amdxvba.h>
    ],
    [XVBAQueryExtension(0,0)],
    [ac_cv_have_xvba="yes" XVBA_DEPS_LIBS="-lXvBAW -lGL -ldl"],
    [ac_cv_have_xvba="no"])
    LIBS="$saved_LIBS"
])
AC_SUBST(XVBA_DEPS_CFLAGS)
AC_SUBST(XVBA_DEPS_LIBS)
AM_CONDITIONAL(USE_XVBA, test "$enable_xvba:$ac_cv_have_xvba" = "yes:yes")

AC_OUTPUT([
    Makefile
    src/Makefile
])
