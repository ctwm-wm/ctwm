/*
 * Taking over the screen
 */

#include "ctwm.h"

#include <stdio.h>
#include <X11/Xproto.h>
#include <X11/Xmu/Error.h>

#include "ctwm_takeover.h"
#include "screen.h"


static bool RedirectError;      /* true ==> another window manager running */
/* for settting RedirectError */
static int CatchRedirectError(Display *display, XErrorEvent *event);
/* for everything else */
static int TwmErrorHandler(Display *display, XErrorEvent *event);



/**
 * Take over as WM for a screen
 */
bool
takeover_screen(ScreenInfo *scr)
{
	unsigned long attrmask;

#ifdef EWMH
	// Early EWMH setup.  This tries to do the EWMH display takeover.
	EwmhInitScreenEarly(scr);
#endif


	/*
	 * Subscribe to various events on the root window.  Because X
	 * only allows a single client to subscribe to
	 * SubstructureRedirect and ButtonPress bits, this also serves to
	 * mutex who is The WM for the root window, and thus (aside from
	 * captive) the Screen.
	 *
	 * To catch whether that failed, we set a special one-shot error
	 * handler to flip a var that we test to find out whether the
	 * redirect failed.
	 */
	XSync(dpy, 0); // Flush possible previous errors
	RedirectError = false;
	XSetErrorHandler(CatchRedirectError);
	attrmask = ColormapChangeMask | EnterWindowMask |
	           PropertyChangeMask | SubstructureRedirectMask |
	           KeyPressMask | ButtonPressMask | ButtonReleaseMask;
#ifdef EWMH
	attrmask |= StructureNotifyMask;
#endif
	if(CLarg.is_captive) {
		attrmask |= StructureNotifyMask;
	}
	XSelectInput(dpy, scr->Root, attrmask);
	XSync(dpy, 0); // Flush the RedirectError, if we had one

	// Back to our normal handler
	XSetErrorHandler(TwmErrorHandler);

	if(RedirectError) {
		fprintf(stderr, "%s: another window manager is already running",
		        ProgramName);
		if(CLarg.MultiScreen && NumScreens > 0) {
			fprintf(stderr, " on screen %d?\n", scr->screen);
		}
		else {
			fprintf(stderr, "?\n");
		}

		// XSetErrorHandler() isn't local to the Screen; it's for the
		// whole connection.  We wind up in a slightly weird state
		// once we've set it up, but decided we aren't taking over
		// this screen, but resetting it would be a little weird too,
		// because maybe we have taken over some other screen.  So,
		// just throw up our hands.
		return false;

	}

	return true;
}


/*
 * Error Handlers.  If a client dies, we'll get a BadWindow error (except for
 * GetGeometry which returns BadDrawable) for most operations that we do before
 * manipulating the client's window.
 */

static XErrorEvent LastErrorEvent;

static int
TwmErrorHandler(Display *display, XErrorEvent *event)
{
	LastErrorEvent = *event;

	if(CLarg.PrintErrorMessages &&                 /* don't be too obnoxious */
	                event->error_code != BadWindow &&       /* watch for dead puppies */
	                (event->request_code != X_GetGeometry &&         /* of all styles */
	                 event->error_code != BadDrawable)) {
		XmuPrintDefaultErrorMessage(display, event, stderr);
	}
	return 0;
}


/* ARGSUSED*/
static int
CatchRedirectError(Display *display, XErrorEvent *event)
{
	RedirectError = true;
	LastErrorEvent = *event;
	return 0;
}
