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


static void Reborder(Time mytime);


/**
 * ???
 */
void
RestoreWithdrawnLocation(TwmWindow *tmp)
{
	int gravx, gravy;
	unsigned int bw, mask;
	XWindowChanges xwc;

	if(tmp->UnmapByMovingFarAway && !visible(tmp)) {
		XMoveWindow(dpy, tmp->frame, tmp->frame_x, tmp->frame_y);
	}
	if(tmp->squeezed) {
		Squeeze(tmp);
	}
	if(XGetGeometry(dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y,
	                &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {

		GetGravityOffsets(tmp, &gravx, &gravy);
		if(gravy < 0) {
			xwc.y -= tmp->title_height;
		}
		xwc.x += gravx * tmp->frame_bw3D;
		xwc.y += gravy * tmp->frame_bw3D;

		if(bw != tmp->old_bw) {
			int xoff, yoff;

			if(!Scr->ClientBorderWidth) {
				xoff = gravx;
				yoff = gravy;
			}
			else {
				xoff = 0;
				yoff = 0;
			}

			xwc.x -= (xoff + 1) * tmp->old_bw;
			xwc.y -= (yoff + 1) * tmp->old_bw;
		}
		if(!Scr->ClientBorderWidth) {
			xwc.x += gravx * tmp->frame_bw;
			xwc.y += gravy * tmp->frame_bw;
		}

		mask = (CWX | CWY);
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

		if(tmp->winbox && tmp->winbox->twmwin && tmp->frame) {
			int xbox, ybox;
			if(XGetGeometry(dpy, tmp->frame, &JunkRoot, &xbox, &ybox,
			                &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {
				ReparentWindow(dpy, tmp, WinWin, Scr->Root, xbox, ybox);
			}
		}
		XConfigureWindow(dpy, tmp->w, mask, &xwc);

		if(tmp->wmhints->flags & IconWindowHint) {
			XUnmapWindow(dpy, tmp->wmhints->icon_window);
		}

	}
}


/**
 * Restore some window positions/etc in preparation for going away.
 */
static void
Reborder(Time mytime)
{
	TwmWindow *tmp;                     /* temp twm window structure */
	int scrnum;
	ScreenInfo *savedScreen;            /* Its better to avoid coredumps */

	/* put a border back around all windows */

	XGrabServer(dpy);
	savedScreen = Scr;
	for(scrnum = 0; scrnum < NumScreens; scrnum++) {
		if((Scr = ScreenList[scrnum]) == NULL) {
			continue;
		}

		InstallColormaps(0, &Scr->RootColormaps);       /* force reinstall */
		for(tmp = Scr->FirstWindow; tmp != NULL; tmp = tmp->next) {
			RestoreWithdrawnLocation(tmp);
			XMapWindow(dpy, tmp->w);
		}
	}
	Scr = savedScreen;
	XUngrabServer(dpy);
	SetFocus(NULL, mytime);
}


/**
 * Cleanup and exit twm
 */
void
Done(void)
{
#ifdef SOUNDS
	play_exit_sound();
#endif
	Reborder(CurrentTime);
#ifdef EWMH
	EwmhTerminate();
#endif /* EWMH */
	XDeleteProperty(dpy, Scr->Root, XA_WM_WORKSPACESLIST);
	if(CLarg.is_captive) {
		RemoveFromCaptiveList(Scr->captivename);
	}
	XCloseDisplay(dpy);
	exit(0);
}


/**
 * exec() ourself to restart.
 */
void
DoRestart(Time t)
{
	StopAnimation();
	XSync(dpy, 0);
	Reborder(t);
	XSync(dpy, 0);

	if(smcConn) {
		SmcCloseConnection(smcConn, 0, NULL);
	}

	fprintf(stderr, "%s:  restarting:  %s\n",
	        ProgramName, *Argv);
	execvp(*Argv, Argv);
	fprintf(stderr, "%s:  unable to restart:  %s\n", ProgramName, *Argv);
}
