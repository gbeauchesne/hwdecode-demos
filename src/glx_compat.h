/*
 *  glx_compat.h - GLX compatibility glue
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

#ifndef GLX_COMPAT_H
#define GLX_COMPAT_H

/* Don't clash with GLXContext type from <GL/glx.h> */
#undef GLX_H
#define GLXContext GLX_Context
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>
#undef GLXContext

#if GL_GLEXT_VERSION >= 85
/* XXX: PFNGLMULTITEXCOORD2FPROC got out of the GL_VERSION_1_3_DEPRECATED
   block and is not defined if GL_VERSION_1_3 is defined in <GL/gl.h>
   Redefine the type here as an interim solution */
typedef void (*PFNGLMULTITEXCOORD2FPROC) (GLenum target, GLfloat s, GLfloat t);
#endif

/* Define missing types for GLX_EXT_texture_from_pixmap */
#if GLX_GLXEXT_VERSION < 18
typedef void (*PFNGLXBINDTEXIMAGEEXTPROC)(Display *, GLXDrawable, int, const int *);
typedef void (*PFNGLXRELEASETEXIMAGEEXTPROC)(Display *, GLXDrawable, int);
#endif

/* Define hooks from GL_NV_vdpau_interop extension */
#if GL_GLEXT_VERSION < 64
typedef GLintptr GLvdpauSurfaceNV;
typedef void (*PFNGLVDPAUINITNVPROC)(const GLvoid *vdpDevice, const GLvoid *getProcAddress);
typedef void (*PFNGLVDPAUFININVPROC)(void);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTERVIDEOSURFACENVPROC)(const GLvoid *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
typedef GLvdpauSurfaceNV (*PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC)(const GLvoid *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
typedef GLboolean (*PFNGLVDPAUISSURFACENVPROC)(GLvdpauSurfaceNV surface);
typedef void (*PFNGLVDPAUUNREGISTERSURFACENVPROC)(GLvdpauSurfaceNV surface);
typedef void (*PFNGLVDPAUGETSURFACEIVNVPROC)(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
typedef void (*PFNGLVDPAUSURFACEACCESSNVPROC)(GLvdpauSurfaceNV surface, GLenum access);
typedef void (*PFNGLVDPAUMAPSURFACESNVPROC)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
typedef void (*PFNGLVDPAUUNMAPSURFACESNVPROC)(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces);
#endif

/* FBO and shaders defines */
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING_EXT
#endif
#ifndef GL_FRAGMENT_PROGRAM
#define GL_FRAGMENT_PROGRAM GL_FRAGMENT_PROGRAM_ARB
#endif
#ifndef GL_PROGRAM_FORMAT_ASCII
#define GL_PROGRAM_FORMAT_ASCII GL_PROGRAM_FORMAT_ASCII_ARB
#endif
#ifndef GL_PROGRAM_ERROR_POSITION
#define GL_PROGRAM_ERROR_POSITION GL_PROGRAM_ERROR_POSITION_ARB
#endif
#ifndef GL_PROGRAM_ERROR_STRING
#define GL_PROGRAM_ERROR_STRING GL_PROGRAM_ERROR_STRING_ARB
#endif
#ifndef GL_PROGRAM_UNDER_NATIVE_LIMITS
#define GL_PROGRAM_UNDER_NATIVE_LIMITS GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB
#endif

#endif /* GLX_COMPAT_H */
