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
		unsigned int mask;

		// Get gravity bits to know how to move stuff around when we take
		// away the decorations.
		GetGravityOffsets(tmp, &gravx, &gravy);

		// Shift for stripping out the titlebar and 3d borders
		if(gravy < 0) {
			xwc.y -= tmp->title_height;
		}
		xwc.x += gravx * tmp->frame_bw3D;
		xwc.y += gravy * tmp->frame_bw3D;

		// If the window used to have a border size different from what
		// it has now (from us), restore the old.
		if(bw != tmp->old_bw) {
			int xoff, yoff;

			if(!Scr->ClientBorderWidth) {
				// We used BorderWidth
				xoff = gravx;
				yoff = gravy;
			}
			else {
				// We used the original
				xoff = 0;
				yoff = 0;
			}

			xwc.x -= (xoff + 1) * tmp->old_bw;
			xwc.y -= (yoff + 1) * tmp->old_bw;
		}

		// Strip out our 2d borders, if we had a size other than the
		// win's original.
		if(!Scr->ClientBorderWidth) {
			xwc.x += gravx * tmp->frame_bw;
			xwc.y += gravy * tmp->frame_bw;
		}


		// Now put together the adjustment.  We'll always be moving the
		// X/Y coords.
		mask = (CWX | CWY);
		// xwc.[xy] already set

		// May be changing the border.
		if(bw != tmp->old_bw) {
			xwc.border_width = tmp->old_bw;
			mask |= CWBorderWidth;
		}

#if 0
		if(tmp->vs) {
			xwc.x += tmp->vs->x;
			xwc.y += tmp->vs->y;
		}
#endif

		// If it's in a WindowBox, reparent it back up to our real root.
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

		// Do the move and possible reborder
		XConfigureWindow(dpy, tmp->w, mask, &xwc);

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
			XReparentWindow(dpy, tw->w, Scr->Root, tw->frame_x, tw->frame_y);
		}

		// We're not actually "letting to" of the windows, by reparenting
		// out of the frame, or cleaning up the TwmWindow struct, etc.
		// This only gets called in preparation for us going away by
		// shutting down or restarting, so cleaning up our internal state
		// is a waste of time.  And X's SaveSet handling will deal with
		// reparenting the windows back away from us when we go away.
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
