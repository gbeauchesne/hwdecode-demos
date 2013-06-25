# Private APIs
PRIVATE_APIS = xvba

noinst_HEADERS =	\
	buffer.h	\
	common.h	\
	crystalhd.h	\
	debug.h		\
	ffmpeg.h	\
	glx.h		\
	glx_compat.h	\
	h264.h		\
	image.h		\
	jpeg.h		\
	mpeg2.h		\
	mpeg4.h		\
	put_bits.h	\
	sysdeps.h	\
	utils.h		\
	utils_glx.h	\
	utils_x11.h	\
	vaapi.h		\
	vaapi_compat.h	\
	vc1.h		\
	vdpau.h		\
	vdpau_gate.h	\
	vo_drm.h	\
	x11.h		\
	xvba.h		\
	xvba_gate.h	\
	$(NULL)

dist_pkgdata_DATA =	\
	h264.mp4	\
	jpeg.jpg	\
	mpeg2.m2v	\
	mpeg4.mp4	\
	vc1.raw		\
	$(NULL)

bin_PROGRAMS =		\
	$(crystalhd_PROGS) \
	$(ffmpeg_PROGS)	\
	$(vaapi_PROGS)	\
	$(vdpau_PROGS)	\
	$(xvba_PROGS)	\
	$(NULL)

x11_display_SOURCES	= x11.c utils_x11.c
glx_display_SOURCES	= glx.c utils_glx.c

display_SOURCES		=
display_CFLAGS		=
display_LIBS		=
if USE_DRM
display_SOURCES		+= vo_drm.c
display_CFLAGS		+= $(DRM_CFLAGS) $(UDEV_CFLAGS)
display_LIBS		+= $(DRM_LIBS) $(UDEV_LIBS)
endif
if USE_X11
display_SOURCES		+= $(x11_display_SOURCES)
display_CFLAGS		+= $(X11_DEPS_CFLAGS)
display_LIBS		+= $(X11_DEPS_LIBS)
endif
if USE_GLX
display_SOURCES		+= $(glx_display_SOURCES)
display_CFLAGS		+= $(GL_DEPS_CFLAGS) $(GLU_DEPS_CFLAGS)
display_LIBS		+= $(GL_DEPS_LIBS) $(GLU_DEPS_LIBS)
endif

if USE_VAAPI
vaapi_PROGS	= vaapi_h264 vaapi_vc1 vaapi_mpeg2 vaapi_mpeg4
if USE_VAAPI_JPEG
vaapi_PROGS	+= vaapi_jpeg
endif
vaapi_source_c	= vaapi.c
vaapi_CFLAGS	= -DUSE_VAAPI
vaapi_LIBS	=
if USE_VAAPI_DRM
vaapi_CFLAGS	+= $(VAAPI_DRM_DEPS_CFLAGS) -DUSE_VAAPI_DRM
vaapi_LIBS	+= $(VAAPI_DRM_DEPS_LIBS)
endif
if USE_VAAPI_X11
vaapi_CFLAGS	+= $(VAAPI_X11_DEPS_CFLAGS) -DUSE_VAAPI_X11
vaapi_LIBS	+= $(VAAPI_X11_DEPS_LIBS)
endif
if USE_VAAPI_GLX
vaapi_CFLAGS	+= $(VAAPI_GLX_DEPS_CFLAGS) -DUSE_VAAPI_GLX
vaapi_LIBS	+= $(VAAPI_GLX_DEPS_LIBS)
endif
if USE_OLD_VAAPI
vaapi_CFLAGS	+= $(OLD_VAAPI_CFLAGS) -DUSE_OLD_VAAPI -DUSE_VAAPI_X11
vaapi_LIBS	+= $(OLD_VAAPI_LIBS)
endif
else
vaapi_PROGS	=
endif

if USE_VDPAU
vdpau_PROGS	= vdpau_h264 vdpau_vc1 vdpau_mpeg2
if USE_VDPAU_MPEG4
vdpau_PROGS	+= vdpau_mpeg4
endif
vdpau_source_c	= vdpau.c vdpau_gate.c
vdpau_CFLAGS	= -DUSE_VDPAU $(VDPAU_DEPS_CFLAGS)
vdpau_LIBS	= $(VDPAU_DEPS_LIBS)
else
vdpau_PROGS	=
endif

if USE_XVBA
xvba_PROGS	= xvba_h264 xvba_vc1
xvba_source_c	= xvba.c xvba_gate.c
xvba_CFLAGS	= -DUSE_XVBA $(XVBA_DEPS_CFLAGS)
xvba_LIBS	= $(XVBA_DEPS_LIBS)
else
xvba_PROGS	=
endif

