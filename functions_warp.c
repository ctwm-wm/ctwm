/*
 * Functions related to the warp ring
 *
 * There are functions that are _named_ like warp-ring funcs, but aren't
 * really, and so aren't here.  Some examples are f.warphere which is
 * workspaces-related, and f.warptoscreen which is screen-related.
 *
 * There are also funcs that aren't really ring related, but I've put
 * here because they're still warping to window related, like f.warpto /
 * f.warptoiconmgr.
 */

#include "ctwm.h"

#include <string.h>

#include "functions_internal.h"
#include "iconmgr.h"
#include "list.h"
#include "otp.h"
#include "screen.h"
#include "win_iconify.h"
#include "win_ring.h"
#include "win_utils.h"

#include <stdio.h>

static void WarpAlongRing(XButtonEvent *ev, bool forward);
static void StartWindowStack(void);
static void FinishWindowStack(void);

DFHANDLER(warpto)
{
	TwmWindow *tw;
	int len;

	len = strlen(action);

#ifdef WARPTO_FROM_ICONMGR
	/* XXX should be iconmgrp? */
	if(len == 0 && tmp_win && tmp_win->iconmgr) {
		printf("curren iconmgr entry: %s", tmp_win->iconmgr->Current);
	}
#endif /* #ifdef WARPTO_FROM_ICONMGR */
	for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
		if(!strncmp(action, tw->name, len)) {
			break;
		}
		if(match(action, tw->name)) {
			break;
		}
	}
	if(!tw) {
		for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
			if(!strncmp(action, tw->class.res_name, len)) {
				break;
			}
			if(match(action, tw->class.res_name)) {
				break;
			}
		}
		if(!tw) {
			for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
				if(!strncmp(action, tw->class.res_class, len)) {
					break;
				}
				if(match(action, tw->class.res_class)) {
					break;
				}
			}
		}
	}

	if(tw) {
		if(Scr->WarpUnmapped || tw->mapped) {
			if(!tw->mapped) {
				DeIconify(tw);
			}
			WarpToWindow(tw, Scr->RaiseOnWarp);
		}
	}
	else {
		XBell(dpy, 0);
	}
}


DFHANDLER(warptoiconmgr)
{
	TwmWindow *tw, *raisewin = NULL;
	int len;
	Window iconwin = None;

	len = strlen(action);
	if(len == 0) {
		if(tmp_win && tmp_win->iconmanagerlist) {
			raisewin = tmp_win->iconmanagerlist->iconmgr->twm_win;
			iconwin = tmp_win->iconmanagerlist->icon;
		}
		else if(Scr->iconmgr->active) {
			raisewin = Scr->iconmgr->twm_win;
			iconwin = Scr->iconmgr->active->w;
		}
	}
	else {
		for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
			if(strncmp(action, tw->icon_name, len) == 0) {
				if(tw->iconmanagerlist &&
				                tw->iconmanagerlist->iconmgr->twm_win->mapped) {
					raisewin = tw->iconmanagerlist->iconmgr->twm_win;
					break;
				}
			}
		}
	}

	if(raisewin) {
		OtpRaise(raisewin, WinWin);
		XWarpPointer(dpy, None, iconwin, 0, 0, 0, 0, 5, 5);
	}
	else {
		XBell(dpy, 0);
	}
}

/* Taken from vtwm version 5.3 */
DFHANDLER(ring)
{
	if(WindowIsOnRing(tmp_win)) {
		/* It's in the ring, let's take it out. */
		UnlinkWindowFromRing(tmp_win);
	}
	else {
		/* Not in the ring, so put it in. */
		AddWindowToRing(tmp_win);
	}
	/*tmp_win->ring.cursor_valid = false;*/
}


DFHANDLER(warpring)
{
	switch(((char *)action)[0]) {
		case 'n':
			WarpAlongRing(&eventp->xbutton, true);
			break;
		case 'p':
			WarpAlongRing(&eventp->xbutton, false);
			break;
		case 's':
			StartWindowStack();
			break;
		case 'f':
			FinishWindowStack();
			break;
		default:
			XBell(dpy, 0);
			break;
	}
}


/*
 * Synthetic function: this is used internally as the action in some
 * magic menus like the TwmWindows et al.
 */
DFHANDLER(winwarp)
{
	tmp_win = (TwmWindow *)action;

	if(! tmp_win) {
		return;
	}
	if(Scr->WarpUnmapped || tmp_win->mapped) {
		if(!tmp_win->mapped) {
			DeIconify(tmp_win);
		}
		WarpToWindow(tmp_win, Scr->RaiseOnWarp);
	}
}


/*
 * Backend util for f.warpring
 */
