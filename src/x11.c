/*
 *  x11.c - X11 common code
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
#include "x11.h"
#include "utils_x11.h"
#include "common.h"
#include <X11/Xutil.h>

#if USE_GLX
#include <GL/glx.h>
#endif

static X11Context *x11_context;

static const int x11_event_mask = (KeyPressMask |
                                   KeyReleaseMask |
                                   ButtonPressMask |
                                   ButtonReleaseMask |
                                   PointerMotionMask |
                                   EnterWindowMask |
                                   ExposureMask |
                                   StructureNotifyMask);

int x11_init(void)
{
    CommonContext * const common = common_get_context();
    Display *dpy;
    int screen, depth;
    Visual *vis;
    Colormap cmap;
    XVisualInfo visualInfo;
    XVisualInfo *vi;
    XSetWindowAttributes xswa;
    unsigned long xswa_mask;
    XWindowAttributes wattr;
    Window root_window, window;
    unsigned long black_pixel, white_pixel;
    GC gc = None;
    XImage *img = NULL;
    Pixmap pixmap = None;
    Window subwindow = None;

    if (x11_context)
        return 0;

    if ((dpy = XOpenDisplay(NULL)) == NULL)
        return -1;

    screen              = DefaultScreen(dpy);
    vis                 = DefaultVisual(dpy, screen);
    root_window         = RootWindow(dpy, screen);
    black_pixel         = BlackPixel(dpy, screen);
    white_pixel         = WhitePixel(dpy, screen);

    XGetWindowAttributes(dpy, root_window, &wattr);
    depth = wattr.depth;
    if (depth != 15 && depth != 16 && depth != 24 && depth != 32)
        depth = 24;

    vi = &visualInfo;
    XMatchVisualInfo(dpy, screen, depth, TrueColor, vi);
    cmap = CopyFromParent;

#if USE_GLX
    if (display_type() == DISPLAY_GLX) {
        static GLint gl_visual_attr[] = {
            GLX_RGBA,
            GLX_RED_SIZE, 1,
            GLX_GREEN_SIZE, 1,
            GLX_BLUE_SIZE, 1,
            GLX_DOUBLEBUFFER,
            GL_NONE
        };
        vi = glXChooseVisual(dpy, screen, gl_visual_attr);
        if (!vi)
            return -1;
        cmap = XCreateColormap(dpy, root_window, vi->visual, AllocNone);
        if (cmap == None)
            return -1;
    }
#endif

    xswa_mask             = CWBorderPixel | CWBackPixel;
    xswa.border_pixel     = black_pixel;
    xswa.background_pixel = white_pixel;

    if (cmap != CopyFromParent) {
        xswa_mask        |= CWColormap;
        xswa.colormap     = cmap;
    }

    window = XCreateWindow(dpy, root_window,
                           0, 0,
                           common->window_size.width,
                           common->window_size.height,
                           0, depth,
                           InputOutput, vi->visual, xswa_mask, &xswa);
    if (window == None)
        return -1;

    XSelectInput(dpy, window, x11_event_mask);

    XMapWindow(dpy, window);
    x11_wait_event(dpy, window, MapNotify);
    x11_wait_event(dpy, window, Expose); /* XXX: workaround an XvBA init bug */

    if (common->use_subwindow_rect) {
        xswa_mask &= ~CWColormap;
        subwindow = XCreateWindow(
            dpy,
            window,
            common->subwindow_rect.x,
            common->subwindow_rect.y,
            common->subwindow_rect.width,
            common->subwindow_rect.height,
            1,
            depth,
            InputOutput,
            vi->visual,
            xswa_mask, &xswa
        );
        if (subwindow == None)
            return -1;

        XSelectInput(dpy, subwindow, x11_event_mask);

        XMapWindow(dpy, subwindow);
        x11_wait_event(dpy, subwindow, MapNotify);
        x11_wait_event(dpy, subwindow, Expose);
    }

    if (vi != &visualInfo)
        XFree(vi);

    if (getimage_mode() != GETIMAGE_NONE) {
        if ((gc = XCreateGC(dpy, window, 0, 0)) == None)
            return -1;

        img = XCreateImage(dpy, vis, 24, ZPixmap, 0,
                           common->image->pixels[0],
                           common->window_size.width,
                           common->window_size.height,
                           32,
                           common->image->pitches[0]);
        if (!img)
            return -1;
    }

    if (getimage_mode() == GETIMAGE_FROM_PIXMAP) {
        pixmap = XCreatePixmap(dpy, window,
                               common->window_size.width,
                               common->window_size.height,
                               32);
        if (pixmap == None)
            return -1;
    }

    if ((x11_context = calloc(1, sizeof(*x11_context))) == NULL)
        return -1;

    x11_context->display        = dpy;
    x11_context->display_width  = DisplayWidth(dpy, screen);
    x11_context->display_height = DisplayHeight(dpy, screen);
    x11_context->screen         = screen;
    x11_context->visual         = vis;
    x11_context->root_window    = root_window;
    x11_context->black_pixel    = black_pixel;
    x11_context->white_pixel    = white_pixel;
    x11_context->window         = window;
    x11_context->window_width   = common->window_size.width;
    x11_context->window_height  = common->window_size.height;
    x11_context->gc             = gc;
    x11_context->image          = img;
    x11_context->pixmap         = pixmap;
    x11_context->subwindow      = subwindow;
    return 0;
}

int x11_exit(void)
{
    X11Context * const x11 = x11_get_context();

    if (!x11)
        return 0;

    if (x11->pixmap) {
        XFreePixmap(x11->display, x11->pixmap);
        x11->pixmap = None;
    }

    if (x11->image) {
        x11->image->data = NULL; /* XImage doesn't own this buffer */
        XDestroyImage(x11->image);
        x11->image = NULL;
    }

    if (x11->gc) {
        XFreeGC(x11->display, x11->gc);
        x11->gc = NULL;
    }

    if (x11->subwindow) {
        XUnmapWindow(x11->display, x11->subwindow);
        x11_wait_event(x11->display, x11->subwindow, UnmapNotify);
        XDestroyWindow(x11->display, x11->subwindow);
        x11->subwindow = None;
    }

    if (x11->window) {
        XUnmapWindow(x11->display, x11->window);
        x11_wait_event(x11->display, x11->window, UnmapNotify);
        XDestroyWindow(x11->display, x11->window);
        x11->window = None;
    }

    if (x11->display) {
        XFlush(x11->display);
        XSync(x11->display, False);
        XCloseDisplay(x11->display);
        x11->display = NULL;
    }

    free(x11_context);
    x11_context = NULL;
    return 0;
}

X11Context *x11_get_context(void)
{
    return x11_context;
}

int x11_display(void)
{
    X11Context * const x11 = x11_get_context();

    if (getimage_mode() != GETIMAGE_NONE) {
        if (!x11->image)
            return -1;

        if (getimage_mode() == GETIMAGE_FROM_PIXMAP) {
            if (x11->pixmap == None)
                return -1;
            if (XGetSubImage(x11->display, x11->pixmap,
                             0, 0, x11->window_width, x11->window_height,
                             AllPlanes, ZPixmap,
                             x11->image, 0, 0) != x11->image)
                return -1;
        }

        XPutImage(x11->display, x11->window, x11->gc, x11->image,
                  0, 0, 0, 0, x11->window_width, x11->window_height);
        XSync(x11->display, False);
    }
    return common_display();
}
