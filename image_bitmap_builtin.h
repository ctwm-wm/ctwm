/*
 * Buildin image bitmaps lookup/generation
 */
#ifndef _CTWM_IMAGE_BITMAP_BUILTIN_H
#define _CTWM_IMAGE_BITMAP_BUILTIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>

Pixmap get_builtin_plain_pixmap(const char *name, unsigned int *widthp,
                                unsigned int *heightp);
Image *get_builtin_scalable_pixmap(const char *name, ColorPair cp);
Image *get_builtin_animated_pixmap(const char *name, ColorPair cp);

#endif /* _CTWM_IMAGE_BITMAP_BUILTIN_H */
