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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>


#include <signal.h>
#include <sys/time.h>


#include "ctwm_atoms.h"
#include "util.h"
#include "animate.h"
#include "events.h"
#include "add_window.h"
#include "gram.tab.h"
#include "screen.h"
#include "icons.h"
#include "cursor.h"
#include "resize.h"
#include "image.h"
#include "decorations.h"


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


/*
 * Rewrite this, possibly in terms of replace_substr().  Alternately, the
 * places it's being used might be better served by being preprocesses
 * into arrays anyway.
 */
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
	ret = malloc(len + 1);
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
 *
 * Currently only used in one place in image_bitmap.c.  I've left this
 * here instead of moving it into images at the moment on the assumption
 * that there might be other places in the codebase where it's useful.
 */
char *
ExpandFilename(const char *name)
{
	char *newname;

	/* If it doesn't start with ~/ then it's not our concern */
	if(name[0] != '~' || name[1] != '/') {
		return strdup(name);
	}

	asprintf(&newname, "%s/%s", Home, &name[1]);

	return newname;
}



void InsertRGBColormap(Atom a, XStandardColormap *maps, int nmaps,
                       bool replace)
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
			InsertRGBColormap(atoms[i], maps, nmaps, false);
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
	bool        save;
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
	Scr->FirstTime = true;
	GetColor(Scr->Monochrome, &cp->shadc, clearcol);
	GetColor(Scr->Monochrome, &cp->shadd,  darkcol);
	Scr->FirstTime = save;
}

bool
UpdateFont(MyFont *font, int height)
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

	asprintf(&basename2, "%s,*", font->basename);
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
	free(basename2);
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


void
SetFocusVisualAttributes(TwmWindow *tmp_win, bool focus)
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
				/*
				 * XXX It seems possible this could be replaced by a
				 * single global 'gray' pixmap; I don't think it actually
				 * varies per window, and I don't see any obvious reason
				 * it can't be reused, so we may be able to save an
				 * allocation for each window by doing so...
				 */
				XSetWindowBorderPixmap(dpy, tmp_win->frame, tmp_win->gray);
				if(tmp_win->title_w) {
					XSetWindowBorderPixmap(dpy, tmp_win->title_w, tmp_win->gray);
				}
			}
		}
	}

	if(focus) {
		bool hil = false;

		if(tmp_win->lolite_wl) {
			XUnmapWindow(dpy, tmp_win->lolite_wl);
		}
		if(tmp_win->lolite_wr) {
			XUnmapWindow(dpy, tmp_win->lolite_wr);
		}
		if(tmp_win->hilite_wl) {
			XMapWindow(dpy, tmp_win->hilite_wl);
			hil = true;
		}
		if(tmp_win->hilite_wr) {
			XMapWindow(dpy, tmp_win->hilite_wr);
			hil = true;
		}
		if(hil && tmp_win->HiliteImage && tmp_win->HiliteImage->next) {
			MaybeAnimate = true;
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

	if(Scr->Focus && (Scr->Focus->isiconmgr)) {
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
		SetFocusVisualAttributes(Scr->Focus, false);
	}
	if(tmp_win)    {
		if(tmp_win->AutoSqueeze && tmp_win->squeezed) {
			AutoSqueeze(tmp_win);
		}
		SetFocusVisualAttributes(tmp_win, true);
	}
	Scr->Focus = tmp_win;
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
	col = malloc(sizeof(struct Colori));
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
	col = malloc(sizeof(struct Colori));
	col->color = cp.back;
	col->pix   = XCreatePixmap(dpy, Scr->Root, w, h, Scr->d_depth);
	Draw3DBorder(col->pix, 0, 0, w, h, 4, cp, off, True, False);
	col->next = colori;
	colori = col;

	return (colori->pix);
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

#undef FBGC


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
	bool                savedRestartPreviousState;

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
	RestartPreviousState = false;
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
			else if(text_list == NULL) {
				stringptr = NULL;
			}
			else if(text_list [0] == NULL) {
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
