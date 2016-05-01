/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/*
 *  [ ctwm ]
 *
 *  Copyright 1992 Claude Lecommandeur.
 *
 * Permission to use, copy, modify  and distribute this software  [ctwm] and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above  copyright notice appear  in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of  Claude Lecommandeur not be used in adverti-
 * sing or  publicity  pertaining to  distribution of  the software  without
 * specific, written prior permission. Claude Lecommandeur make no represen-
 * tations  about the suitability  of this software  for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Claude Lecommandeur DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL  Claude Lecommandeur  BE LIABLE FOR ANY SPECIAL,  INDIRECT OR
 * CONSEQUENTIAL  DAMAGES OR ANY  DAMAGES WHATSOEVER  RESULTING FROM LOSS OF
 * USE, DATA  OR PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR
 * OTHER  TORTIOUS ACTION,  ARISING OUT OF OR IN  CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Claude Lecommandeur [ lecom@sic.epfl.ch ][ April 1992 ]
 */


/***********************************************************************
 *
 * $XConsortium: util.c,v 1.47 91/07/14 13:40:37 rws Exp $
 *
 * utility routines for twm
 *
 * 28-Oct-87 Thomas E. LaStrange        File created
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 * Changed behavior of DontMoveOff/MoveOffResistance to allow
 * moving a window off screen less than #MoveOffResistance pixels.
 * New code will no longer "snap" windows to #MoveOffResistance
 * pixels off screen and instead movements will just be stopped and
 * then resume once movement of #MoveOffResistance have been attempted.
 *
 * 15-December-02 Bjorn Knutsson
 *
 ***********************************************************************/

#include "ctwm.h"

#define LEVITTE_TEST

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/XWDFile.h>


#include <signal.h>
#include <sys/time.h>


#include "ctwm_atoms.h"
#include "util.h"
#include "events.h"
#include "add_window.h"
#include "gram.tab.h"
#include "screen.h"
#include "icons.h"
#include "cursor.h"
#include "resize.h"


#include "image.h"
#include "image_bitmap.h"
#include "image_bitmap_builtin.h"
#ifdef JPEG
#include "image_jpeg.h"
#endif
#if defined (XPM)
#include "image_xpm.h"
#endif


#define MAXANIMATIONSPEED 20

static Image *LoadXwdImage(char  *filename, ColorPair cp);
static Image *GetXwdImage(char  *name, ColorPair cp);
static Image  *Create3DMenuImage(ColorPair cp);
static Image  *Create3DDotImage(ColorPair cp);
static Image  *Create3DResizeImage(ColorPair cp);
static Image  *Create3DZoomImage(ColorPair cp);
static Image  *Create3DBarImage(ColorPair cp);
static Image  *Create3DVertBarImage(ColorPair cp);
static Image  *Create3DResizeAnimation(Bool in, Bool left, Bool top,
                                       ColorPair cp);
static Image  *Create3DCrossImage(ColorPair cp);
static Image  *Create3DIconifyImage(ColorPair cp);
static Image  *Create3DSunkenResizeImage(ColorPair cp);
static Image  *Create3DBoxImage(ColorPair cp);
static void PaintAllDecoration(void);
static void PaintTitleButtons(TwmWindow *tmp_win);

static void swapshort(char *bp, unsigned n);
static void swaplong(char *bp, unsigned n);

/* XXX move to an 'image.c' when we grow it */
bool reportfilenotfound = false;
Colormap AlternateCmap = None;

int  HotX, HotY;

int  Animating        = 0;
int  AnimationSpeed   = 0;
Bool AnimationActive  = False;
Bool MaybeAnimate     = True;
struct timeval AnimateTimeout;

/***********************************************************************
 *
 *  Procedure:
 *      MoveOutline - move a window outline
 *
 *  Inputs:
 *      root        - the window we are outlining
 *      x           - upper left x coordinate
 *      y           - upper left y coordinate
 *      width       - the width of the rectangle
 *      height      - the height of the rectangle
 *      bw          - the border width of the frame
 *      th          - title height
 *
 ***********************************************************************
 */

