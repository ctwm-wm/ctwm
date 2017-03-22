/*
 * Copyright 1992, 2005 Stefan Monnier.
 *
 * Author:  Stefan Monnier [ monnier@lia.di.epfl.ch ]
 * Adapted for use with more than one virtual screen by
 * Olaf "Rhialto" Seibert <rhialto@falu.nl>.
 *
 * $Id: otp.c,v 1.9 2005/04/08 16:59:25 monnier Exp $
 *
 * handles all the OnTopPriority-related issues.
 *
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "otp.h"
#include "screen.h"
#include "util.h"
#include "icons.h"
#include "list.h"
#include "events.h"
#include "event_handlers.h"
#include "vscreen.h"

#define DEBUG_OTP       0
#if DEBUG_OTP
#define DPRINTF(x)      fprintf x
#else
#define DPRINTF(x)
#endif

#if defined(NDEBUG)
# define CHECK_OTP      0
#else
# define CHECK_OTP      1
#endif

/* number of priorities known to ctwm: [0..ONTOP_MAX] */
#define OTP_ZERO 8
#define OTP_MAX (OTP_ZERO * 2)

struct OtpWinList {
	OtpWinList *above;
	OtpWinList *below;
	TwmWindow  *twm_win;
	WinType     type;
	bool        switching;
	int         priority;
	int         pri_base;   // Base priority
	unsigned    pri_aflags; // Flags that might alter it; OTP_AFLAG_*
};

struct OtpPreferences {
	name_list  *priorityL[OTP_MAX + 1];
	int         priority;
	name_list  *switchingL;
	bool        switching;
};

typedef struct Box {
	int x;
	int y;
	int width;
	int height;
} Box;


static bool OtpCheckConsistencyVS(VirtualScreen *currentvs, Window vroot);
static int OwlEffectivePriority(OtpWinList *owl);

static OtpWinList *bottomOwl = NULL;

static Box BoxOfOwl(OtpWinList *owl)
{
	Box b;

	switch(owl->type) {
		case IconWin: {
			Icon *icon = owl->twm_win->icon;

			b.x = icon->w_x;
			b.y = icon->w_y;
			b.width = icon->w_width;
			b.height = icon->w_height;
			break;
		}
		case WinWin: {
			TwmWindow *twm_win = owl->twm_win;

			b.x = twm_win->frame_x;
			b.y = twm_win->frame_y;
			b.width = twm_win->frame_width;
			b.height = twm_win->frame_height;
			break;
		}
		default:
			assert(false);
	}
	return b;
}


static bool BoxesIntersect(Box *b1, Box *b2)
{
	bool interX = (b1->x + b1->width > b2->x) && (b2->x + b2->width > b1->x);
	bool interY = (b1->y + b1->height > b2->y) && (b2->y + b2->height > b1->y);

	return (interX && interY);
}


static bool isIntersectingWith(OtpWinList *owl1, OtpWinList *owl2)
{
	Box b1 = BoxOfOwl(owl1);
	Box b2 = BoxOfOwl(owl2);

	return BoxesIntersect(&b1, &b2);
}


static bool isOnScreen(OtpWinList *owl)
{
	TwmWindow *twm_win = owl->twm_win;

	return (((owl->type == IconWin) ? twm_win->iconified : twm_win->mapped)
	        && OCCUPY(twm_win, Scr->currentvs->wsw->currentwspc));
}


bool isTransientOf(TwmWindow *trans, TwmWindow *main)
{
	return (trans->istransient && trans->transientfor == main->w);
}

bool isGroupLeader(TwmWindow *twm_win)
{
	return ((twm_win->group == 0)
	        || (twm_win->group == twm_win->w));
}

bool isGroupLeaderOf(TwmWindow *leader, TwmWindow *twm_win)
{
	return (isGroupLeader(leader)
	        && !isGroupLeader(twm_win)
	        && (leader->group == twm_win->group));
}

bool isSmallTransientOf(TwmWindow *trans, TwmWindow *main)
{
	int trans_area, main_area;

	if(isTransientOf(trans, main)) {
		assert(trans->frame);
		trans_area = trans->frame_width * trans->frame_height;
		main_area = main->frame_width * main->frame_height;

		return (trans_area < ((main_area * Scr->TransientOnTop) / 100));
	}
	else {
		return false;
	}
}

static Window WindowOfOwl(OtpWinList *owl)
{
	return (owl->type == IconWin)
	       ? owl->twm_win->icon->w : owl->twm_win->frame;
}

