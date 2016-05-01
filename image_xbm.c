/*
 * XBM image handling functions
 */

#include "ctwm.h"

#include <X11/X.h>
#include <X11/Xmu/Drawing.h>

#include "screen.h"
#include "types.h"

#include "image.h"
#include "image_xbm.h"


static Image *LoadBitmapImage(char  *name, ColorPair cp);

static Pixmap CreateXLogoPixmap(unsigned int *widthp, unsigned int *heightp);
static Pixmap CreateResizePixmap(unsigned int *widthp, unsigned int *heightp);
static Pixmap CreateQuestionPixmap(unsigned int *widthp, unsigned int *heightp);
static Pixmap CreateMenuPixmap(unsigned int *widthp, unsigned int *heightp);
static Pixmap CreateDotPixmap(unsigned int *widthp, unsigned int *heightp);


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

Pixmap FindBitmap(char *name, unsigned int *widthp,
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
		int i;
		static struct {
			char *name;
			Pixmap(*proc)(unsigned int *wp, unsigned int *hp);
		} pmtab[] = {
			{ TBPM_DOT,         CreateDotPixmap },
			{ TBPM_ICONIFY,     CreateDotPixmap },
			{ TBPM_RESIZE,      CreateResizePixmap },
			{ TBPM_XLOGO,       CreateXLogoPixmap },
			{ TBPM_DELETE,      CreateXLogoPixmap },
			{ TBPM_MENU,        CreateMenuPixmap },
			{ TBPM_QUESTION,    CreateQuestionPixmap },
		};

		for(i = 0; i < (sizeof pmtab) / (sizeof pmtab[0]); i++) {
			if(strcasecmp(pmtab[i].name, name) == 0) {
				return (*pmtab[i].proc)(widthp, heightp);
			}
		}
		fprintf(stderr, "%s:  no such built-in bitmap \"%s\"\n",
		        ProgramName, name);
		return None;
	}

	/*
	 * Generate a full pathname if any special prefix characters (such as ~)
	 * are used.  If the bigname is different from name, bigname will need to
	 * be freed.
	 */
	bigname = ExpandFilename(name);
	if(!bigname) {
		return None;
	}

	/*
	 * look along bitmapFilePath resource same as toolkit clients
	 */
	pm = XmuLocateBitmapFile(ScreenOfDisplay(dpy, Scr->screen), bigname, NULL,
	                         0, (int *)widthp, (int *)heightp, &HotX, &HotY);
	if(pm == None && Scr->IconDirectory && bigname[0] != '/') {
		if(bigname != name) {
			free(bigname);
		}
		/*
		 * Attempt to find icon in old IconDirectory (now obsolete)
		 */
		bigname = (char *) malloc(strlen(name) + strlen(Scr->IconDirectory) + 2);
		if(!bigname) {
			fprintf(stderr,
			        "%s:  unable to allocate memory for \"%s/%s\"\n",
			        ProgramName, Scr->IconDirectory, name);
			return None;
		}
		(void) sprintf(bigname, "%s/%s", Scr->IconDirectory, name);
		if(XReadBitmapFile(dpy, Scr->Root, bigname, widthp, heightp, &pm,
		                   &HotX, &HotY) != BitmapSuccess) {
			pm = None;
		}
	}
	if(bigname != name) {
		free(bigname);
	}
	if((pm == None) && reportfilenotfound) {
		fprintf(stderr, "%s:  unable to find bitmap \"%s\"\n", ProgramName, name);
	}

	return pm;
}

Pixmap GetBitmap(char *name)
{
	return FindBitmap(name, &JunkWidth, &JunkHeight);
}

static Image *LoadBitmapImage(char  *name, ColorPair cp)
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




