/*
 * XPM image handling functions
 */

#include "ctwm.h"

#include <stdio.h>
#include <string.h>

#include <X11/xpm.h>

#include "screen.h"
#include "types.h"
#include "util.h"

#include "image.h"
#include "image_xpm.h"

static Image *LoadXpmImage(char  *name, ColorPair cp);
static void   xpmErrorMessage(int status, char *name, char *fullname);

static int reportxpmerror = 1;


/*
 * External entry point
 */
Image *
GetXpmImage(char *name, ColorPair cp)
{
	Image   *image, *s;
	char    *pref, *perc;
	int     i;

	/* For non-animated requests, just load the file */
	if(! strchr(name, '%')) {
		return (LoadXpmImage(name, cp));
	}

	/*
	 * For animated requests, we load a series of files, replacing the %
	 * with numbers in series.
	 */
	s = image = None;

	/* Working copy of the filename split to before/after the % */
	pref = strdup(name);
	perc = strchr(pref, '%');
	*perc = '\0';
	if(*++perc == '\0') {
		/* Safety belt */
		fprintf(stderr, "Nothing after the percent in '%s'!\n", name);
		free(pref);
		return None;
	}

	/* "foo%.xpm" -> [ "foo1.xpm", "foo2.xpm", ...] */
	for(i = 1 ; ; i++) {
		char path[256];
		Image *r;

		snprintf(path, 256, "%s%d%s", pref, i, perc);

		/*
		 * Load this image, and set ->next so it's explicitly the
		 * [current] tail of the list.
		 */
		r = LoadXpmImage(path, cp);
		if(r == None) {
			break;
		}
		r->next = None;

		/*
		 * If it's the first, it's the head (image) we return, as well as
		 * our current tail marker (s).  Else, append to that tail.
		 */
		if(image == None) {
			s = image = r;
		}
		else {
			s->next = r;
			s = r;
		}
	}
	free(pref);

	/* Set the tail to loop back to the head */
	if(s != None) {
		s->next = image;
	}

	if(image == None) {
		fprintf(stderr, "Cannot open any %s XPM file\n", name);
	}
	return (image);
}



/*
 * Internal backend
 */
static Image *
LoadXpmImage(char *name, ColorPair cp)
{
	char        *fullname;
	Image       *image;
	int         status;
	Colormap    stdcmap = Scr->RootColormaps.cwins[0]->colormap->c;
	XpmAttributes attributes;
	static XpmColorSymbol overrides[] = {
		{"Foreground", NULL, 0},
		{"Background", NULL, 0},
		{"HiShadow", NULL, 0},
		{"LoShadow", NULL, 0}
	};

	fullname = ExpandPixmapPath(name);
	if(! fullname) {
		return (None);
	}

	image = (Image *) malloc(sizeof(Image));
	if(image == None) {
		free(fullname);
		return (None);
	}

	attributes.valuemask  = 0;
	attributes.valuemask |= XpmSize;
	attributes.valuemask |= XpmReturnPixels;
	attributes.valuemask |= XpmColormap;
	attributes.valuemask |= XpmDepth;
	attributes.valuemask |= XpmVisual;
	attributes.valuemask |= XpmCloseness;
	attributes.valuemask |= XpmColorSymbols;

	attributes.numsymbols = 4;
	attributes.colorsymbols = overrides;
	overrides[0].pixel = cp.fore;
	overrides[1].pixel = cp.back;
	overrides[2].pixel = cp.shadd;
	overrides[3].pixel = cp.shadc;


	attributes.colormap  = AlternateCmap ? AlternateCmap : stdcmap;
	attributes.depth     = Scr->d_depth;
	attributes.visual    = Scr->d_visual;
	attributes.closeness = 65535; /* Never fail */
	status = XpmReadFileToPixmap(dpy, Scr->Root, fullname,
	                             &(image->pixmap), &(image->mask), &attributes);
	free(fullname);
	if(status != XpmSuccess) {
		xpmErrorMessage(status, name, fullname);
		free(image);
		return (None);
	}
	image->width  = attributes.width;
	image->height = attributes.height;
	image->next   = None;
	return (image);
}

static void
xpmErrorMessage(int status, char *name, char *fullname)
{
	switch(status) {
		case XpmSuccess:
			/* No error */
			return;

		case XpmColorError:
			if(reportxpmerror)
				fprintf(stderr,
				        "Could not parse or alloc requested color : %s\n",
				        fullname);
			return;

		case XpmOpenFailed:
			if(1 || (reportxpmerror && reportfilenotfound)) {
				fprintf(stderr, "unable to locate XPM file : %s\n", fullname);
			}
			return;

		case XpmFileInvalid:
			fprintf(stderr, "invalid XPM file : %s\n", fullname);
			return;

		case XpmNoMemory:
			if(reportxpmerror) {
				fprintf(stderr, "Not enough memory for XPM file : %s\n", fullname);
			}
			return;

		case XpmColorFailed:
			if(reportxpmerror) {
				fprintf(stderr, "Color not found in : %s\n", fullname);
			}
			return;

		default :
			fprintf(stderr, "Unknown error in : %s\n", fullname);
			return;
	}
}