static void
WarpAlongRing(XButtonEvent *ev, bool forward)
{
	TwmWindow *r, *head;

	if(Scr->RingLeader) {
		head = Scr->RingLeader;
	}
	else if(!(head = Scr->Ring)) {
		return;
	}

	if(forward) {
		for(r = head->ring.next; r != head; r = r->ring.next) {
			if(r->mapped && (Scr->WarpRingAnyWhere || visible(r))) {
				break;
			}
		}
	}
	else {
		for(r = head->ring.prev; r != head; r = r->ring.prev) {
			if(r->mapped && (Scr->WarpRingAnyWhere || visible(r))) {
				break;
			}
		}
	}

	/*
	 * Note: (Scr->Focus == NULL) is necessary when we move to (or
	 * are in) a workspace that has a single window, and we're not
	 * on that window (but the window is head), and we want f.warpring
	 * to warp to it.
	 * Generalised that is also true if we are on a window but it is
	 * not on the ring.
	 * TODO: on an empty screen, it still moves the mouse cursor...
	 */

	if(r != head
	                || Scr->Focus == NULL
	                || !WindowIsOnRing(Scr->Focus)) {
		TwmWindow *p = Scr->RingLeader, *t;

		printf("WarpAlongRing to %p (new RingLeader)\n", r);
		Scr->RingLeader = r;
		WarpToWindow(r, true);

		if(p && p->mapped &&
		                (t = GetTwmWindow(ev->window)) &&
		                p == t) {
			p->ring.cursor_valid = true;
			p->ring.curs_x = ev->x_root - t->frame_x;
			p->ring.curs_y = ev->y_root - t->frame_y;
#ifdef DEBUG
			/* XXX This is the Tmp_win [now] internal to the event code? */
			fprintf(stderr,
			        "WarpAlongRing: cursor_valid := true; x := %d (%d-%d), y := %d (%d-%d)\n",
			        Tmp_win->ring.curs_x, ev->x_root, t->frame_x, Tmp_win->ring.curs_y, ev->y_root,
			        t->frame_y);
#endif
			/*
			 * The check if the cursor position is inside the window is now
			 * done in WarpToWindow().
			 */
		}
	}
}

/*
 * Functions to help with creating "window stack" from the window ring.
 * Principle of operation:
 * - on Alt-down (assuming Alt-Tab), call StartWindowStack()
 *   through a mapping:   "Alt_L" = : all : f.warpring "startstack"
 * - When typing Tab while Alt is still down, call WarpAlongRing() as usual
 *   with the usual mapping "Tab" = alt : all : f.warpring "forward"
 * - On Alt-up, call FinishWindowStack() through a mapping
 *   "Alt_L" = up : all : f.warpring "finishstack"
 *   This moves the window we landed on to the "top" of the stack (ring).
 */
static void
StartWindowStack(void)
{
	/* Reset the ring to the "fixed" place of the bottom of the stack */
	Scr->RingLeader = Scr->Ring;
	Scr->BottomOfStack = Scr->RingLeader;
	printf("StartWindowStack: set BottomOfStack to %p\n", Scr->BottomOfStack);
	/*
	 * The situation is now as follows.
	 * Window 1 is the currently active window, as indicated by RingLeader.
	 * The first f.warpring "next" goes to window 2, the TopOfStack.
	 * No explicit pointer is needed since it is by definition BOT->next.
	 *
	 * window     1  | 2   3   4   5   ...
	 * --------------+---+---+---+---+------
	 * pointers  RL  |
	 *           BOT |TOP
	 *           Ring|
	 */
}

static void
FinishWindowStack(void)
{
	/*
	 * Check that StartWindowStack() was called before;
	 * that WarpAlongRing() has been called at least once;
	 * that there are at least 2 windows in the ring.
	 */
	TwmWindow *tomove = Scr->RingLeader;
	printf("FinishWindowStack: BottomOfStack=%p, RingLeader=%p\n",
	       Scr->BottomOfStack, Scr->RingLeader);

	if(!Scr->BottomOfStack) {
		printf("BottomOfStack is NULL\n");
		return;
	}
	if(!Scr->RingLeader) {
		printf("RingLeader is NULL\n");
		Scr->BottomOfStack = NULL;
		return;
	}
	/*
	 * Did we actually move to another window?
	 */
	if(Scr->RingLeader == Scr->BottomOfStack) {
		printf("BottomOfStack is the same as RingLeader\n");
		Scr->BottomOfStack = NULL;
		return;
	}
	/*
	 * These used to be equal. If Ring is removed from the ring, it gets
	 * pointed to another window (or NULL).  So if they're not equal now,
	 * BottomOfStack is a dangling pointer.
	 */
	if(Scr->Ring != Scr->BottomOfStack) {
		printf("BottomOfStack is no longer equal to Ring\n");
		Scr->BottomOfStack = NULL;
		return;
	}
	/*
	 * If a window's next pointer points to itself, it must be the only
	 * window. In that case there is nothing to do.
	 */
	if(Scr->RingLeader->ring.next == Scr->RingLeader) {
		printf("RingLeader -> next is same as RingLeader: only 1 window\n");
		Scr->BottomOfStack = NULL;
		return;
	}
	/*
	 * If the window to move is already where we want to move it,
	 * there is nothing to do. Probably due to not enough windows.
	 */
	if(Scr->BottomOfStack->ring.prev == tomove) {
		printf("RingLeader already before BottomOfStack\n");
		Scr->BottomOfStack = NULL;
		return;
	}

	/*
	 * The situation after WarpAlongRing() is now as follows:
	 *
	 * window     1  | 2   3   4   5   ...
	 * --------------+---+---+---+---+------
	 * pointers      |        RL
	 *           BOT |TOP
	 *           Ring|
	 */
	/*
	 * Remove RingLeader from its place in the ring,
	 * and put it before BottomOfStack.
	 */
	UnlinkWindowFromRing(tomove);
	AddWindowToRingUnchecked(tomove, Scr->BottomOfStack->ring.prev);
	Scr->RingLeader = tomove;       /* not really necessary */
	Scr->Ring = tomove;
	/*
	 * The situation after WarpAlongRing() is now as follows:
	 *
	 * window     4  | 1   2   3   5   ...
	 * --------------+---+---+---+---+------
	 * pointers  RL  |(BOT)
	 *           Ring|TOP
	 */
	Scr->BottomOfStack = NULL;      /* no worries about dangling pointers */
}