if USE_FFMPEG
ffmpeg_PROGS	= ffmpeg_h264 ffmpeg_vc1 ffmpeg_mpeg2 ffmpeg_mpeg4
ffmpeg_source_c	= ffmpeg.c ffmpeg_video.c
ffmpeg_CFLAGS	= \
	-DUSE_FFMPEG		\
	$(LIBAVCODEC_CFLAGS)	\
	$(LIBAVFORMAT_CFLAGS)	\
	$(LIBSWSCALE_CFLAGS)	\
	$(LIBAVUTIL_CFLAGS)	\
	$(NULL)
ffmpeg_LIBS	= \
	$(LIBAVCODEC_LIBS)	\
	$(LIBAVFORMAT_LIBS)	\
	$(LIBSWSCALE_LIBS)	\
	$(LIBAVUTIL_LIBS)	\
	$(NULL)
if USE_VAAPI
ffmpeg_source_c	+= $(vaapi_source_c)
ffmpeg_CFLAGS	+= $(vaapi_CFLAGS)
ffmpeg_LIBS	+= $(vaapi_LIBS)
endif
else
ffmpeg_PROGS	=
endif

if USE_CRYSTALHD
crystalhd_PROGS		= crystalhd_mpeg2 crystalhd_vc1 crystalhd_h264
crystalhd_source_c	= crystalhd.c crystalhd_video.c
crystalhd_CFLAGS	= -DUSE_CRYSTALHD $(CRYSTALHD_CFLAGS)
crystalhd_LIBS		= $(CRYSTALHD_LIBS)
else
crystalhd_PROGS		=
endif

common_SOURCES		= common.c debug.c utils.c image.c buffer.c $(display_SOURCES)
common_CFLAGS		= $(LIBSWSCALE_CFLAGS) $(CAIRO_CFLAGS) $(display_CFLAGS)
common_LIBS		= $(LIBSWSCALE_LIBS) $(CAIRO_LIBS) $(display_LIBS)

vaapi_common_SOURCES	= $(common_SOURCES) $(vaapi_source_c)
vaapi_common_CFLAGS	= $(common_CFLAGS) $(vaapi_CFLAGS)
vaapi_common_LIBS	= $(common_LIBS) $(vaapi_LIBS)
vaapi_h264_SOURCES	= $(vaapi_common_SOURCES) h264.c vaapi_h264.c
vaapi_h264_CFLAGS	= $(vaapi_common_CFLAGS)
vaapi_h264_LDADD	= $(vaapi_common_LIBS)
vaapi_vc1_SOURCES	= $(vaapi_common_SOURCES) vc1.c vaapi_vc1.c
vaapi_vc1_CFLAGS	= $(vaapi_common_CFLAGS)
vaapi_vc1_LDADD		= $(vaapi_common_LIBS)
vaapi_mpeg2_SOURCES	= $(vaapi_common_SOURCES) mpeg2.c vaapi_mpeg2.c
vaapi_mpeg2_CFLAGS	= $(vaapi_common_CFLAGS)
vaapi_mpeg2_LDADD	= $(vaapi_common_LIBS)
vaapi_mpeg4_SOURCES	= $(vaapi_common_SOURCES) mpeg4.c vaapi_mpeg4.c
vaapi_mpeg4_CFLAGS	= $(vaapi_common_CFLAGS)
vaapi_mpeg4_LDADD	= $(vaapi_common_LIBS)
vaapi_jpeg_SOURCES	= $(vaapi_common_SOURCES) jpeg.c vaapi_jpeg.c
vaapi_jpeg_CFLAGS	= $(vaapi_common_CFLAGS)
vaapi_jpeg_LDADD	= $(vaapi_common_LIBS)

vdpau_common_SOURCES	= $(common_SOURCES) $(vdpau_source_c)
vdpau_common_CFLAGS	= $(common_CFLAGS) $(vdpau_CFLAGS)
vdpau_common_LIBS	= $(common_LIBS) $(vdpau_LIBS)
vdpau_h264_SOURCES	= $(vdpau_common_SOURCES) h264.c vdpau_h264.c
vdpau_h264_CFLAGS	= $(vdpau_common_CFLAGS)
vdpau_h264_LDADD	= $(vdpau_common_LIBS)
vdpau_vc1_SOURCES	= $(vdpau_common_SOURCES) vc1.c vdpau_vc1.c
vdpau_vc1_CFLAGS	= $(vdpau_common_CFLAGS)
vdpau_vc1_LDADD		= $(vdpau_common_LIBS)
vdpau_mpeg2_SOURCES	= $(vdpau_common_SOURCES) mpeg2.c vdpau_mpeg2.c
vdpau_mpeg2_CFLAGS	= $(vdpau_common_CFLAGS)
vdpau_mpeg2_LDADD	= $(vdpau_common_LIBS)
vdpau_mpeg4_SOURCES	= $(vdpau_common_SOURCES) mpeg4.c vdpau_mpeg4.c
vdpau_mpeg4_CFLAGS	= $(vdpau_common_CFLAGS)
vdpau_mpeg4_LDADD	= $(vdpau_common_LIBS)

