/*
 * Window decoration routines
 */


#include "ctwm.h"

#include <stdio.h>
#include <stdbool.h>

#include "gram.tab.h"
#include "image.h"
#include "screen.h"
#include "util.h"  // Border drawing

#include "decorations.h"


/* Internal bits */
static void ComputeWindowTitleOffsets(TwmWindow *tmp_win, unsigned int width,
                                      bool squeeze);

typedef enum { TopLeft, TopRight, BottomRight, BottomLeft } CornerType;
static void Draw3DCorner(Window w, int x, int y, int width, int height,
                         int thick, int bw, ColorPair cp, CornerType type);



/*
 * First, the bits for setting up the frame window
 */

/***********************************************************************
 *
 *  Procedure:
 *      SetupWindow - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the TwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *      bw      - the border width of the frame window or -1 not to change
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ***********************************************************************
 */
void
SetupWindow(TwmWindow *tmp_win, int x, int y, int w, int h, int bw)
{
	SetupFrame(tmp_win, x, y, w, h, bw, False);
}

void
SetupFrame(TwmWindow *tmp_win, int x, int y, int w, int h, int bw,
           Bool sendEvent)        /* whether or not to force a send */
{
	XEvent client_event;
	XWindowChanges frame_wc, xwc;
	unsigned long frame_mask, xwcm;
	int title_width, title_height;
	int reShape;

#ifdef DEBUG
	fprintf(stderr, "SetupWindow: x=%d, y=%d, w=%d, h=%d, bw=%d\n",
	        x, y, w, h, bw);
#endif

	if(bw < 0) {
		bw = tmp_win->frame_bw;        /* -1 means current frame width */
	}
	{
		int scrw, scrh;

		scrw = Scr->rootw;
		scrh = Scr->rooth;

#define MARGIN  16                      /* one "average" cursor width */

		if(x >= scrw) {
			x = scrw - MARGIN;
		}
		if(y >= scrh) {
			y = scrh - MARGIN;
		}
		if((x + w + bw <= 0)) {
			x = -w + MARGIN;
		}
		if((y + h + bw <= 0)) {
			y = -h + MARGIN;
		}
	}

	if(tmp_win->iconmgr) {
		tmp_win->iconmgrp->width = w - (2 * tmp_win->frame_bw3D);
		h = tmp_win->iconmgrp->height + tmp_win->title_height +
		    (2 * tmp_win->frame_bw3D);
	}

	/*
	 * According to the July 27, 1988 ICCCM draft, we should send a
	 * "synthetic" ConfigureNotify event to the client if the window
	 * was moved but not resized.
	 */
	if(((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
	                (w == tmp_win->frame_width && h == tmp_win->frame_height)) ||
	                (bw != tmp_win->frame_bw)) {
		sendEvent = TRUE;
	}

	xwcm = CWWidth;
	title_width  = xwc.width = w - (2 * tmp_win->frame_bw3D);
	title_height = Scr->TitleHeight + bw;
	ComputeWindowTitleOffsets(tmp_win, xwc.width, true);

	reShape = (tmp_win->wShaped ? TRUE : FALSE);
	if(tmp_win->squeeze_info/* && !tmp_win->squeezed*/) {       /* check for title shaping */
		title_width = tmp_win->rightx + Scr->TBInfo.rightoff;
		if(title_width < xwc.width) {
			xwc.width = title_width;
			if(tmp_win->frame_height != h ||
			                tmp_win->frame_width != w ||
			                tmp_win->frame_bw != bw ||
			                title_width != tmp_win->title_width) {
				reShape = TRUE;
			}
		}
		else {
			if(!tmp_win->wShaped) {
				reShape = TRUE;
			}
			title_width = xwc.width;
		}
	}

	tmp_win->title_width = title_width;
	if(tmp_win->title_height) {
		tmp_win->title_height = title_height;
	}

	if(tmp_win->title_w) {
		if(bw != tmp_win->frame_bw) {
			xwc.border_width = bw;
			tmp_win->title_x = xwc.x = tmp_win->frame_bw3D - bw;
			tmp_win->title_y = xwc.y = tmp_win->frame_bw3D - bw;
			xwcm |= (CWX | CWY | CWBorderWidth);
		}

		XConfigureWindow(dpy, tmp_win->title_w, xwcm, &xwc);
	}
	if(tmp_win->attr.width != w) {
		tmp_win->widthEverChangedByUser = True;
	}

	if(tmp_win->attr.height != (h - tmp_win->title_height)) {
		tmp_win->heightEverChangedByUser = True;
	}

	if(!tmp_win->squeezed) {
		tmp_win->attr.width  = w - (2 * tmp_win->frame_bw3D);
		tmp_win->attr.height = h - tmp_win->title_height - (2 * tmp_win->frame_bw3D);
	}
	if(tmp_win->squeezed) {
		if(x != tmp_win->frame_x) {
			tmp_win->actual_frame_x += x - tmp_win->frame_x;
		}
		if(y != tmp_win->frame_y) {
			tmp_win->actual_frame_y += y - tmp_win->frame_y;
		}
	}
	/*
	 * fix up frame and assign size/location values in tmp_win
	 */
	frame_mask = 0;
	if(bw != tmp_win->frame_bw) {
		frame_wc.border_width = tmp_win->frame_bw = bw;
		if(bw == 0) {
			tmp_win->frame_bw3D = 0;
		}
		frame_mask |= CWBorderWidth;
	}
	tmp_win->frame_x = x;
	tmp_win->frame_y = y;
	if(tmp_win->UnmapByMovingFarAway && !visible(tmp_win)) {
		frame_wc.x = Scr->rootw  + 1;
		frame_wc.y = Scr->rooth + 1;
	}
	else {
		frame_wc.x = tmp_win->frame_x;
		frame_wc.y = tmp_win->frame_y;
	}
	frame_wc.width = tmp_win->frame_width = w;
	frame_wc.height = tmp_win->frame_height = h;
	frame_mask |= (CWX | CWY | CWWidth | CWHeight);
	XConfigureWindow(dpy, tmp_win->frame, frame_mask, &frame_wc);

	XMoveResizeWindow(dpy, tmp_win->w, tmp_win->frame_bw3D,
	                  tmp_win->title_height + tmp_win->frame_bw3D,
	                  tmp_win->attr.width, tmp_win->attr.height);

	/*
	 * fix up highlight window
	 */
	if(tmp_win->title_height && tmp_win->hilite_wl) {
		xwc.width = (tmp_win->name_x - tmp_win->highlightxl - 2);
		if(xwc.width <= 0) {
			xwc.x = Scr->rootw; /* move offscreen */
			xwc.width = 1;
		}
		else {
			xwc.x = tmp_win->highlightxl;
		}

		xwcm = CWX | CWWidth;
		XConfigureWindow(dpy, tmp_win->hilite_wl, xwcm, &xwc);
	}
	if(tmp_win->title_height && tmp_win->hilite_wr) {
		xwc.width = (tmp_win->rightx - tmp_win->highlightxr);
		if(Scr->TBInfo.nright > 0) {
			xwc.width -= 2 * Scr->TitlePadding;
		}
		if(Scr->use3Dtitles) {
			xwc.width -= Scr->TitleButtonShadowDepth;
		}
		if(xwc.width <= 0) {
			xwc.x = Scr->rootw; /* move offscreen */
			xwc.width = 1;
		}
		else {
			xwc.x = tmp_win->highlightxr;
		}

		xwcm = CWX | CWWidth;
		XConfigureWindow(dpy, tmp_win->hilite_wr, xwcm, &xwc);
	}
	if(tmp_win->title_height && tmp_win->lolite_wl) {
		xwc.width = (tmp_win->name_x - tmp_win->highlightxl);
		if(Scr->use3Dtitles) {
			xwc.width -= 4;
		}
		if(xwc.width <= 0) {
			xwc.x = Scr->rootw;        /* move offscreen */
			xwc.width = 1;
		}
		else {
			xwc.x = tmp_win->highlightxl;
		}

		xwcm = CWX | CWWidth;
		XConfigureWindow(dpy, tmp_win->lolite_wl, xwcm, &xwc);
	}
	if(tmp_win->title_height && tmp_win->lolite_wr) {
		xwc.width = (tmp_win->rightx - tmp_win->highlightxr);
		if(Scr->TBInfo.nright > 0) {
			xwc.width -= Scr->TitlePadding;
		}
		if(Scr->use3Dtitles) {
			xwc.width -= 4;
		}
		if(xwc.width <= 0) {
			xwc.x = Scr->rootw;        /* move offscreen */
			xwc.width = 1;
		}
		else {
			xwc.x = tmp_win->highlightxr;
		}

		xwcm = CWX | CWWidth;
		XConfigureWindow(dpy, tmp_win->lolite_wr, xwcm, &xwc);
	}
	if(HasShape && reShape) {
		SetFrameShape(tmp_win);
	}
	WMapSetupWindow(tmp_win, x, y, w, h);
	if(sendEvent) {
		client_event.type = ConfigureNotify;
		client_event.xconfigure.display = dpy;
		client_event.xconfigure.event = tmp_win->w;
		client_event.xconfigure.window = tmp_win->w;
		client_event.xconfigure.x = (x + tmp_win->frame_bw - tmp_win->old_bw
		                             + tmp_win->frame_bw3D);
		client_event.xconfigure.y = (y + tmp_win->frame_bw +
		                             tmp_win->title_height - tmp_win->old_bw
		                             + tmp_win->frame_bw3D);
		client_event.xconfigure.width = tmp_win->attr.width;
		client_event.xconfigure.height = tmp_win->attr.height;
		client_event.xconfigure.border_width = tmp_win->old_bw;
		/* Real ConfigureNotify events say we're above title window, so ... */
		/* what if we don't have a title ????? */
		client_event.xconfigure.above = tmp_win->frame;
		client_event.xconfigure.override_redirect = False;
		XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
	}
}


void
SetFrameShape(TwmWindow *tmp)
{
	/*
	 * see if the titlebar needs to move
	 */
	if(tmp->title_w) {
		int oldx = tmp->title_x, oldy = tmp->title_y;
		ComputeTitleLocation(tmp);
		if(oldx != tmp->title_x || oldy != tmp->title_y) {
			XMoveWindow(dpy, tmp->title_w, tmp->title_x, tmp->title_y);
		}
	}

	/*
	 * The frame consists of the shape of the contents window offset by
	 * title_height or'ed with the shape of title_w (which is always
	 * rectangular).
	 */
	if(tmp->wShaped) {
		/*
		 * need to do general case
		 */
		XShapeCombineShape(dpy, tmp->frame, ShapeBounding,
		                   tmp->frame_bw3D, tmp->title_height + tmp->frame_bw3D, tmp->w,
		                   ShapeBounding, ShapeSet);
		if(tmp->title_w) {
			XShapeCombineShape(dpy, tmp->frame, ShapeBounding,
			                   tmp->title_x + tmp->frame_bw,
			                   tmp->title_y + tmp->frame_bw,
			                   tmp->title_w, ShapeBounding,
			                   ShapeUnion);
		}
	}
	else {
		/*
		 * can optimize rectangular contents window
		 */
		if(tmp->squeeze_info && !tmp->squeezed) {
			XRectangle  newBounding[2];
			XRectangle  newClip[2];
			int fbw2 = 2 * tmp->frame_bw;

			/*
			 * Build the border clipping rectangles; one around title, one
			 * around window.  The title_[xy] field already have had frame_bw
			 * subtracted off them so that they line up properly in the frame.
			 *
			 * The frame_width and frame_height do *not* include borders.
			 */
			/* border */
			newBounding[0].x = tmp->title_x - tmp->frame_bw3D;
			newBounding[0].y = tmp->title_y - tmp->frame_bw3D;
			newBounding[0].width = tmp->title_width + fbw2 + 2 * tmp->frame_bw3D;
			newBounding[0].height = tmp->title_height;
			newBounding[1].x = -tmp->frame_bw;
			newBounding[1].y = Scr->TitleHeight;
			newBounding[1].width = tmp->attr.width + fbw2 + 2 * tmp->frame_bw3D;
			newBounding[1].height = tmp->attr.height + fbw2 + 2 * tmp->frame_bw3D;
			XShapeCombineRectangles(dpy, tmp->frame, ShapeBounding, 0, 0,
			                        newBounding, 2, ShapeSet, YXBanded);
			/* insides */
			newClip[0].x = tmp->title_x + tmp->frame_bw - tmp->frame_bw3D;
			newClip[0].y = 0;
			newClip[0].width = tmp->title_width + 2 * tmp->frame_bw3D;
			newClip[0].height = Scr->TitleHeight + tmp->frame_bw3D;
			newClip[1].x = 0;
			newClip[1].y = tmp->title_height;
			newClip[1].width = tmp->attr.width + 2 * tmp->frame_bw3D;
			newClip[1].height = tmp->attr.height + 2 * tmp->frame_bw3D;
			XShapeCombineRectangles(dpy, tmp->frame, ShapeClip, 0, 0,
			                        newClip, 2, ShapeSet, YXBanded);
		}
		else {
			(void) XShapeCombineMask(dpy, tmp->frame, ShapeBounding, 0, 0,
			                         None, ShapeSet);
			(void) XShapeCombineMask(dpy, tmp->frame, ShapeClip, 0, 0,
			                         None, ShapeSet);
		}
	}
}



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
	/* y position is always the same */
	tmp->title_y = tmp->frame_bw3D - tmp->frame_bw;

	/* x can vary depending on squeezing */
	if(tmp->squeeze_info && !tmp->squeezed) {
		SqueezeInfo *si = tmp->squeeze_info;
		int basex;
		int maxwidth = tmp->frame_width;
		int tw = tmp->title_width + 2 * tmp->frame_bw3D;

		/* figure label base from squeeze info (justification fraction) */
		if(si->denom == 0) {            /* num is pixel based */
			basex = si->num;
		}
		else {                          /* num/denom is fraction */
			basex = ((si->num * maxwidth) / si->denom);
		}
		if(si->num < 0) {
			basex += maxwidth;
		}

		/* adjust for left (nop), center, right justify */
		switch(si->justify) {
			case J_CENTER:
				basex -= tw / 2;
				break;
			case J_RIGHT:
				basex -= tw - 1;
				break;
		}

		/* Clip */
		if(basex > maxwidth - tw) {
			basex = maxwidth - tw;
		}
		if(basex < 0) {
			basex = 0;
		}

		tmp->title_x = basex - tmp->frame_bw + tmp->frame_bw3D;
	}
	else {
		tmp->title_x = tmp->frame_bw3D - tmp->frame_bw;
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
	ComputeWindowTitleOffsets(tmp_win, tmp_win->attr.width, false);

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
static void
ComputeWindowTitleOffsets(TwmWindow *tmp_win, unsigned int width, bool squeeze)
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
		/*
		 * If SunkFocusWindowTitle, then we "sink in" the whole title
		 * window when it's focused.  Otherwise (!SunkFocus || !focused)
		 * it's popped up.
		 */
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

	/* Setup the X graphics context for the drawing */
	FB(tmp_win->title.fore, tmp_win->title.back);

	/* And write in the name */
	if(Scr->use3Dtitles) {
		/*
		 * Do a bunch of trying to chop the length down until it will fit
		 * into the space.  This doesn't seem to actually accomplish
		 * anything at the moment, as somehow we wind up with nothing
		 * visible in the case of a long enough title.
		 */
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

		/*
		 * Write it in.  The Y position is subtly different from the
		 * !3Dtitles case due to the potential bordering around it.  It's
		 * not quite clear whether it should be.
		 */
		((Scr->Monochrome != COLOR) ? XmbDrawImageString : XmbDrawString)
		(dpy, tmp_win->title_w, Scr->TitleBarFont.font_set,
		 Scr->NormalGC,
		 tmp_win->name_x,
		 (Scr->TitleHeight - logical_rect.height) / 2 + (- logical_rect.y),
		 tmp_win->name, len);
	}
	else {
		/*
		 * XXX The 3Dtitle case above has attempted correction for a lot of
		 * stuff.  It's not entirely clear that it's either needed there,
		 * or not needed here.  It's also not obvious that the magic
		 * it does to support monochrome isn't applicable here, thought
		 * it may be a side effect of differences in how the backing
		 * titlebar is painted.  This requires investigation, and either
		 * fixing the wrong or documentation of why it's right.
		 */
		XmbDrawString(dpy, tmp_win->title_w, Scr->TitleBarFont.font_set,
		              Scr->NormalGC,
		              tmp_win->name_x, Scr->TitleBarFont.y,
		              tmp_win->name, strlen(tmp_win->name));
	}
}


/*
 * Painting in the buttons on the titlebar
 */
/* Iterate and show them all */
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

/* Blit the pixmap into the right place */
void
PaintTitleButton(TwmWindow *tmp_win, TBWindow *tbw)
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
             int thick, int bw, ColorPair cp, CornerType type)
{
	XRectangle rects [2];

	switch(type) {
		case TopLeft:
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x + thick - bw, y + thick - bw,
			             width - thick + 2 * bw, height - thick + 2 * bw,
			             bw, cp, on, True, False);
			break;
		case TopRight:
			Draw3DBorder(w, x, y, width, height, bw, cp, off, True, False);
			Draw3DBorder(w, x, y + thick - bw,
			             width - thick + bw, height - thick,
			             bw, cp, on, True, False);
			break;
		case BottomRight:
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
		case BottomLeft:
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
		default:
			/* Bad code */
			fprintf(stderr, "Internal error: Invalid Draw3DCorner type %d\n",
			        type);
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

	/* Set coloring based on focus/highlight state */
	cp = (focus && tmp_win->highlight) ? tmp_win->borderC : tmp_win->border_tile;

	/*
	 * If there's no height to the title bar (e.g., on NoTitle windows),
	 * there's nothing much to corner around, so we can just border up
	 * the whole thing.  Since the bordering on the frame is "below" the
	 * real window, we can just draw one giant square, and then one
	 * slightly smaller (but still larger than the real-window itself)
	 * square on top of it, and voila; border!
	 */
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

	/*
	 * Otherwise, we have to draw corners, which means we have to
	 * individually draw the 4 side borders between them as well.
	 *
	 * So start by laying out the 4 corners.
	 */

	/* How far the corners extend along the sides */
#define CORNERLEN (Scr->TitleHeight + tmp_win->frame_bw3D)

	Draw3DCorner(tmp_win->frame,
	             tmp_win->title_x - tmp_win->frame_bw3D,
	             0,
	             CORNERLEN, CORNERLEN,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, TopLeft);
	Draw3DCorner(tmp_win->frame,
	             tmp_win->title_x + tmp_win->title_width - Scr->TitleHeight,
	             0,
	             CORNERLEN, CORNERLEN,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, TopRight);
	Draw3DCorner(tmp_win->frame,
	             tmp_win->frame_width  - CORNERLEN,
	             tmp_win->frame_height - CORNERLEN,
	             CORNERLEN, CORNERLEN,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, BottomRight);
	Draw3DCorner(tmp_win->frame,
	             0,
	             tmp_win->frame_height - CORNERLEN,
	             CORNERLEN, CORNERLEN,
	             tmp_win->frame_bw3D, Scr->BorderShadowDepth, cp, BottomLeft);


	/*
	 * And draw the borders on the 4 sides between the corners
	 */
	/* Top */
	Draw3DBorder(tmp_win->frame,
	             tmp_win->title_x + Scr->TitleHeight,
	             0,
	             tmp_win->title_width - 2 * Scr->TitleHeight,
	             tmp_win->frame_bw3D,
	             Scr->BorderShadowDepth, cp, off, True, False);
	/* Bottom */
	Draw3DBorder(tmp_win->frame,
	             tmp_win->frame_bw3D + Scr->TitleHeight,
	             tmp_win->frame_height - tmp_win->frame_bw3D,
	             tmp_win->frame_width - 2 * CORNERLEN,
	             tmp_win->frame_bw3D,
	             Scr->BorderShadowDepth, cp, off, True, False);
	/* Left */
	Draw3DBorder(tmp_win->frame,
	             0,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D,
	             tmp_win->frame_height - 2 * CORNERLEN,
	             Scr->BorderShadowDepth, cp, off, True, False);
	/* Right */
	Draw3DBorder(tmp_win->frame,
	             tmp_win->frame_width  - tmp_win->frame_bw3D,
	             Scr->TitleHeight + tmp_win->frame_bw3D,
	             tmp_win->frame_bw3D,
	             tmp_win->frame_height - 2 * CORNERLEN,
	             Scr->BorderShadowDepth, cp, off, True, False);

#undef CORNERLEN


	/*
	 * If SqueezeTitle is set for the window, and the window isn't
	 * squeezed away (whether because it's focused, or it's just not
	 * squeezed at all), then we need to draw a "top" border onto the
	 * bare bits of the window to the left/right of where the titlebar
	 * is.
	 */
	if(tmp_win->squeeze_info && !tmp_win->squeezed) {
		/* To the left */
		Draw3DBorder(tmp_win->frame,
		             0,
		             Scr->TitleHeight,
		             tmp_win->title_x,
		             tmp_win->frame_bw3D,
		             Scr->BorderShadowDepth, cp, off, True, False);
		/* And the right */
		Draw3DBorder(tmp_win->frame,
		             tmp_win->title_x + tmp_win->title_width,
		             Scr->TitleHeight,
		             tmp_win->frame_width - tmp_win->title_x - tmp_win->title_width,
		             tmp_win->frame_bw3D,
		             Scr->BorderShadowDepth, cp, off, True, False);
	}
}
