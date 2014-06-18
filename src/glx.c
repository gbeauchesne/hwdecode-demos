/*
 *  glx.c - GLX common code
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

#define _GNU_SOURCE 1
#include "sysdeps.h"
#include "glx.h"
#include "common.h"
#include "x11.h"
#include "utils_x11.h"
#include "utils_glx.h"
#include <dlfcn.h>
#include <limits.h>

#if USE_VAAPI
#include "vaapi.h"
#endif

#if USE_VDPAU
#include "vdpau.h"
#endif

static GLXContext *glx_context;

int glx_init(void)
{
    GLContextState old_cs;
    GLXContext *glx;
    X11Context *x11;

    if (glx_context)
        return 0;

    if (x11_init() < 0)
        return -1;
    x11 = x11_get_context();

    glx = calloc(1, sizeof(*glx));
    if (!glx)
        return -1;
    glx_context = glx;

    old_cs.display = x11->display;
    old_cs.window  = x11->window;
    old_cs.context = NULL;
    glx->cs = gl_create_context(x11->display, x11->screen, &old_cs);
    if (!glx->cs)
        return -1;
    if (!gl_set_current_context(glx->cs, NULL))
        return -1;

    gl_init_context(glx->cs);

#define FOVY     60.0f
#define ASPECT   1.0f
#define Z_NEAR   0.1f
#define Z_FAR    100.0f
#define Z_CAMERA 0.869f

    glViewport(0, 0, x11->window_width, x11->window_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOVY, ASPECT, Z_NEAR, Z_FAR);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(-0.5f, -0.5f, -Z_CAMERA);
    glScalef(1.0f / (GLfloat)x11->window_width,
             -1.0f / (GLfloat)x11->window_height,
             1.0f / (GLfloat)x11->window_width);
    glTranslatef(0.0f, -1.0f * (GLfloat)x11->window_height, 0.0f);

    glClearColor(1.0, 1.0, 1.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}

static GLenum get_texture_target(void)
{
    GLenum target;

    switch (common_get_context()->glx_texture_target) {
    case TEXTURE_TARGET_2D:   target = GL_TEXTURE_2D;            break;
    case TEXTURE_TARGET_RECT: target = GL_TEXTURE_RECTANGLE_ARB; break;
    default:                  target = GL_NONE;                  break;
    }
    ASSERT(target != GL_NONE);
    return target;
}

static GLenum get_texture_format(void)
{
    GLenum format;

    switch (common_get_context()->glx_texture_format) {
    case IMAGE_RGBA: format = GL_RGBA; break;
    case IMAGE_BGRA: format = GL_BGRA; break;
    default:         format = GL_NONE; break;
    }
    ASSERT(format != GL_NONE);
    return format;
}

int glx_init_texture(unsigned int width, unsigned int height)
{
    CommonContext * const common = common_get_context();
    GLXContext * const glx = glx_get_context();
    GLVTable * const gl_vtable = gl_get_vtable();

    if (!gl_vtable->has_texture_non_power_of_two)
        return -1;

    if (common->use_glx_texture_size) {
        width  = common->glx_texture_size.width;
        height = common->glx_texture_size.height;
    }

    const GLenum target = get_texture_target();
    glx->texture = gl_create_texture(
        target,
        get_texture_format(),
        width,
        height
    );
    if (!glx->texture)
        return -1;

    glx->texture_target = target;
    glx->texture_width  = width;
    glx->texture_height = height;
    return 0;
}

int glx_exit(void)
{
    GLXContext * const glx = glx_get_context();

    if (!glx)
        return -1;

    if (glx->texture) {
        glDeleteTextures(1, &glx->texture);
        glx->texture = 0;
    }

    if (glx->fbo) {
        gl_destroy_framebuffer_object(glx->fbo);
        glx->fbo = NULL;
    }

    if (glx->pixo) {
        gl_destroy_pixmap_object(glx->pixo);
        glx->pixo = NULL;
    }

    if (glx->cs) {
        gl_destroy_context(glx->cs);
        glx->cs = NULL;
    }

    free(glx_context);
    glx_context = NULL;
    return 0;
}

GLXContext *glx_get_context(void)
{
    return glx_context;
}

static inline int use_tfp(void)
{
    return glx_get_context()->use_tfp;
}

static inline int use_fbo(void)
{
    return glx_get_context()->use_fbo;
}

static void render_background(void)
{
    X11Context * const x11 = x11_get_context();

    /* Original code from Mirco Muller (MacSlow):
       <http://cgit.freedesktop.org/~macslow/gl-gst-player/> */
    GLfloat fStartX = 0.0f;
    GLfloat fStartY = 0.0f;
    GLfloat fWidth  = (GLfloat)x11->window_width;
    GLfloat fHeight = (GLfloat)x11->window_height;

    glBegin(GL_QUADS);
    {
        /* top third, darker grey to white */
        glColor3f(0.85f, 0.85f, 0.85f);
        glVertex3f(fStartX, fStartY, 0.0f);
        glColor3f(0.85f, 0.85f, 0.85f);
        glVertex3f(fStartX + fWidth, fStartY, 0.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(fStartX + fWidth, fStartY + fHeight / 3.0f, 0.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(fStartX, fStartY + fHeight / 3.0f, 0.0f);

        /* middle third, just plain white */
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(fStartX, fStartY + fHeight / 3.0f, 0.0f);
        glVertex3f(fStartX + fWidth, fStartY + fHeight / 3.0f, 0.0f);
        glVertex3f(fStartX + fWidth, fStartY + 2.0f * fHeight / 3.0f, 0.0f);
        glVertex3f(fStartX, fStartY + 2.0f * fHeight / 3.0f, 0.0f);

        /* bottom third, white to lighter grey */
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(fStartX, fStartY + 2.0f * fHeight / 3.0f, 0.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(fStartX + fWidth, fStartY + 2.0f * fHeight / 3.0f, 0.0f);
        glColor3f(0.62f, 0.66f, 0.69f);
        glVertex3f(fStartX + fWidth, fStartY + fHeight, 0.0f);
        glColor3f(0.62f, 0.66f, 0.69f);
        glVertex3f(fStartX, fStartY + fHeight, 0.0f);
    }
    glEnd();
}

static int render_offscreen(void)
{
    GLXContext * const glx = glx_get_context();
    const GLenum target    = glx->texture_target;
    const unsigned int w   = glx->texture_width;
    const unsigned int h   = glx->texture_height;

    if (!use_fbo())
        return 0;

    gl_bind_framebuffer_object(glx->fbo);
    if (glx->pixo && !gl_bind_pixmap_object(glx->pixo))
        return -1;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    {
        float tw, th;
        switch (target) {
        case GL_TEXTURE_2D:
            tw = 1.0f;
            th = 1.0f;
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            tw = (float)w;
            th = (float)h;
            break;
        default:
            tw = 0.0f;
            th = 0.0f;
            ASSERT(target == GL_TEXTURE_2D ||
                   target == GL_TEXTURE_RECTANGLE_ARB);
            break;
        }

        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, th  ); glVertex2i(0, h);
        glTexCoord2f(tw  , th  ); glVertex2i(w, h);
        glTexCoord2f(tw  , 0.0f); glVertex2i(w, 0);
    }
    glEnd();

    if (glx->pixo && !gl_unbind_pixmap_object(glx->pixo))
        return -1;
    gl_unbind_framebuffer_object(glx->fbo);
    return 0;
}

static int glx_bind_texture(void)
{
    GLXContext * const glx = glx_get_context();

#if USE_VDPAU
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    if (vdpau->use_vdpau_glx_interop && common->vdpau_glx_output_surface) {
        GLVdpSurface * const s = vdpau->glx_surface;
        if (vdpau_glx_begin_render_surface() < 0)
            return -1;
        glBindTexture(s->target, s->textures[0]);
        return 0;
    }
#endif

    glBindTexture(glx->texture_target, glx->texture);
#if USE_VAAPI
    if (vaapi_glx_begin_render_surface() < 0)
        return -1;
#else
    if (!use_fbo() && glx->pixo && !gl_bind_pixmap_object(glx->pixo))
        return -1;
#endif
    return 0;
}

static int glx_unbind_texture(void)
{
    GLXContext * const glx = glx_get_context();

#if USE_VDPAU
    CommonContext * const common = common_get_context();
    VDPAUContext * const vdpau = vdpau_get_context();
    if (vdpau->use_vdpau_glx_interop && common->vdpau_glx_output_surface) {
        GLVdpSurface * const s = vdpau->glx_surface;
        if (vdpau_glx_end_render_surface() < 0)
            return -1;
        glBindTexture(s->target, 0);
        return 0;
    }
#endif

#if USE_VAAPI
    if (vaapi_glx_end_render_surface() < 0)
        return -1;
#else
    if (!use_fbo() && glx->pixo && !gl_unbind_pixmap_object(glx->pixo))
        return -1;
#endif
    glBindTexture(glx->texture_target, 0);
    return 0;
}

static int render_frame(void)
{
    X11Context * const x11 = x11_get_context();
    const unsigned int w   = x11->window_width;
    const unsigned int h   = x11->window_height;

    GLXContext * const glx = glx_get_context();
    const GLenum target    = glx->texture_target;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    {
        float tw, th;
        switch (target) {
        case GL_TEXTURE_2D:
            tw = 1.0f;
            th = 1.0f;
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            tw = (float)glx->texture_width;
            th = (float)glx->texture_height;
            break;
        default:
            tw = 0.0f;
            th = 0.0f;
            ASSERT(target == GL_TEXTURE_2D ||
                   target == GL_TEXTURE_RECTANGLE_ARB);
            break;
        }

        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, th  ); glVertex2i(0, h);
        glTexCoord2f(tw  , th  ); glVertex2i(w, h);
        glTexCoord2f(tw  , 0.0f); glVertex2i(w, 0);
    }
    glEnd();
    return 0;
}