/* ARGSUSED */
void MoveOutline(Window root,
                 int x, int y, int width, int height, int bw, int th)
{
	static int  lastx = 0;
	static int  lasty = 0;
	static int  lastWidth = 0;
	static int  lastHeight = 0;
	static int  lastBW = 0;
	static int  lastTH = 0;
	int         xl, xr, yt, yb, xinnerl, xinnerr, yinnert, yinnerb;
	int         xthird, ythird;
	XSegment    outline[18];
	XSegment   *r;

	if(x == lastx && y == lasty && width == lastWidth && height == lastHeight
	                && lastBW == bw && th == lastTH) {
		return;
	}

	r = outline;

#define DRAWIT() \
    if (lastWidth || lastHeight)                        \
    {                                                   \
        xl = lastx;                                     \
        xr = lastx + lastWidth - 1;                     \
        yt = lasty;                                     \
        yb = lasty + lastHeight - 1;                    \
        xinnerl = xl + lastBW;                          \
        xinnerr = xr - lastBW;                          \
        yinnert = yt + lastTH + lastBW;                 \
        yinnerb = yb - lastBW;                          \
        xthird = (xinnerr - xinnerl) / 3;               \
        ythird = (yinnerb - yinnert) / 3;               \
                                                        \
        r->x1 = xl;                                     \
        r->y1 = yt;                                     \
        r->x2 = xr;                                     \
        r->y2 = yt;                                     \
        r++;                                            \
                                                        \
        r->x1 = xl;                                     \
        r->y1 = yb;                                     \
        r->x2 = xr;                                     \
        r->y2 = yb;                                     \
        r++;                                            \
                                                        \
        r->x1 = xl;                                     \
        r->y1 = yt;                                     \
        r->x2 = xl;                                     \
        r->y2 = yb;                                     \
        r++;                                            \
                                                        \
        r->x1 = xr;                                     \
        r->y1 = yt;                                     \
        r->x2 = xr;                                     \
        r->y2 = yb;                                     \
        r++;                                            \
                                                        \
        r->x1 = xinnerl + xthird;                       \
        r->y1 = yinnert;                                \
        r->x2 = r->x1;                                  \
        r->y2 = yinnerb;                                \
        r++;                                            \
                                                        \
        r->x1 = xinnerl + (2 * xthird);                 \
        r->y1 = yinnert;                                \
        r->x2 = r->x1;                                  \
        r->y2 = yinnerb;                                \
        r++;                                            \
                                                        \
        r->x1 = xinnerl;                                \
        r->y1 = yinnert + ythird;                       \
        r->x2 = xinnerr;                                \
        r->y2 = r->y1;                                  \
        r++;                                            \
                                                        \
        r->x1 = xinnerl;                                \
        r->y1 = yinnert + (2 * ythird);                 \
        r->x2 = xinnerr;                                \
        r->y2 = r->y1;                                  \
        r++;                                            \
                                                        \
        if (lastTH != 0) {                              \
            r->x1 = xl;                                 \
            r->y1 = yt + lastTH;                        \
            r->x2 = xr;                                 \
            r->y2 = r->y1;                              \
            r++;                                        \
        }                                               \
    }

	/* undraw the old one, if any */
	DRAWIT();

	lastx = x;
	lasty = y;
	lastWidth = width;
	lastHeight = height;
	lastBW = bw;
	lastTH = th;

	/* draw the new one, if any */
	DRAWIT();

#undef DRAWIT


	if(r != outline) {
		XDrawSegments(dpy, root, Scr->DrawGC, outline, r - outline);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      Zoom - zoom in or out of an icon
 *
 *  Inputs:
 *      wf      - window to zoom from
 *      wt      - window to zoom to
 *
 ***********************************************************************
 */

void Zoom(Window wf, Window wt)
{
	int fx, fy, tx, ty;                 /* from, to */
	unsigned int fw, fh, tw, th;        /* from, to */
	long dx, dy, dw, dh;
	long z;
	int j;

	if((Scr->IconifyStyle != ICONIFY_NORMAL) || !Scr->DoZoom
	                || Scr->ZoomCount < 1) {
		return;
	}

	if(wf == None || wt == None) {
		return;
	}

	XGetGeometry(dpy, wf, &JunkRoot, &fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);
	XGetGeometry(dpy, wt, &JunkRoot, &tx, &ty, &tw, &th, &JunkBW, &JunkDepth);

	dx = (long) tx - (long) fx; /* going from -> to */
	dy = (long) ty - (long) fy; /* going from -> to */
	dw = (long) tw - (long) fw; /* going from -> to */
	dh = (long) th - (long) fh; /* going from -> to */
	z = (long)(Scr->ZoomCount + 1);

	for(j = 0; j < 2; j++) {
		long i;

		XDrawRectangle(dpy, Scr->Root, Scr->DrawGC, fx, fy, fw, fh);
		for(i = 1; i < z; i++) {
			int x = fx + (int)((dx * i) / z);
			int y = fy + (int)((dy * i) / z);
			unsigned width = (unsigned)(((long) fw) + (dw * i) / z);
			unsigned height = (unsigned)(((long) fh) + (dh * i) / z);

			XDrawRectangle(dpy, Scr->Root, Scr->DrawGC,
			               x, y, width, height);
		}
		XDrawRectangle(dpy, Scr->Root, Scr->DrawGC, tx, ty, tw, th);
	}
}


char *ExpandFilePath(char *path)
{
	char *ret, *colon, *p;
	int  len;

	len = 0;
	p   = path;
	while((colon = strchr(p, ':'))) {
		len += colon - p + 1;
		if(*p == '~') {
			len += HomeLen - 1;
		}
		p = colon + 1;
	}
	if(*p == '~') {
		len += HomeLen - 1;
	}
	len += strlen(p);
	ret = (char *) malloc(len + 1);
	*ret = 0;

	p   = path;
	while((colon = strchr(p, ':'))) {
		*colon = '\0';
		if(*p == '~') {
			strcat(ret, Home);
			strcat(ret, p + 1);
		}
		else {
			strcat(ret, p);
		}
		*colon = ':';
		strcat(ret, ":");
		p = colon + 1;
	}
	if(*p == '~') {
		strcat(ret, Home);
		strcat(ret, p + 1);
	}
	else {
		strcat(ret, p);
	}
	return ret;
}

/***********************************************************************
 *
 *  Procedure:
 *      ExpandFilename - expand the tilde character to HOME
 *              if it is the first character of the filename
 *
 *  Returned Value:
 *      a pointer to the new name
 *
 *  Inputs:
 *      name    - the filename to expand
 *
 ***********************************************************************
 */

char *ExpandFilename(char *name)
{
	char *newname;

	if(name[0] != '~') {
		return name;
	}

	newname = (char *) malloc(HomeLen + strlen(name) + 2);
	if(!newname) {
		fprintf(stderr,
		        "%s:  unable to allocate %lu bytes to expand filename %s/%s\n",
		        ProgramName, (unsigned long) HomeLen + strlen(name) + 2,
		        Home, &name[1]);
	}
	else {
		(void) sprintf(newname, "%s/%s", Home, &name[1]);
	}

	return newname;
}

char *ExpandPixmapPath(char *name)
{
	char    *ret, *colon;

	ret = NULL;
	if(name[0] == '~') {
		ret = (char *) malloc(HomeLen + strlen(name) + 2);
		sprintf(ret, "%s/%s", Home, &name[1]);
	}
	else if(name[0] == '/') {
		ret = (char *) malloc(strlen(name) + 1);
		strcpy(ret, name);
	}
	else if(Scr->PixmapDirectory) {
		char *p = Scr->PixmapDirectory;
		while((colon = strchr(p, ':'))) {
			*colon = '\0';
			ret = (char *) malloc(strlen(p) + strlen(name) + 2);
			sprintf(ret, "%s/%s", p, name);
			*colon = ':';
			if(!access(ret, R_OK)) {
				return (ret);
			}
			free(ret);
			p = colon + 1;
		}
		ret = (char *) malloc(strlen(p) + strlen(name) + 2);
		sprintf(ret, "%s/%s", p, name);
	}
	return (ret);
}

/***********************************************************************
 *
 *  Procedure:
 *      GetUnknownIcon - read in the bitmap file for the unknown icon
 *
 *  Inputs:
 *      name - the filename to read
 *
 ***********************************************************************
 */

void GetUnknownIcon(char *name)
{
	Scr->UnknownImage = GetImage(name, Scr->IconC);
}


void MaskScreen(char *file)
{
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	XEvent event;
	Cursor waitcursor;
	int x, y;
	ColorPair WelcomeCp;
	XColor black;

	NewFontCursor(&waitcursor, "watch");

	valuemask = (CWBackingStore | CWSaveUnder | CWBackPixel |
	             CWOverrideRedirect | CWEventMask | CWCursor);
	attributes.backing_store     = NotUseful;
	attributes.save_under        = False;
	attributes.override_redirect = True;
	attributes.event_mask        = ExposureMask;
	attributes.cursor            = waitcursor;
	attributes.background_pixel  = Scr->Black;
	Scr->WindowMask = XCreateWindow(dpy, Scr->Root, 0, 0,
	                                (unsigned int) Scr->rootw,
	                                (unsigned int) Scr->rooth,
	                                (unsigned int) 0,
	                                CopyFromParent, (unsigned int) CopyFromParent,
	                                (Visual *) CopyFromParent, valuemask,
	                                &attributes);
	XMapWindow(dpy, Scr->WindowMask);
	XMaskEvent(dpy, ExposureMask, &event);

	if(Scr->Monochrome != COLOR) {
		return;
	}

	WelcomeCp.fore = Scr->Black;
	WelcomeCp.back = Scr->White;
	Scr->WelcomeCmap  = XCreateColormap(dpy, Scr->WindowMask, Scr->d_visual,
	                                    AllocNone);
	if(! Scr->WelcomeCmap) {
		return;
	}
	XSetWindowColormap(dpy, Scr->WindowMask, Scr->WelcomeCmap);
	black.red   = 0;
	black.green = 0;
	black.blue  = 0;
	XAllocColor(dpy, Scr->WelcomeCmap, &black);

	AlternateCmap = Scr->WelcomeCmap;
	if(! file) {
		Scr->WelcomeImage  = GetImage("xwd:welcome.xwd", WelcomeCp);
#ifdef XPM
		if(Scr->WelcomeImage == None) {
			Scr->WelcomeImage  = GetImage("xpm:welcome.xpm", WelcomeCp);
		}
#endif
	}
	else {
		Scr->WelcomeImage  = GetImage(file, WelcomeCp);
	}
	AlternateCmap = None;
	if(Scr->WelcomeImage == None) {
		return;
	}

	if(CLarg.is_captive) {
		XSetWindowColormap(dpy, Scr->WindowMask, Scr->WelcomeCmap);
		XSetWMColormapWindows(dpy, Scr->Root, &(Scr->WindowMask), 1);
	}
	else {
		XInstallColormap(dpy, Scr->WelcomeCmap);
	}

	Scr->WelcomeGC = XCreateGC(dpy, Scr->WindowMask, 0, NULL);
	x = (Scr->rootw  -  Scr->WelcomeImage->width) / 2;
	y = (Scr->rooth - Scr->WelcomeImage->height) / 2;

	XSetWindowBackground(dpy, Scr->WindowMask, black.pixel);
	XClearWindow(dpy, Scr->WindowMask);
	XCopyArea(dpy, Scr->WelcomeImage->pixmap, Scr->WindowMask, Scr->WelcomeGC, 0, 0,
	          Scr->WelcomeImage->width, Scr->WelcomeImage->height, x, y);
}

void UnmaskScreen(void)
{
	struct timeval      timeout;
	Colormap            stdcmap = Scr->RootColormaps.cwins[0]->colormap->c;
	Colormap            cmap;
	XColor              colors [256], stdcolors [256];
	int                 i, j, usec;

	usec = 6000;
	timeout.tv_usec = usec % (unsigned long) 1000000;
	timeout.tv_sec  = usec / (unsigned long) 1000000;

	if(Scr->WelcomeImage) {
		Pixel pixels [256];

		cmap = Scr->WelcomeCmap;
		for(i = 0; i < 256; i++) {
			pixels [i] = i;
			colors [i].pixel = i;
		}
		XQueryColors(dpy, cmap, colors, 256);
		XFreeColors(dpy, cmap, pixels, 256, 0L);
		XFreeColors(dpy, cmap, pixels, 256, 0L);   /* Ah Ah */

		for(i = 0; i < 256; i++) {
			colors [i].pixel = i;
			colors [i].flags = DoRed | DoGreen | DoBlue;
			stdcolors [i].red   = colors [i].red;
			stdcolors [i].green = colors [i].green;
			stdcolors [i].blue  = colors [i].blue;
		}
		for(i = 0; i < 128; i++) {
			for(j = 0; j < 256; j++) {
				colors [j].red   = stdcolors [j].red   * ((127.0 - i) / 128.0);
				colors [j].green = stdcolors [j].green * ((127.0 - i) / 128.0);
				colors [j].blue  = stdcolors [j].blue  * ((127.0 - i) / 128.0);
			}
			XStoreColors(dpy, cmap, colors, 256);
			select(0, NULL, NULL, NULL, &timeout);
		}
		XFreeColors(dpy, cmap, pixels, 256, 0L);
		XFreeGC(dpy, Scr->WelcomeGC);
		FreeImage(Scr->WelcomeImage);
	}
	if(Scr->Monochrome != COLOR) {
		goto fin;
	}

	cmap = XCreateColormap(dpy, Scr->Root, Scr->d_visual, AllocNone);
	if(! cmap) {
		goto fin;
	}
	for(i = 0; i < 256; i++) {
		colors [i].pixel = i;
		colors [i].red   = 0;
		colors [i].green = 0;
		colors [i].blue  = 0;
		colors [i].flags = DoRed | DoGreen | DoBlue;
	}
	XStoreColors(dpy, cmap, colors, 256);

	if(CLarg.is_captive) {
		XSetWindowColormap(dpy, Scr->Root, cmap);
	}
	else {
		XInstallColormap(dpy, cmap);
	}

	XUnmapWindow(dpy, Scr->WindowMask);
	XClearWindow(dpy, Scr->Root);
	XSync(dpy, 0);
	PaintAllDecoration();

	for(i = 0; i < 256; i++) {
		stdcolors [i].pixel = i;
	}
	XQueryColors(dpy, stdcmap, stdcolors, 256);
	for(i = 0; i < 128; i++) {
		for(j = 0; j < 256; j++) {
			colors [j].pixel = j;
			colors [j].red   = stdcolors [j].red   * (i / 127.0);
			colors [j].green = stdcolors [j].green * (i / 127.0);
			colors [j].blue  = stdcolors [j].blue  * (i / 127.0);
			colors [j].flags = DoRed | DoGreen | DoBlue;
		}
		XStoreColors(dpy, cmap, colors, 256);
		select(0, NULL, NULL, NULL, &timeout);
	}

	if(CLarg.is_captive) {
		XSetWindowColormap(dpy, Scr->Root, stdcmap);
	}
	else {
		XInstallColormap(dpy, stdcmap);
	}

	XFreeColormap(dpy, cmap);

fin:
	if(Scr->WelcomeCmap) {
		XFreeColormap(dpy, Scr->WelcomeCmap);
	}
	XDestroyWindow(dpy, Scr->WindowMask);
	Scr->WindowMask = (Window) 0;
}



void TryToAnimate(void)
{
	struct timeval  tp;
	static unsigned long lastsec;
	static long lastusec;
	unsigned long gap;

	if(Animating > 1) {
		return;        /* rate limiting */
	}

	gettimeofday(&tp, NULL);
	gap = ((tp.tv_sec - lastsec) * 1000000) + (tp.tv_usec - lastusec);
	if(tracefile) {
		fprintf(tracefile, "Time = %lu, %ld, %ld, %ld, %lu\n", lastsec,
		        lastusec, (long)tp.tv_sec, (long)tp.tv_usec, gap);
		fflush(tracefile);
	}
	gap *= AnimationSpeed;
	if(gap < 1000000) {
		return;
	}
	if(tracefile) {
		fprintf(tracefile, "Animate\n");
		fflush(tracefile);
	}
	Animate();
	lastsec  = tp.tv_sec;
	lastusec = tp.tv_usec;
}

void StartAnimation(void)
{

	if(AnimationSpeed > MAXANIMATIONSPEED) {
		AnimationSpeed = MAXANIMATIONSPEED;
	}
	if(AnimationSpeed <= 0) {
		AnimationSpeed = 0;
	}
	if(AnimationActive) {
		return;
	}
	switch(AnimationSpeed) {
		case 0 :
			return;
		case 1 :
			AnimateTimeout.tv_sec  = 1;
			AnimateTimeout.tv_usec = 0;
			break;
		default :
			AnimateTimeout.tv_sec  = 0;
			AnimateTimeout.tv_usec = 1000000 / AnimationSpeed;
	}
	AnimationActive = True;
}

void StopAnimation(void)
{
	AnimationActive = False;
}

void SetAnimationSpeed(int speed)
{
	AnimationSpeed = speed;
	if(AnimationSpeed > MAXANIMATIONSPEED) {
		AnimationSpeed = MAXANIMATIONSPEED;
	}
}

void ModifyAnimationSpeed(int incr)
{

	if((AnimationSpeed + incr) < 0) {
		return;
	}
	if((AnimationSpeed + incr) == 0) {
		if(AnimationActive) {
			StopAnimation();
		}
		AnimationSpeed = 0;
		return;
	}
	AnimationSpeed += incr;
	if(AnimationSpeed > MAXANIMATIONSPEED) {
		AnimationSpeed = MAXANIMATIONSPEED;
	}

	if(AnimationSpeed == 1) {
		AnimateTimeout.tv_sec  = 1;
		AnimateTimeout.tv_usec = 0;
	}
	else {
		AnimateTimeout.tv_sec  = 0;
		AnimateTimeout.tv_usec = 1000000 / AnimationSpeed;
	}
	AnimationActive = True;
}


void Animate(void)
{
	TwmWindow   *t;
	int         scrnum;
	ScreenInfo  *scr;
	int         i;
	TBWindow    *tbw;
	int         nb;

	if(AnimationSpeed == 0) {
		return;
	}
	if(Animating > 1) {
		return;        /* rate limiting */
	}

	/* Impossible? */
	if(NumScreens < 1) {
		return;
	}

	MaybeAnimate = False;
	scr = NULL;
	for(scrnum = 0; scrnum < NumScreens; scrnum++) {
		if((scr = ScreenList [scrnum]) == NULL) {
			continue;
		}

		for(t = scr->FirstWindow; t != NULL; t = t->next) {
			if(! visible(t)) {
				continue;
			}
			if(t->icon_on && t->icon && t->icon->bm_w && t->icon->image &&
			                t->icon->image->next) {
				AnimateIcons(scr, t->icon);
				MaybeAnimate = True;
			}
			else if(t->mapped && t->titlebuttons) {
				nb = scr->TBInfo.nleft + scr->TBInfo.nright;
				for(i = 0, tbw = t->titlebuttons; i < nb; i++, tbw++) {
					if(tbw->image && tbw->image->next) {
						AnimateButton(tbw);
						MaybeAnimate = True;
					}
				}
			}
		}
		if(scr->Focus) {
			t = scr->Focus;
			if(t->mapped && t->titlehighlight && t->title_height &&
			                t->HiliteImage && t->HiliteImage->next) {
				AnimateHighlight(t);
				MaybeAnimate = True;
			}
		}
	}
	MaybeAnimate |= AnimateRoot();
	if(MaybeAnimate) {
		Animating++;
		SendEndAnimationMessage(scr->currentvs->wsw->w, LastTimestamp());
	}
	XFlush(dpy);
	return;
}

void InsertRGBColormap(Atom a, XStandardColormap *maps, int nmaps,
                       Bool replace)
{
	StdCmap *sc = NULL;

	if(replace) {                       /* locate existing entry */
		for(sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
			if(sc->atom == a) {
				break;
			}
		}
	}

	if(!sc) {                           /* no existing, allocate new */
		sc = calloc(1, sizeof(StdCmap));
		if(!sc) {
			fprintf(stderr, "%s:  unable to allocate %lu bytes for StdCmap\n",
			        ProgramName, (unsigned long) sizeof(StdCmap));
			return;
		}
	}

	if(replace) {                       /* just update contents */
		if(sc->maps) {
			XFree((char *) maps);
		}
		if(sc == Scr->StdCmapInfo.mru) {
			Scr->StdCmapInfo.mru = NULL;
		}
	}
	else {                              /* else appending */
		sc->next = NULL;
		sc->atom = a;
		if(Scr->StdCmapInfo.tail) {
			Scr->StdCmapInfo.tail->next = sc;
		}
		else {
			Scr->StdCmapInfo.head = sc;
		}
		Scr->StdCmapInfo.tail = sc;
	}
	sc->nmaps = nmaps;
	sc->maps = maps;

	return;
}

void RemoveRGBColormap(Atom a)
{
	StdCmap *sc, *prev;

	prev = NULL;
	for(sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
		if(sc->atom == a) {
			break;
		}
		prev = sc;
	}
	if(sc) {                            /* found one */
		if(sc->maps) {
			XFree((char *) sc->maps);
		}
		if(prev) {
			prev->next = sc->next;
		}
		if(Scr->StdCmapInfo.head == sc) {
			Scr->StdCmapInfo.head = sc->next;
		}
		if(Scr->StdCmapInfo.tail == sc) {
			Scr->StdCmapInfo.tail = prev;
		}
		if(Scr->StdCmapInfo.mru == sc) {
			Scr->StdCmapInfo.mru = NULL;
		}
	}
	return;
}

void LocateStandardColormaps(void)
{
	Atom *atoms;
	int natoms;
	int i;

	atoms = XListProperties(dpy, Scr->Root, &natoms);
	for(i = 0; i < natoms; i++) {
		XStandardColormap *maps = NULL;
		int nmaps;

		if(XGetRGBColormaps(dpy, Scr->Root, &maps, &nmaps, atoms[i])) {
			/* if got one, then append to current list */
			InsertRGBColormap(atoms[i], maps, nmaps, False);
		}
	}
	if(atoms) {
		XFree((char *) atoms);
	}
	return;
}

void GetColor(int kind, Pixel *what, char *name)
{
	XColor color;
	Colormap cmap = Scr->RootColormaps.cwins[0]->colormap->c;

#ifndef TOM
	if(!Scr->FirstTime) {
		return;
	}
#endif

	if(Scr->Monochrome != kind) {
		return;
	}

	if(! XParseColor(dpy, cmap, name, &color)) {
		fprintf(stderr, "%s:  invalid color name \"%s\"\n", ProgramName, name);
		return;
	}
	if(! XAllocColor(dpy, cmap, &color)) {
		/* if we could not allocate the color, let's see if this is a
		 * standard colormap
		 */
		XStandardColormap *stdcmap = NULL;

		if(! XParseColor(dpy, cmap, name, &color)) {
			fprintf(stderr, "%s:  invalid color name \"%s\"\n", ProgramName, name);
			return;
		}

		/*
		 * look through the list of standard colormaps (check cache first)
		 */
		if(Scr->StdCmapInfo.mru && Scr->StdCmapInfo.mru->maps &&
		                (Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex].colormap ==
		                 cmap)) {
			stdcmap = &(Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex]);
		}
		else {
			StdCmap *sc;

			for(sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
				int i;

				for(i = 0; i < sc->nmaps; i++) {
					if(sc->maps[i].colormap == cmap) {
						Scr->StdCmapInfo.mru = sc;
						Scr->StdCmapInfo.mruindex = i;
						stdcmap = &(sc->maps[i]);
						goto gotit;
					}
				}
			}
		}

gotit:
		if(stdcmap) {
			color.pixel = (stdcmap->base_pixel +
			               ((Pixel)(((float)color.red / 65535.0) *
			                        stdcmap->red_max + 0.5) *
			                stdcmap->red_mult) +
			               ((Pixel)(((float)color.green / 65535.0) *
			                        stdcmap->green_max + 0.5) *
			                stdcmap->green_mult) +
			               ((Pixel)(((float)color.blue  / 65535.0) *
			                        stdcmap->blue_max + 0.5) *
			                stdcmap->blue_mult));
		}
		else {
			fprintf(stderr, "%s:  unable to allocate color \"%s\"\n",
			        ProgramName, name);
			return;
		}
	}

	*what = color.pixel;
	return;
}

