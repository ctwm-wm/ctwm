/*
 * Core event handling
 */
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
 * $XConsortium: events.c,v 1.182 91/07/17 13:59:14 dave Exp $
 *
 * twm event handling
 *
 * 17-Nov-87 Thomas E. LaStrange                File created
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 *
 ***********************************************************************/

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/extensions/shape.h>

#include "add_window.h"
#include "animate.h"
#include "captive.h"
#include "colormaps.h"
#include "events.h"
#include "event_handlers.h"
#include "event_names.h"
#include "functions.h"
#include "iconmgr.h"
#include "image.h"
#include "otp.h"
#include "screen.h"
#include "version.h"
#include "win_iconify.h"
#include "workspace_manager.h"
#ifdef SOUNDS
#include "sound.h"
#endif


static bool StashEventTime(XEvent *ev);
static ScreenInfo *GetTwmScreen(XEvent *event);
static void CtwmNextEvent(Display *display, XEvent  *event);
static ScreenInfo *FindScreenInfo(Window w);
static void dumpevent(XEvent *e);

FILE *tracefile = NULL;

#define MAX_X_EVENT 256
event_proc EventHandler[MAX_X_EVENT]; /* event handler jump table */
int Context = C_NO_CONTEXT;     /* current button press context */
XEvent Event;                   /* the current event */

Window DragWindow;              /* variables used in moving windows */
int origDragX;
int origDragY;
int DragX;
int DragY;
unsigned int DragWidth;
unsigned int DragHeight;
unsigned int DragBW;
int CurrentDragX;
int CurrentDragY;

Time lastTimestamp = CurrentTime;       /* until Xlib does this for us */

/* Maybe more staticizable later? */
bool enter_flag;
bool leave_flag;
TwmWindow *enter_win, *raise_win, *leave_win, *lower_win;
TwmWindow *ButtonWindow; /* button press window structure */

/*
 * Not static because shared with colormaps.c and other events files, but
 * not listed in events.h since nowhere else needs it.
 */
bool ColortableThrashing;
TwmWindow *Tmp_win; // the current twm window; shared with other event code

int ButtonPressed = -1;
bool Cancel = false;


/*#define TRACE_FOCUS*/
/*#define TRACE*/


void AutoRaiseWindow(TwmWindow *tmp)
{
	OtpRaise(tmp, WinWin);

	if(ActiveMenu && ActiveMenu->w) {
		XRaiseWindow(dpy, ActiveMenu->w);
	}
	XSync(dpy, 0);
	enter_win = NULL;
	enter_flag = true;
	raise_win = tmp;
	WMapRaise(tmp);
}

void SetRaiseWindow(TwmWindow *tmp)
{
	enter_flag = true;
	enter_win = NULL;
	raise_win = tmp;
	leave_win = NULL;
	leave_flag = false;
	lower_win = NULL;
	XSync(dpy, 0);
}

void AutoPopupMaybe(TwmWindow *tmp)
{
	if(LookInList(Scr->AutoPopupL, tmp->full_name, &tmp->class)
	                || Scr->AutoPopup) {
		if(OCCUPY(tmp, Scr->currentvs->wsw->currentwspc)) {
			if(!tmp->mapped) {
				DeIconify(tmp);
				SetRaiseWindow(tmp);
			}
		}
		else {
			tmp->mapped = true;
		}
	}
}

void AutoLowerWindow(TwmWindow *tmp)
{
	OtpLower(tmp, WinWin);

	if(ActiveMenu && ActiveMenu->w) {
		XRaiseWindow(dpy, ActiveMenu->w);
	}
	XSync(dpy, 0);
	enter_win = NULL;
	enter_flag = false;
	raise_win = NULL;
	leave_win = NULL;
	leave_flag = true;
	lower_win = tmp;
	WMapLower(tmp);
}


/***********************************************************************
 *
 *  Procedure:
 *      InitEvents - initialize the event jump table
 *
 ***********************************************************************
 */