bool OtpCheckConsistency(void)
{
#if DEBUG_OTP
	VirtualScreen *tvs;
	bool result = true;

	for(tvs = Scr->vScreenList; tvs != NULL; tvs = tvs->next) {
		fprintf(stderr, "OtpCheckConsistencyVS: vs:(x,y)=(%d,%d)\n",
		        tvs->x, tvs->y);
		result = result && OtpCheckConsistencyVS(tvs, tvs->window);
	}
	return result;
#else
	return OtpCheckConsistencyVS(Scr->currentvs, Scr->Root);
#endif
}

static bool OtpCheckConsistencyVS(VirtualScreen *currentvs, Window vroot)
{
#if CHECK_OTP
	OtpWinList *owl;
	TwmWindow *twm_win;
	Window root, parent, *children;
	unsigned int nchildren;
	int priority = 0;
	int stack = -1;
	int nwins = 0;

	XQueryTree(dpy, vroot, &root, &parent, &children, &nchildren);

#if DEBUG_OTP
	{
		int i;
		fprintf(stderr, "XQueryTree: %d children:\n", nchildren);

		for(i = 0; i < nchildren; i++) {
			fprintf(stderr, "[%d]=%x ", i, (unsigned int)children[i]);
		}
		fprintf(stderr, "\n");
	}
#endif

	for(owl = bottomOwl; owl != NULL; owl = owl->above) {
		twm_win = owl->twm_win;

		/* check the back arrows are correct */
		assert(((owl->type == IconWin) && (owl == twm_win->icon->otp))
		       || ((owl->type == WinWin) && (owl == twm_win->otp)));

		/* check the doubly linked list's consistency */
		if(owl->below == NULL) {
			assert(owl == bottomOwl);
		}
		else {
			assert(owl->below->above == owl);
		}

		/* check the ordering of the priority */
		assert(owl->priority <= OTP_MAX);
		assert(owl->priority >= priority);
		priority = owl->priority;

#if DEBUG_OTP

		fprintf(stderr, "checking owl: pri %d w=%x stack=%d",
		        priority, (unsigned int)WindowOfOwl(owl), stack);
		if(twm_win) {
			fprintf(stderr, " title=%s occupation=%x ",
			        twm_win->name,
			        (unsigned int)twm_win->occupation);
			if(owl->twm_win->vs) {
				fprintf(stderr, " vs:(x,y)=(%d,%d)",
				        twm_win->vs->x,
				        twm_win->vs->y);
			}
			else {
				fprintf(stderr, " vs:NULL");
			}
			if(owl->twm_win->parent_vs) {
				fprintf(stderr, " parent_vs:(x,y)=(%d,%d)",
				        twm_win->parent_vs->x,
				        twm_win->parent_vs->y);
			}
			else {
				fprintf(stderr, " parent_vs:NULL");
			}
		}
		fprintf(stderr, " %s\n", (owl->type == WinWin ? "Window" : "Icon"));
#endif

		/* count the number of twm windows */
		if(owl->type == WinWin) {
			nwins++;
		}

		if(twm_win->winbox) {
			/*
			 * We can't check windows in a WindowBox, since they are
			 * not direct children of the Root window.
			 */
			DPRINTF((stderr, "Can't check this window, it is in a WinBox\n"));
			continue;
		}

		/*
		 * Check only windows from the current vitual screen; the others
		 * won't show up in the tree from XQueryTree().
		 */
		if(currentvs == twm_win->parent_vs) {
			/* check the window's existence. */
			Window windowOfOwl = WindowOfOwl(owl);

#if DEBUG_OTP
			int i;
			for(i = 0; i < nchildren && windowOfOwl != children[i];) {
				i++;
			}
			fprintf(stderr, "search for owl in stack -> i=%d\n", i);
			assert(i > stack && "Window not in good place in stack");
			assert(i < nchildren && "Window was not found in stack");
			if(0) {
				char buf[128];
				snprintf(buf, 128, "xwininfo -all -id %d", (int)windowOfOwl);
				system(buf);
			}

			/* we know that this always increases stack (assert i>stack) */
			stack = i;
#else /* DEBUG_OTP */
			/* check against the Xserver's stack */
			do {
				stack++;
				DPRINTF((stderr, "stack++: children[%d] = %x\n", stack,
				         (unsigned int)children[stack]));
				assert(stack < nchildren);
			}
			while(windowOfOwl != children[stack]);
#endif /* DEBUG_OTP */
		}
	}

	XFree(children);

	/* by decrementing nwins, check that all the wins are in our list */
	for(twm_win = Scr->FirstWindow; twm_win != NULL; twm_win = twm_win->next) {
		nwins--;
	}
	/* if we just removed a win, it might still be somewhere, hence the -1 */
	assert((nwins <= 0) && (nwins >= -1));
#endif
	return true;
}


