/*
 * General image handling function bits
 */
#ifndef _IMAGE_H
#define _IMAGE_H

#include <stdbool.h>

#include "types.h"


/* Widely used through the codebase */
struct Image {
	Pixmap pixmap;
	Pixmap mask;
	int    width;
	int    height;
	Image *next;
};


Image *GetImage(char *name, ColorPair cp);
void FreeImage(Image *image);


/* Used internally in image*.c */
extern bool reportfilenotfound;
extern Colormap AlternateCmap;

char *ExpandPixmapPath(char *name);
Image *get_image_anim_cp(const char *name, ColorPair cp,
                         Image * (*imgloader)(char *, ColorPair));

#endif /* _IMAGE_H */
