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
			Pixmap pm = (*pmtab[i].proc)(widthp, heightp);
			if(pm == None) {
				fprintf(stderr, "%s:  unable to build bitmap \"%s\"\n",
				        ProgramName, name);
				return None;
			}
			return pm;
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



/*
 * Next, the "3D/scalable" builtins.  These are the ones specified with
 * names like ":xpm:resize".  I'm not entirely clear on how these differ
 * from ":resize"; they both vary by UseThreeDTitles and look the same.
 * But, whatever.
 *
 * These yield [ctwm struct] Image's rather than [X11 type] Pixmap's.
 */
#define DEF_BI_SPM(nm) Image *nm(ColorPair cp)
static DEF_BI_SPM(Create3DMenuImage);
static DEF_BI_SPM(Create3DDotImage);
static DEF_BI_SPM(Create3DResizeImage);
static DEF_BI_SPM(Create3DZoomImage);
static DEF_BI_SPM(Create3DBarImage);
static DEF_BI_SPM(Create3DVertBarImage);
static DEF_BI_SPM(Create3DCrossImage);
static DEF_BI_SPM(Create3DIconifyImage);
static DEF_BI_SPM(Create3DSunkenResizeImage);
static DEF_BI_SPM(Create3DBoxImage);


/*
 * Main lookup
 *
 * This is where we find ":xpm:something".  Note that these are _not_
 * XPM's, and have no relation to the configurable XPM support, which we
 * get with images specified as "xpm:something" (no leading colon).
 * That's not confusing at all.
 */
Image *get_builtin_scalable_pixmap(char *name, ColorPair cp)
{
	int    i;
	static struct {
		char *name;
		DEF_BI_SPM((*proc));
	} pmtab[] = {
		/* Lookup for ":xpm:" pixmaps */
		{ TBPM_3DDOT,       Create3DDotImage },
		{ TBPM_3DRESIZE,    Create3DResizeImage },
		{ TBPM_3DMENU,      Create3DMenuImage },
		{ TBPM_3DZOOM,      Create3DZoomImage },
		{ TBPM_3DBAR,       Create3DBarImage },
		{ TBPM_3DVBAR,      Create3DVertBarImage },
		{ TBPM_3DCROSS,     Create3DCrossImage },
		{ TBPM_3DICONIFY,   Create3DIconifyImage },
		{ TBPM_3DBOX,       Create3DBoxImage },
		{ TBPM_3DSUNKEN_RESIZE, Create3DSunkenResizeImage },
	};

	/* Seatbelts */
	if(!name || (strncmp(name, ":xpm:", 5) != 0)) {
		return None;
	}

	for(i = 0; i < (sizeof pmtab) / (sizeof pmtab[0]); i++) {
		if(strcasecmp(pmtab[i].name, name) == 0) {
			Image *image = (*pmtab[i].proc)(cp);
			if(image == None) {
				fprintf(stderr, "%s:  unable to build pixmap \"%s\"\n",
				        ProgramName, name);
				return (None);
			}
			return image;
		}
	}

	fprintf(stderr, "%s:  no such built-in pixmap \"%s\"\n", ProgramName, name);
	return (None);
}



#define LEVITTE_TEST
static DEF_BI_SPM(Create3DCrossImage)
{
	Image *image;
	int        h;
	int    point;
	int midpoint;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}
	point = 4;
	midpoint = h / 2;

	image = (Image *)malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);

#ifdef LEVITTE_TEST
	FB(cp.shadc, cp.shadd);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 1, point - 1, point - 1,
	          point + 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 1, point, point,
	          point + 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point - 1, point + 1, midpoint - 2,
	          midpoint);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, midpoint, midpoint + 2,
	          h - point - 3, h - point - 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, point + 1, h - point - 3,
	          h - point - 2);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point - 1, h - point - 2,
	          midpoint - 2, midpoint);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, midpoint, midpoint - 2,
	          h - point - 2, point - 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, h - point - 2,
	          h - point - 2, point);
#endif

	FB(cp.shadd, cp.shadc);
#ifdef LEVITTE_TEST
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 2, point + 1,
	          h - point - 1, h - point - 2);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 2, point, midpoint,
	          midpoint - 2);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, midpoint + 2, midpoint, h - point,
	          h - point - 2);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, h - point, h - point - 2,
	          h - point - 2, h - point);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, h - point - 1, h - point - 2,
	          h - point - 2, h - point - 1);
#else
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, point, h - point - 1,
	          h - point - 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point - 1, point, h - point - 1,
	          h - point);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, point - 1, h - point,
	          h - point - 1);
#endif

#ifdef LEVITTE_TEST
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, h - point - 1, point,
	          h - point - 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, h - point - 1, point,
	          h - point - 1, point);