static void RemoveOwl(OtpWinList *owl)
{
	if(owl->above != NULL) {
		owl->above->below = owl->below;
	}
	if(owl->below != NULL) {
		owl->below->above = owl->above;
	}
	else {
		bottomOwl = owl->above;
	}
	owl->below = NULL;
	owl->above = NULL;
}


/**
 * For the purpose of putting a window above another,
 * they need to have the same parent, i.e. be in the same
 * VirtualScreen.
 */
static OtpWinList *GetOwlAtOrBelowInVS(OtpWinList *owl, VirtualScreen *vs)
{
	while(owl != NULL && owl->twm_win->parent_vs != vs) {
		owl = owl->below;
	}

	return owl;
}

/*
 * Windows in a box don't really occur in the stacking order of the
 * root window.
 * In the OWL list, keep them just on top of their box, in their
 * respective order of course.
 * Therefore we may need to update the owl we're going to be above.
 */
static OtpWinList *GetOwlAtOrBelowInWinbox(OtpWinList **owlp, WindowBox *wb)
{
	OtpWinList *owl = *owlp;

	while(owl != NULL && owl->twm_win->winbox != wb) {
		owl = owl->below;
	}

	if(owl == NULL) {
		/* we have gone below the box: put it just on top of it */
		*owlp = wb->twmwin->otp;
	}
	else {
		*owlp = owl;
	}
	return owl;
}


static void InsertOwlAbove(OtpWinList *owl, OtpWinList *other_owl)
{
#if DEBUG_OTP
	fprintf(stderr, "InsertOwlAbove owl->priority=%d w=%x parent_vs:(x,y)=(%d,%d)",
	        owl->priority,
	        (unsigned int)WindowOfOwl(owl),
	        owl->twm_win->parent_vs->x,
	        owl->twm_win->parent_vs->y);
	if(other_owl != NULL) {
		fprintf(stderr, "other_owl->priority=%d w=%x parent_vs:(x,y)=(%d,%d)",
		        other_owl->priority,
		        (unsigned int)WindowOfOwl(other_owl),
		        owl->twm_win->parent_vs->x,
		        owl->twm_win->parent_vs->y);
	}
	fprintf(stderr, "\n");
#endif

	assert(owl->above == NULL);
	assert(owl->below == NULL);


	if(other_owl == NULL) {
		DPRINTF((stderr, "Bottom-most window overall\n"));
		/* special case for the lowest window overall */
		assert(owl->priority <= bottomOwl->priority);

		/* pass the action to the Xserver */
		XLowerWindow(dpy, WindowOfOwl(owl));

		/* update the list */
		owl->above = bottomOwl;
		owl->above->below = owl;
		bottomOwl = owl;
	}
	else {
		WindowBox *winbox = owl->twm_win->winbox;
		OtpWinList *vs_owl;

		if(winbox != NULL) {
			vs_owl = GetOwlAtOrBelowInWinbox(&other_owl, winbox);
		}
		else {

			vs_owl = GetOwlAtOrBelowInVS(other_owl, owl->twm_win->parent_vs);
		}

		assert(owl->priority >= other_owl->priority);
		if(other_owl->above != NULL) {
			assert(owl->priority <= other_owl->above->priority);
		}

		if(vs_owl == NULL) {
			DPRINTF((stderr, "Bottom-most window in VirtualScreen or window box\n"));
			/* special case for the lowest window in this virtual screen or window box */

			/* pass the action to the Xserver */
			XLowerWindow(dpy, WindowOfOwl(owl));
		}
		else {
			XWindowChanges xwc;
			int xwcm;

			DPRINTF((stderr, "General case\n"));
			/* general case */
			assert(vs_owl->priority <= other_owl->priority);
			assert(owl->twm_win->parent_vs == vs_owl->twm_win->parent_vs);

			/* pass the action to the Xserver */
			xwcm = CWStackMode | CWSibling;
			xwc.sibling = WindowOfOwl(vs_owl);
			xwc.stack_mode = Above;
			XConfigureWindow(dpy, WindowOfOwl(owl), xwcm, &xwc);
		}

		/* update the list */
		owl->below = other_owl;
		owl->above = other_owl->above;
		owl->below->above = owl;
		if(owl->above != NULL) {
			owl->above->below = owl;
		}
	}
}


/* should owl stay above other_owl if other_owl was raised ? */
static bool shouldStayAbove(OtpWinList *owl, OtpWinList *other_owl)
{
	return ((owl->type == WinWin)
	        && (other_owl->type == WinWin)
	        && isSmallTransientOf(owl->twm_win, other_owl->twm_win));
}


