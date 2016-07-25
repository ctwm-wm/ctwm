/*
 * Occupation handling bits
 */

#include "ctwm.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xatom.h>

#include "add_window.h"
#include "ctwm_atoms.h"
#include "drawing.h"
#include "events.h"
#include "ewmh.h"
#include "screen.h"
#include "otp.h"
#include "util.h"

// Gotten via screen.h
//#include "occupation.h"


static int GetMaskFromResource(TwmWindow *win, char *res);
static char *mk_nullsep_string(const char *prop, int len);

static bool CanChangeOccupation(TwmWindow **twm_winp);

int fullOccupation = 0;

/*
 * The window whose occupation is currently being manipulated.
 *
 * XXX Should probably be static, but currently needed in
 * WMapDestroyWindow().  Revisit.
 */
TwmWindow *occupyWin = NULL;


static XrmOptionDescRec table [] = {
	{"-xrm",            NULL,           XrmoptionResArg, (XPointer) NULL},
};



/*
 * Setup the occupation of a TwmWindow.  Called as part of the
 * AddWindow() process.
 */
void
SetupOccupation(TwmWindow *twm_win, int occupation_hint)
{
	char      **cliargv = NULL;
	int       cliargc;
	WorkSpace *ws;

	/* If there aren't any config'd workspaces, there's only 0 */
	if(! Scr->workSpaceManagerActive) {
		twm_win->occupation = 1 << 0;   /* occupy workspace #0 */
		/* more?... */

		return;
	}

	/* Workspace manager window doesn't get futzed with */
	if(twm_win->iswspmgr) {
		return;
	}

	/*twm_win->occupation = twm_win->iswinbox ? fullOccupation : 0;*/
	twm_win->occupation = 0;

	/* Specified in any Occupy{} config params? */
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		if(LookInList(ws->clientlist, twm_win->full_name, &twm_win->class)) {
			twm_win->occupation |= 1 << ws->number;
		}
	}

	/* OccupyAll{} */
	if(LookInList(Scr->OccupyAll, twm_win->full_name, &twm_win->class)) {
		twm_win->occupation = fullOccupation;
	}

	/* See if it specified in -xrm stuff */
	if(XGetCommand(dpy, twm_win->w, &cliargv, &cliargc)) {
		Bool status;
		char *str_type;
		XrmValue value;
		XrmDatabase db = NULL;

		XrmParseCommand(&db, table, 1, "ctwm", &cliargc, cliargv);
		status = XrmGetResource(db, "ctwm.workspace", "Ctwm.Workspace",
		                        &str_type, &value);
		if((status == True) && (value.size != 0)) {
			/* Copy the value.addr because it's in XRM memory not ours */
			char wrkSpcList[512];
			safe_strncpy(wrkSpcList, value.addr, MIN(value.size, 512));

			twm_win->occupation = GetMaskFromResource(twm_win, value.addr);
		}
		XrmDestroyDatabase(db);
		XFreeStringList(cliargv);
	}

	/* Does it have a property telling us */
	if(RestartPreviousState) {
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytesafter;
		unsigned char *prop;

		if(XGetWindowProperty(dpy, twm_win->w, XA_WM_OCCUPATION, 0L, 2500, False,
		                      XA_STRING, &actual_type, &actual_format, &nitems,
		                      &bytesafter, &prop) == Success) {
			if(nitems != 0) {
				twm_win->occupation = GetMaskFromProperty(prop, nitems);
				XFree(prop);
			}
		}
	}

#ifdef EWMH
	/* Maybe EWMH has something to tell us? */
	if(twm_win->occupation == 0) {
		twm_win->occupation = EwmhGetOccupation(twm_win);
	}