void InitEvents(void)
{
	int i;


	ResizeWindow = (Window) 0;
	DragWindow = (Window) 0;
	enter_flag = false;
	enter_win = raise_win = NULL;
	leave_flag = false;
	leave_win = lower_win = NULL;

	for(i = 0; i < MAX_X_EVENT; i++) {
		EventHandler[i] = HandleUnknown;
	}

	EventHandler[Expose] = HandleExpose;
	EventHandler[CreateNotify] = HandleCreateNotify;
	EventHandler[DestroyNotify] = HandleDestroyNotify;
	EventHandler[MapRequest] = HandleMapRequest;
	EventHandler[MapNotify] = HandleMapNotify;
	EventHandler[UnmapNotify] = HandleUnmapNotify;
	EventHandler[MotionNotify] = HandleMotionNotify;
	EventHandler[ButtonRelease] = HandleButtonRelease;
	EventHandler[ButtonPress] = HandleButtonPress;
	EventHandler[EnterNotify] = HandleEnterNotify;
	EventHandler[LeaveNotify] = HandleLeaveNotify;
	EventHandler[ConfigureRequest] = HandleConfigureRequest;
	EventHandler[ClientMessage] = HandleClientMessage;
	EventHandler[PropertyNotify] = HandlePropertyNotify;
	EventHandler[KeyPress] = HandleKeyPress;
	EventHandler[KeyRelease] = HandleKeyRelease;
	EventHandler[ColormapNotify] = HandleColormapNotify;
	EventHandler[VisibilityNotify] = HandleVisibilityNotify;
	EventHandler[FocusIn] = HandleFocusChange;
	EventHandler[FocusOut] = HandleFocusChange;
	EventHandler[CirculateNotify] = HandleCirculateNotify;
	if(HasShape) {
		EventHandler[ShapeEventBase + ShapeNotify] = HandleShapeNotify;
	}
#ifdef EWMH
	EventHandler[SelectionClear] = HandleSelectionClear;
#endif
}



static bool
StashEventTime(XEvent *ev)
{
	switch(ev->type) {
		case KeyPress:
		case KeyRelease:
			lastTimestamp = ev->xkey.time;
			return true;
		case ButtonPress:
		case ButtonRelease:
			lastTimestamp = ev->xbutton.time;
			return true;
		case MotionNotify:
			lastTimestamp = ev->xmotion.time;
			return true;
		case EnterNotify:
		case LeaveNotify:
			lastTimestamp = ev->xcrossing.time;
			return true;
		case PropertyNotify:
			lastTimestamp = ev->xproperty.time;
			return true;
		case SelectionClear:
			lastTimestamp = ev->xselectionclear.time;
			return true;
		case SelectionRequest:
			lastTimestamp = ev->xselectionrequest.time;
			return true;
		case SelectionNotify:
			lastTimestamp = ev->xselection.time;
			return true;
	}
	return false;
}


/*
 * WindowOfEvent - return the window about which this event is concerned; this
 * window may not be the same as XEvent.xany.window (the first window listed
 * in the structure).
 */
Window WindowOfEvent(XEvent *e)
{
	/*
	 * Each window subfield is marked with whether or not it is the same as
	 * XEvent.xany.window or is different (which is the case for some of the
	 * notify events).
	 */
	switch(e->type) {
		case KeyPress:
		case KeyRelease:
			return e->xkey.window;                       /* same */
		case ButtonPress:
		case ButtonRelease:
			return e->xbutton.window;                 /* same */
		case MotionNotify:
			return e->xmotion.window;                  /* same */
		case EnterNotify:
		case LeaveNotify:
			return e->xcrossing.window;                 /* same */
		case FocusIn:
		case FocusOut:
			return e->xfocus.window;                       /* same */
		case KeymapNotify:
			return e->xkeymap.window;                  /* same */
		case Expose:
			return e->xexpose.window;                        /* same */
		case GraphicsExpose:
			return e->xgraphicsexpose.drawable;      /* same */
		case NoExpose:
			return e->xnoexpose.drawable;                  /* same */
		case VisibilityNotify:
			return e->xvisibility.window;          /* same */
		case CreateNotify:
			return e->xcreatewindow.window;            /* DIFF */
		case DestroyNotify:
			return e->xdestroywindow.window;          /* DIFF */
		case UnmapNotify:
			return e->xunmap.window;                    /* DIFF */
		case MapNotify:
			return e->xmap.window;                        /* DIFF */
		case MapRequest:
			return e->xmaprequest.window;                /* DIFF */
		case ReparentNotify:
			return e->xreparent.window;              /* DIFF */
		case ConfigureNotify:
			return e->xconfigure.window;            /* DIFF */
		case ConfigureRequest:
			return e->xconfigurerequest.window;    /* DIFF */
		case GravityNotify:
			return e->xgravity.window;                /* DIFF */
		case ResizeRequest:
			return e->xresizerequest.window;          /* same */
		case CirculateNotify:
			return e->xcirculate.window;            /* DIFF */
		case CirculateRequest:
			return e->xcirculaterequest.window;    /* DIFF */
		case PropertyNotify:
			return e->xproperty.window;              /* same */
		case SelectionClear:
			return e->xselectionclear.window;        /* same */
		case SelectionRequest:
			return e->xselectionrequest.requestor;  /* DIFF */
		case SelectionNotify:
			return e->xselection.requestor;         /* same */
		case ColormapNotify:
			return e->xcolormap.window;              /* same */
		case ClientMessage:
			return e->xclient.window;                 /* same */
		case MappingNotify:
			return None;
	}
	return None;
}