static void RaiseSmallTransientsOfAbove(OtpWinList *owl, OtpWinList *other_owl)
{
	OtpWinList *trans_owl, *tmp_owl;

	/* the icons have no transients and we can't have windows below NULL */
	if((owl->type != WinWin) || other_owl == NULL) {
		return;
	}

	/* beware: we modify the list as we scan it. This is the reason for tmp */
	for(trans_owl = other_owl->below; trans_owl != NULL; trans_owl = tmp_owl) {
		tmp_owl = trans_owl->below;
		if(shouldStayAbove(trans_owl, owl)) {
			RemoveOwl(trans_owl);
			trans_owl->priority = owl->priority;
			InsertOwlAbove(trans_owl, other_owl);
		}
	}
}


static OtpWinList *OwlRightBelow(int priority)
{
	OtpWinList *owl1, *owl2;

	/* in case there isn't anything below */
	if(priority <= bottomOwl->priority) {
		return NULL;
	}

	for(owl1 = bottomOwl, owl2 = owl1->above;
	                (owl2 != NULL) && (owl2->priority < priority);
	                owl1 = owl2, owl2 = owl2->above);

	assert(owl2 == owl1->above);
	assert(owl1->priority < priority);
	assert((owl2 == NULL) || (owl2->priority >= priority));


	return owl1;
}

static void InsertOwl(OtpWinList *owl, int where)
{
	OtpWinList *other_owl;
	int priority;

	DPRINTF((stderr, "InsertOwl %s\n",
	         (where == Above) ? "Above" :
	         (where == Below) ? "Below" :
	         "???"));
	assert(owl->above == NULL);
	assert(owl->below == NULL);
	assert((where == Above) || (where == Below));

	priority = (where == Above) ? owl->priority : owl->priority - 1;

	if(bottomOwl == NULL) {
		/* for the first window: just insert it in the list */
		bottomOwl = owl;
	}
	else {
		other_owl = OwlRightBelow(priority + 1);

		/* make sure other_owl is not one of the transients */
		while((other_owl != NULL)
		                && shouldStayAbove(other_owl, owl)) {
			other_owl->priority = owl->priority;
			other_owl = other_owl->below;
		}

		/* raise the transient windows that should stay on top */
		RaiseSmallTransientsOfAbove(owl, other_owl);

		/* now go ahead and put the window where it should go */
		InsertOwlAbove(owl, other_owl);
	}
}


static void SetOwlPriority(OtpWinList *owl, int new_pri, int where)
{
	DPRINTF((stderr, "SetOwlPriority(%d)\n", new_pri));

	/* make sure the values are within bounds */
	if(new_pri < 0) {
		new_pri = 0;
	}
	if(new_pri > OTP_MAX) {
		new_pri = OTP_MAX;
	}

	RemoveOwl(owl);
	owl->priority = new_pri;
	InsertOwl(owl, where);

	assert(owl->priority == new_pri);
}


static void TryToMoveTransientsOfTo(OtpWinList *owl, int priority, int where)
{
	OtpWinList *other_owl, *tmp_owl;

	/* the icons have no transients */
	if(owl->type != WinWin) {
		return;
	}

	other_owl = OwlRightBelow(owl->priority);
	other_owl = (other_owl == NULL) ? bottomOwl : other_owl->above;
	assert(other_owl->priority >= owl->priority);

	/* !beware! we're changing the list as we scan it, hence the tmp_owl */
	while((other_owl != NULL) && (other_owl->priority == owl->priority)) {
		tmp_owl = other_owl->above;
		if((other_owl->type == WinWin)
		                && isTransientOf(other_owl->twm_win, owl->twm_win)) {
			SetOwlPriority(other_owl, priority, where);
		}
		other_owl = tmp_owl;
	}
}

static void TryToSwitch(OtpWinList *owl, int where)
{
	int priority;

	if(!owl->switching) {
		return;
	}

	priority = OTP_MAX - owl->priority;
	if(((where == Above) && (priority > owl->priority)) ||
	                ((where == Below) && (priority < owl->priority))) {
		TryToMoveTransientsOfTo(owl, priority, where);
		owl->priority = priority;
	}
}

static void RaiseOwl(OtpWinList *owl)
{
	TryToSwitch(owl, Above);
	RemoveOwl(owl);
	InsertOwl(owl, Above);
}


static void LowerOwl(OtpWinList *owl)
{
	TryToSwitch(owl, Below);
	RemoveOwl(owl);
	InsertOwl(owl, Below);
}

static bool isHiddenBy(OtpWinList *owl, OtpWinList *other_owl)
{
	/* doesn't check that owl is on screen */
	return (isOnScreen(other_owl)
	        && isIntersectingWith(owl, other_owl));
}

