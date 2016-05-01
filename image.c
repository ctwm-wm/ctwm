/*
 * Image handling functions
 *
 * This provides some general and hub stuff.  Details of different image
 * type functions, and generation of builtins, go in their own files.
 */

#include "ctwm.h"

#include <stdio.h>
#include <string.h>

#include "screen.h"
#include "types.h"
#include "util.h"

#include "image.h"
#include "image_bitmap.h"
#include "image_bitmap_builtin.h"
#include "image_xwd.h"
#ifdef JPEG
#include "image_jpeg.h"
#endif
#if defined (XPM)
#include "image_xpm.h"
#endif

/* Flag (maybe should be retired */
bool reportfilenotfound = false;
Colormap AlternateCmap = None;


/*
 * Find (load/generate) an image by name
 */
Image
*GetImage(char *name, ColorPair cp)
{
	name_list **list;
	char fullname [256];
	Image *image;

	if(name == NULL) {
		return (None);
	}
	image = None;

	list = &Scr->ImageCache;
	if(0)
		/* dummy */ ;
#ifdef XPM
	else if((name [0] == '@') || (strncmp(name, "xpm:", 4) == 0)) {
		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);

		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			int startn = (name [0] == '@') ? 1 : 4;
			if((image = GetXpmImage(name + startn, cp)) != None) {
				AddToList(list, fullname, image);
			}
		}
	}
#endif
#ifdef JPEG
	else if(strncmp(name, "jpeg:", 5) == 0) {
		if((image = (Image *) LookInNameList(*list, name)) == None) {
			if((image = GetJpegImage(&name [5])) != None) {
				AddToList(list, name, image);
			}
		}
	}
#endif
	else if((strncmp(name, "xwd:", 4) == 0) || (name [0] == '|')) {
		int startn = (name [0] == '|') ? 0 : 4;
		if((image = (Image *) LookInNameList(*list, name)) == None) {
			if((image = GetXwdImage(&name [startn], cp)) != None) {
				AddToList(list, name, image);
			}
		}
	}
	else if(strncmp(name, ":xpm:", 5) == 0) {
		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			image = get_builtin_scalable_pixmap(name, cp);
			if(image == None) {
				/* g_b_s_p() already warned */
				return (None);
			}
			AddToList(list, fullname, image);
		}
	}
	else if(strncmp(name, "%xpm:", 5) == 0) {
		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			image = get_builtin_animated_pixmap(name, cp);
			if(image == None) {
				/* g_b_a_p() already warned */
				return (None);
			}
			AddToList(list, fullname, image);
		}
	}
	else if(name [0] == ':') {
		unsigned int    width, height;
		Pixmap          pm = 0;
		XGCValues       gcvalues;

		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			pm = get_builtin_plain_pixmap(name, &width, &height);
			if(pm == None) {
				/* g_b_p_p() already warned */
				return (None);
			}
			image = (Image *) malloc(sizeof(Image));
			image->pixmap = XCreatePixmap(dpy, Scr->Root, width, height, Scr->d_depth);
			if(Scr->rootGC == (GC) 0) {
				Scr->rootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);
			}
			gcvalues.background = cp.back;
			gcvalues.foreground = cp.fore;
			XChangeGC(dpy, Scr->rootGC, GCForeground | GCBackground, &gcvalues);
			XCopyPlane(dpy, pm, image->pixmap, Scr->rootGC, 0, 0, width, height, 0, 0,
			           (unsigned long) 1);
			image->mask   = None;
			image->width  = width;
			image->height = height;
			image->next   = None;
			AddToList(list, fullname, image);
		}
	}
	else {
		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			if((image = GetBitmapImage(name, cp)) != None) {
				AddToList(list, fullname, image);
			}
		}
	}
	return (image);
}


/*
 * Cleanup an image
 */
void
FreeImage(Image *image)
{
	Image *im, *im2;

	im = image;
	while(im != None) {
		if(im->pixmap) {
			XFreePixmap(dpy, im->pixmap);
		}
		if(im->mask) {
			XFreePixmap(dpy, im->mask);
		}
		im2 = im->next;
		free(im);
		im = im2;
	}
}