static int render_reflection(void)
{
    X11Context * const x11 = x11_get_context();
    const unsigned int w   = x11->window_width;
    const unsigned int h   = x11->window_height;
    const unsigned int rh  = 100;
    GLfloat ry = 1.0f - (GLfloat)rh / (GLfloat)h;

    GLXContext * const glx = glx_get_context();
    const GLenum target    = glx->texture_target;

    glBegin(GL_QUADS);
    {
        float tw, th;
        switch (target) {
        case GL_TEXTURE_2D:
            tw = 1.0f;
            th = 1.0f;
            break;
        case GL_TEXTURE_RECTANGLE_ARB:
            tw = (float)glx->texture_width;
            th = (float)glx->texture_height;
            ry = ry * th;
            break;
        default:
            tw = 0.0f;
            th = 0.0f;
            ASSERT(target == GL_TEXTURE_2D ||
                   target == GL_TEXTURE_RECTANGLE_ARB);
            break;
        }

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTexCoord2f(0.0f, th); glVertex2i(0, 0);
        glTexCoord2f(tw  , th); glVertex2i(w, 0);

        glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
        glTexCoord2f(tw  , ry); glVertex2i(w, rh);
        glTexCoord2f(0.0f, ry); glVertex2i(0, rh);
    }
    glEnd();
    return 0;
}