static Pixmap CreateXLogoPixmap(unsigned int *widthp, unsigned int *heightp)
{
	int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(h < 0) {
		h = 0;
	}

	*widthp = *heightp = (unsigned int) h;
	if(Scr->tbpm.xlogo == None) {
		GC gc, gcBack;

		Scr->tbpm.xlogo = XCreatePixmap(dpy, Scr->Root, h, h, 1);
		gc = XCreateGC(dpy, Scr->tbpm.xlogo, 0L, NULL);
		XSetForeground(dpy, gc, 0);
		XFillRectangle(dpy, Scr->tbpm.xlogo, gc, 0, 0, h, h);
		XSetForeground(dpy, gc, 1);
		gcBack = XCreateGC(dpy, Scr->tbpm.xlogo, 0L, NULL);
		XSetForeground(dpy, gcBack, 0);

		/*
		 * draw the logo large so that it gets as dense as possible; then white
		 * out the edges so that they look crisp
		 */
		XmuDrawLogo(dpy, Scr->tbpm.xlogo, gc, gcBack, -1, -1, h + 2, h + 2);
		XDrawRectangle(dpy, Scr->tbpm.xlogo, gcBack, 0, 0, h - 1, h - 1);

		/*
		 * done drawing
		 */
		XFreeGC(dpy, gc);
		XFreeGC(dpy, gcBack);
	}
	return Scr->tbpm.xlogo;
}


static Pixmap CreateResizePixmap(unsigned int *widthp, unsigned int *heightp)
{
	int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(h < 1) {
		h = 1;
	}

	*widthp = *heightp = (unsigned int) h;
	if(Scr->tbpm.resize == None) {
		XPoint  points[3];
		GC gc;
		int w;
		int lw;

		/*
		 * create the pixmap
		 */
		Scr->tbpm.resize = XCreatePixmap(dpy, Scr->Root, h, h, 1);
		gc = XCreateGC(dpy, Scr->tbpm.resize, 0L, NULL);
		XSetForeground(dpy, gc, 0);
		XFillRectangle(dpy, Scr->tbpm.resize, gc, 0, 0, h, h);
		XSetForeground(dpy, gc, 1);
		lw = h / 16;
		if(lw == 1) {
			lw = 0;
		}
		XSetLineAttributes(dpy, gc, lw, LineSolid, CapButt, JoinMiter);

		/*
		 * draw the resize button,
		 */
		w = (h * 2) / 3;
		points[0].x = w;
		points[0].y = 0;
		points[1].x = w;
		points[1].y = w;
		points[2].x = 0;
		points[2].y = w;
		XDrawLines(dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);
		w = w / 2;
		points[0].x = w;
		points[0].y = 0;
		points[1].x = w;
		points[1].y = w;
		points[2].x = 0;
		points[2].y = w;
		XDrawLines(dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);

		/*
		 * done drawing
		 */
		XFreeGC(dpy, gc);
	}
	return Scr->tbpm.resize;
}


#define questionmark_width 8
#define questionmark_height 8
static char questionmark_bits[] = {
	0x38, 0x7c, 0x64, 0x30, 0x18, 0x00, 0x18, 0x18
};

static Pixmap CreateQuestionPixmap(unsigned int *widthp,
                                   unsigned int *heightp)
{
	*widthp = questionmark_width;
	*heightp = questionmark_height;
	if(Scr->tbpm.question == None) {
		Scr->tbpm.question = XCreateBitmapFromData(dpy, Scr->Root,
		                     questionmark_bits,
		                     questionmark_width,
		                     questionmark_height);
	}
	/*
	 * this must succeed or else we are in deep trouble elsewhere
	 */
	return Scr->tbpm.question;
}


static Pixmap CreateMenuPixmap(unsigned int *widthp, unsigned int *heightp)
{
	return (CreateMenuIcon(Scr->TBInfo.width - Scr->TBInfo.border * 2, widthp,
	                       heightp));
}

static Pixmap CreateDotPixmap(unsigned int *widthp, unsigned int *heightp)
{
	int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;

	h = h * 3 / 4;
	if(h < 1) {
		h = 1;
	}
	if(!(h & 1)) {
		h--;
	}
	*widthp = *heightp = (unsigned int) h;
	if(Scr->tbpm.delete == None) {
		GC  gc;
		Pixmap pix;

		pix = Scr->tbpm.delete = XCreatePixmap(dpy, Scr->Root, h, h, 1);
		gc = XCreateGC(dpy, pix, 0L, NULL);
		XSetLineAttributes(dpy, gc, h, LineSolid, CapRound, JoinRound);
		XSetForeground(dpy, gc, 0L);
		XFillRectangle(dpy, pix, gc, 0, 0, h, h);
		XSetForeground(dpy, gc, 1L);
		XDrawLine(dpy, pix, gc, h / 2, h / 2, h / 2, h / 2);
		XFreeGC(dpy, gc);
	}
	return Scr->tbpm.delete;
}