#endif /* EWMH */

	/* Icon Managers shouldn't get altered */
	/* XXX Should this be up near the top? */
	if(twm_win->isiconmgr) {
		return;        /* someone tried to modify occupation of icon managers */
	}


	/*
	 * Transient-ish things go with their parents unless
	 * TransientHasOccupation set in the config.
	 */
	if(! Scr->TransientHasOccupation) {
		TwmWindow *t;

		if(twm_win->istransient) {
			t = GetTwmWindow(twm_win->transientfor);
			if(t != NULL) {
				twm_win->occupation = t->occupation;
			}
		}
		else if(twm_win->group != 0) {
			t = GetTwmWindow(twm_win->group);
			if(t != NULL) {
				twm_win->occupation = t->occupation;
			}
		}
	}


	/* If we were told something specific, go with that */
	if(occupation_hint != 0) {
		twm_win->occupation = occupation_hint;
	}

	/* If it's apparently-nonsensical, put it in its vs's workspace */
	if((twm_win->occupation & fullOccupation) == 0) {
		twm_win->occupation = 1 << twm_win->vs->wsw->currentwspc->number;
	}

	/*
	 * If the occupation would not show it in the current vscreen,
	 * make it vanish.
	 *
	 * If it could be shown in one of the other vscreens, change the vscreen.
	 */
	if(!OCCUPY(twm_win, twm_win->vs->wsw->currentwspc)) {
		VirtualScreen *vs;

		twm_win->vs = NULL;

		if(Scr->numVscreens > 1) {
			for(vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
				if(OCCUPY(twm_win, vs->wsw->currentwspc)) {
					twm_win->vs = vs;
					twm_win->parent_vs = vs;
					break;
				}
			}
		}
	}


	/*
	 * Set the property for the occupation.  Mask off getting
	 * PropertyChange events around that; we're changing it, we don't
	 * need X telling us it's changing!
	 *
	 * XXX should probably make a util func for that process...
	 */
	{
		XWindowAttributes winattrs;
		long eventMask;
		char *wsstr;
		int  len;

		if(!XGetWindowAttributes(dpy, twm_win->w, &winattrs)) {
			/* Window is horked, not much we can do */
			return;
		}
		eventMask = winattrs.your_event_mask;
		XSelectInput(dpy, twm_win->w, eventMask & ~PropertyChangeMask);

		/* Set the property for the occupation */
		len = GetPropertyFromMask(twm_win->occupation, &wsstr);
		XChangeProperty(dpy, twm_win->w, XA_WM_OCCUPATION, XA_STRING, 8,
		                PropModeReplace, (unsigned char *) wsstr, len);
		free(wsstr);

#ifdef EWMH
		EwmhSet_NET_WM_DESKTOP(twm_win);
#endif

		/* Restore event mask */
		XSelectInput(dpy, twm_win->w, eventMask);
	}

	/* Set WM_STATE prop */
	{
		int state = NormalState;
		Window icon;

		if(!(RestartPreviousState
		                && GetWMState(twm_win->w, &state, &icon)
		                && (state == NormalState || state == IconicState
		                    || state == InactiveState))) {
			if(twm_win->wmhints && (twm_win->wmhints->flags & StateHint)) {
				state = twm_win->wmhints->initial_state;
			}
		}
		if(visible(twm_win)) {
			if(state == InactiveState) {
				SetMapStateProp(twm_win, NormalState);
			}
		}
		else {
			if(state == NormalState) {
				SetMapStateProp(twm_win, InactiveState);
			}
		}
	}
}


static bool
CanChangeOccupation(TwmWindow **twm_winp)
{
	TwmWindow *twm_win;

	if(!Scr->workSpaceManagerActive) {
		return false;
	}
	if(occupyWin != NULL) {
		return false;
	}
	twm_win = *twm_winp;
	if(twm_win->isiconmgr) {
		return false;
	}
	if(!Scr->TransientHasOccupation) {
		if(twm_win->istransient) {
			return false;
		}
		if(twm_win->group != (Window) 0 && twm_win->group != twm_win->w) {
			/*
			 * When trying to modify a group member window,
			 * operate on the group leader instead
			 * (and thereby on all group member windows as well).
			 * If we can't find the group leader, pretend it isn't set.
			 */
			twm_win = GetTwmWindow(twm_win->group);
			if(!twm_win) {
				return true;
			}
			*twm_winp = twm_win;
		}
	}
	return true;
}


void
Occupy(TwmWindow *twm_win)
{
	int          x, y;
	unsigned int width, height;
	int          xoffset, yoffset;
	Window       w;
	struct OccupyWindow    *occupyWindow;
	TwmWindow *occupy_twm;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}

	/* Grab our one screen-wide f.occupy window */
	occupyWindow = Scr->workSpaceMgr.occupyWindow;
	occupyWindow->tmpOccupation = twm_win->occupation;
	w = occupyWindow->w;

	/* Figure where to put it so it's centered on the cursor */
	XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY, &width, &height,
	             &JunkBW, &JunkDepth);
	XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkRoot, &JunkX, &JunkY,
	              &x, &y, &JunkMask);
	x -= (width  / 2);
	y -= (height / 2);
	if(x < 0) {
		x = 0;
	}
	if(y < 0) {
		y = 0;
	}
	xoffset = width  + 2 * Scr->BorderWidth;
	yoffset = height + 2 * Scr->BorderWidth + Scr->TitleHeight;

	/* ... (but not off the screen!) */
	if((x + xoffset) > Scr->rootw) {
		x = Scr->rootw - xoffset;
	}
	if((y + yoffset) > Scr->rooth) {
		y = Scr->rooth - yoffset;
	}

	occupy_twm = occupyWindow->twm_win;
	occupy_twm->occupation = twm_win->occupation;

	/* Move the occupy window to where it should be */
	if(occupy_twm->parent_vs != twm_win->parent_vs) {
		occupy_twm->vs = twm_win->parent_vs;
		occupy_twm->frame_x = x;
		occupy_twm->frame_y = y;
		ReparentFrameAndIcon(occupy_twm);
	}
	else {
		XMoveWindow(dpy, occupyWindow->twm_win->frame, x, y);
	}

	/* And show it */
	SetMapStateProp(occupy_twm, NormalState);
	XMapWindow(dpy, occupyWindow->w);
	XMapWindow(dpy, occupy_twm->frame);

	/* XXX Must be a better way to express "all the way on top" */
	OtpSetPriority(occupy_twm, WinWin, 0, Above);

	/* Mark it shown, and stash what window we're showing it for */
	occupyWindow->twm_win->mapped = true;
	occupyWin = twm_win;
}