void FixRootEvent(XEvent *e)
{
	if(Scr->Root == Scr->RealRoot) {
		return;
	}

	switch(e->type) {
		case KeyPress:
		case KeyRelease:
			e->xkey.x_root -= Scr->rootx;
			e->xkey.y_root -= Scr->rooty;
			e->xkey.root    = Scr->Root;
			break;
		case ButtonPress:
		case ButtonRelease:
			e->xbutton.x_root -= Scr->rootx;
			e->xbutton.y_root -= Scr->rooty;
			e->xbutton.root    = Scr->Root;
			break;
		case MotionNotify:
			e->xmotion.x_root -= Scr->rootx;
			e->xmotion.y_root -= Scr->rooty;
			e->xmotion.root    = Scr->Root;
			break;
		case EnterNotify:
		case LeaveNotify:
			e->xcrossing.x_root -= Scr->rootx;
			e->xcrossing.y_root -= Scr->rooty;
			e->xcrossing.root    = Scr->Root;
			break;
		default:
			break;
	}
}


/* Move this next to GetTwmWindow()? */
static ScreenInfo *GetTwmScreen(XEvent *event)
{
	ScreenInfo *scr;

	if(XFindContext(dpy, event->xany.window, ScreenContext,
	                (XPointer *)&scr) == XCNOENT) {
		scr = FindScreenInfo(WindowOfEvent(event));
	}

	return scr;
}

/***********************************************************************
 *
 *  Procedure:
 *      DispatchEvent2 -
 *      handle a single X event stored in global var Event
 *      this routine for is for a call during an f.move
 *
 ***********************************************************************
 */
bool
DispatchEvent2(void)
{
	Window w = Event.xany.window;
	ScreenInfo *thisScr;

	StashEventTime(&Event);
	Tmp_win = GetTwmWindow(w);
	thisScr = GetTwmScreen(&Event);

	dumpevent(&Event);

	if(!thisScr) {
		return false;
	}
	Scr = thisScr;

	FixRootEvent(&Event);

#ifdef SOUNDS
	play_sound(Event.type);
#endif

	if(menuFromFrameOrWindowOrTitlebar) {
		if(Event.type == Expose) {
			HandleExpose();
		}
	}
	else {
		if(Event.type >= 0 && Event.type < MAX_X_EVENT) {
			(*EventHandler[Event.type])();
		}
	}

	return true;
}

/***********************************************************************
 *
 *  Procedure:
 *      DispatchEvent - handle a single X event stored in global var Event
 *
 ***********************************************************************
 */
bool
DispatchEvent(void)
{
	Window w = Event.xany.window;
	ScreenInfo *thisScr;

	StashEventTime(&Event);
	Tmp_win = GetTwmWindow(w);
	thisScr = GetTwmScreen(&Event);

	dumpevent(&Event);

	if(!thisScr) {
		return false;
	}
	Scr = thisScr;

	if(CLarg.is_captive) {
		if((Event.type == ConfigureNotify)
		                && (Event.xconfigure.window == Scr->CaptiveRoot)) {
			ConfigureCaptiveRootWindow(&Event);
			return false;
		}
	}
	FixRootEvent(&Event);
	if(Event.type >= 0 && Event.type < MAX_X_EVENT) {
#ifdef SOUNDS
		play_sound(Event.type);
#endif
		(*EventHandler[Event.type])();
	}
	return true;
}


/***********************************************************************
 *
 *  Procedure:
 *      HandleEvents - handle X events
 *
 ***********************************************************************
 */

void HandleEvents(void)
{
	while(1) {
		if(enter_flag && !QLength(dpy)) {
			if(enter_win && enter_win != raise_win) {
				AutoRaiseWindow(enter_win);   /* sets enter_flag T */
			}
			else {
				enter_flag = false;
			}
		}
		if(leave_flag && !QLength(dpy)) {
			if(leave_win && leave_win != lower_win) {
				AutoLowerWindow(leave_win);  /* sets leave_flag T */
			}
			else {
				leave_flag = false;
			}
		}
		if(ColortableThrashing && !QLength(dpy) && Scr) {
			InstallColormaps(ColormapNotify, NULL);
		}
		WindowMoved = false;

		CtwmNextEvent(dpy, &Event);

		if(Event.type < 0 || Event.type >= MAX_X_EVENT) {
			XtDispatchEvent(&Event);
		}
		else {
			(void) DispatchEvent();
		}
	}
}


