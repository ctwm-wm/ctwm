/*
 * Various operations done on windows.
 */

#include "ctwm.h"

#include <stdio.h>

#include "animate.h"
#include "decorations.h"
#include "drawing.h"
#include "iconmgr.h"
#include "image.h"
#include "menus.h"  // AutoSqueeze
#include "screen.h"
#include "win_ops.h"


/*
 * Update the visuals of a window (e.g., its own decorations and its
 * representation in the icon manager) for having/losing focus.
 *
 * Formerly in util.c
 */
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
		             tmp_win->title, bs, false, false);
	}
	tmp_win->hasfocusvisible = focus;
}


/*
 * Shift the focus to a given window, and do whatever subsidiary ops that
 * entails.
 *
 * Formerly in util.c
 */
void
SetFocus(TwmWindow *tmp_win, Time tim)
{
	Window w = (tmp_win ? tmp_win->w : PointerRoot);
	bool f_iconmgr = false;

	if(Scr->Focus && (Scr->Focus->isiconmgr)) {
		f_iconmgr = true;
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