static void TinyRaiseOwl(OtpWinList *owl)
{
	OtpWinList *other_owl = owl->above;

	while((other_owl != NULL) && (other_owl->priority == owl->priority)) {
		if(isHiddenBy(owl, other_owl)
		                && !shouldStayAbove(other_owl, owl)) {
			RemoveOwl(owl);
			RaiseSmallTransientsOfAbove(owl, other_owl);
			InsertOwlAbove(owl, other_owl);
			return;
		}
		else {
			other_owl = other_owl->above;
		}
	}
}

static void TinyLowerOwl(OtpWinList *owl)
{
	OtpWinList *other_owl = owl->below;

	while((other_owl != NULL) && (other_owl->priority == owl->priority)) {
		if(isHiddenBy(owl, other_owl)) {
			RemoveOwl(owl);
			InsertOwlAbove(owl, other_owl->below);
			return;
		}
		else {
			other_owl = other_owl->below;
		}
	}
}

static void RaiseLowerOwl(OtpWinList *owl)
{
	OtpWinList *other_owl;
	int priority;

	priority = MAX(owl->priority, OTP_MAX - owl->priority);

	for(other_owl = owl->above;
	                (other_owl != NULL) && (other_owl->priority <= priority);
	                other_owl = other_owl->above) {
		if(isHiddenBy(owl, other_owl)
		                && !shouldStayAbove(other_owl, owl)) {
			RaiseOwl(owl);
			return;
		}
	}
	LowerOwl(owl);
}


void OtpRaise(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	RaiseOwl(owl);

	OtpCheckConsistency();
#ifdef EWMH
	EwmhSet_NET_CLIENT_LIST_STACKING();
#endif /* EWMH */
}


void OtpLower(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	LowerOwl(owl);

	OtpCheckConsistency();
#ifdef EWMH
	EwmhSet_NET_CLIENT_LIST_STACKING();
#endif /* EWMH */
}


void OtpRaiseLower(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	RaiseLowerOwl(owl);

	OtpCheckConsistency();
#ifdef EWMH
	EwmhSet_NET_CLIENT_LIST_STACKING();
#endif /* EWMH */
}


void OtpTinyRaise(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	TinyRaiseOwl(owl);

	OtpCheckConsistency();
#ifdef EWMH
	EwmhSet_NET_CLIENT_LIST_STACKING();
#endif /* EWMH */
}


void OtpTinyLower(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	TinyLowerOwl(owl);

	OtpCheckConsistency();
#ifdef EWMH
	EwmhSet_NET_CLIENT_LIST_STACKING();
#endif /* EWMH */
}


/*
 * XCirculateSubwindows() is complicated by the fact that it restacks only
 * in case of overlapping windows. Therefore it seems easier to not
 * try to emulate that but to leave it to the X server.
 *
 * If XCirculateSubwindows() actually does something, it sends a
 * CirculateNotify event, but you only receive it if
 * SubstructureNotifyMask is selected on the root window.
 * However... if that is done from the beginning, for some reason all
 * windows disappear when ctwm starts or exits.
 * Maybe SubstructureNotifyMask interferes with SubstructureRedirectMask?
 *
 * To get around that, the SubstructureNotifyMask is selected only
 * temporarily here when wanted.
 */

void OtpCirculateSubwindows(VirtualScreen *vs, int direction)
{
	Window w = vs->window;
	XWindowAttributes winattrs;
	Bool circulated;

	DPRINTF((stderr, "OtpCirculateSubwindows %d\n", direction));

	XGetWindowAttributes(dpy, w, &winattrs);
	XSelectInput(dpy, w, winattrs.your_event_mask | SubstructureNotifyMask);
	XCirculateSubwindows(dpy, w, direction);
	XSelectInput(dpy, w, winattrs.your_event_mask);
	/*
	 * Now we should get the CirculateNotify event.
	 * It usually seems to arrive soon enough, but just to be sure, look
	 * ahead in the message queue to see if it can be expedited.
	 */
	circulated = XCheckTypedWindowEvent(dpy, w, CirculateNotify, &Event);
	if(circulated) {
		HandleCirculateNotify();
	}
}

/*
 * Update our list of Owls after the Circulate action, and also
 * enforce the priority by possibly restacking the window again.
 */

void OtpHandleCirculateNotify(VirtualScreen *vs, TwmWindow *twm_win,
                              WinType wintype, int place)
{
	switch(place) {
		case PlaceOnTop:
			OtpRaise(twm_win, wintype);
			break;
		case PlaceOnBottom:
			OtpLower(twm_win, wintype);
			break;
		default:
			DPRINTF((stderr, "OtpHandleCirculateNotify: place=%d\n", place));
			assert(0 &&
			       "OtpHandleCirculateNotify: place equals PlaceOnTop nor PlaceOnBottom");
	}
}