void
OccupyHandleButtonEvent(XEvent *event)
{
	WorkSpace    *ws;
	OccupyWindow *occupyW;
	Window       buttonW;

	if(! Scr->workSpaceManagerActive) {
		return;
	}
	if(occupyWin == NULL) {
		return;
	}

	buttonW = event->xbutton.window;
	if(buttonW == 0) {
		return;        /* icon */
	}

	XGrabPointer(dpy, Scr->Root, True,
	             ButtonPressMask | ButtonReleaseMask,
	             GrabModeAsync, GrabModeAsync,
	             Scr->Root, None, CurrentTime);

	occupyW = Scr->workSpaceMgr.occupyWindow;
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		if(occupyW->obuttonw [ws->number] == buttonW) {
			break;
		}
	}
	if(ws != NULL) {
		int mask = 1 << ws->number;
		if((occupyW->tmpOccupation & mask) == 0) {
			PaintWsButton(OCCUPYWINDOW, NULL, occupyW->obuttonw [ws->number],
			              ws->label, ws->cp, on);
		}
		else {
			PaintWsButton(OCCUPYWINDOW, NULL, occupyW->obuttonw [ws->number],
			              ws->label, ws->cp, off);
		}
		occupyW->tmpOccupation ^= mask;
	}
	else if(buttonW == occupyW->OK) {
		if(occupyW->tmpOccupation == 0) {
			return;
		}
		ChangeOccupation(occupyWin, occupyW->tmpOccupation);
		XUnmapWindow(dpy, occupyW->twm_win->frame);
		occupyW->twm_win->mapped = false;
		occupyW->twm_win->occupation = 0;
		occupyWin = NULL;
		XSync(dpy, 0);
	}
	else if(buttonW == occupyW->cancel) {
		XUnmapWindow(dpy, occupyW->twm_win->frame);
		occupyW->twm_win->mapped = false;
		occupyW->twm_win->occupation = 0;
		occupyWin = NULL;
		XSync(dpy, 0);
	}
	else if(buttonW == occupyW->allworkspc) {
		for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
			PaintWsButton(OCCUPYWINDOW, NULL, occupyW->obuttonw [ws->number],
			              ws->label, ws->cp, on);
		}
		occupyW->tmpOccupation = fullOccupation;
	}
	if(ButtonPressed == -1) {
		XUngrabPointer(dpy, CurrentTime);
	}
}


void
OccupyAll(TwmWindow *twm_win)
{
	IconMgr *save;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}
	save = Scr->iconmgr;
	Scr->iconmgr = Scr->workSpaceMgr.workSpaceList->iconmgr;
	ChangeOccupation(twm_win, fullOccupation);
	Scr->iconmgr = save;
}


void
AddToWorkSpace(char *wname, TwmWindow *twm_win)
{
	WorkSpace *ws;
	int newoccupation;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}
	ws = GetWorkspace(wname);
	if(!ws) {
		return;
	}

	if(twm_win->occupation & (1 << ws->number)) {
		return;
	}
	newoccupation = twm_win->occupation | (1 << ws->number);
	ChangeOccupation(twm_win, newoccupation);
}


void
RemoveFromWorkSpace(char *wname, TwmWindow *twm_win)
{
	WorkSpace *ws;
	int newoccupation;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}
	ws = GetWorkspace(wname);
	if(!ws) {
		return;
	}

	newoccupation = twm_win->occupation & ~(1 << ws->number);
	if(!newoccupation) {
		return;
	}
	ChangeOccupation(twm_win, newoccupation);
}


void
ToggleOccupation(char *wname, TwmWindow *twm_win)
{
	WorkSpace *ws;
	int newoccupation;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}
	ws = GetWorkspace(wname);
	if(!ws) {
		return;
	}

	newoccupation = twm_win->occupation ^ (1 << ws->number);
	if(!newoccupation) {
		return;
	}
	ChangeOccupation(twm_win, newoccupation);
}


void
MoveToNextWorkSpace(VirtualScreen *vs, TwmWindow *twm_win)
{
	WorkSpace *wlist1, *wlist2;
	int newoccupation;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}

	wlist1 = vs->wsw->currentwspc;
	wlist2 = wlist1->next;
	wlist2 = wlist2 ? wlist2 : Scr->workSpaceMgr.workSpaceList;

	newoccupation = (twm_win->occupation ^ (1 << wlist1->number))
	                | (1 << wlist2->number);
	ChangeOccupation(twm_win, newoccupation);
}


void
MoveToNextWorkSpaceAndFollow(VirtualScreen *vs, TwmWindow *twm_win)
{
	if(!CanChangeOccupation(&twm_win)) {
		return;
	}

	MoveToNextWorkSpace(vs, twm_win);
	GotoNextWorkSpace(vs);
#if 0
	OtpRaise(twm_win, WinWin);  /* XXX really do this? */
#endif
}


