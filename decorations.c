/*
 * Window decoration routiens
 */


#include "ctwm.h"

#include <stdio.h>

#include "gram.tab.h"
#include "image.h"
#include "screen.h"
#include "util.h"  // Border drawing

#include "decorations.h"


/* Internal funcs */
static void Draw3DCorner(Window w, int x, int y, int width, int height,
                         int thick, int bw, ColorPair cp, int type);



/*
 * Bits related to setting up titlebars.  Their subwindows, icons,
 * highlights, etc.
 */

/*
 * ComputeTitleLocation - calculate the position of the title window; we need
 * to take the frame_bw into account since we want (0,0) of the title window
 * to line up with (0,0) of the frame window.
 */
void
ComputeTitleLocation(TwmWindow *tmp)
{
	tmp->title_x = tmp->frame_bw3D - tmp->frame_bw;
	tmp->title_y = tmp->frame_bw3D - tmp->frame_bw;

	if(tmp->squeeze_info && !tmp->squeezed) {
		SqueezeInfo *si = tmp->squeeze_info;
		int basex;
		int maxwidth = tmp->frame_width;
		int tw = tmp->title_width + 2 * tmp->frame_bw3D;

		/*
		 * figure label base from squeeze info (justification fraction)
		 */
		if(si->denom == 0) {            /* num is pixel based */
			basex = si->num;
		}
		else {                          /* num/denom is fraction */
			basex = ((si->num * maxwidth) / si->denom);
		}
		if(si->num < 0) {
			basex += maxwidth;
		}

		/*
		 * adjust for left (nop), center, right justify and clip
		 */
		switch(si->justify) {
			case J_CENTER:
				basex -= tw / 2;
				break;
			case J_RIGHT:
				basex -= tw - 1;
				break;
		}
		if(basex > maxwidth - tw) {
			basex = maxwidth - tw;
		}
		if(basex < 0) {
			basex = 0;
		}

		tmp->title_x = basex - tmp->frame_bw + tmp->frame_bw3D;
	}
}


/*
 * Despite being called "TitlebarButtons", this actually sets up most of
 * the subwindows inside the titlebar.  There are windows for each
 * button, but also windows for the shifting-color regions on un/focus.
 */
void
CreateWindowTitlebarButtons(TwmWindow *tmp_win)
{
	unsigned long valuemask;            /* mask for create windows */
	XSetWindowAttributes attributes;    /* attributes for create windows */
	int leftx, rightx, y;
	TitleButton *tb;
	int nb;

	if(tmp_win->title_height == 0) {
		tmp_win->hilite_wl = (Window) 0;
		tmp_win->hilite_wr = (Window) 0;
		tmp_win->lolite_wl = (Window) 0;
		tmp_win->lolite_wr = (Window) 0;
		return;
	}


	/*
	 * create the title bar windows; let the event handler deal with painting
	 * so that we don't have to spend two pixmaps (or deal with hashing)
	 */
	ComputeWindowTitleOffsets(tmp_win, tmp_win->attr.width, False);

	leftx = y = Scr->TBInfo.leftx;
	rightx = tmp_win->rightx;

	attributes.win_gravity = NorthWestGravity;
	attributes.background_pixel = tmp_win->title.back;
	attributes.border_pixel = tmp_win->title.fore;
	attributes.event_mask = (ButtonPressMask | ButtonReleaseMask |
	                         ExposureMask);
	attributes.cursor = Scr->ButtonCursor;
	valuemask = (CWWinGravity | CWBackPixel | CWBorderPixel | CWEventMask |
	             CWCursor);

	tmp_win->titlebuttons = NULL;
	nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
	if(nb > 0) {
		/*
		 * XXX Rework this into a proper array, either NULL-terminated or
		 * with a stored size, instead of manually implementing a re-calc
		 * of the size and incrementing pointers every time we want to
		 * walk this.
		 */
		tmp_win->titlebuttons = calloc(nb, sizeof(TBWindow));
		if(!tmp_win->titlebuttons) {
			fprintf(stderr, "%s:  unable to allocate %d titlebuttons\n",
			        ProgramName, nb);
		}
		else {
			TBWindow *tbw;
			int boxwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);
			unsigned int h = (Scr->TBInfo.width - Scr->TBInfo.border * 2);

			for(tb = Scr->TBInfo.head, tbw = tmp_win->titlebuttons; tb;
			                tb = tb->next, tbw++) {
				int x;
				if(tb->rightside) {
					x = rightx;
					rightx += boxwidth;
					attributes.win_gravity = NorthEastGravity;
				}
				else {
					x = leftx;
					leftx += boxwidth;
					attributes.win_gravity = NorthWestGravity;
				}
				tbw->window = XCreateWindow(dpy, tmp_win->title_w, x, y, h, h,
				                            (unsigned int) Scr->TBInfo.border,
				                            0, (unsigned int) CopyFromParent,
				                            (Visual *) CopyFromParent,
				                            valuemask, &attributes);
				/*
				 * XXX Can we just use tb->image for this instead?  I
				 * think we can.  The TBInfo.head list is assembled in
				 * calls to CreateTitleButton(), which happen during
				 * config file parsing, and then during
				 * InitTitlebarButtons(), which then goes through and
				 * tb->image = GetImage()'s each of the entries.  I don't
				 * believe anything ever gets added to TBInfo.head after
				 * that.  And the setting in ITB() could only fail in
				 * cases that would presumably also fail for us here.  So
				 * this whole block is redundant?
				 */
				tbw->image = GetImage(tb->name, tmp_win->title);
				if(! tbw->image) {
					tbw->image = GetImage(TBPM_QUESTION, tmp_win->title);
					if(! tbw->image) {          /* cannot happen (see util.c) */
						fprintf(stderr, "%s:  unable to add titlebar button \"%s\"\n",
						        ProgramName, tb->name);
					}
				}
				tbw->info = tb;
			}
		}
	}

	CreateHighlightWindows(tmp_win);
	CreateLowlightWindows(tmp_win);
	XMapSubwindows(dpy, tmp_win->title_w);
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
	return;
}