void OtpSetPriority(TwmWindow *twm_win, WinType wintype, int new_pri, int where)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	int priority = OTP_ZERO + new_pri;

	DPRINTF((stderr, "OtpSetPriority: new_pri=%d\n", new_pri));
	assert(owl != NULL);

	if(twm_win->winbox != NULL || twm_win->iswinbox) {
		return;
	}

	if(ABS(new_pri) > OTP_ZERO) {
		DPRINTF((stderr, "invalid OnTopPriority value: %d\n", new_pri));
	}
	else {
		TryToMoveTransientsOfTo(owl, priority, where);
		SetOwlPriority(owl, priority, where);
	}

	OtpCheckConsistency();
}


void OtpChangePriority(TwmWindow *twm_win, WinType wintype, int relpriority)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	int priority = owl->priority + relpriority;
	int where;

	if(twm_win->winbox != NULL || twm_win->iswinbox) {
		return;
	}

	where = relpriority < 0 ? Below : Above;

	TryToMoveTransientsOfTo(owl, priority, where);
	SetOwlPriority(owl, priority, where);

	OtpCheckConsistency();
}


void OtpSwitchPriority(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	int priority = OTP_MAX - owl->priority;
	int where;

	assert(owl != NULL);

	if(twm_win->winbox != NULL || twm_win->iswinbox) {
		return;
	}

	where = priority < OTP_ZERO ? Below : Above;
	TryToMoveTransientsOfTo(owl, priority, where);
	SetOwlPriority(owl, priority, where);

	OtpCheckConsistency();
}


void OtpToggleSwitching(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	assert(owl != NULL);

	if(twm_win->winbox != NULL || twm_win->iswinbox) {
		return;
	}

	owl->switching = !owl->switching;

	OtpCheckConsistency();
}


void OtpForcePlacement(TwmWindow *twm_win, int where, TwmWindow *other_win)
{
	OtpWinList *owl = twm_win->otp;
	OtpWinList *other_owl = other_win->otp;

	assert(twm_win->otp != NULL);
	assert(other_win->otp != NULL);

	if(where == BottomIf) {
		where = Below;
	}
	if(where != Below) {
		where = Above;
	}

	/* remove the owl to change it */
	RemoveOwl(owl);

	/* set the priority to keep the state consistent */
	owl->priority = other_owl->priority;

	/* put the owl back into the list */
	if(where == Below) {
		other_owl = other_owl->below;
	}
	InsertOwlAbove(owl, other_owl);

	OtpCheckConsistency();
}


static void ApplyPreferences(OtpPreferences *prefs, OtpWinList *owl)
{
	int i;
	TwmWindow *twm_win = owl->twm_win;

	/* check PrioritySwitch */
	if(LookInList(prefs->switchingL, twm_win->name, &twm_win->class)) {
		owl->switching = !prefs->switching;
	}

	/* check OnTopPriority */
	for(i = 0; i <= OTP_MAX; i++) {
		if(LookInList(prefs->priorityL[i],
		                twm_win->name, &twm_win->class)) {
			owl->pri_base = i;
		}
	}
}

static void RecomputeOwlValues(OtpPreferences *prefs, OtpWinList *owl)
{
	int old_pri;

	old_pri = owl->priority;
	ApplyPreferences(prefs, owl);
	if(old_pri != owl->priority) {
		RemoveOwl(owl);
		InsertOwl(owl, Above);
	}
}

void OtpRecomputeValues(TwmWindow *twm_win)
{
	assert(twm_win->otp != NULL);

	RecomputeOwlValues(Scr->OTP, twm_win->otp);
	if(twm_win->icon != NULL) {
		RecomputeOwlValues(Scr->IconOTP, twm_win->icon->otp);
	}

	OtpCheckConsistency();
}


static void free_OtpWinList(OtpWinList *owl)
{
	assert(owl->above == NULL);
	assert(owl->below == NULL);
	free(owl);
}


void OtpRemove(TwmWindow *twm_win, WinType wintype)
{
	OtpWinList **owlp;
	owlp = (wintype == IconWin) ? &twm_win->icon->otp : &twm_win->otp;

	assert(*owlp != NULL);

	RemoveOwl(*owlp);
	free_OtpWinList(*owlp);
	*owlp = NULL;

	OtpCheckConsistency();
}


static OtpWinList *new_OtpWinList(TwmWindow *twm_win,
                                  WinType wintype,
                                  bool switching,
                                  int priority)
{
	OtpWinList *owl = malloc(sizeof(OtpWinList));

	owl->above = NULL;
	owl->below = NULL;
	owl->twm_win = twm_win;
	owl->type = wintype;
	owl->switching = switching;
	owl->priority = priority;
	owl->pri_base = priority;
	owl->pri_aflags = 0;

	return owl;
}

