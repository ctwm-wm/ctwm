/*
 * Functions related to the window ring.
 */

#include "ctwm.h"

#include "screen.h"
#include "win_ring.h"

void
UnlinkWindowFromRing(TwmWindow *win)
{
	TwmWindow *prev = win->ring.prev;
	TwmWindow *next = win->ring.next;

	/*
	* 1. Unlink window
	* 2. If window was only thing in ring, null out ring
	* 3. If window was ring leader, set to next (or null)
	*
	* If the window is the only one in the ring, prev == next == win,
	* so the unlinking effectively is a NOP, but that doesn't matter.
	*/
	prev->ring.next = next;
	next->ring.prev = prev;

	win->ring.next = win->ring.prev = NULL;

	if(Scr->Ring == win) {
		Scr->Ring = (next != win ? next : NULL);
	}

	if(!Scr->Ring || Scr->RingLeader == win) {
		Scr->RingLeader = Scr->Ring;
	}
}

static void
AddWindowToRingUnchecked(TwmWindow *win, TwmWindow *after)
{
	TwmWindow *before = after->ring.next;

	win->ring.next = before;
	win->ring.prev = after;

	after->ring.next = win;
	before->ring.prev = win;
}

void
AddWindowToRing(TwmWindow *win)
{
	if(Scr->Ring) {
		AddWindowToRingUnchecked(win, Scr->Ring);
	}
	else {
		win->ring.next = win->ring.prev = Scr->Ring = win;
	}
}