void GetShadeColors(ColorPair *cp)
{
	XColor      xcol;
	Colormap    cmap = Scr->RootColormaps.cwins[0]->colormap->c;
	int         save;
	float       clearfactor;
	float       darkfactor;
	char        clearcol [32], darkcol [32];

	clearfactor = (float) Scr->ClearShadowContrast / 100.0;
	darkfactor  = (100.0 - (float) Scr->DarkShadowContrast)  / 100.0;
	xcol.pixel = cp->back;
	XQueryColor(dpy, cmap, &xcol);

	sprintf(clearcol, "#%04x%04x%04x",
	        xcol.red   + (unsigned short)((65535 -   xcol.red) * clearfactor),
	        xcol.green + (unsigned short)((65535 - xcol.green) * clearfactor),
	        xcol.blue  + (unsigned short)((65535 -  xcol.blue) * clearfactor));
	sprintf(darkcol,  "#%04x%04x%04x",
	        (unsigned short)(xcol.red   * darkfactor),
	        (unsigned short)(xcol.green * darkfactor),
	        (unsigned short)(xcol.blue  * darkfactor));

	save = Scr->FirstTime;
	Scr->FirstTime = True;
	GetColor(Scr->Monochrome, &cp->shadc, clearcol);
	GetColor(Scr->Monochrome, &cp->shadd,  darkcol);
	Scr->FirstTime = save;
}

Bool UpdateFont(MyFont *font, int height)
{
	int prev = font->avg_height;
	font->avg_fheight = (font->avg_fheight * font->avg_count + height)
	                    / (font->avg_count + 1);
	font->avg_count++;
	/* Arbitrary limit.  */
	if(font->avg_count >= 256) {
		font->avg_count = 256;
	}
	font->avg_height = (int)(font->avg_fheight + 0.5);
	/* fprintf (stderr, "Updating avg with %d(%d) + %d -> %d(%f)\n",
	 *       prev, font->avg_count, height,
	 *       font->avg_height, font->avg_fheight); */
	return (prev != font->avg_height);
}

void GetFont(MyFont *font)
{
	char *deffontname = "fixed,*";
	char **missing_charset_list_return;
	int missing_charset_count_return;
	char *def_string_return;
	XFontSetExtents *font_extents;
	XFontStruct **xfonts;
	char **font_names;
	int i;
	int ascent;
	int descent;
	int fnum;
	char *basename2;

	if(font->font_set != NULL) {
		XFreeFontSet(dpy, font->font_set);
	}

	basename2 = (char *)malloc(strlen(font->basename) + 3);
	if(basename2) {
		sprintf(basename2, "%s,*", font->basename);
	}
	else {
		basename2 = font->basename;
	}
	if((font->font_set = XCreateFontSet(dpy, basename2,
	                                    &missing_charset_list_return,
	                                    &missing_charset_count_return,
	                                    &def_string_return)) == NULL) {
		fprintf(stderr, "Failed to get fontset %s\n", basename2);
		if(Scr->DefaultFont.basename) {
			deffontname = Scr->DefaultFont.basename;
		}
		if((font->font_set = XCreateFontSet(dpy, deffontname,
		                                    &missing_charset_list_return,
		                                    &missing_charset_count_return,
		                                    &def_string_return)) == NULL) {
			fprintf(stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n",
			        ProgramName, font->basename, deffontname);
			exit(1);
		}
	}
	if(basename2 != font->basename) {
		free(basename2);
	}
	font_extents = XExtentsOfFontSet(font->font_set);

	fnum = XFontsOfFontSet(font->font_set, &xfonts, &font_names);
	for(i = 0, ascent = 0, descent = 0; i < fnum; i++) {
		ascent = MaxSize(ascent, (*xfonts)->ascent);
		descent = MaxSize(descent, (*xfonts)->descent);
		xfonts++;
	}

	font->height = font_extents->max_logical_extent.height;
	font->y = ascent;
	font->ascent = ascent;
	font->descent = descent;
	font->avg_height = 0;
	font->avg_fheight = 0.0;
	font->avg_count = 0;
}