#else
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, h - point - 1,
	          h - point - 1, point);
#endif
#ifdef LEVITTE_TEST
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 1, h - point - 1,
	          h - point - 1, point + 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point + 1, h - point, midpoint,
	          midpoint + 2);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, midpoint + 2, midpoint, h - point,
	          point + 1);
#else
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point - 1, h - point - 1,
	          h - point - 1, point - 1);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, h - point, h - point,
	          point);
#endif

	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;

	return (image);
}
#undef LEVITTE_TEST

static DEF_BI_SPM(Create3DIconifyImage)
{
	Image *image;
	int     h;
	int point;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}
	point = ((h / 2 - 2) * 2 + 1) / 3;

	image = (Image *)malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	FB(cp.shadd, cp.shadc);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, point, h / 2, h - point);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, point, point, h - point, point);

	FB(cp.shadc, cp.shadd);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, h - point, point, h / 2 + 1,
	          h - point);
	XDrawLine(dpy, image->pixmap, Scr->NormalGC, h - point - 1, point + 1,
	          h / 2 + 1, h - point - 1);

	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;

	return (image);
}

static DEF_BI_SPM(Create3DSunkenResizeImage)
{
	int     h;
	Image *image;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *)malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap, 3, 3, h - 6, h - 6, 1, cp, on, True, False);
	Draw3DBorder(image->pixmap, 3, ((h - 6) / 3) + 3, ((h - 6) * 2 / 3) + 1,
	             ((h - 6) * 2 / 3) + 1, 1, cp, on, True, False);
	Draw3DBorder(image->pixmap, 3, ((h - 6) * 2 / 3) + 3, ((h - 6) / 3) + 1,
	             ((h - 6) / 3) + 1, 1, cp, on, True, False);

	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;

	return (image);
}

static DEF_BI_SPM(Create3DBoxImage)
{
	int     h;
	Image   *image;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap, (h / 2) - 4, (h / 2) - 4, 9, 9, 1, cp,
	             off, True, False);

	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;

	return (image);
}

static DEF_BI_SPM(Create3DDotImage)
{
	Image *image;
	int   h;
	static int idepth = 2;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap, (h / 2) - idepth,
	             (h / 2) - idepth,
	             2 * idepth + 1,
	             2 * idepth + 1,
	             idepth, cp, off, True, False);
	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

static DEF_BI_SPM(Create3DBarImage)
{
	Image *image;
	int   h;
	static int idepth = 2;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap,
	             Scr->TitleButtonShadowDepth + 2,
	             (h / 2) - idepth,
	             h - 2 * (Scr->TitleButtonShadowDepth + 2),
	             2 * idepth + 1,
	             idepth, cp, off, True, False);
	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

static DEF_BI_SPM(Create3DVertBarImage)
{
	Image *image;
	int   h;
	static int idepth = 2;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap,
	             (h / 2) - idepth,
	             Scr->TitleButtonShadowDepth + 2,
	             2 * idepth + 1,
	             h - 2 * (Scr->TitleButtonShadowDepth + 2),
	             idepth, cp, off, True, False);
	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

static DEF_BI_SPM(Create3DMenuImage)
{
	Image *image;
	int   h, i;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	for(i = 4; i < h - 7; i += 5) {
		Draw3DBorder(image->pixmap, 4, i, h - 8, 4, 2, cp, off, True, False);
	}
	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

static DEF_BI_SPM(Create3DResizeImage)
{
	Image *image;
	int   h;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap, 0, h / 4, ((3 * h) / 4) + 1, ((3 * h) / 4) + 1, 2,
	             cp, off, True, False);
	Draw3DBorder(image->pixmap, 0, h / 2, (h / 2) + 1, (h / 2) + 1, 2, cp, off,
	             True, False);
	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

static DEF_BI_SPM(Create3DZoomImage)
{
	Image *image;
	int         h;
	static int idepth = 2;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = (Image *) malloc(sizeof(Image));
	if(! image) {
		return (None);
	}
	image->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	if(image->pixmap == None) {
		free(image);
		return (None);
	}

	Draw3DBorder(image->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off,
	             True, False);
	Draw3DBorder(image->pixmap, Scr->TitleButtonShadowDepth + 2,
	             Scr->TitleButtonShadowDepth + 2,
	             h - 2 * (Scr->TitleButtonShadowDepth + 2),
	             h - 2 * (Scr->TitleButtonShadowDepth + 2),
	             idepth, cp, off, True, False);

	image->mask   = None;
	image->width  = h;
	image->height = h;
	image->next   = None;
	return (image);
}

#undef DEF_BI_SPM