xvba_common_SOURCES	= $(common_SOURCES) $(xvba_source_c)
xvba_common_CFLAGS	= $(common_CFLAGS) $(xvba_CFLAGS)
xvba_common_LIBS	= $(common_LIBS) $(xvba_LIBS)
xvba_h264_SOURCES	= $(xvba_common_SOURCES) h264.c xvba_h264.c
xvba_h264_CFLAGS	= $(xvba_common_CFLAGS)
xvba_h264_LDADD		= $(xvba_common_LIBS)
xvba_vc1_SOURCES	= $(xvba_common_SOURCES) vc1.c xvba_vc1.c
xvba_vc1_CFLAGS		= $(xvba_common_CFLAGS)
xvba_vc1_LDADD		= $(xvba_common_LIBS)

ffmpeg_common_SOURCES	= $(common_SOURCES) $(ffmpeg_source_c)
ffmpeg_common_CFLAGS	= $(common_CFLAGS) $(ffmpeg_CFLAGS)
ffmpeg_common_LIBS	= $(common_LIBS) $(ffmpeg_LIBS)
ffmpeg_h264_SOURCES	= $(ffmpeg_common_SOURCES) h264.c
ffmpeg_h264_CFLAGS	= $(ffmpeg_common_CFLAGS) -DUSE_H264
ffmpeg_h264_LDADD	= $(ffmpeg_common_LIBS)
ffmpeg_vc1_SOURCES	= $(ffmpeg_common_SOURCES) vc1.c
ffmpeg_vc1_CFLAGS	= $(ffmpeg_common_CFLAGS) -DUSE_VC1
ffmpeg_vc1_LDADD	= $(ffmpeg_common_LIBS)
ffmpeg_mpeg2_SOURCES	= $(ffmpeg_common_SOURCES) mpeg2.c
ffmpeg_mpeg2_CFLAGS	= $(ffmpeg_common_CFLAGS) -DUSE_MPEG2
ffmpeg_mpeg2_LDADD	= $(ffmpeg_common_LIBS)
ffmpeg_mpeg4_SOURCES	= $(ffmpeg_common_SOURCES) mpeg4.c
ffmpeg_mpeg4_CFLAGS	= $(ffmpeg_common_CFLAGS) -DUSE_MPEG4
ffmpeg_mpeg4_LDADD	= $(ffmpeg_common_LIBS)

crystalhd_common_SOURCES = $(common_SOURCES) $(crystalhd_source_c)
crystalhd_common_CFLAGS	= $(common_CFLAGS) $(crystalhd_CFLAGS)
crystalhd_common_LIBS	= $(common_LIBS) $(crystalhd_LIBS)
crystalhd_mpeg2_SOURCES	= $(crystalhd_common_SOURCES) mpeg2.c
crystalhd_mpeg2_CFLAGS	= $(crystalhd_common_CFLAGS) -DUSE_MPEG2
crystalhd_mpeg2_LDADD	= $(crystalhd_common_LIBS)
crystalhd_vc1_SOURCES	= $(crystalhd_common_SOURCES) vc1.c
crystalhd_vc1_CFLAGS	= $(crystalhd_common_CFLAGS) -DUSE_VC1
crystalhd_vc1_LDADD	= $(crystalhd_common_LIBS)
crystalhd_h264_SOURCES	= $(crystalhd_common_SOURCES) h264.c
crystalhd_h264_CFLAGS	= $(crystalhd_common_CFLAGS) -DUSE_H264
crystalhd_h264_LDADD	= $(crystalhd_common_LIBS)

EXTRA_DIST = \
	xvba.supp

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = Makefile.in config.h.in

# Filter out private files and add empty files instead to keep automake happy
dist-hook:
	for p in $(PRIVATE_APIS); do					\
	    case ":$(FORCE_DIST):" in					\
	    (*":$$p:"*) ;; (*)						\
	    for f in $(distdir)/*$$p*.*; do echo > $$f; done;;	\
	    esac;							\
	done
