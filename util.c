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
#include "iconmgr.h"
#include "icons.h"
#include "cursor.h"
#include "resize.h"
#include "image.h"
#include "win_decorations.h"
#include "drawing.h"
#include "vscreen.h"


/* Handle for debug tracing */
FILE *tracefile = NULL;


/*
 * Rewrite this, possibly in terms of replace_substr().  Alternately, the
 * places it's being used might be better served by being preprocessed
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


void
GetColor(int kind, Pixel *what, const char *name)
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


#if 0
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
#endif


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
		XFree(prop);  /* Don't ever do anything with it */
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
			XFree(children);
			if(w == parent) {
				return;
			}
			XQueryTree(dpy, parent, &root, &parent, &children, &nchildren);
		}
		XFree(children);
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


/*
 * A safe strncpy(), which always ensures NUL-termination.
 *
 * XXX This is really just a slightly pessimized implementation of
 * strlcpy().  Maybe we should use that instead, with a local
 * implementation for systems like glibc-users that lack it?
 */
void
safe_strncpy(char *dest, const char *src, size_t size)
{
	strncpy(dest, src, size - 1);
	dest[size - 1] = '\0';
}