static OtpWinList *AddNewOwl(TwmWindow *twm_win, WinType wintype,
                             OtpWinList *parent)
{
	OtpWinList *owl;
	OtpPreferences *prefs = (wintype == IconWin) ? Scr->IconOTP : Scr->OTP;

	/* make the new owl */
	owl = new_OtpWinList(twm_win, wintype,
	                     prefs->switching, prefs->priority);

	/* inherit the default attributes from the parent window if appropriate */
	if(parent != NULL) {
		owl->pri_base = parent->pri_base;
		owl->switching = parent->switching;
	}

	/* now see if the preferences have something to say */
	if(!(parent != NULL && twm_win->istransient)) {
		ApplyPreferences(prefs, owl);
	}

	/* finally put the window where it should go */
	InsertOwl(owl, Above);

	return owl;
}

void OtpAdd(TwmWindow *twm_win, WinType wintype)
{
	TwmWindow *other_win;
	OtpWinList *parent = NULL;
	OtpWinList **owlp;
	owlp = (wintype == IconWin) ? &twm_win->icon->otp : &twm_win->otp;

	assert(*owlp == NULL);

	/* windows in boxes *must* inherit priority from the box */
	if(twm_win->winbox) {
		parent = twm_win->winbox->twmwin->otp;
		parent->switching = false;
	}
	/* in case it's a transient, find the parent */
	else if(wintype == WinWin && (twm_win->istransient
	                              || !isGroupLeader(twm_win))) {
		other_win = Scr->FirstWindow;
		while(other_win != NULL
		                && !isTransientOf(twm_win, other_win)
		                && !isGroupLeaderOf(other_win, twm_win)) {
			other_win = other_win->next;
		}
		if(other_win != NULL) {
			parent = other_win->otp;
		}
	}

	/* make the new owl */
	*owlp = AddNewOwl(twm_win, wintype, parent);

	assert(*owlp != NULL);
	OtpCheckConsistency();
}

void OtpReassignIcon(TwmWindow *twm_win, Icon *old_icon)
{
	if(old_icon != NULL) {
		/* Transfer OWL to new Icon */
		Icon *new_icon = twm_win->icon;
		if(new_icon != NULL) {
			new_icon->otp = old_icon->otp;
			old_icon->otp = NULL;
		}
	}
	else {
		/* Create a new OWL for this Icon */
		OtpAdd(twm_win, IconWin);
	}
}

void OtpFreeIcon(TwmWindow *twm_win)
{
	/* Remove the icon's OWL, if any */
	Icon *cur_icon = twm_win->icon;
	if(cur_icon != NULL) {
		OtpRemove(twm_win, IconWin);
	}
}

name_list **OtpScrSwitchingL(ScreenInfo *scr, WinType wintype)
{
	OtpPreferences *prefs = (wintype == IconWin) ? scr->IconOTP : scr->OTP;

	assert(prefs != NULL);

	return &(prefs->switchingL);
}


void OtpScrSetSwitching(ScreenInfo *scr, WinType wintype, bool switching)
{
#ifndef NDEBUG
	OtpPreferences *prefs = (wintype == IconWin) ? scr->IconOTP : scr->OTP;

	assert(prefs != NULL);
#endif

	scr->OTP->switching = switching;
}


void OtpScrSetZero(ScreenInfo *scr, WinType wintype, int priority)
{
	OtpPreferences *prefs = (wintype == IconWin) ? scr->IconOTP : scr->OTP;

	assert(prefs != NULL);

	if(ABS(priority) > OTP_ZERO) {
		fprintf(stderr, "invalid OnTopPriority value: %d\n", priority);
		return;
	}

	prefs->priority = priority + OTP_ZERO;
}


name_list **OtpScrPriorityL(ScreenInfo *scr, WinType wintype, int priority)
{
	OtpPreferences *prefs = (wintype == IconWin) ? scr->IconOTP : scr->OTP;

	assert(prefs != NULL);

	if(ABS(priority) > OTP_ZERO) {
		fprintf(stderr, "invalid OnTopPriority value: %d\n", priority);
		priority = 0;
	}
	return &(prefs->priorityL[priority + OTP_ZERO]);
}


static OtpPreferences *new_OtpPreferences(void)
{
	OtpPreferences *pref = malloc(sizeof(OtpPreferences));
	int i;

	/* initialize default values */
	for(i = 0; i <= OTP_MAX; i++) {
		pref->priorityL[i] = NULL;
	}
	pref->priority = OTP_ZERO;
	pref->switchingL = NULL;
	pref->switching = false;

	return pref;
}

static void free_OtpPreferences(OtpPreferences *pref)
{
	int i;

	FreeList(&pref->switchingL);
	for(i = 0; i <= OTP_MAX; i++) {
		FreeList(&pref->priorityL[i]);
	}

	free(pref);
}

