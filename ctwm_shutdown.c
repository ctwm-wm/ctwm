/*
 * Shutdown (/restart) bits
 */

#include "ctwm.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "animate.h"
#include "captive.h"
#include "colormaps.h"
#include "ctwm_atoms.h"
#include "ctwm_shutdown.h"
#include "screen.h"
#include "session.h"
#ifdef SOUNDS
# include "sound.h"
#endif
#include "otp.h"
#include "win_ops.h"
#include "win_utils.h"


static void RestoreForShutdown(Time mytime);


/**
 * Put a window back where it should be if we don't (any longer) control
 * it.  Essentially cancels out the repositioning due to our frame and
 * decorations, and restores the original border it had before we put our
 * own on it.
 */
void
RestoreWinConfig(TwmWindow *tmp)
{
	XWindowChanges xwc;
	unsigned int bw;

	// If this window is "unmapped" by moving it way offscreen, and is in
	// that state, move it back onto the window.
	if(tmp->UnmapByMovingFarAway && !visible(tmp)) {
		XMoveWindow(dpy, tmp->frame, tmp->frame_x, tmp->frame_y);
	}

	// If it's squeezed, unsqueeze it.
	if(tmp->squeezed) {
		Squeeze(tmp);
	}

	// Look up geometry bits.  Failure means ???  Maybe the window
	// disappeared on us?
	if(XGetGeometry(dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y,
	                &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {
		int gravx, gravy;
		int newx, newy;
		const int borders = tmp->frame_bw + tmp->frame_bw3D - tmp->old_bw;

		// Get gravity bits to know how to move stuff around when we take
		// away the decorations.
		GetGravityOffsets(tmp, &gravx, &gravy);

		// We want to move the window to where it should be "outside" of
		// our frame.  This varies depending on the window gravity
		// detail, and we have to account for that, since on re-startup
		// we'll be doing it to resposition it after we re-wrap it.
		//
		// e.g., in simple "NorthWest" gravity, we just made the frame
		// start where the window did, and shifted the app window right
		// (by the border width) and down (by the border width +
		// titlebar).  However, "SouthEast" gravity means the bottom
		// right of the frame is where thw windows' was, and the window
		// itself shifted left/up by the border.  Compare a window with
		// specified geometry "+0+0" with one using "-0-0".
		newx = tmp->frame_x;
		newy = tmp->frame_y;


		// So, first consider the north/south gravity.  If gravity is
		// North, we put the top of the frame where the window was and
		// shifted everything inside down, so the top of the frame now is
		// where the window should be put.  With South-y gravity, the
		// window should wind up at the bottom of the frame, which means
		// we need to shift it down by the titlebar height, plus twice
		// the border width.  But if the vertical gravity is neutral,
		// then the window needs to wind up staying right where it is,
		// because we built the frame around it without moving it.
		//
		// Previous code here (and code elsewhere) expressed this
		// calculation by the rather confusing idiom ((gravy + 1) *
		// border_width), which gives the right answer, but is confusing
		// as hell...
		if(gravy < 0) {
			// North; where the frame starts (already set)
		}
		else if(gravy > 0) {
			// South; shift down title + 2*border
			newy += tmp->title_height + 2 * borders;
		}
		else {
			// Neutral; down by the titlebar + border.
			newy += tmp->title_height + borders;
		}


		// Now east/west.  West means align with the frame start, easy
		// means align with the frame right edge, neutral means where it
		// already is.
		if(gravx < 0) {
			// West; it's already right
		}
		else if(gravx > 0) {
			// East; shift over by 2*border
			newx += 2 * borders;
		}
		else {
			// Neutral; over by the left border
			newx += borders;
		}


		// If it's in a WindowBox, reparent the frame back up to our real root
		if(tmp->winbox && tmp->winbox->twmwin && tmp->frame) {
			int xbox, ybox;
			unsigned int j_bw;
			// XXX This isn't right, right?  This will give coords
			// relative to the window box, but we're using them relative
			// to the real screen root?
			if(XGetGeometry(dpy, tmp->frame, &JunkRoot, &xbox, &ybox,
			                &JunkWidth, &JunkHeight, &j_bw, &JunkDepth)) {
				ReparentWindow(dpy, tmp, WinWin, Scr->Root, xbox, ybox);
			}
		}


		// Restore the original window border if there were one
		if(tmp->old_bw) {
			XSetWindowBorderWidth(dpy, tmp->w, tmp->old_bw);
		}

		// Reparent and move back to where it otter be
		XReparentWindow(dpy, tmp->w, Scr->Root, newx, newy);

		// If it came with a pre-made icon window, hide it
		if(tmp->wmhints->flags & IconWindowHint) {
			XUnmapWindow(dpy, tmp->wmhints->icon_window);
		}
	}

	// Done
	return;
}


/**
 * Restore some window positions/etc in preparation for going away.
 */
static void
RestoreForShutdown(Time mytime)
{
	ScreenInfo *savedScr = Scr;  // We need Scr flipped around...

	XGrabServer(dpy);

	for(int scrnum = 0; scrnum < NumScreens; scrnum++) {
		if((Scr = ScreenList[scrnum]) == NULL) {
			continue;
		}

		// Force reinstalling any colormaps
		InstallColormaps(0, &Scr->RootColormaps);

		// Pull all the windows out of their frames and reposition them
		// where the frame was.  This will preserve their positions when
		// we restart, or put them back where they were before we
		// started.  Do it from the bottom up of our stacking order to
		// preserve the stacking.
		//
		// XXX We used to call RestoreWinConfig() instead of this, which
		// does a lot more work, but doesn't reparent.  However, I can't
		// tell that it's actually _useful_ work, compared to just "put
		// where the frame is".  And it won't preserve the stacking
		// position; we need to reparent back to the root to be able to
		// set the bare windows' stacking.
		for(TwmWindow *tw = OtpBottomWin() ; tw != NULL
		                ; tw = OtpNextWinUp(tw)) {
			if(tw->isiconmgr || tw->iswspmgr || tw->isoccupy) {
				// Don't bother with internals...
				continue;
			}
			RestoreWinConfig(tw);
		}
	}

	XUngrabServer(dpy);

	// Restore
	Scr = savedScr;

	// Focus away from any windows
	SetFocus(NULL, mytime);
}


/**
 * Cleanup and exit ctwm
 */
void
DoShutdown(void)
{

#ifdef SOUNDS
	// Denounce ourselves
	play_exit_sound();
#endif

	// Restore windows/colormaps for our absence.
	RestoreForShutdown(CurrentTime);

#ifdef EWMH
	// Clean up EWMH properties
	EwmhTerminate();
#endif

	// Clean up our list of workspaces
	XDeleteProperty(dpy, Scr->Root, XA_WM_WORKSPACESLIST);

	// Shut down captive stuff
	if(CLarg.is_captive) {
		RemoveFromCaptiveList(Scr->captivename);
	}

	// Close up shop
	XCloseDisplay(dpy);
	exit(0);
}


/**
 * exec() ourself to restart.
 */
void
DoRestart(Time t)
{
	// Don't try to do any further animating
	StopAnimation();
	XSync(dpy, 0);

	// Replace all the windows/colormaps as if we were going away.  'cuz
	// we are.
	RestoreForShutdown(t);
	XSync(dpy, 0);

	// Shut down session management connection cleanly.
	if(smcConn) {
		SmcCloseConnection(smcConn, 0, NULL);
	}

	// Re-run ourself
	fprintf(stderr, "%s:  restarting:  %s\n", ProgramName, *Argv);
	execvp(*Argv, Argv);

	// Uh oh, we shouldn't get here...
	fprintf(stderr, "%s:  unable to restart:  %s\n", ProgramName, *Argv);

	// We should probably un-RestoreForShutdown() etc.  If that exec
	// fails, we're in a really weird state...
	XBell(dpy, 0);
}