void
MoveToPrevWorkSpace(VirtualScreen *vs, TwmWindow *twm_win)
{
	WorkSpace *wlist1, *wlist2;
	int newoccupation;

	if(!CanChangeOccupation(&twm_win)) {
		return;
	}

	wlist1 = Scr->workSpaceMgr.workSpaceList;
	wlist2 = vs->wsw->currentwspc;
	if(wlist1 == NULL) {
		return;
	}

	while(wlist1->next != wlist2 && wlist1->next != NULL) {
		wlist1 = wlist1->next;
	}

	newoccupation = (twm_win->occupation ^ (1 << wlist2->number))
	                | (1 << wlist1->number);
	ChangeOccupation(twm_win, newoccupation);
}


void
MoveToPrevWorkSpaceAndFollow(VirtualScreen *vs, TwmWindow *twm_win)
{
	if(!CanChangeOccupation(&twm_win)) {
		return;
	}

	MoveToPrevWorkSpace(vs, twm_win);
	GotoPrevWorkSpace(vs);
#if 0
	OtpRaise(twm_win, WinWin);          /* XXX really do this? */
#endif
}


void
ChangeOccupation(TwmWindow *tmp_win, int newoccupation)
{
	TwmWindow *t;
	WorkSpace *ws;
	int oldoccupation;
	long eventMask;
	int changedoccupation;

	if((newoccupation == 0)
	                ||  /* in case the property has been broken by another client */
	                (newoccupation == tmp_win->occupation)) {
		char *namelist;
		int  len;
		XWindowAttributes winattrs;

		XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
		eventMask = winattrs.your_event_mask;
		XSelectInput(dpy, tmp_win->w, eventMask & ~PropertyChangeMask);

		len = GetPropertyFromMask(tmp_win->occupation, &namelist);
		XChangeProperty(dpy, tmp_win->w, XA_WM_OCCUPATION, XA_STRING, 8,
		                PropModeReplace, (unsigned char *) namelist, len);
		free(namelist);
#ifdef EWMH
		EwmhSet_NET_WM_DESKTOP(tmp_win);
#endif
		XSelectInput(dpy, tmp_win->w, eventMask);
		return;
	}
	oldoccupation = tmp_win->occupation;
	tmp_win->occupation = newoccupation & ~oldoccupation;
	AddIconManager(tmp_win);
	tmp_win->occupation = newoccupation;
	RemoveIconManager(tmp_win);

	if(tmp_win->vs && !OCCUPY(tmp_win, tmp_win->vs->wsw->currentwspc)) {
		Vanish(tmp_win->vs, tmp_win);
	}
	/*
	 * Try to find an(other) virtual screen which shows a workspace
	 * where the window has occupation, so that the window can be shown
	 * there now.
	 */
	if(!tmp_win->vs) {
		VirtualScreen *vs;
		for(vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
			if(OCCUPY(tmp_win, vs->wsw->currentwspc)) {
				DisplayWin(vs, tmp_win);
				break;
			}
		}
	}

	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		int mask = 1 << ws->number;
		if(oldoccupation & mask) {
			if(!(newoccupation & mask)) {
				int final_x, final_y;
				RemoveWindowFromRegion(tmp_win);
				if(PlaceWindowInRegion(tmp_win, &final_x, &final_y)) {
					XMoveWindow(dpy, tmp_win->frame, final_x, final_y);
				}
			}
			break;
		}
	}

	{
		XWindowAttributes winattrs;

		XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
		eventMask = winattrs.your_event_mask;
		XSelectInput(dpy, tmp_win->w, eventMask & ~PropertyChangeMask);
	}

	{
		char *namelist;
		int  len;
		len = GetPropertyFromMask(newoccupation, &namelist);
		XChangeProperty(dpy, tmp_win->w, XA_WM_OCCUPATION, XA_STRING, 8,
		                PropModeReplace, (unsigned char *) namelist, len);
		free(namelist);
	}

#ifdef EWMH
	EwmhSet_NET_WM_DESKTOP(tmp_win);
#endif
	XSelectInput(dpy, tmp_win->w, eventMask);

	if(!WMapWindowMayBeAdded(tmp_win)) {
		newoccupation = 0;
	}
	if(Scr->workSpaceMgr.noshowoccupyall) {
		/* We can safely change new/oldoccupation here, it's only used
		 * for WMapAddToList()/WMapRemoveFromList() from here on.
		 */
		/* if (newoccupation == fullOccupation)
		    newoccupation = 0; */
		if(oldoccupation == fullOccupation) {
			oldoccupation = 0;
		}
	}
	changedoccupation = oldoccupation ^ newoccupation;
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		int mask = 1 << ws->number;
		if(changedoccupation & mask) {
			if(newoccupation & mask) {
				WMapAddToList(tmp_win, ws);
			}
			else {
				WMapRemoveFromList(tmp_win, ws);
				if(Scr->SaveWorkspaceFocus && ws->save_focus == tmp_win) {
					ws->save_focus = NULL;
				}
			}
		}
	}

	if(! Scr->TransientHasOccupation) {
		for(t = Scr->FirstWindow; t != NULL; t = t->next) {
			if(t != tmp_win &&
			                ((t->istransient && t->transientfor == tmp_win->w) ||
			                 t->group == tmp_win->w)) {
				ChangeOccupation(t, tmp_win->occupation);
			}
		}
	}
}