void OtpScrInitData(ScreenInfo *scr)
{
	if(scr->OTP != NULL) {
		free_OtpPreferences(scr->OTP);
	}
	if(scr->IconOTP != NULL) {
		free_OtpPreferences(scr->IconOTP);
	}
	scr->OTP = new_OtpPreferences();
	scr->IconOTP = new_OtpPreferences();
}

int ReparentWindow(Display *display, TwmWindow *twm_win, WinType wintype,
                   Window parent, int x, int y)
{
	int result;
	OtpWinList *owl = (wintype == IconWin) ? twm_win->icon->otp : twm_win->otp;
	OtpWinList *other = owl->below;
	assert(owl != NULL);

	DPRINTF((stderr, "ReparentWindow: w=%x type=%d\n",
	         (unsigned int)WindowOfOwl(owl), wintype));
	result = XReparentWindow(display, WindowOfOwl(owl), parent, x, y);
	/* The raise was already done by XReparentWindow, so this call
	   just re-places the window at the right spot in the list
	   and enforces priority settings. */
	RemoveOwl(owl);
	InsertOwlAbove(owl, other);
	OtpCheckConsistency();
	return result;
}

void
ReparentWindowAndIcon(Display *display, TwmWindow *twm_win,
                      Window parent, int win_x, int win_y,
                      int icon_x, int icon_y)
{
	OtpWinList *win_owl = twm_win->otp;
	assert(twm_win->icon != NULL);
	OtpWinList *icon_owl = twm_win->icon->otp;
	assert(win_owl != NULL);
	assert(icon_owl != NULL);
	OtpWinList *below_win = win_owl->below;
	OtpWinList *below_icon = icon_owl->below;

	DPRINTF((stderr, "ReparentWindowAndIcon %x\n", (unsigned int)twm_win->frame));
	XReparentWindow(display, twm_win->frame, parent, win_x, win_y);
	XReparentWindow(display, twm_win->icon->w, parent, icon_x, icon_y);
	/* The raise was already done by XReparentWindow, so this call
	   just re-places the window at the right spot in the list
	   and enforces priority settings. */
	RemoveOwl(win_owl);
	RemoveOwl(icon_owl);
	if(below_win != icon_owl) {
		/*
		 * Only insert the window above something if it isn't the icon,
		 * because that isn't back yet.
		 */
		InsertOwlAbove(win_owl, below_win);
		InsertOwlAbove(icon_owl, below_icon);
	}
	else {
		/* In such a case, do it in the opposite order. */
		InsertOwlAbove(icon_owl, below_icon);
		InsertOwlAbove(win_owl, below_win);
	}
	OtpCheckConsistency();
	return;
}

/* Iterators.  */
TwmWindow *OtpBottomWin()
{
	OtpWinList *owl = bottomOwl;
	while(owl && owl->type != WinWin) {
		owl = owl->above;
	}
	return owl ? owl->twm_win : NULL;
}

TwmWindow *OtpTopWin()
{
	OtpWinList *owl = bottomOwl, *top = NULL;
	while(owl) {
		if(owl->type == WinWin) {
			top = owl;
		}
		owl = owl->above;
	}
	return top ? top->twm_win : NULL;
}

TwmWindow *OtpNextWinUp(TwmWindow *twm_win)
{
	OtpWinList *owl = twm_win->otp->above;
	while(owl && owl->type != WinWin) {
		owl = owl->above;
	}
	return owl ? owl->twm_win : NULL;
}

TwmWindow *OtpNextWinDown(TwmWindow *twm_win)
{
	OtpWinList *owl = twm_win->otp->below;
	while(owl && owl->type != WinWin) {
		owl = owl->below;
	}
	return owl ? owl->twm_win : NULL;
}

int OtpGetPriority(TwmWindow *twm_win)
{
	OtpWinList *owl = twm_win->otp;

	return owl->priority - OTP_ZERO;
}


/*
 * Calculating effective priorities
 */
int
OtpEffectivePriority(TwmWindow *twm_win)
{
	assert(twm_win != NULL);
	assert(twm_win->otp != NULL);

	return OwlEffectivePriority(twm_win->otp);
}

static int
OwlEffectivePriority(OtpWinList *owl)
{
	int pri;

	assert(owl != NULL);

	pri = owl->pri_base;
#ifdef EWMH
	if(owl->pri_aflags & OTP_AFLAG_ABOVE) {
		pri += EWMH_PRI_ABOVE;
	}
	if(owl->pri_aflags & OTP_AFLAG_BELOW) {
		pri -= EWMH_PRI_ABOVE;
	}
#endif

	return pri;
}