void SetFocusVisualAttributes(TwmWindow *tmp_win, Bool focus)
{
	if(! tmp_win) {
		return;
	}

	if(focus == tmp_win->hasfocusvisible) {
		return;
	}
	if(tmp_win->highlight) {
		if(Scr->use3Dborders) {
			PaintBorders(tmp_win, focus);
		}
		else {
			if(focus) {
				XSetWindowBorder(dpy, tmp_win->frame, tmp_win->borderC.back);
				if(tmp_win->title_w) {
					XSetWindowBorder(dpy, tmp_win->title_w, tmp_win->borderC.back);
				}
			}
			else {
				XSetWindowBorderPixmap(dpy, tmp_win->frame, tmp_win->gray);
				if(tmp_win->title_w) {
					XSetWindowBorderPixmap(dpy, tmp_win->title_w, tmp_win->gray);
				}
			}
		}
	}

	if(focus) {
		Bool hil = False;

		if(tmp_win->lolite_wl) {
			XUnmapWindow(dpy, tmp_win->lolite_wl);
		}
		if(tmp_win->lolite_wr) {
			XUnmapWindow(dpy, tmp_win->lolite_wr);
		}
		if(tmp_win->hilite_wl) {
			XMapWindow(dpy, tmp_win->hilite_wl);
			hil = True;
		}
		if(tmp_win->hilite_wr) {
			XMapWindow(dpy, tmp_win->hilite_wr);
			hil = True;
		}
		if(hil && tmp_win->HiliteImage && tmp_win->HiliteImage->next) {
			MaybeAnimate = True;
		}
		if(tmp_win->iconmanagerlist) {
			ActiveIconManager(tmp_win->iconmanagerlist);
		}
	}
	else {
		if(tmp_win->hilite_wl) {
			XUnmapWindow(dpy, tmp_win->hilite_wl);
		}
		if(tmp_win->hilite_wr) {
			XUnmapWindow(dpy, tmp_win->hilite_wr);
		}
		if(tmp_win->lolite_wl) {
			XMapWindow(dpy, tmp_win->lolite_wl);
		}
		if(tmp_win->lolite_wr) {
			XMapWindow(dpy, tmp_win->lolite_wr);
		}
		if(tmp_win->iconmanagerlist) {
			NotActiveIconManager(tmp_win->iconmanagerlist);
		}
	}
	if(Scr->use3Dtitles && Scr->SunkFocusWindowTitle && tmp_win->title_height) {
		ButtonState bs;

		bs = focus ? on : off;
		Draw3DBorder(tmp_win->title_w, Scr->TBInfo.titlex, 0,
		             tmp_win->title_width - Scr->TBInfo.titlex -
		             Scr->TBInfo.rightoff - Scr->TitlePadding,
		             Scr->TitleHeight, Scr->TitleShadowDepth,
		             tmp_win->title, bs, False, False);
	}
	tmp_win->hasfocusvisible = focus;
}

static void move_to_head(TwmWindow *t)
{
	if(t == NULL) {
		return;
	}
	if(Scr->FirstWindow == t) {
		return;
	}

	/* Unlink t from current position */
	if(t->prev) {
		t->prev->next = t->next;
	}
	if(t->next) {
		t->next->prev = t->prev;
	}

	/* Re-link t at head */
	t->next = Scr->FirstWindow;
	if(Scr->FirstWindow != NULL) {
		Scr->FirstWindow->prev = t;
	}
	t->prev = NULL;
	Scr->FirstWindow = t;
}

/*
 * Moves window 't' after window 'after'.
 *
 * If 'after' == NULL, puts it at the head.
 * If 't' == NULL, does nothing.
 * If the 't' is already after 'after', does nothing.
 */

void move_to_after(TwmWindow *t, TwmWindow *after)
{
	if(after == NULL) {
		move_to_head(t);
		return;
	}
	if(t == NULL) {
		return;
	}
	if(after->next == t) {
		return;
	}

	/* Unlink t from current position */
	if(t->prev) {
		t->prev->next = t->next;
	}
	if(t->next) {
		t->next->prev = t->prev;
	}

	/* Re-link t after 'after' */
	t->next = after->next;
	if(after->next) {
		after->next->prev = t;
	}
	t->prev = after;
	after->next = t;
}

/*
 * SetFocus - separate routine to set focus to make things more understandable
 * and easier to debug
 */
void SetFocus(TwmWindow *tmp_win, Time tim)
{
	Window w = (tmp_win ? tmp_win->w : PointerRoot);
	int f_iconmgr = 0;

	if(Scr->Focus && (Scr->Focus->iconmgr)) {
		f_iconmgr = 1;
	}
	if(Scr->SloppyFocus && (w == PointerRoot) && (!f_iconmgr)) {
		return;
	}

	XSetInputFocus(dpy, w, RevertToPointerRoot, tim);
#ifdef EWMH
	EwmhSet_NET_ACTIVE_WINDOW(w);
#endif
	if(Scr->Focus == tmp_win) {
		return;
	}

	if(Scr->Focus) {
		if(Scr->Focus->AutoSqueeze && !Scr->Focus->squeezed) {
			AutoSqueeze(Scr->Focus);
		}
		SetFocusVisualAttributes(Scr->Focus, False);
	}
	if(tmp_win)    {
		if(tmp_win->AutoSqueeze && tmp_win->squeezed) {
			AutoSqueeze(tmp_win);
		}
		SetFocusVisualAttributes(tmp_win, True);
	}
	Scr->Focus = tmp_win;
}


static Image *Create3DCrossImage(ColorPair cp)
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

static Image *Create3DIconifyImage(ColorPair cp)
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

static Image *Create3DSunkenResizeImage(ColorPair cp)
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

static Image *Create3DBoxImage(ColorPair cp)
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

static Image *Create3DDotImage(ColorPair cp)
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

static Image *Create3DBarImage(ColorPair cp)
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

static Image *Create3DVertBarImage(ColorPair cp)
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

static Image *Create3DMenuImage(ColorPair cp)
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

static Image *Create3DResizeImage(ColorPair cp)
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

static Image *Create3DZoomImage(ColorPair cp)
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

struct Colori {
	Pixel color;
	Pixmap pix;
	struct Colori *next;
};

Pixmap Create3DMenuIcon(unsigned int height,
                        unsigned int *widthp, unsigned int *heightp,
                        ColorPair cp)
{
	unsigned int h, w;
	int         i;
	struct Colori *col;
	static struct Colori *colori = NULL;

	h = height;
	w = h * 7 / 8;
	if(h < 1) {
		h = 1;
	}
	if(w < 1) {
		w = 1;
	}
	*widthp  = w;
	*heightp = h;

	for(col = colori; col; col = col->next) {
		if(col->color == cp.back) {
			break;
		}
	}
	if(col != NULL) {
		return (col->pix);
	}
	col = (struct Colori *) malloc(sizeof(struct Colori));
	col->color = cp.back;
	col->pix   = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
	col->next = colori;
	colori = col;

	Draw3DBorder(col->pix, 0, 0, w, h, 1, cp, off, True, False);
	for(i = 3; i + 5 < h; i += 5) {
		Draw3DBorder(col->pix, 4, i, w - 8, 3, 1, Scr->MenuC, off, True, False);
	}
	return (colori->pix);
}

Pixmap Create3DIconManagerIcon(ColorPair cp)
{
	unsigned int w, h;
	struct Colori *col;
	static struct Colori *colori = NULL;

	w = siconify_width;
	h = siconify_height;

	for(col = colori; col; col = col->next) {
		if(col->color == cp.back) {
			break;
		}
	}
	if(col != NULL) {
		return (col->pix);
	}
	col = (struct Colori *) malloc(sizeof(struct Colori));
	col->color = cp.back;
	col->pix   = XCreatePixmap(dpy, Scr->Root, w, h, Scr->d_depth);
	Draw3DBorder(col->pix, 0, 0, w, h, 4, cp, off, True, False);
	col->next = colori;
	colori = col;

	return (colori->pix);
}

static Image *Create3DResizeAnimation(Bool in, Bool left, Bool top,
                                      ColorPair cp)
{
	int         h, i, j;
	Image       *image, *im, *im1;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = im1 = None;
	for(i = (in ? 0 : (h / 4) - 1); (i < h / 4) && (i >= 0); i += (in ? 1 : -1)) {
		im = (Image *) malloc(sizeof(Image));
		if(! im) {
			return (None);
		}
		im->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
		if(im->pixmap == None) {
			free(im);
			return (None);
		}
		Draw3DBorder(im->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off, True,
		             False);
		for(j = i; j <= h; j += (h / 4)) {
			Draw3DBorder(im->pixmap, (left ? 0 : j), (top ? 0 : j),
			             h - j, h - j, 2, cp, off, True, False);
		}
		im->mask   = None;
		im->width  = h;
		im->height = h;
		im->next   = None;
		if(image == None) {
			image = im1 = im;
		}
		else {
			im1->next = im;
			im1 = im;
		}
	}
	if(im1 != None) {
		im1->next = image;
	}
	return (image);
}

static Image *Create3DResizeInTopAnimation(ColorPair cp)
{
	return Create3DResizeAnimation(TRUE, FALSE, TRUE, cp);
}

static Image *Create3DResizeOutTopAnimation(ColorPair cp)
{
	return Create3DResizeAnimation(False, FALSE, TRUE, cp);
}

static Image *Create3DResizeInBotAnimation(ColorPair cp)
{
	return Create3DResizeAnimation(TRUE, TRUE, FALSE, cp);
}

static Image *Create3DResizeOutBotAnimation(ColorPair cp)
{
	return Create3DResizeAnimation(False, TRUE, FALSE, cp);
}

static Image *Create3DMenuAnimation(Bool up, ColorPair cp)
{
	int   h, i, j;
	Image *image, *im, *im1;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	image = im1 = None;
	for(j = (up ? 4 : 0); j != (up ? -1 : 5); j += (up ? -1 : 1)) {
		im = (Image *) malloc(sizeof(Image));
		if(! im) {
			return (None);
		}
		im->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
		if(im->pixmap == None) {
			free(im);
			return (None);
		}
		Draw3DBorder(im->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off, True,
		             False);
		for(i = j; i < h - 3; i += 5) {
			Draw3DBorder(im->pixmap, 4, i, h - 8, 4, 2, cp, off, True, False);
		}
		im->mask   = None;
		im->width  = h;
		im->height = h;
		im->next   = None;
		if(image == None) {
			image = im1 = im;
		}
		else {
			im1->next = im;
			im1 = im;
		}
	}
	if(im1 != None) {
		im1->next = image;
	}
	return (image);
}

static Image *Create3DMenuUpAnimation(ColorPair cp)
{
	return Create3DMenuAnimation(TRUE, cp);
}

static Image *Create3DMenuDownAnimation(ColorPair cp)
{
	return Create3DMenuAnimation(False, cp);
}

