/*
 * Builtin bitmap image generation/lookup.
 */

#include "ctwm.h"

#include <X11/X.h>
#include <X11/Xmu/Drawing.h>

#include "screen.h"
#include "types.h"
#include "util.h"

#include "image.h"
#include "image_bitmap_builtin.h"


/*
 * Firstly, the plain built-in titlebar symbols.  These are the ones
 * specified with names like ":resize".  For various reasons, these
 * currently return Pixmap's, unlike most of our other builtins that
 * generate Image's.  Possible cleanup candidate.
 */
#define DEF_BI_PPM(nm) Pixmap nm(unsigned int *widthp, unsigned int *heightp)
static DEF_BI_PPM(CreateXLogoPixmap);
static DEF_BI_PPM(CreateResizePixmap);
static DEF_BI_PPM(CreateQuestionPixmap);
static DEF_BI_PPM(CreateMenuPixmap);
static DEF_BI_PPM(CreateDotPixmap);



/*
 * Look up and return a ":something" (not a ":xpm:something").
 *
 * Names of the form :name refer to hardcoded images that are scaled to
 * look nice in title buttons.  Eventually, it would be nice to put in a
 * menu symbol as well....
 */
Pixmap get_builtin_plain_pixmap(char *name, unsigned int *widthp,
                  unsigned int *heightp)
{
	int i;
	static struct {
		char *name;
		DEF_BI_PPM((*proc));
	} pmtab[] = {
		/* Lookup table for our various default pixmaps */
		{ TBPM_DOT,         CreateDotPixmap },
		{ TBPM_ICONIFY,     CreateDotPixmap },
		{ TBPM_RESIZE,      CreateResizePixmap },
		{ TBPM_XLOGO,       CreateXLogoPixmap },
		{ TBPM_DELETE,      CreateXLogoPixmap },
		{ TBPM_MENU,        CreateMenuPixmap },
		{ TBPM_QUESTION,    CreateQuestionPixmap },
	};

	/* Seatbelts */
	if(!name || name[0] != ':') {
		return None;
	}
	if(!widthp || !heightp) {
		return None;
	}


	/* Find it */
	for(i = 0; i < (sizeof pmtab) / (sizeof pmtab[0]); i++) {
		if(strcasecmp(pmtab[i].name, name) == 0) {
			return (*pmtab[i].proc)(widthp, heightp);
		}
	}

	/* Didn't find it */
	fprintf(stderr, "%s:  no such built-in bitmap \"%s\"\n",
		    ProgramName, name);
	return None;
}


/*
 * Individual generators for those plain pixmaps
 */
DEF_BI_PPM(CreateXLogoPixmap)
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


DEF_BI_PPM(CreateResizePixmap)
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

DEF_BI_PPM(CreateQuestionPixmap)
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
#undef questionmark_height
#undef questionmark_width


DEF_BI_PPM(CreateMenuPixmap)
{
	return (CreateMenuIcon(Scr->TBInfo.width - Scr->TBInfo.border * 2, widthp,
	                       heightp));
}

DEF_BI_PPM(CreateDotPixmap)
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

#undef DEF_BI_PPM