void
WmgrRedoOccupation(TwmWindow *win)
{
	WorkSpace *ws;
	int       newoccupation;

	if(LookInList(Scr->OccupyAll, win->full_name, &win->class)) {
		newoccupation = fullOccupation;
	}
	else {
		newoccupation = 0;
		for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
			if(LookInList(ws->clientlist, win->full_name, &win->class)) {
				newoccupation |= 1 << ws->number;
			}
		}
	}
	if(newoccupation != 0) {
		ChangeOccupation(win, newoccupation);
	}
}


void
WMgrRemoveFromCurrentWorkSpace(VirtualScreen *vs, TwmWindow *win)
{
	WorkSpace *ws;
	int       newoccupation;

	ws = vs->wsw->currentwspc;
	if(!ws) {
		/* Impossible? */
		return;
	}
	if(! OCCUPY(win, ws)) {
		return;
	}

	newoccupation = win->occupation & ~(1 << ws->number);
	if(newoccupation == 0) {
		return;
	}

	ChangeOccupation(win, newoccupation);
}


void
WMgrAddToCurrentWorkSpaceAndWarp(VirtualScreen *vs, char *winname)
{
	TwmWindow *tw;
	int       newoccupation;

	for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
		if(match(winname, tw->full_name)) {
			break;
		}
	}
	if(!tw) {
		for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
			if(match(winname, tw->class.res_name)) {
				break;
			}
		}
		if(!tw) {
			for(tw = Scr->FirstWindow; tw != NULL; tw = tw->next) {
				if(match(winname, tw->class.res_class)) {
					break;
				}
			}
		}
	}
	if(!tw) {
		XBell(dpy, 0);
		return;
	}
	if((! Scr->WarpUnmapped) && (! tw->mapped)) {
		XBell(dpy, 0);
		return;
	}
	if(! OCCUPY(tw, vs->wsw->currentwspc)) {
		newoccupation = tw->occupation | (1 << vs->wsw->currentwspc->number);
		ChangeOccupation(tw, newoccupation);
	}

	if(! tw->mapped) {
		DeIconify(tw);
	}
	WarpToWindow(tw, Scr->RaiseOnWarp);
}



static ColorPair occupyButtoncp;

static char *ok_string         = "OK",
             *cancel_string     = "Cancel",
              *everywhere_string = "All";

/*
 * Create the Occupy window. Do not do the layout of the parts, only
 * calculate the initial total size. For the layout, call
 * ResizeOccupyWindow() at the end.
 * There is only one Occupy window (per Screen), it is reparented to each
 * virtual screen as needed.
 */