static Image *Create3DZoomAnimation(Bool in, Bool out, int n, ColorPair cp)
{
	int         h, i, j, k;
	Image       *image, *im, *im1;

	h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
	if(!(h & 1)) {
		h--;
	}

	if(n == 0) {
		n = (h / 2) - 2;
	}

	image = im1 = None;
	for(j = (out ? -1 : 1) ; j < (in ? 2 : 0); j += 2) {
		for(k = (j > 0 ? 0 : n - 1) ; (k >= 0) && (k < n); k += j) {
			im = (Image *) malloc(sizeof(Image));
			im->pixmap = XCreatePixmap(dpy, Scr->Root, h, h, Scr->d_depth);
			Draw3DBorder(im->pixmap, 0, 0, h, h, Scr->TitleButtonShadowDepth, cp, off, True,
			             False);
			for(i = 2 + k; i < (h / 2); i += n) {
				Draw3DBorder(im->pixmap, i, i, h - (2 * i), h - (2 * i), 2, cp, off, True,
				             False);
			}
			im->mask   = None;
			im->width  = h;
			im->height = h;
			im->next   = None;
			if(image == None) {
				image = im1 = im;
			}
			else {
				im1->next = im;
				im1 = im;
			}
		}
	}
	if(im1 != None) {
		im1->next = image;
	}
	return (image);
}

static Image *Create3DMazeInAnimation(ColorPair cp)
{
	return Create3DZoomAnimation(TRUE, FALSE, 6, cp);
}

static Image *Create3DMazeOutAnimation(ColorPair cp)
{
	return Create3DZoomAnimation(FALSE, TRUE, 6, cp);
}

static Image *Create3DZoomInAnimation(ColorPair cp)
{
	return Create3DZoomAnimation(TRUE, FALSE, 0, cp);
}

static Image *Create3DZoomOutAnimation(ColorPair cp)
{
	return Create3DZoomAnimation(FALSE, TRUE, 0, cp);
}

static Image *Create3DZoomInOutAnimation(ColorPair cp)
{
	return Create3DZoomAnimation(TRUE, TRUE, 0, cp);
}


Pixmap CreateMenuIcon(int height, unsigned int *widthp, unsigned int *heightp)
{
	int h, w;
	int ih, iw;
	int ix, iy;
	int mh, mw;
	int tw, th;
	int lw, lh;
	int lx, ly;
	int lines, dly;
	int offset;
	int bw;

	h = height;
	w = h * 7 / 8;
	if(h < 1) {
		h = 1;
	}
	if(w < 1) {
		w = 1;
	}
	*widthp = w;
	*heightp = h;
	if(Scr->tbpm.menu == None) {
		Pixmap  pix;
		GC      gc;

		pix = Scr->tbpm.menu = XCreatePixmap(dpy, Scr->Root, w, h, 1);
		gc = XCreateGC(dpy, pix, 0L, NULL);
		XSetForeground(dpy, gc, 0L);
		XFillRectangle(dpy, pix, gc, 0, 0, w, h);
		XSetForeground(dpy, gc, 1L);
		ix = 1;
		iy = 1;
		ih = h - iy * 2;
		iw = w - ix * 2;
		offset = ih / 8;
		mh = ih - offset;
		mw = iw - offset;
		bw = mh / 16;
		if(bw == 0 && mw > 2) {
			bw = 1;
		}
		tw = mw - bw * 2;
		th = mh - bw * 2;
		XFillRectangle(dpy, pix, gc, ix, iy, mw, mh);
		XFillRectangle(dpy, pix, gc, ix + iw - mw, iy + ih - mh, mw, mh);
		XSetForeground(dpy, gc, 0L);
		XFillRectangle(dpy, pix, gc, ix + bw, iy + bw, tw, th);
		XSetForeground(dpy, gc, 1L);
		lw = tw / 2;
		if((tw & 1) ^ (lw & 1)) {
			lw++;
		}
		lx = ix + bw + (tw - lw) / 2;

		lh = th / 2 - bw;
		if((lh & 1) ^ ((th - bw) & 1)) {
			lh++;
		}
		ly = iy + bw + (th - bw - lh) / 2;

		lines = 3;
		if((lh & 1) && lh < 6) {
			lines--;
		}
		dly = lh / (lines - 1);
		while(lines--) {
			XFillRectangle(dpy, pix, gc, lx, ly, lw, bw);
			ly += dly;
		}
		XFreeGC(dpy, gc);
	}
	return Scr->tbpm.menu;
}

#define FBGC(gc, fix_fore, fix_back)\
    Gcv.foreground = fix_fore;\
    Gcv.background = fix_back;\
    XChangeGC(dpy, gc, GCForeground|GCBackground,&Gcv)

void Draw3DBorder(Window w, int x, int y, int width, int height, int bw,
                  ColorPair cp, int state, int fill, int forcebw)
{
	int           i;
	XGCValues     gcv;
	unsigned long gcm;

	if((width < 1) || (height < 1)) {
		return;
	}
	if(Scr->Monochrome != COLOR) {
		if(fill) {
			gcm = GCFillStyle;
			gcv.fill_style = FillOpaqueStippled;
			XChangeGC(dpy, Scr->BorderGC, gcm, &gcv);
			XFillRectangle(dpy, w, Scr->BorderGC, x, y, width, height);
		}
		gcm  = 0;
		gcm |= GCLineStyle;
		gcv.line_style = (state == on) ? LineSolid : LineDoubleDash;
		gcm |= GCFillStyle;
		gcv.fill_style = FillSolid;
		XChangeGC(dpy, Scr->BorderGC, gcm, &gcv);
		for(i = 0; i < bw; i++) {
			XDrawLine(dpy, w, Scr->BorderGC, x,                 y + i,
			          x + width - i - 1, y + i);
			XDrawLine(dpy, w, Scr->BorderGC, x + i,                  y,
			          x + i, y + height - i - 1);
		}

		gcm  = 0;
		gcm |= GCLineStyle;
		gcv.line_style = (state == on) ? LineDoubleDash : LineSolid;
		gcm |= GCFillStyle;
		gcv.fill_style = FillSolid;
		XChangeGC(dpy, Scr->BorderGC, gcm, &gcv);
		for(i = 0; i < bw; i++) {
			XDrawLine(dpy, w, Scr->BorderGC, x + width - i - 1,          y + i,
			          x + width - i - 1, y + height - 1);
			XDrawLine(dpy, w, Scr->BorderGC, x + i,         y + height - i - 1,
			          x + width - 1, y + height - i - 1);
		}
		return;
	}

	if(fill) {
		FBGC(Scr->BorderGC, cp.back, cp.fore);
		XFillRectangle(dpy, w, Scr->BorderGC, x, y, width, height);
	}
	if(Scr->BeNiceToColormap) {
		int dashoffset = 0;

		gcm  = 0;
		gcm |= GCLineStyle;
		gcv.line_style = (forcebw) ? LineSolid : LineDoubleDash;
		gcm |= GCBackground;
		gcv.background = cp.back;
		XChangeGC(dpy, Scr->BorderGC, gcm, &gcv);

		if(state == on) {
			XSetForeground(dpy, Scr->BorderGC, Scr->Black);
		}
		else {
			XSetForeground(dpy, Scr->BorderGC, Scr->White);
		}
		for(i = 0; i < bw; i++) {
			XDrawLine(dpy, w, Scr->BorderGC, x + i,     y + dashoffset,
			          x + i, y + height - i - 1);
			XDrawLine(dpy, w, Scr->BorderGC, x + dashoffset,    y + i,
			          x + width - i - 1, y + i);
			dashoffset = 1 - dashoffset;
		}
		XSetForeground(dpy, Scr->BorderGC, ((state == on) ? Scr->White : Scr->Black));
		for(i = 0; i < bw; i++) {
			XDrawLine(dpy, w, Scr->BorderGC, x + i,         y + height - i - 1,
			          x + width - 1, y + height - i - 1);
			XDrawLine(dpy, w, Scr->BorderGC, x + width - i - 1,          y + i,
			          x + width - i - 1, y + height - 1);
		}
		return;
	}
	if(state == on) {
		FBGC(Scr->BorderGC, cp.shadd, cp.shadc);
	}
	else             {
		FBGC(Scr->BorderGC, cp.shadc, cp.shadd);
	}
	for(i = 0; i < bw; i++) {
		XDrawLine(dpy, w, Scr->BorderGC, x,                 y + i,
		          x + width - i - 1, y + i);
		XDrawLine(dpy, w, Scr->BorderGC, x + i,                  y,
		          x + i, y + height - i - 1);
	}

	if(state == on) {
		FBGC(Scr->BorderGC, cp.shadc, cp.shadd);
	}
	else             {
		FBGC(Scr->BorderGC, cp.shadd, cp.shadc);
	}
	for(i = 0; i < bw; i++) {
		XDrawLine(dpy, w, Scr->BorderGC, x + width - i - 1,          y + i,
		          x + width - i - 1, y + height - 1);
		XDrawLine(dpy, w, Scr->BorderGC, x + i,         y + height - i - 1,
		          x + width - 1, y + height - i - 1);
	}
	return;
}

void Draw3DCorner(Window w,
                  int x, int y, int width, int height, int thick, int bw,
                  ColorPair cp, int type)
{
	XRectangle rects [2];

	switch(type) {
		case 0 :
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x + thick - bw, y + thick - bw,
			             width - thick + 2 * bw, height - thick + 2 * bw,
			             bw, cp, on, True, False);
			break;
		case 1 :
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x, y + thick - bw,
			             width - thick + bw, height - thick,
			             bw, cp, on, True, False);
			break;
		case 2 :
			rects [0].x      = x + width - thick;
			rects [0].y      = y;
			rects [0].width  = thick;
			rects [0].height = height;
			rects [1].x      = x;
			rects [1].y      = y + width - thick;
			rects [1].width  = width - thick;
			rects [1].height = thick;
			XSetClipRectangles(dpy, Scr->BorderGC, 0, 0, rects, 2, Unsorted);
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x, y,
			             width - thick + bw, height - thick + bw,
			             bw, cp, on, True, False);
			XSetClipMask(dpy, Scr->BorderGC, None);
			break;
		case 3 :
			rects [0].x      = x;
			rects [0].y      = y;
			rects [0].width  = thick;
			rects [0].height = height;
			rects [1].x      = x + thick;
			rects [1].y      = y + height - thick;
			rects [1].width  = width - thick;
			rects [1].height = thick;
			XSetClipRectangles(dpy, Scr->BorderGC, 0, 0, rects, 2, Unsorted);
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x + thick - bw, y,
			             width - thick, height - thick + bw,
			             bw, cp, on, True, False);
			XSetClipMask(dpy, Scr->BorderGC, None);
			break;
	}
	return;
}

static void PaintAllDecoration(void)
{
	TwmWindow *tmp_win;
	VirtualScreen *vs;

	for(tmp_win = Scr->FirstWindow; tmp_win != NULL; tmp_win = tmp_win->next) {
		if(! visible(tmp_win)) {
			continue;
		}
		if(tmp_win->mapped == TRUE) {
			if(tmp_win->frame_bw3D) {
				PaintBorders(tmp_win,
				             (tmp_win->highlight && tmp_win == Scr->Focus));
			}
			if(tmp_win->title_w) {
				PaintTitle(tmp_win);
			}
			if(tmp_win->titlebuttons) {
				PaintTitleButtons(tmp_win);
			}
		}
		else if((tmp_win->icon_on == TRUE)  &&
		                !Scr->NoIconTitlebar    &&
		                tmp_win->icon           &&
		                tmp_win->icon->w        &&
		                !tmp_win->icon->w_not_ours &&
		                ! LookInList(Scr->NoIconTitle, tmp_win->full_name, &tmp_win->class)) {
			PaintIcon(tmp_win);
		}
	}
	for(vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
		PaintWorkSpaceManager(vs);
	}
}

