/*
 * Copyright notice...
 */

#ifndef _CTWM_XRANDR_H
#define _CTWM_XRANDR_H

#include "r_layout.h"

#include <X11/Xlib.h>


RLayout *XrandrNewLayout(Display *dpy, Window rootw);

#endif /* _CTWM_XRANDR_H */
