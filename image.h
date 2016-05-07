/*
 * General image handling function bits
 */
#ifndef _CTWM_IMAGE_H
#define _CTWM_IMAGE_H

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


Image *GetImage(const char *name, ColorPair cp);
Image *AllocImage(void);
void FreeImage(Image *image);


/* Used internally in image*.c */
extern bool reportfilenotfound;
extern Colormap AlternateCmap;

char *ExpandPixmapPath(const char *name);
Image *get_image_anim_cp(const char *name, ColorPair cp,
                         Image * (*imgloader)(const char *, ColorPair));

#endif /* _CTWM_IMAGE_H */