void
CreateOccupyWindow(void)
{
	int           width, height, lines, columns;
	int           bwidth, bheight, owidth, oheight, hspace, vspace;
	int           min_bwidth, min_width;
	int           i, j;
	Window        w, OK, cancel, allworkspc;
	char          *name, *icon_name;
	ColorPair     cp;
	TwmWindow     *tmp_win;
	WorkSpace     *ws;
	XSizeHints    sizehints;
	XWMHints      wmhints;
	MyFont        font;
	XSetWindowAttributes attr;
	XWindowAttributes    wattr;
	unsigned long attrmask;
	OccupyWindow  *occwin;
	VirtualScreen *vs;
	XRectangle inc_rect;
	XRectangle logical_rect;
	int Dummy = 1;

	occwin = Scr->workSpaceMgr.occupyWindow;
	occwin->font     = Scr->IconManagerFont;
	occwin->cp       = Scr->IconManagerC;
#ifdef COLOR_BLIND_USER
	occwin->cp.shadc = Scr->White;
	occwin->cp.shadd = Scr->Black;
#else
	if(!Scr->BeNiceToColormap) {
		GetShadeColors(&occwin->cp);
	}
#endif
	vs        = Scr->vScreenList;
	name      = occwin->name;
	icon_name = occwin->icon_name;
	lines     = Scr->workSpaceMgr.lines;
	columns   = Scr->workSpaceMgr.columns;
	bwidth    = vs->wsw->bwidth;
	bheight   = vs->wsw->bheight;
	oheight   = bheight;
	vspace    = occwin->vspace;
	hspace    = occwin->hspace;
	cp        = occwin->cp;
	height    = ((bheight + vspace) * lines) + oheight + (2 * vspace);
	font      = occwin->font;
	XmbTextExtents(font.font_set, ok_string, strlen(ok_string),
	               &inc_rect, &logical_rect);
	min_bwidth = logical_rect.width;
	XmbTextExtents(font.font_set, cancel_string, strlen(cancel_string),
	               &inc_rect, &logical_rect);
	i = logical_rect.width;
	if(i > min_bwidth) {
		min_bwidth = i;
	}
	XmbTextExtents(font.font_set, everywhere_string, strlen(everywhere_string),
	               &inc_rect, &logical_rect);
	i = logical_rect.width;
	if(i > min_bwidth) {
		min_bwidth = i;
	}
	min_bwidth = (min_bwidth + hspace); /* normal width calculation */
	width = columns * (bwidth  + hspace);
	min_width = 3 * (min_bwidth + hspace); /* width by text width */

	if(columns < 3) {
		owidth = min_bwidth + 2 * Scr->WMgrButtonShadowDepth + 2;
		if(width < min_width) {
			width = min_width;
		}
	}
	else {
		owidth = min_bwidth + 2 * Scr->WMgrButtonShadowDepth + 2;
		width  = columns * (bwidth  + hspace);
	}
	occwin->lines   = lines;
	occwin->columns = columns;
	occwin->owidth  = owidth;

	w = occwin->w = XCreateSimpleWindow(dpy, Scr->Root, 0, 0, width, height,
	                                    1, Scr->Black, cp.back);
	occwin->obuttonw = calloc(Scr->workSpaceMgr.count, sizeof(Window));
	i = 0;
	j = 0;
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		Window bw =
		        occwin->obuttonw [j * columns + i] =
		                XCreateSimpleWindow(dpy, w,
		                                    Dummy /* x */,
		                                    Dummy /* y */,
		                                    Dummy /* width */,
		                                    Dummy /* height */,
		                                    0, Scr->Black, ws->cp.back);
		XMapWindow(dpy, bw);
		i++;
		if(i == columns) {
			i = 0;
			j++;
		}
	}
	GetColor(Scr->Monochrome, &(occupyButtoncp.back), "gray50");
	occupyButtoncp.fore = Scr->White;
	if(!Scr->BeNiceToColormap) {
		GetShadeColors(&occupyButtoncp);
	}

	OK = XCreateSimpleWindow(dpy, w, Dummy, Dummy, Dummy, Dummy, 0,
	                         Scr->Black, occupyButtoncp.back);
	XMapWindow(dpy, OK);
	cancel = XCreateSimpleWindow(dpy, w, Dummy, Dummy, Dummy, Dummy, 0,
	                             Scr->Black, occupyButtoncp.back);
	XMapWindow(dpy, cancel);
	allworkspc = XCreateSimpleWindow(dpy, w, Dummy, Dummy, Dummy, Dummy, 0,
	                                 Scr->Black, occupyButtoncp.back);
	XMapWindow(dpy, allworkspc);

	occwin->OK         = OK;
	occwin->cancel     = cancel;
	occwin->allworkspc = allworkspc;

	sizehints.flags       = PBaseSize | PMinSize | PResizeInc;
	sizehints.base_width  = columns;
	sizehints.base_height = lines;
	sizehints.width_inc   = columns;
	sizehints.height_inc  = lines;
	sizehints.min_width   = 2 * columns;
	sizehints.min_height  = 2 * lines;
	XSetStandardProperties(dpy, w, name, icon_name, None, NULL, 0, &sizehints);

	wmhints.flags         = InputHint | StateHint;
	wmhints.input         = True;
	wmhints.initial_state = NormalState;
	XSetWMHints(dpy, w, &wmhints);
	tmp_win = AddWindow(w, AWT_NORMAL, Scr->iconmgr, Scr->currentvs);
	if(! tmp_win) {
		fprintf(stderr, "cannot create occupy window, exiting...\n");
		exit(1);
	}
	tmp_win->vs = NULL;
	tmp_win->occupation = 0;

	attrmask = 0;
	attr.cursor = Scr->ButtonCursor;
	attrmask |= CWCursor;
	XChangeWindowAttributes(dpy, w, attrmask, &attr);

	XGetWindowAttributes(dpy, w, &wattr);
	attrmask = wattr.your_event_mask | KeyPressMask | KeyReleaseMask | ExposureMask;
	XSelectInput(dpy, w, attrmask);

	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		Window bw = occwin->obuttonw [ws->number];
		XSelectInput(dpy, bw, ButtonPressMask | ButtonReleaseMask | ExposureMask);
		XSaveContext(dpy, bw, TwmContext, (XPointer) tmp_win);
		XSaveContext(dpy, bw, ScreenContext, (XPointer) Scr);
	}
	XSelectInput(dpy, occwin->OK,
	             ButtonPressMask | ButtonReleaseMask | ExposureMask);
	XSaveContext(dpy, occwin->OK, TwmContext, (XPointer) tmp_win);
	XSaveContext(dpy, occwin->OK, ScreenContext, (XPointer) Scr);
	XSelectInput(dpy, occwin->cancel,
	             ButtonPressMask | ButtonReleaseMask | ExposureMask);
	XSaveContext(dpy, occwin->cancel, TwmContext, (XPointer) tmp_win);
	XSaveContext(dpy, occwin->cancel, ScreenContext, (XPointer) Scr);
	XSelectInput(dpy, occwin->allworkspc,
	             ButtonPressMask | ButtonReleaseMask | ExposureMask);
	XSaveContext(dpy, occwin->allworkspc, TwmContext, (XPointer) tmp_win);
	XSaveContext(dpy, occwin->allworkspc, ScreenContext, (XPointer) Scr);

	SetMapStateProp(tmp_win, WithdrawnState);
	occwin->twm_win = tmp_win;
	Scr->workSpaceMgr.occupyWindow = occwin;

	tmp_win->attr.width = width;
	tmp_win->attr.height = height;
	ResizeOccupyWindow(tmp_win);        /* place all parts in the right place */
}