int glx_display(void)
{
    CommonContext * const common = common_get_context();
    GLXContext * const glx = glx_get_context();
    X11Context * const x11 = x11_get_context();

    if (glx->texture == 0)
        return -1;

    if (getimage_mode() != GETIMAGE_NONE)
        return -1;

    if (use_tfp())
        printf("GLX: use texture_from_pixmap extension (TFP)\n");

    if (use_fbo()) {
        printf("GLX: use framebuffer_object extension (FBO)\n");
        if (render_offscreen() < 0)
            return -1;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    render_background();

    if (glx_bind_texture() < 0)
        return -1;
    glPushMatrix();
    if (common->glx_use_reflection) {
        glRotatef(20.0f, 0.0f, 1.0f, 0.0f);
        glTranslatef(50.0f, 0.0f, 0.0f);
    }
    if (render_frame() < 0)
        return -1;
    if (common->glx_use_reflection) {
        glPushMatrix();
        glTranslatef(0.0, (GLfloat)x11->window_height + 5.0f, 0.0f);
        if (render_reflection() < 0)
            return -1;
        glPopMatrix();
    }
    glPopMatrix();
    if (glx_unbind_texture() < 0)
        return -1;

    gl_swap_buffers(glx->cs);
    return common_display();
}

Pixmap glx_get_pixmap(void)
{
    GLXContext * const glx = glx_get_context();
    X11Context * const x11 = x11_get_context();

    if (!glx->pixo) {
        glx->pixo = gl_create_pixmap_object(
            x11->display,
            glx->texture_target,
            glx->texture_width,
            glx->texture_height
        );
        if (!glx->pixo)
            return None;
    }

    if (common_get_context()->glx_use_fbo) {
        glx->fbo = gl_create_framebuffer_object(
            glx->texture_target,
            glx->texture,
            glx->texture_width,
            glx->texture_height
        );
        if (!glx->fbo)
            return None;
        glx->use_fbo = 1;
    }

    glx->use_tfp = 1;
    return glx->pixo->pixmap;
}