/*
 * Figure out where the window title and the hi/lolite windows go within
 * the titlebar as a whole.
 *
 * For a particular window, called during the AddWindow() process.
 */
void
ComputeWindowTitleOffsets(TwmWindow *tmp_win, unsigned int width, Bool squeeze)
{
	int titlew = width - Scr->TBInfo.titlex - Scr->TBInfo.rightoff;

	switch(Scr->TitleJustification) {
		case J_LEFT :
			tmp_win->name_x = Scr->TBInfo.titlex;
			if(Scr->use3Dtitles) {
				tmp_win->name_x += Scr->TitleShadowDepth + 2;
			}
			break;
		case J_CENTER :
			tmp_win->name_x = Scr->TBInfo.titlex + (titlew - tmp_win->name_width) / 2;
			break;
		case J_RIGHT :
			tmp_win->name_x = Scr->TBInfo.titlex + titlew - tmp_win->name_width;
			if(Scr->use3Dtitles) {
				tmp_win->name_x -= Scr->TitleShadowDepth - 2;
			}
			break;
	}
	if(Scr->use3Dtitles) {
		if(tmp_win->name_x < (Scr->TBInfo.titlex + 2 * Scr->TitleShadowDepth)) {
			tmp_win->name_x = Scr->TBInfo.titlex + 2 * Scr->TitleShadowDepth;
		}
	}
	else if(tmp_win->name_x < Scr->TBInfo.titlex) {
		tmp_win->name_x = Scr->TBInfo.titlex;
	}
	tmp_win->highlightxl = Scr->TBInfo.titlex;
	tmp_win->highlightxr = tmp_win->name_x + tmp_win->name_width + 2;

	if(Scr->use3Dtitles) {
		tmp_win->highlightxl += Scr->TitleShadowDepth;
	}
	if(tmp_win->hilite_wr || Scr->TBInfo.nright > 0) {
		tmp_win->highlightxr += Scr->TitlePadding;
	}
	tmp_win->rightx = width - Scr->TBInfo.rightoff;
	if(squeeze && tmp_win->squeeze_info && !tmp_win->squeezed) {
		int rx = (tmp_win->highlightxr
		          + (tmp_win->hilite_wr ? Scr->TBInfo.width * 2 : 0)
		          + (Scr->TBInfo.nright > 0 ? Scr->TitlePadding : 0)
		          + Scr->FramePadding);
		if(rx < tmp_win->rightx) {
			tmp_win->rightx = rx;
		}
	}
	return;
}


/*
 * Creation/destruction of "hi/lowlight windows".  These are the
 * portion[s] of the title bar which change color/form to indicate focus.
 */