void PaintBorders(TwmWindow *tmp_win, Bool focus)
{
	ColorPair cp;

	cp = (focus && tmp_win->highlight) ? tmp_win->borderC : tmp_win->border_tile;
	if(tmp_win->title_height == 0) {
		Draw3DBorder(tmp_win->frame,
		             0,
		             0,
		             tmp_win->frame_width,
		             tmp_win->frame_height,
		             Scr->BorderShadowDepth, cp, off, True, False);
		Draw3DBorder(tmp_win->frame,
		             tmp_win->frame_bw3D - Scr->BorderShadowDepth,
		             tmp_win->frame_bw3D - Scr->BorderShadowDepth,
		             tmp_win->frame_width  - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderShadowDepth,
		             tmp_win->frame_height - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderShadowDepth,
		             Scr->BorderShadowDepth, cp, on, True, False);
		return;
	}
	Draw3DCorner(tmp_win->frame,
	             tmp_win->title_x - tmp_win->frame_bw3D,
	             0,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, 0);
	Draw3DCorner(tmp_win->frame,
	             tmp_win->title_x + tmp_win->title_width - Scr->TitleHeight,
	             0,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, 1);
	Draw3DCorner(tmp_win->frame,
	             tmp_win->frame_width  - (Scr->TitleHeight + tmp_win->frame_bw3D),
	             tmp_win->frame_height - (Scr->TitleHeight + tmp_win->frame_bw3D),
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, 2);
	Draw3DCorner(tmp_win->frame,
	             0,
	             tmp_win->frame_height - (Scr->TitleHeight + tmp_win->frame_bw3D),
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, 3);

	Draw3DBorder(tmp_win->frame,
	             tmp_win->title_x + Scr->TitleHeight,
	             0,
	             tmp_win->title_width - 2 * Scr->TitleHeight,
	             tmp_win->frame_bw3D,
	             Scr->BorderShadowDepth, cp, off, True, False);
	Draw3DBorder(tmp_win->frame,
	             tmp_win->frame_bw3D + Scr->TitleHeight,
	             tmp_win->frame_height - tmp_win->frame_bw3D,
	             tmp_win->frame_width - 2 * (Scr->TitleHeight + tmp_win->frame_bw3D),
	             tmp_win->frame_bw3D,
	             Scr->BorderShadowDepth, cp, off, True, False);
	Draw3DBorder(tmp_win->frame,
	             0,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D,
	             tmp_win->frame_height - 2 * (Scr->TitleHeight + tmp_win->frame_bw3D),
	             Scr->BorderShadowDepth, cp, off, True, False);
	Draw3DBorder(tmp_win->frame,
	             tmp_win->frame_width  - tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D,
	             tmp_win->frame_height - 2 * (Scr->TitleHeight + tmp_win->frame_bw3D),
	             Scr->BorderShadowDepth, cp, off, True, False);

	if(tmp_win->squeeze_info && !tmp_win->squeezed) {
		Draw3DBorder(tmp_win->frame,
		             0,
		             Scr->TitleHeight,
		             tmp_win->title_x,
		             tmp_win->frame_bw3D,
		             Scr->BorderShadowDepth, cp, off, True, False);
		Draw3DBorder(tmp_win->frame,
		             tmp_win->title_x + tmp_win->title_width,
		             Scr->TitleHeight,
		             tmp_win->frame_width - tmp_win->title_x - tmp_win->title_width,
		             tmp_win->frame_bw3D,
		             Scr->BorderShadowDepth, cp, off, True, False);
	}
}

void PaintTitle(TwmWindow *tmp_win)
{
	int width, mwidth, len;
	XRectangle ink_rect;
	XRectangle logical_rect;

	if(Scr->use3Dtitles) {
		if(Scr->SunkFocusWindowTitle && (Scr->Focus == tmp_win) &&
		                (tmp_win->title_height != 0))
			Draw3DBorder(tmp_win->title_w, Scr->TBInfo.titlex, 0,
			             tmp_win->title_width - Scr->TBInfo.titlex -
			             Scr->TBInfo.rightoff - Scr->TitlePadding,
			             Scr->TitleHeight, Scr->TitleShadowDepth,
			             tmp_win->title, on, True, False);
		else
			Draw3DBorder(tmp_win->title_w, Scr->TBInfo.titlex, 0,
			             tmp_win->title_width - Scr->TBInfo.titlex -
			             Scr->TBInfo.rightoff - Scr->TitlePadding,
			             Scr->TitleHeight, Scr->TitleShadowDepth,
			             tmp_win->title, off, True, False);
	}
	FB(tmp_win->title.fore, tmp_win->title.back);
	if(Scr->use3Dtitles) {
		len    = strlen(tmp_win->name);
		XmbTextExtents(Scr->TitleBarFont.font_set,
		               tmp_win->name, strlen(tmp_win->name),
		               &ink_rect, &logical_rect);
		width = logical_rect.width;
		mwidth = tmp_win->title_width  - Scr->TBInfo.titlex -
		         Scr->TBInfo.rightoff  - Scr->TitlePadding  -
		         Scr->TitleShadowDepth - 4;
		while((len > 0) && (width > mwidth)) {
			len--;
			XmbTextExtents(Scr->TitleBarFont.font_set,
			               tmp_win->name, len,
			               &ink_rect, &logical_rect);
			width = logical_rect.width;
		}
		((Scr->Monochrome != COLOR) ? XmbDrawImageString : XmbDrawString)
		(dpy, tmp_win->title_w, Scr->TitleBarFont.font_set,
		 Scr->NormalGC,
		 tmp_win->name_x,
		 (Scr->TitleHeight - logical_rect.height) / 2 + (- logical_rect.y),
		 tmp_win->name, len);
	}
	else
		XmbDrawString(dpy, tmp_win->title_w, Scr->TitleBarFont.font_set,
		              Scr->NormalGC,
		              tmp_win->name_x, Scr->TitleBarFont.y,
		              tmp_win->name, strlen(tmp_win->name));
}

void PaintIcon(TwmWindow *tmp_win)
{
	int         width, twidth, mwidth, len, x;
	Icon        *icon;
	XRectangle ink_rect;
	XRectangle logical_rect;

	if(!tmp_win || !tmp_win->icon) {
		return;
	}
	icon = tmp_win->icon;
	if(!icon->has_title) {
		return;
	}

	x     = 0;
	width = icon->w_width;
	if(Scr->ShrinkIconTitles && icon->title_shrunk) {
		x     = GetIconOffset(icon);
		width = icon->width;
	}
	len    = strlen(tmp_win->icon_name);
	XmbTextExtents(Scr->IconFont.font_set,
	               tmp_win->icon_name, len,
	               &ink_rect, &logical_rect);
	twidth = logical_rect.width;
	mwidth = width - 2 * (Scr->IconManagerShadowDepth + ICON_MGR_IBORDER);
	if(Scr->use3Diconmanagers) {
		Draw3DBorder(icon->w, x, icon->height, width,
		             Scr->IconFont.height +
		             2 * (Scr->IconManagerShadowDepth + ICON_MGR_IBORDER),
		             Scr->IconManagerShadowDepth, icon->iconc, off, False, False);
	}
	while((len > 0) && (twidth > mwidth)) {
		len--;
		XmbTextExtents(Scr->IconFont.font_set,
		               tmp_win->icon_name, len,
		               &ink_rect, &logical_rect);
		twidth = logical_rect.width;
	}
	FB(icon->iconc.fore, icon->iconc.back);
	XmbDrawString(dpy, icon->w, Scr->IconFont.font_set, Scr->NormalGC,
	              x + ((mwidth - twidth) / 2) +
	              Scr->IconManagerShadowDepth + ICON_MGR_IBORDER,
	              icon->y, tmp_win->icon_name, len);
}

void PaintTitleButton(TwmWindow *tmp_win, TBWindow  *tbw)
{
	TitleButton *tb = tbw->info;

	XCopyArea(dpy, tbw->image->pixmap, tbw->window, Scr->NormalGC,
	          tb->srcx, tb->srcy, tb->width, tb->height,
	          tb->dstx, tb->dsty);
	return;
}

static void PaintTitleButtons(TwmWindow *tmp_win)
{
	int i;
	TBWindow *tbw;
	int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

	for(i = 0, tbw = tmp_win->titlebuttons; i < nb; i++, tbw++) {
		if(tbw) {
			PaintTitleButton(tmp_win, tbw);
		}
	}
}

void AdoptWindow(void)
{
	unsigned long       data [2];
	Window              localroot, w;
	unsigned char       *prop;
	unsigned long       bytesafter;
	unsigned long       len;
	Atom                actual_type;
	int                 actual_format;
	XEvent              event;
	Window              root, parent, child, *children;
	unsigned int        nchildren, key_buttons;
	int                 root_x, root_y, win_x, win_y;
	int                 ret;
	int                 savedRestartPreviousState;

	localroot = w = RootWindow(dpy, Scr->screen);
	XGrabPointer(dpy, localroot, False,
	             ButtonPressMask | ButtonReleaseMask,
	             GrabModeAsync, GrabModeAsync,
	             None, Scr->SelectCursor, CurrentTime);

	XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask, &event);
	child = event.xbutton.subwindow;
	while(1) {
		if(child == (Window) 0) {
			break;
		}

		w = XmuClientWindow(dpy, child);
		ret = XGetWindowProperty(dpy, w, XA_WM_WORKSPACESLIST, 0L, 512,
		                         False, XA_STRING, &actual_type, &actual_format, &len,
		                         &bytesafter, &prop);
		XFree((char *)prop);  /* Don't ever do anything with it */
		if(ret != Success) {
			break;
		}
		if(len == 0) { /* it is not a local root window */
			break;        /* it is not a local root window */
		}
		localroot = w;
		XQueryPointer(dpy, localroot, &root, &child, &root_x, &root_y,
		              &win_x, &win_y, &key_buttons);
	}
	XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask, &event);
	XUngrabPointer(dpy, CurrentTime);

	if(localroot == Scr->Root) {
		return;
	}
	if(w == localroot) {   /* try to not adopt an ancestor */
		XQueryTree(dpy, Scr->Root, &root, &parent, &children, &nchildren);
		while(parent != (Window) 0) {
			XFree((char *) children);
			if(w == parent) {
				return;
			}
			XQueryTree(dpy, parent, &root, &parent, &children, &nchildren);
		}
		XFree((char *) children);
		if(w == root) {
			return;
		}
	}
	if(localroot == RootWindow(dpy, Scr->screen)) {
		XWithdrawWindow(dpy, w, Scr->screen);
	}
	else {
		XUnmapWindow(dpy, w);
	}
	XReparentWindow(dpy, w, Scr->Root, 0, 0);

	data [0] = (unsigned long) NormalState;
	data [1] = (unsigned long) None;

	XChangeProperty(dpy, w, XA_WM_STATE, XA_WM_STATE, 32,
	                PropModeReplace, (unsigned char *) data, 2);
	XFlush(dpy);
	/*
	 * We don't want to "restore" the occupation that the window had
	 * in its former environment. For one, the names of the workspaces
	 * may be different. And if not, the window will initially be
	 * shown in the current workspace, which may be at odds with that
	 * occupation (and confusion ensues).
	 *
	 * Hypermove has the same problem, but that is a "push" operation
	 * (initiated by the originating window manager) so we don't know
	 * when it happens...
	 */
	savedRestartPreviousState = RestartPreviousState;
	RestartPreviousState = False;
	SimulateMapRequest(w);
	RestartPreviousState = savedRestartPreviousState;
	return;
}