void
PaintOccupyWindow(void)
{
	WorkSpace    *ws;
	OccupyWindow *occwin;
	int          width, height;

	occwin = Scr->workSpaceMgr.occupyWindow;
	width  = occwin->width;
	height = occwin->height;

	Draw3DBorder(occwin->w, 0, 0, width, height, 2, occwin->cp, off, true, false);

	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		Window bw = occwin->obuttonw [ws->number];
		if(occwin->tmpOccupation & (1 << ws->number)) {
			PaintWsButton(OCCUPYWINDOW, NULL, bw, ws->label, ws->cp, on);
		}
		else {
			PaintWsButton(OCCUPYWINDOW, NULL, bw, ws->label, ws->cp, off);
		}
	}
	PaintWsButton(OCCUPYBUTTON, NULL, occwin->OK,         ok_string,
	              occupyButtoncp, off);
	PaintWsButton(OCCUPYBUTTON, NULL, occwin->cancel,     cancel_string,
	              occupyButtoncp, off);
	PaintWsButton(OCCUPYBUTTON, NULL, occwin->allworkspc, everywhere_string,
	              occupyButtoncp, off);
}


/*
 * Turn a ctwm.workspace resource string into an occupation mask.  n.b.;
 * this changes the 'res' arg in-place.
 */
static int
GetMaskFromResource(TwmWindow *win, char *res)
{
	WorkSpace *ws;
	int       mask;
	enum { O_SET, O_ADD, O_REM } mode;
	char *wrkSpcName, *tokst;

	/*
	 * This can set the occupation to a specific set of workspaces ("ws1
	 * ws3"), add to the set it woudl have otherwise ("+ws1 ws3"), or
	 * remove from the set it would otherwise ("-ws1 ws3").  The +/-
	 * apply to the whole expression, not to the individual entries in
	 * it.  So first, figure out what we're doing.
	 */
	mode = O_SET;
	if(*res == '+') {
		mode = O_ADD;
		res++;
	}
	else if(*res == '-') {
		mode = O_REM;
		res++;
	}

	/*
	 * Walk through the string adding the workspaces specified into the
	 * mask of what we're doing.
	 */
	mask = 0;
	for(wrkSpcName = strtok_r(res, " ", &tokst) ; wrkSpcName
	                ; wrkSpcName = strtok_r(NULL, " ", &tokst)) {
		if(strcmp(wrkSpcName, "all") == 0) {
			mask = fullOccupation;
			break;
		}
		if(strcmp(wrkSpcName, "current") == 0) {
			VirtualScreen *vs = Scr->currentvs;
			if(vs) {
				mask |= (1 << vs->wsw->currentwspc->number);
			}
			continue;
		}

		ws = GetWorkspace(wrkSpcName);
		if(ws != NULL) {
			mask |= (1 << ws->number);
		}
		else {
			fprintf(stderr, "unknown workspace : %s\n", wrkSpcName);
		}
	}

	/*
	 * And return that mask, with necessary alterations depending on +/-
	 * specified.
	 */
	switch(mode) {
		case O_SET:
			return (mask);
		case O_ADD:
			return (mask | win->occupation);
		case O_REM:
			return (win->occupation & ~mask);
	}

	/* Can't get here */
	fprintf(stderr, "%s(): Unreachable.\n", __func__);
	return 0;
}


/*
 * Turns a \0-separated buffer of workspace names into an occupation
 * bitmask.
 */
unsigned int
GetMaskFromProperty(unsigned char *_prop, unsigned long len)
{
	char         wrkSpcName [256];
	WorkSpace    *ws;
	unsigned int mask;
	int          l;
	char         *prop;

	mask = 0;
	l = 0;
	prop = (char *) _prop;
	while(l < len) {
		strcpy(wrkSpcName, prop);
		l    += strlen(prop) + 1;
		prop += strlen(prop) + 1;
		if(strcmp(wrkSpcName, "all") == 0) {
			mask = fullOccupation;
			break;
		}

		ws = GetWorkspace(wrkSpcName);
		if(ws != NULL) {
			mask |= (1 << ws->number);
		}
		else {
			fprintf(stderr, "unknown workspace : %s\n", wrkSpcName);
		}
	}

#if 0
	{
		char *dbs = mk_nullsep_string((char *)_prop, len);
		fprintf(stderr, "%s('%s') -> 0x%x\n", __func__, dbs, mask);
		free(dbs);
	}
#endif

	return (mask);
}