void
CreateHighlightWindows(TwmWindow *tmp_win)
{
	XSetWindowAttributes attributes;    /* attributes for create windows */
	GC gc;
	XGCValues gcv;
	unsigned long valuemask;
	int h = (Scr->TitleHeight - 2 * Scr->FramePadding);
	int y = Scr->FramePadding;

	if(! tmp_win->titlehighlight) {
		tmp_win->hilite_wl = (Window) 0;
		tmp_win->hilite_wr = (Window) 0;
		return;
	}
	/*
	 * If a special highlight pixmap was given, use that.  Otherwise,
	 * use a nice, even gray pattern.  The old horizontal lines look really
	 * awful on interlaced monitors (as well as resembling other looks a
	 * little bit too closely), but can be used by putting
	 *
	 *                 Pixmaps { TitleHighlight "hline2" }
	 *
	 * (or whatever the horizontal line bitmap is named) in the startup
	 * file.  If all else fails, use the foreground color to look like a
	 * solid line.
	 */

	if(! tmp_win->HiliteImage) {
		if(Scr->HighlightPixmapName) {
			tmp_win->HiliteImage = GetImage(Scr->HighlightPixmapName, tmp_win->title);
		}
	}
	if(! tmp_win->HiliteImage) {
		Pixmap pm = None;
		Pixmap bm = None;

		if(Scr->use3Dtitles && (Scr->Monochrome != COLOR)) {
			bm = XCreateBitmapFromData(dpy, tmp_win->title_w,
			                           (char *)black_bits, gray_width, gray_height);
		}
		else {
			bm = XCreateBitmapFromData(dpy, tmp_win->title_w,
			                           (char *)gray_bits, gray_width, gray_height);
		}

		pm = XCreatePixmap(dpy, tmp_win->title_w, gray_width, gray_height,
		                   Scr->d_depth);
		gcv.foreground = tmp_win->title.fore;
		gcv.background = tmp_win->title.back;
		gcv.graphics_exposures = False;
		gc = XCreateGC(dpy, pm, (GCForeground | GCBackground | GCGraphicsExposures),
		               &gcv);
		if(gc) {
			XCopyPlane(dpy, bm, pm, gc, 0, 0, gray_width, gray_height, 0, 0, 1);
			tmp_win->HiliteImage = AllocImage();
			tmp_win->HiliteImage->pixmap = pm;
			tmp_win->HiliteImage->width  = gray_width;
			tmp_win->HiliteImage->height = gray_height;
			tmp_win->HiliteImage->mask   = None;
			tmp_win->HiliteImage->next   = None;
			XFreeGC(dpy, gc);
		}
		else {
			XFreePixmap(dpy, pm);
			pm = None;
		}
		XFreePixmap(dpy, bm);
	}
	if(tmp_win->HiliteImage) {
		valuemask = CWBackPixmap;
		attributes.background_pixmap = tmp_win->HiliteImage->pixmap;
	}
	else {
		valuemask = CWBackPixel;
		attributes.background_pixel = tmp_win->title.fore;
	}

	if(Scr->use3Dtitles) {
		y += Scr->TitleShadowDepth;
		h -= 2 * Scr->TitleShadowDepth;
	}
	if(Scr->TitleJustification == J_LEFT) {
		tmp_win->hilite_wl = (Window) 0;
	}
	else {
		tmp_win->hilite_wl = XCreateWindow(dpy, tmp_win->title_w, 0, y,
		                                   (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		                                   (unsigned int) 0, Scr->d_depth, (unsigned int) CopyFromParent,
		                                   Scr->d_visual, valuemask, &attributes);
	}

	if(Scr->TitleJustification == J_RIGHT) {
		tmp_win->hilite_wr = (Window) 0;
	}
	else {
		tmp_win->hilite_wr = XCreateWindow(dpy, tmp_win->title_w, 0, y,
		                                   (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		                                   (unsigned int) 0,  Scr->d_depth, (unsigned int) CopyFromParent,
		                                   Scr->d_visual, valuemask, &attributes);
	}
}


void
DeleteHighlightWindows(TwmWindow *tmp_win)
{
	if(tmp_win->HiliteImage) {
		if(Scr->HighlightPixmapName) {
			/*
			 * Image obtained from GetImage(): it is in a cache
			 * so we don't need to free it. There will not be multiple
			 * copies if the same xpm:foo image is requested again.
			 */
		}
		else {
			XFreePixmap(dpy, tmp_win->HiliteImage->pixmap);
			free(tmp_win->HiliteImage);
		}
		tmp_win->HiliteImage = NULL;
	}
}


void
CreateLowlightWindows(TwmWindow *tmp_win)
{
	XSetWindowAttributes attributes;    /* attributes for create windows */
	unsigned long valuemask;
	int h = (Scr->TitleHeight - 2 * Scr->FramePadding);
	int y = Scr->FramePadding;
	ColorPair cp;

	if(!Scr->UseSunkTitlePixmap || ! tmp_win->titlehighlight) {
		tmp_win->lolite_wl = (Window) 0;
		tmp_win->lolite_wr = (Window) 0;
		return;
	}
	/*
	 * If a special highlight pixmap was given, use that.  Otherwise,
	 * use a nice, even gray pattern.  The old horizontal lines look really
	 * awful on interlaced monitors (as well as resembling other looks a
	 * little bit too closely), but can be used by putting
	 *
	 *                 Pixmaps { TitleHighlight "hline2" }
	 *
	 * (or whatever the horizontal line bitmap is named) in the startup
	 * file.  If all else fails, use the foreground color to look like a
	 * solid line.
	 */

	if(! tmp_win->LoliteImage) {
		if(Scr->HighlightPixmapName) {
			cp = tmp_win->title;
			cp.shadc = tmp_win->title.shadd;
			cp.shadd = tmp_win->title.shadc;
			tmp_win->LoliteImage = GetImage(Scr->HighlightPixmapName, cp);
		}
	}
	if(tmp_win->LoliteImage) {
		valuemask = CWBackPixmap;
		attributes.background_pixmap = tmp_win->LoliteImage->pixmap;
	}
	else {
		valuemask = CWBackPixel;
		attributes.background_pixel = tmp_win->title.fore;
	}

	if(Scr->use3Dtitles) {
		y += 2;
		h -= 4;
	}
	if(Scr->TitleJustification == J_LEFT) {
		tmp_win->lolite_wl = (Window) 0;
	}
	else {
		tmp_win->lolite_wl = XCreateWindow(dpy, tmp_win->title_w, 0, y,
		                                   (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		                                   (unsigned int) 0, Scr->d_depth, (unsigned int) CopyFromParent,
		                                   Scr->d_visual, valuemask, &attributes);
	}

	if(Scr->TitleJustification == J_RIGHT) {
		tmp_win->lolite_wr = (Window) 0;
	}
	else {
		tmp_win->lolite_wr = XCreateWindow(dpy, tmp_win->title_w, 0, y,
		                                   (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		                                   (unsigned int) 0,  Scr->d_depth, (unsigned int) CopyFromParent,
		                                   Scr->d_visual, valuemask, &attributes);
	}
}

/*
 * There is no DeleteLowlightWindows() as a counterpart to the
 * HighlightWindows variant.  That func doesn't delete the [sub-]window;
 * that happens semi-autotically when the frame window is destroyed.  It
 * only cleans up the Pixmap if there is one.  And the only way the
 * Lowlight window can wind up with a pixmap is as a copy of the
 * highlight window one, in which case when THAT delete gets called all
 * the cleanup is done.
 */




/*
 * Painting the titlebars.  The actual displaying of the stuff that's
 * figured or stored above.
 */

/*
 * Write in the window title
 */
void
PaintTitle(TwmWindow *tmp_win)
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


/*
 * Painting in the buttons on the titlebar
 */
void
PaintTitleButtons(TwmWindow *tmp_win)
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

void
PaintTitleButton(TwmWindow *tmp_win, TBWindow  *tbw)
{
	TitleButton *tb = tbw->info;

	XCopyArea(dpy, tbw->image->pixmap, tbw->window, Scr->NormalGC,
	          tb->srcx, tb->srcy, tb->width, tb->height,
	          tb->dstx, tb->dsty);
	return;
}




/*
 * Stuff for window borders
 */


/*
 * Currently only used in drawing window decoration borders.  Contrast
 * with Draw3DBorder() which is used for all sorts of generalized
 * drawing.
 */
static void
Draw3DCorner(Window w, int x, int y, int width, int height,
             int thick, int bw, ColorPair cp, int type)
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


/*
 * Draw the borders onto the frame for a window
 */
void
PaintBorders(TwmWindow *tmp_win, Bool focus)
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
