/*
 *  glx.h - GLX common code
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

#ifndef GLX_H
#define GLX_H

#include <X11/X.h>
#include "utils_glx.h"

typedef struct _GLXContext GLXContext;

struct _GLXContext {
    GLContextState      *cs;
    GLuint               texture;
    GLenum               texture_target;
    unsigned int         texture_width;
    unsigned int         texture_height;
    GLPixmapObject      *pixo;
    GLFramebufferObject *fbo;
    unsigned int         use_tfp : 1;
    unsigned int         use_fbo : 1;
};

GLXContext *glx_get_context(void);

int glx_init(void);
int glx_init_texture(unsigned int width, unsigned int height);
int glx_exit(void);
int glx_display(void);

Pixmap glx_get_pixmap(void);

#endif /* GLX_H */
