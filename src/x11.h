/*
 *  x11.h - X11 common code
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

#ifndef X11_H
#define X11_H

#include <X11/Xlib.h>

typedef struct _X11Context X11Context;

struct _X11Context {
    Display            *display;
    unsigned int        display_width;
    unsigned int        display_height;
    int                 screen;
    Visual             *visual;
    Window              root_window;
    unsigned long       black_pixel;
    unsigned long       white_pixel;
    Window              window;
    unsigned int        window_width;
    unsigned int        window_height;
    GC                  gc;
    XImage             *image;
    Pixmap              pixmap;
    Window              subwindow;
};

X11Context *x11_get_context(void);

int x11_init(void);
int x11_exit(void);
int x11_display(void);

#endif /* X11_H */
