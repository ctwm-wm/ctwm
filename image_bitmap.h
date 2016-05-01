/*
 * XBM image handling function bits
 */
#ifndef _IMAGE_XBM_H
#define _IMAGE_XBM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>

#include "types.h"
#include "util.h"

Pixmap FindBitmap(char *name, unsigned int *widthp,
                  unsigned int *heightp);
Image *GetBitmapImage(char *name, ColorPair cp);
Pixmap GetBitmap(char *name);


#endif /* _IMAGE_XBM_H */