void RescueWindows(void)
{
	TwmWindow *twm_win = Scr->FirstWindow;

	while(twm_win) {
		VirtualScreen *vs = twm_win->vs;
		if(vs) {
			/*
			 * Check if this window seems completely out of sight.
			 */
			int x = twm_win->frame_x;
			int y = twm_win->frame_y;
			int w = twm_win->frame_width;
			int h = twm_win->frame_height;
			int bw = twm_win->frame_bw;
			int fullw = w + 2 * bw;
			int fullh = h + 2 * bw;
			int old_x = x, old_y = y;
			struct Icon *i;

#define MARGIN  20

			if(x >= vs->w - MARGIN) {
				x = vs->w - fullw;
			}
			if(y >= vs->h - MARGIN) {
				y = vs->h - fullh;
			}
			if((x + fullw <= MARGIN)) {
				x = 0;
			}
			if((y + fullh <= MARGIN)) {
				y = 0;
			}

			if(x != old_x || y != old_y) {
				SetupWindow(twm_win, x, y, w, h, -1);
			}

			/*
			 * If there is an icon, check it too.
			 */
			i = twm_win->icon;
			if(i != NULL) {
				x = i->w_x;
				y = i->w_y;
				w = i->w_width;
				h = i->w_height;
				old_x = x;
				old_y = y;

				if(x >= vs->w - MARGIN) {
					x = vs->w - w;
				}
				if(y >= vs->h - MARGIN) {
					y = vs->h - h;
				}
				if((x + w <= MARGIN)) {
					x = 0;
				}
				if((y + h <= MARGIN)) {
					y = 0;
				}

				if(x != old_x || y != old_y) {
					XMoveWindow(dpy, i->w, x, y);
					i->w_x = x;
					i->w_y = y;
				}
			}
#undef MARGIN
		}
		twm_win = twm_win->next;
	}
}

void DebugTrace(char *file)
{
	if(!file) {
		return;
	}
	if(tracefile) {
		fprintf(stderr, "stop logging events\n");
		if(tracefile != stderr) {
			fclose(tracefile);
		}
		tracefile = NULL;
	}
	else {
		if(strcmp(file, "stderr")) {
			tracefile = fopen(file, "w");
		}
		else {
			tracefile = stderr;
		}
		fprintf(stderr, "logging events to : %s\n", file);
	}
}


void SetBorderCursor(TwmWindow *tmp_win, int x, int y)
{
	Cursor cursor;
	XSetWindowAttributes attr;
	int h, fw, fh, wd;

	if(!tmp_win) {
		return;
	}

	/* Use the max of these, but since one is always 0 we can add them. */
	wd = tmp_win->frame_bw + tmp_win->frame_bw3D;
	h = Scr->TitleHeight + wd;
	fw = tmp_win->frame_width;
	fh = tmp_win->frame_height;

#if defined DEBUG && DEBUG
	fprintf(stderr, "wd=%d h=%d, fw=%d fh=%d x=%d y=%d\n",
	        wd, h, fw, fh, x, y);
#endif

	/*
	 * If not using 3D borders:
	 *
	 * The left border has negative x coordinates,
	 * The top border (above the title) has negative y coordinates.
	 * The title is TitleHeight high, the next wd pixels are border.
	 * The bottom border has coordinates >= the frame height.
	 * The right border has coordinates >= the frame width.
	 *
	 * If using 3D borders: all coordinates are >= 0, and all coordinates
	 * are higher by the border width.
	 *
	 * Since we only get events when we're actually in the border, we simply
	 * allow for both cases at the same time.
	 */

	if((x < -wd) || (y < -wd)) {
		cursor = Scr->FrameCursor;
	}
	else if(x < h) {
		if(y < h) {
			cursor = TopLeftCursor;
		}
		else if(y >= fh - h) {
			cursor = BottomLeftCursor;
		}
		else {
			cursor = LeftCursor;
		}
	}
	else if(x >= fw - h) {
		if(y < h) {
			cursor = TopRightCursor;
		}
		else if(y >= fh - h) {
			cursor = BottomRightCursor;
		}
		else {
			cursor = RightCursor;
		}
	}
	else if(y < h) {    /* also include title bar in top border area */
		cursor = TopCursor;
	}
	else if(y >= fh - h) {
		cursor = BottomCursor;
	}
	else {
		cursor = Scr->FrameCursor;
	}
	attr.cursor = cursor;
	XChangeWindowAttributes(dpy, tmp_win->frame, CWCursor, &attr);
	tmp_win->curcurs = cursor;
}

