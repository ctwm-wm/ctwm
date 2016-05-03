/*
 * Bitmap image handling function bits
 */
#ifndef _IMAGE_BITMAP_H
#define _IMAGE_BITMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>

#include "types.h"
#include "util.h"

Image *GetBitmapImage(const char *name, ColorPair cp);
Pixmap GetBitmap(const char *name);

/* Used for cursors */
extern int HotX, HotY;

#endif /* _IMAGE_BITMAP_H */
