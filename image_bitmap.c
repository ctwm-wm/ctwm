/*
 * Bitmap image handling functions
 *
 * These are what's called "XBM", and represented by the Pixmap X data
 * type (which has noting to do with "XPM" X PixMaps, just to confuse
 * you).  This is the format used for various builtin images, buttons,
 * and cursors, and is also the fallback type for any user-specified
 * images.  Assuming any user has made and specified an XBM since the
 * 1980's.
 */

#include "ctwm.h"

#include <X11/X.h>
#include <X11/Xmu/Drawing.h>

#include "screen.h"
#include "types.h"

#include "image.h"
#include "image_bitmap.h"
#include "image_bitmap_builtin.h"


static Image *LoadBitmapImage(char  *name, ColorPair cp);
static Pixmap FindBitmap(char *name, unsigned int *widthp,
                         unsigned int *heightp);


/*
 * External API's
 */

/* Simple load-by-name */
Pixmap
GetBitmap(char *name)
{
	return FindBitmap(name, &JunkWidth, &JunkHeight);
}


/*
 * Load with FG/BG adjusted to given colorpair
 */
Image *
GetBitmapImage(char  *name, ColorPair cp)
{
	Image       *image, *r, *s;
	char        path [128], pref [128];
	char        *perc;
	int         i;

	if(! strchr(name, '%')) {
		return (LoadBitmapImage(name, cp));
	}
	s = image = None;
	strcpy(pref, name);
	perc  = strchr(pref, '%');
	*perc = '\0';
	for(i = 1;; i++) {
		sprintf(path, "%s%d%s", pref, i, perc + 1);
		r = LoadBitmapImage(path, cp);
		if(r == None) {
			break;
		}
		r->next = None;
		if(image == None) {
			s = image = r;
		}
		else {
			s->next = r;
			s = r;
		}
	}
	if(s != None) {
		s->next = image;
	}
	if(image == None) {
		fprintf(stderr, "Cannot open any %s bitmap file\n", name);
	}
	return (image);
}


/*
 * Internal bits used by the above
 */

/***********************************************************************
 *
 *  Procedure:
 *      FindBitmap - read in a bitmap file and return size
 *
 *  Returned Value:
 *      the pixmap associated with the bitmap
 *      widthp  - pointer to width of bitmap
 *      heightp - pointer to height of bitmap
 *
 *  Inputs:
 *      name    - the filename to read
 *
 ***********************************************************************
 */
static Pixmap
FindBitmap(char *name, unsigned int *widthp,
           unsigned int *heightp)
{
	char *bigname;
	Pixmap pm;

	if(!name) {
		return None;
	}

	/*
	 * Names of the form :name refer to hardcoded images that are scaled to
	 * look nice in title buttons.  Eventually, it would be nice to put in a
	 * menu symbol as well....
	 */
	if(name[0] == ':') {
		return get_builtin_plain_pixmap(name, widthp, heightp);
	}

	/*
	 * Generate a full pathname with any special prefix characters (such
	 * as ~) expanded.
	 */
	bigname = ExpandFilename(name);
	if(!bigname) {
		/* Something failed bad */
		return None;
	}

	/*
	 * look along bitmapFilePath resource same as toolkit clients
	 */
	pm = XmuLocateBitmapFile(ScreenOfDisplay(dpy, Scr->screen), bigname, NULL,
	                         0, (int *)widthp, (int *)heightp, &HotX, &HotY);
	if(pm == None && Scr->IconDirectory && bigname[0] != '/') {
		/*
		 * Didn't find it.  Attempt to find icon in old IconDirectory
		 * (now obsolete)
		 */
		free(bigname);
		asprintf(&bigname, "%s/%s", Scr->IconDirectory, name);
		if(XReadBitmapFile(dpy, Scr->Root, bigname, widthp, heightp, &pm,
		                   &HotX, &HotY) != BitmapSuccess) {
			pm = None;
		}
	}
	free(bigname);
	if((pm == None) && reportfilenotfound) {
		fprintf(stderr, "%s:  unable to find bitmap \"%s\"\n", ProgramName, name);
	}

	return pm;
}


static Image *
LoadBitmapImage(char  *name, ColorPair cp)
{
	Image        *image;
	Pixmap       bm;
	unsigned int width, height;
	XGCValues    gcvalues;

	if(Scr->rootGC == (GC) 0) {
		Scr->rootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);
	}
	bm = FindBitmap(name, &width, &height);
	if(bm == None) {
		return (None);
	}

	image = (Image *) malloc(sizeof(Image));
	image->pixmap = XCreatePixmap(dpy, Scr->Root, width, height, Scr->d_depth);
	gcvalues.background = cp.back;
	gcvalues.foreground = cp.fore;
	XChangeGC(dpy, Scr->rootGC, GCForeground | GCBackground, &gcvalues);
	XCopyPlane(dpy, bm, image->pixmap, Scr->rootGC, 0, 0, width, height,
	           0, 0, (unsigned long) 1);
	XFreePixmap(dpy, bm);
	image->mask   = None;
	image->width  = width;
	image->height = height;
	image->next   = None;
	return (image);
}