static void CtwmNextEvent(Display *display, XEvent  *event)
{
	int         found;
	fd_set      mask;
	int         fd;
	struct timeval timeout, *tout = NULL;
	const bool animate = (AnimationActive && MaybeAnimate);

#define nextEvent(event) XtAppNextEvent(appContext, event);

	if(RestartFlag) {
		DoRestart(CurrentTime);
	}
	if(XEventsQueued(display, QueuedAfterFlush) != 0) {
		nextEvent(event);
		return;
	}
	fd = ConnectionNumber(display);

	if(animate) {
		TryToAnimate();
	}
	if(RestartFlag) {
		DoRestart(CurrentTime);
	}
	if(! MaybeAnimate) {
		nextEvent(event);
		return;
	}
	if(animate) {
		tout = (AnimationSpeed > 0) ? &timeout : NULL;
	}
	while(1) {
		FD_ZERO(&mask);
		FD_SET(fd, &mask);
		if(animate) {
			timeout = AnimateTimeout;
		}
		found = select(fd + 1, &mask, NULL, NULL, tout);
		if(RestartFlag) {
			DoRestart(CurrentTime);
		}
		if(found < 0) {
			if(errno != EINTR) {
				perror("select");
			}
			continue;
		}
		if(FD_ISSET(fd, &mask)) {
			nextEvent(event);
			return;
		}
		if(found == 0) {
			if(animate) {
				TryToAnimate();
			}
			if(RestartFlag) {
				DoRestart(CurrentTime);
			}
			if(! MaybeAnimate) {
				nextEvent(event);
				return;
			}
			continue;
		}
	}

#undef nextEvent

	/* NOTREACHED */
}

void SynthesiseFocusOut(Window w)
{
	XEvent event;

#ifdef TRACE_FOCUS
	fprintf(stderr, "Synthesizing FocusOut on %x\n", w);
#endif

	event.type = FocusOut;
	event.xfocus.window = w;
	event.xfocus.mode = NotifyNormal;
	event.xfocus.detail = NotifyPointer;

	XPutBackEvent(dpy, &event);
}


void SynthesiseFocusIn(Window w)
{
	XEvent event;

#ifdef TRACE_FOCUS
	fprintf(stderr, "Synthesizing FocusIn on %x\n", w);
#endif

	event.type = FocusIn;
	event.xfocus.window = w;
	event.xfocus.mode = NotifyNormal;
	event.xfocus.detail = NotifyPointer;

	XPutBackEvent(dpy, &event);

}


void SimulateMapRequest(Window w)
{
	Event.xmaprequest.window = w;
	HandleMapRequest();
}


/***********************************************************************
 *
 *  Procedure:
 *      Transient - checks to see if the window is a transient
 *
 *  Returned Value:
 *      true    - window is a transient
 *      false   - window is not a transient
 *
 *  Inputs:
 *      w       - the window to check
 *
 ***********************************************************************
 */

bool
Transient(Window w, Window *propw)
{
	return (bool)XGetTransientForHint(dpy, w, propw);
}


/***********************************************************************
 *
 *  Procedure:
 *      FindScreenInfo - get ScreenInfo struct associated with a given window
 *
 *  Returned Value:
 *      ScreenInfo struct
 *
 *  Inputs:
 *      w       - the window
 *
 ***********************************************************************
 */

static ScreenInfo *
FindScreenInfo(Window w)
{
	XWindowAttributes attr;
	int scrnum;

	attr.screen = NULL;
	if(XGetWindowAttributes(dpy, w, &attr)) {
		for(scrnum = 0; scrnum < NumScreens; scrnum++) {
			if(ScreenList[scrnum] != NULL &&
			                (ScreenOfDisplay(dpy, ScreenList[scrnum]->screen) ==
			                 attr.screen)) {
				return ScreenList[scrnum];
			}
		}
	}

	return NULL;
}


static void dumpevent(XEvent *e)
{
	const char *name = "Unknown event";

	if(! tracefile) {
		return;
	}

	/* Whatsit? */
	name = event_name_by_num(e->type);
	if(!name) {
		name = "Unknown event";
	}

	/* Tell about it */
	fprintf(tracefile, "event:  %s in window 0x%x\n", name,
	        (unsigned int)e->xany.window);
	switch(e->type) {
		case KeyPress:
		case KeyRelease:
			fprintf(tracefile, "     :  +%d,+%d (+%d,+%d)  state=%d, keycode=%d\n",
			        e->xkey.x, e->xkey.y,
			        e->xkey.x_root, e->xkey.y_root,
			        e->xkey.state, e->xkey.keycode);
			break;
		case ButtonPress:
		case ButtonRelease:
			fprintf(tracefile, "     :  +%d,+%d (+%d,+%d)  state=%d, button=%d\n",
			        e->xbutton.x, e->xbutton.y,
			        e->xbutton.x_root, e->xbutton.y_root,
			        e->xbutton.state, e->xbutton.button);
			break;
	}
}