Image *GetImage(char *name, ColorPair cp)
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
		int    i;
		static struct {
			char *name;
			Image *(*proc)(ColorPair colorpair);
		} pmtab[] = {
			{ TBPM_3DDOT,       Create3DDotImage },
			{ TBPM_3DRESIZE,    Create3DResizeImage },
			{ TBPM_3DMENU,      Create3DMenuImage },
			{ TBPM_3DZOOM,      Create3DZoomImage },
			{ TBPM_3DBAR,       Create3DBarImage },
			{ TBPM_3DVBAR,      Create3DVertBarImage },
			{ TBPM_3DCROSS,     Create3DCrossImage },
			{ TBPM_3DICONIFY,   Create3DIconifyImage },
			{ TBPM_3DSUNKEN_RESIZE,     Create3DSunkenResizeImage },
			{ TBPM_3DBOX,       Create3DBoxImage }
		};

		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			for(i = 0; i < (sizeof pmtab) / (sizeof pmtab[0]); i++) {
				if(strcasecmp(pmtab[i].name, name) == 0) {
					image = (*pmtab[i].proc)(cp);
					if(image == None) {
						fprintf(stderr,
						        "%s:  unable to build pixmap \"%s\"\n", ProgramName, name);
						return (None);
					}
					break;
				}
			}
			if(image == None) {
				fprintf(stderr, "%s:  no such built-in pixmap \"%s\"\n", ProgramName, name);
				return (None);
			}
			AddToList(list, fullname, image);
		}
	}
	else if(strncmp(name, "%xpm:", 5) == 0) {
		int    i;
		static struct {
			char *name;
			Image *(*proc)(ColorPair colorpair);
		} pmtab[] = {
			{ "%xpm:menu-up", Create3DMenuUpAnimation },
			{ "%xpm:menu-down", Create3DMenuDownAnimation },
			{ "%xpm:resize", Create3DZoomOutAnimation }, /* compatibility */
			{ "%xpm:resize-out-top", Create3DResizeInTopAnimation },
			{ "%xpm:resize-in-top", Create3DResizeOutTopAnimation },
			{ "%xpm:resize-out-bot", Create3DResizeInBotAnimation },
			{ "%xpm:resize-in-bot", Create3DResizeOutBotAnimation },
			{ "%xpm:maze-out", Create3DMazeOutAnimation },
			{ "%xpm:maze-in", Create3DMazeInAnimation },
			{ "%xpm:zoom-out", Create3DZoomOutAnimation },
			{ "%xpm:zoom-in", Create3DZoomInAnimation },
			{ "%xpm:zoom-inout", Create3DZoomInOutAnimation }
		};

		sprintf(fullname, "%s%dx%d", name, (int) cp.fore, (int) cp.back);
		if((image = (Image *) LookInNameList(*list, fullname)) == None) {
			for(i = 0; i < (sizeof pmtab) / (sizeof pmtab[0]); i++) {
				if(strcasecmp(pmtab[i].name, name) == 0) {
					image = (*pmtab[i].proc)(cp);
					if(image == None) {
						fprintf(stderr,
						        "%s:  unable to build pixmap \"%s\"\n", ProgramName, name);
						return (None);
					}
					break;
				}
			}
			if(image == None) {
				fprintf(stderr, "%s:  no such built-in pixmap \"%s\"\n", ProgramName, name);
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

void FreeImage(Image *image)
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

static void compress(XImage *image, XColor *colors, int *ncolors);

static Image *LoadXwdImage(char *filename, ColorPair cp)
{
	FILE        *file;
	char        *fullname;
	XColor      colors [256];
	XWDColor    xwdcolors [256];
	unsigned    buffer_size;
	XImage      *image;
	unsigned char *imagedata;
	Pixmap      pixret;
	Visual      *visual;
	char        win_name [256];
	int         win_name_size;
	int         ispipe;
	int         i, len;
	int         w, h, depth, ncolors;
	int         scrn;
	Colormap    cmap;
	Colormap    stdcmap = Scr->RootColormaps.cwins[0]->colormap->c;
	GC          gc;
	XGCValues   gcvalues;
	XWDFileHeader header;
	Image       *ret;
	Bool        anim;
	unsigned long swaptest = 1;

	ispipe = 0;
	anim   = False;
	if(filename [0] == '|') {
		file = (FILE *) popen(filename + 1, "r");
		if(file == NULL) {
			return (None);
		}
		ispipe = 1;
		anim = AnimationActive;
		if(anim) {
			StopAnimation();
		}
		goto file_opened;
	}
	fullname = ExpandPixmapPath(filename);
	if(! fullname) {
		return (None);
	}
	file = fopen(fullname, "r");
	free(fullname);
	if(file == NULL) {
		if(reportfilenotfound) {
			fprintf(stderr, "unable to locate %s\n", filename);
		}
		return (None);
	}
file_opened:
	len = fread((char *) &header, sizeof(header), 1, file);
	if(len != 1) {
		fprintf(stderr, "ctwm: cannot read %s\n", filename);
		return (None);
	}
	if(*(char *) &swaptest) {
		swaplong((char *) &header, sizeof(header));
	}
	if(header.file_version != XWD_FILE_VERSION) {
		fprintf(stderr, "ctwm: XWD file format version mismatch : %s\n", filename);
		return (None);
	}
	win_name_size = header.header_size - sizeof(header);
	len = fread(win_name, win_name_size, 1, file);
	if(len != 1) {
		fprintf(stderr, "file %s has not the correct format\n", filename);
		return (None);
	}

	if(header.pixmap_format == XYPixmap) {
		fprintf(stderr, "ctwm: XYPixmap XWD file not supported : %s\n", filename);
		return (None);
	}
	w       = header.pixmap_width;
	h       = header.pixmap_height;
	depth   = header.pixmap_depth;
	ncolors = header.ncolors;
	len = fread((char *) xwdcolors, sizeof(XWDColor), ncolors, file);
	if(len != ncolors) {
		fprintf(stderr, "file %s has not the correct format\n", filename);
		return (None);
	}
	if(*(char *) &swaptest) {
		for(i = 0; i < ncolors; i++) {
			swaplong((char *) &xwdcolors [i].pixel, 4);
			swapshort((char *) &xwdcolors [i].red, 3 * 2);
		}
	}
	for(i = 0; i < ncolors; i++) {
		colors [i].pixel = xwdcolors [i].pixel;
		colors [i].red   = xwdcolors [i].red;
		colors [i].green = xwdcolors [i].green;
		colors [i].blue  = xwdcolors [i].blue;
		colors [i].flags = xwdcolors [i].flags;
		colors [i].pad   = xwdcolors [i].pad;
	}

	scrn    = Scr->screen;
	cmap    = AlternateCmap ? AlternateCmap : stdcmap;
	visual  = Scr->d_visual;
	gc      = DefaultGC(dpy, scrn);

	buffer_size = header.bytes_per_line * h;
	imagedata = (unsigned char *) malloc(buffer_size);
	if(! imagedata) {
		fprintf(stderr, "cannot allocate memory for image %s\n", filename);
		return (None);
	}
	len = fread(imagedata, (int) buffer_size, 1, file);
	if(len != 1) {
		free(imagedata);
		fprintf(stderr, "file %s has not the correct format\n", filename);
		return (None);
	}
	if(ispipe) {
		pclose(file);
	}
	else {
		fclose(file);
	}

	image = XCreateImage(dpy, visual,  depth, header.pixmap_format,
	                     0, (char *) imagedata, w, h,
	                     header.bitmap_pad, header.bytes_per_line);
	if(image == None) {
		free(imagedata);
		fprintf(stderr, "cannot create image for %s\n", filename);
		return (None);
	}
	if(header.pixmap_format == ZPixmap) {
		compress(image, colors, &ncolors);
	}
	if(header.pixmap_format != XYBitmap) {
		for(i = 0; i < ncolors; i++) {
			XAllocColor(dpy, cmap, &(colors [i]));
		}
		for(i = 0; i < buffer_size; i++) {
			imagedata [i] = (unsigned char) colors [imagedata [i]].pixel;
		}
	}
	if(w > Scr->rootw) {
		w = Scr->rootw;
	}
	if(h > Scr->rooth) {
		h = Scr->rooth;
	}

	ret = (Image *) malloc(sizeof(Image));
	if(! ret) {
		fprintf(stderr, "unable to allocate memory for image : %s\n", filename);
		free(image);
		free(imagedata);
		for(i = 0; i < ncolors; i++) {
			XFreeColors(dpy, cmap, &(colors [i].pixel), 1, 0L);
		}
		return (None);
	}
	if(header.pixmap_format == XYBitmap) {
		gcvalues.foreground = cp.fore;
		gcvalues.background = cp.back;
		XChangeGC(dpy, gc, GCForeground | GCBackground, &gcvalues);
	}
	if((w > (Scr->rootw / 2)) || (h > (Scr->rooth / 2))) {
		int x, y;

		pixret = XCreatePixmap(dpy, Scr->Root, Scr->rootw,
		                       Scr->rooth, Scr->d_depth);
		x = (Scr->rootw  - w) / 2;
		y = (Scr->rooth - h) / 2;
		XFillRectangle(dpy, pixret, gc, 0, 0, Scr->rootw, Scr->rooth);
		XPutImage(dpy, pixret, gc, image, 0, 0, x, y, w, h);
		ret->width  = Scr->rootw;
		ret->height = Scr->rooth;
	}
	else {
		pixret = XCreatePixmap(dpy, Scr->Root, w, h, depth);
		XPutImage(dpy, pixret, gc, image, 0, 0, 0, 0, w, h);
		ret->width  = w;
		ret->height = h;
	}
	XDestroyImage(image);

	ret->pixmap = pixret;
	ret->mask   = None;
	ret->next   = None;
	return (ret);
}

static Image *GetXwdImage(char *name, ColorPair cp)
{
	Image *image, *r, *s;
	char  path [128];
	char  pref [128], *perc;
	int   i;

	if(! strchr(name, '%')) {
		return (LoadXwdImage(name, cp));
	}
	s = image = None;
	strcpy(pref, name);
	perc  = strchr(pref, '%');
	*perc = '\0';
	for(i = 1;; i++) {
		sprintf(path, "%s%d%s", pref, i, perc + 1);
		r = LoadXwdImage(path, cp);
		if(r == None) {
			break;
		}
		r->next   = None;
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
		fprintf(stderr, "Cannot open any %s xwd file\n", name);
	}
	return (image);
}

static void compress(XImage *image, XColor *colors, int *ncolors)
{
	unsigned char ind  [256];
	unsigned int  used [256];
	int           i, j, size, nused;
	unsigned char color;
	XColor        newcolors [256];
	unsigned char *imagedata;

	for(i = 0; i < 256; i++) {
		used [i] = 0;
		ind  [i] = 0;
	}
	nused = 0;
	size  = image->bytes_per_line * image->height;
	imagedata = (unsigned char *) image->data;
	for(i = 0; i < size; i++) {
		if((i % image->bytes_per_line) > image->width) {
			continue;
		}
		color = imagedata [i];
		if(used [color] == 0) {
			for(j = 0; j < nused; j++) {
				if((colors [color].red   == newcolors [j].red)   &&
				                (colors [color].green == newcolors [j].green) &&
				                (colors [color].blue  == newcolors [j].blue)) {
					break;
				}
			}
			ind  [color] = j;
			used [color] = 1;
			if(j == nused) {
				newcolors [j].red   = colors [color].red;
				newcolors [j].green = colors [color].green;
				newcolors [j].blue  = colors [color].blue;
				nused++;
			}
		}
	}
	for(i = 0; i < size; i++) {
		imagedata [i] = ind [imagedata [i]];
	}
	for(i = 0; i < nused; i++) {
		colors [i] = newcolors [i];
	}
	*ncolors = nused;
}


static void swapshort(char *bp, unsigned n)
{
	char c;
	char *ep = bp + n;

	while(bp < ep) {
		c = *bp;
		*bp = *(bp + 1);
		bp++;
		*bp++ = c;
	}
}

static void swaplong(char *bp, unsigned n)
{
	char c;
	char *ep = bp + n;
	char *sp;

	while(bp < ep) {
		sp = bp + 3;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		sp = bp + 1;
		c = *sp;
		*sp = *bp;
		*bp++ = c;
		bp += 2;
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      GetWMPropertyString - Get Window Manager text property and
 *                              convert it to a string.
 *
 *  Returned Value:
 *      (char *) - pointer to the malloc'd string or NULL
 *
 *  Inputs:
 *      w       - the id of the window whose property is to be retrieved
 *      prop    - property atom (typically WM_NAME or WM_ICON_NAME)
 *
 ***********************************************************************
 */

unsigned char *GetWMPropertyString(Window w, Atom prop)
{
	XTextProperty       text_prop;
	char                **text_list;
	int                 text_list_count;
	unsigned char       *stringptr;
	int                 status, len = -1;

	(void)XGetTextProperty(dpy, w, &text_prop, prop);
	if(text_prop.value != NULL) {
		if(text_prop.encoding == XA_STRING
		                || text_prop.encoding == XA_COMPOUND_TEXT) {
			/* property is encoded as compound text - convert to locale string */
			status = XmbTextPropertyToTextList(dpy, &text_prop,
			                                   &text_list, &text_list_count);
			if(text_list_count == 0) {
				stringptr = NULL;
			}
			else if(text_list == (char **)0) {
				stringptr = NULL;
			}
			else if(text_list [0] == (char *)0) {
				stringptr = NULL;
			}
			else if(status < 0 || text_list_count < 0) {
				switch(status) {
					case XConverterNotFound:
						fprintf(stderr,
						        "%s: Converter not found; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
					case XNoMemory:
						fprintf(stderr,
						        "%s: Insufficient memory; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
					case XLocaleNotSupported:
						fprintf(stderr,
						        "%s: Locale not supported; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
				}
				stringptr = NULL;
				/*
				   don't call XFreeStringList - text_list appears to have
				   invalid address if status is bad
				   XFreeStringList(text_list);
				*/
			}
			else {
				len = strlen(text_list[0]);
				stringptr = memcpy(malloc(len + 1), text_list[0], len + 1);
				XFreeStringList(text_list);
			}
		}
		else {
			/* property is encoded in a format we don't understand */
			fprintf(stderr,
			        "%s: Encoding not STRING or COMPOUND_TEXT; unable to decode property %s of window ID %lx.\n",
			        ProgramName, XGetAtomName(dpy, prop), w);
			stringptr = NULL;
		}
		XFree(text_prop.value);
	}
	else {
		stringptr = NULL;
	}

	return stringptr;
}

void FreeWMPropertyString(char *prop)
{
	if(prop && (char *)prop != NoName) {
		free(prop);
	}
}

static void ConstrainLeftTop(int *value, int border)
{
	if(*value < border) {
		if(Scr->MoveOffResistance < 0 ||
		                *value > border - Scr->MoveOffResistance) {
			*value = border;
		}
		else if(Scr->MoveOffResistance > 0 &&
		                *value <= border - Scr->MoveOffResistance) {
			*value = *value + Scr->MoveOffResistance;
		}
	}
}

static void ConstrainRightBottom(int *value, int size1, int border, int size2)
{
	if(*value + size1 > size2 - border) {
		if(Scr->MoveOffResistance < 0 ||
		                *value + size1 < size2 - border + Scr->MoveOffResistance) {
			*value = size2 - size1 - border;
		}
		else if(Scr->MoveOffResistance > 0 &&
		                *value + size1 >= size2 - border + Scr->MoveOffResistance) {
			*value = *value - Scr->MoveOffResistance;
		}
	}
}

void ConstrainByBorders1(int *left, int width, int *top, int height)
{
	ConstrainRightBottom(left, width, Scr->BorderRight, Scr->rootw);
	ConstrainLeftTop(left, Scr->BorderLeft);
	ConstrainRightBottom(top, height, Scr->BorderBottom, Scr->rooth);
	ConstrainLeftTop(top, Scr->BorderTop);
}

void ConstrainByBorders(TwmWindow *twmwin,
                        int *left, int width, int *top, int height)
{
	if(twmwin->winbox) {
		XWindowAttributes attr;
		XGetWindowAttributes(dpy, twmwin->winbox->window, &attr);
		ConstrainRightBottom(left, width, 0, attr.width);
		ConstrainLeftTop(left, 0);
		ConstrainRightBottom(top, height, 0, attr.height);
		ConstrainLeftTop(top, 0);
	}
	else {
		ConstrainByBorders1(left, width, top, height);
	}
}