/*
 * Turns an occupation mask into a \0-separated buffer (not really a
 * string) of the workspace names.
 */
int
GetPropertyFromMask(unsigned int mask, char **prop)
{
	WorkSpace *ws;
	int       len;
	char      *wss[MAXWORKSPACE];
	int       i;

	/* If it's everything, just say 'all' */
	if(mask == fullOccupation) {
		*prop = strdup("all");
		return 3;
	}

	/* Stash up pointers to all the labels for WSen it's in */
	memset(wss, 0, sizeof(wss));
	i = 0;
	len = 0;
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		if(mask & (1 << ws->number)) {
			wss[i++] = ws->label;
			len += strlen(ws->label) + 1;
		}
	}

	/* Assemble them into \0-separated string */
	*prop = malloc(len);
	len = 0;
	for(i = 0 ; wss[i] != NULL ; i++) {
		strcpy((*prop + len), wss[i]);
		len += strlen(wss[i]) + 1; // Skip past \0
	}

#if 0
	{
		char *dbs = mk_nullsep_string(*prop, len);
		fprintf(stderr, "%s(0x%x) -> %d:'%s'\n", __func__, mask, len, dbs);
		free(dbs);
	}
#endif

	return len;
}


/*
 * Generate a printable variant of the null-separated strings we use for
 * stashing in XA_WM_OCCUPATION.  Used for debugging
 * Get{Property,Mask}From{Mask,Property}().
 */
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
static char *
mk_nullsep_string(const char *prop, int len)
{
	char *dbs;
	int i, j;

	/*
	 * '\0' => "\\0" means we need longer than input; *2 is overkill,
	 * but always sufficient, and it's cheap.
	 */
	dbs = malloc(len * 2);
	i = j = 0;
	while(i < len) {
		size_t slen = strlen(prop + i);

		strcpy(dbs + j, (prop + i));
		i += slen + 1;
		strcpy(dbs + j + slen, "\\0");
		j += slen + 2;
	}

	return dbs;
}
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif


/*
 * Add a client name to a list determining which workspaces it will
 * occupy.  Used in handling the Occupy { } block in config file.
 */
bool
AddToClientsList(char *workspace, char *client)
{
	WorkSpace *ws;

	/* "all" is a magic workspace value which makes it occupy anywhere */
	if(strcmp(workspace, "all") == 0) {
		for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
			AddToList(&ws->clientlist, client, "");
		}
		return true;
	}

	/* If prefixed with "ws:", strip the prefix and lookup by WS name */
	if(strncmp(workspace, "ws:", 3) == 0) {
		if((ws = GetWorkspace(workspace + 3)) != NULL) {
			AddToList(&ws->clientlist, client, "");
			return true;
		}
	}

	/* Else find that named workspace and all this to it */
	if((ws = GetWorkspace(workspace)) != NULL) {
		AddToList(&ws->clientlist, client, "");
		return true;
	}

	/* Couldn't figure where to put it */
	return false;
}



void
ResizeOccupyWindow(TwmWindow *win)
{
	int        bwidth, bheight, owidth, oheight;
	int        hspace, vspace;
	int        lines, columns;
	int        neww, newh;
	WorkSpace  *ws;
	int        i, j, x, y;
	OccupyWindow *occwin = Scr->workSpaceMgr.occupyWindow;

	neww = win->attr.width;
	newh = win->attr.height;
	if(occwin->width == neww && occwin->height == newh) {
		return;
	}

	hspace  = occwin->hspace;
	vspace  = occwin->vspace;
	lines   = Scr->workSpaceMgr.lines;
	columns = Scr->workSpaceMgr.columns;
	bwidth  = (neww -  columns    * hspace) / columns;
	bheight = (newh - (lines + 2) * vspace) / (lines + 1);
	owidth  = occwin->owidth;
	oheight = bheight;

	i = 0;
	j = 0;
	for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
		XMoveResizeWindow(dpy, occwin->obuttonw [j * columns + i],
		                  i * (bwidth  + hspace) + (hspace / 2),
		                  j * (bheight + vspace) + (vspace / 2),
		                  bwidth, bheight);
		i++;
		if(i == columns) {
			i = 0;
			j++;
		}
	}
	hspace = (neww - 3 * owidth) / 4;
	x = hspace;
	y = ((bheight + vspace) * lines) + ((3 * vspace) / 2);
	XMoveResizeWindow(dpy, occwin->OK, x, y, owidth, oheight);
	x += owidth + hspace;
	XMoveResizeWindow(dpy, occwin->cancel, x, y, owidth, oheight);
	x += owidth + hspace;
	XMoveResizeWindow(dpy, occwin->allworkspc, x, y, owidth, oheight);

	occwin->width   = neww;
	occwin->height  = newh;
	occwin->bwidth  = bwidth;
	occwin->bheight = bheight;
	occwin->owidth  = owidth;
	PaintOccupyWindow();
}
