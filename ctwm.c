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
 * $XConsortium: twm.c,v 1.124 91/05/08 11:01:54 dave Exp $
 *
 * twm - "Tom's Window Manager"
 *
 * 27-Oct-87 Thomas E. LaStrange        File created
 * 10-Oct-90 David M. Sternlicht        Storing saved colors on root
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 ***********************************************************************/

#if defined(USE_SIGNALS) && defined(__sgi)
#  define _BSD_SIGNALS
#endif

#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#ifdef __WAIT_FOR_CHILDS
#  include <sys/wait.h>
#endif

#include <fcntl.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Error.h>
#include <X11/SM/SMlib.h>
#include <X11/Xlocale.h>


#include "ctwm.h"
#include "ctwm_atoms.h"
#include "add_window.h"
#include "gc.h"
#include "parse.h"
#include "version.h"
#include "menus.h"
#include "events.h"
#include "util.h"
#include "screen.h"
#include "icons.h"
#include "iconmgr.h"
#include "session.h"
#include "otp.h"
#include "cursor.h"
#include "windowbox.h"
#ifdef SOUNDS
#  include "sound.h"
#endif


/* I'm not sure this is meaningful anymore */
#ifndef PIXMAP_DIRECTORY
#define PIXMAP_DIRECTORY "/usr/lib/X11/twm"
#endif


XtAppContext appContext;        /* Xt application context */
Display *dpy;                   /* which display are we talking to */
Window ResizeWindow;            /* the window we are resizing */

int  captive      = FALSE;
char *captivename = NULL;

int NumScreens;                 /* number of screens in ScreenList */
int HasShape;                   /* server supports shape extension? */
int ShapeEventBase, ShapeErrorBase;
ScreenInfo **ScreenList;        /* structures for each screen */
ScreenInfo *Scr = NULL;         /* the cur and prev screens */
int PreviousScreen;             /* last screen that we were on */
int FirstScreen;                /* TRUE ==> first screen of display */
#ifdef DEBUG
Bool ShowWelcomeWindow = False;
#else
Bool ShowWelcomeWindow = True;
#endif
static int RedirectError;       /* TRUE ==> another window manager running */
/* for settting RedirectError */
static int CatchRedirectError(Display *display, XErrorEvent *event);
/* for everything else */
static int TwmErrorHandler(Display *display, XErrorEvent *event);
char Info[INFO_LINES][INFO_SIZE];               /* info strings to print */
int InfoLines;
unsigned int InfoWidth, InfoHeight;
static Window CreateRootWindow(int x, int y,
                               unsigned int width, unsigned int height);
static void DisplayInfo(void);
static void InternUsefulAtoms(void);
static void InitVariables(void);
static void usage(void) __attribute__((noreturn));

Cursor  UpperLeftCursor;
Cursor  TopRightCursor,
        TopLeftCursor,
        BottomRightCursor,
        BottomLeftCursor,
        LeftCursor,
        RightCursor,
        TopCursor,
        BottomCursor;

Cursor RightButt;
Cursor MiddleButt;
Cursor LeftButt;

XContext TwmContext;            /* context for twm windows */
XContext MenuContext;           /* context for all menu windows */
XContext ScreenContext;         /* context to get screen data */
XContext ColormapContext;       /* context for colormap operations */

XClassHint NoClass;             /* for applications with no class */

XGCValues Gcv;

char *Home;                     /* the HOME environment variable */
int HomeLen;                    /* length of Home */

int HandlingEvents = FALSE;     /* are we handling events yet? */

Window JunkRoot;                /* junk window */
Window JunkChild;               /* junk window */
int JunkX;                      /* junk variable */
int JunkY;                      /* junk variable */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

char *ProgramName;
int Argc;
char **Argv;

/* Command-line args */
ctwm_cl_args CLarg = {
	.MultiScreen     = TRUE,
	.Monochrome      = FALSE,
	.cfgchk          = 0,
	.InitFile        = NULL,
	.display_name    = NULL,
	.PrintErrorMessages = False,
	.ShowWelcomeWindow  = False, // XXX UNIMPLEMENTED
#ifdef USEM4
	.KeepTmpFile     = False,
	.keepM4_filename = NULL,
	.GoThroughM4     = True,
#endif
#ifdef EWMH
	.ewmh_replace    = 0,
#endif
};

Bool RestartPreviousState = False;      /* try to restart in previous state */

Bool RestartFlag = 0;
SIGNAL_T Restart(int signum);
SIGNAL_T Crash(int signum);
#ifdef __WAIT_FOR_CHILDS
SIGNAL_T ChildExit(int signum);
#endif

/***********************************************************************
 *
 *  Procedure:
 *      main - start of twm
 *
 ***********************************************************************
 */

int main(int argc, char **argv, char **environ)
{
	Window croot, parent, *children;
	unsigned int nchildren;
	int i, j;
	unsigned long valuemask;    /* mask for create windows */
	XSetWindowAttributes attributes;    /* attributes for create windows */
	int numManaged, firstscrn, lastscrn, scrnum;
	int zero = 0;
	char *restore_filename = NULL;
	char *client_id = NULL;
	char *welcomefile;
	int  screenmasked;
	static int crootx = 100;
	static int crooty = 100;
	static unsigned int crootw = 1280;
	static unsigned int crooth =  768;
	int ch, optidx;
	/*    static unsigned int crootw = 2880; */
	/*    static unsigned int crooth = 1200; */
	Window capwin = (Window) 0;
	IconRegion *ir;

	XRectangle ink_rect;
	XRectangle logical_rect;

	(void)setlocale(LC_ALL, "");

	ProgramName = argv[0];
	Argc = argc;
	Argv = argv;


	/*
	 * Backward-compat cheat: accept a few old-style long args if they
	 * came first.  Of course, this assumed argv[x] is editable, which on
	 * most systems it is, and C99 requires it.
	 */
	if(argc > 1) {
#define CHK(x) else if(strcmp(argv[1], (x)) == 0)
		if(0) {
			/* nada */
		}
		CHK("-version") {
			printf("%s\n", VersionNumber);
			exit(0);
		}
		CHK("-info") {
			DisplayInfo();
			exit(0);
		}
		CHK("-cfgchk") {
			CLarg.cfgchk = 1;
			*argv[1] = '\0';
		}
		CHK("-display") {
			if(argc <= 2 || strlen(argv[2]) < 1) {
				usage();
			}
			CLarg.display_name = strdup(argv[2]);

			*argv[1] = '\0';
			*argv[2] = '\0';
		}
#undef CHK
	}


	/*
	 * Setup long options for arg parsing
	 */
	static struct option long_options[] = {
		/* Simple flags */
		{ "single",    no_argument,       &CLarg.MultiScreen, FALSE },
		{ "mono",      no_argument,       &CLarg.Monochrome, TRUE },
		{ "verbose",   no_argument,       NULL, 'v' },
		{ "quiet",     no_argument,       NULL, 'q' },
		{ "nowelcome", no_argument,       NULL, 'W' },

		/* Config/file related */
		{ "file",      required_argument, NULL, 'f' },
		{ "cfgchk",    no_argument,       &CLarg.cfgchk, 1 },

		/* Show something and exit right away */
		{ "help",      no_argument,       NULL, 'h' },
		{ "version",   no_argument,       NULL, 0 },
		{ "info",      no_argument,       NULL, 0 },

		/* Misc control bits */
		{ "display",   required_argument, NULL, 'd' },
		{ "window",    optional_argument, NULL, 'w' },
		{ "name",      required_argument, NULL, 0 },
		{ "xrm",       required_argument, NULL, 0 },

#ifdef EWMH
		{ "replace",   no_argument,       &CLarg.ewmh_replace, 1 },
#endif

		/* M4 control params */
#ifdef USEM4
		{ "keep-defs", no_argument,       NULL, 'k' },
		{ "keep",      required_argument, NULL, 'K' },
		{ "nom4",      no_argument,       NULL, 'n' },
#endif

		/* Random session-related bits */
		{ "clientId",  required_argument, NULL, 0 },
		{ "restore",   required_argument, NULL, 0 },
	};

	/*
	 * Short aliases for some
	 *
	 * I assume '::' for optional args is portable; getopt_long(3)
	 * doesn't describe it, but it's a GNU extension for getopt(3).
	 */
	const char *short_options = "vqWf:hd:w::"
#ifdef USEM4
	                            "kK:n"
#endif
	                            ;

	/*
	 * Parse out the args
	 */
	optidx = 0;
	while((ch = getopt_long(argc, argv, short_options, long_options,
	                        &optidx)) != -1) {
		switch(ch) {
			/* First handle the simple cases that have short args */
			case 'v':
				CLarg.PrintErrorMessages = True;
				break;
			case 'q':
				CLarg.PrintErrorMessages = False;
				break;
			case 'W':
				ShowWelcomeWindow = False;
				break;
			case 'f':
				CLarg.InitFile = optarg;
				break;
			case 'h':
				usage();
			case 'd':
				CLarg.display_name = optarg;
				break;
			case 'w':
				captive = True;
				CLarg.MultiScreen = False;
				if(optarg != NULL) {
					sscanf(optarg, "%x", (unsigned int *)&capwin);
					/* Failure will just leave capwin as initialized */
				}
				break;

#ifdef USEM4
			/* Args that only mean anything if we're built with m4 */
			case 'k':
				CLarg.KeepTmpFile = True;
				break;
			case 'K':
				CLarg.keepM4_filename = optarg;
				break;
			case 'n':
				CLarg.GoThroughM4 = False;
				break;
#endif


			/*
			 * Now the stuff that doesn't have short variants.  Many of
			 * them just set flags, so we don't need to do anything with
			 * them.  Only the ones with NULL flags matter.
			 */
			case 0:
				if(long_options[optidx].flag != NULL) {
					/* It only existed to set a flag; we're done */
					break;
				}

#define IFIS(x) if(strcmp(long_options[optidx].name, (x)) == 0)
				IFIS("version") {
					printf("%s\n", VersionNumber);
					exit(0);
				}
				IFIS("info") {
					DisplayInfo();
					exit(0);
				}
				IFIS("name") {
					captivename = optarg;
					break;
				}
				IFIS("xrm") {
					/*
					 * Quietly ignored by us; Xlib processes it
					 * internally in XtToolkitInitialize();
					 */
					break;
				}
				IFIS("clientId") {
					client_id = optarg;
					break;
				}
				IFIS("restore") {
					restore_filename = optarg;
					break;
				}
#undef IFIS

				/* Don't think it should be possible to get here... */
				fprintf(stderr, "Internal error in getopt: '%s' unhandled.\n",
				        long_options[optidx].name);
				usage();

			/* Something totally unexpected */
			case '?':
				/* getopt_long() already printed an error */
				usage();

			default:
				/* Uhhh...  */
				fprintf(stderr, "Internal error: getopt confused us.\n");
				usage();
		}
	}


#define newhandler(sig, action) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, action)

	newhandler(SIGINT, Done);
	signal(SIGHUP, Restart);
	newhandler(SIGQUIT, Done);
	newhandler(SIGTERM, Done);
#ifdef __WAIT_FOR_CHILDS
	newhandler(SIGCHLD, ChildExit);
#endif
	signal(SIGALRM, SIG_IGN);
#ifdef NOTRAP
	signal(SIGSEGV, Crash);
	signal(SIGBUS,  Crash);
#endif

#undef newhandler

	Home = getenv("HOME");
	if(Home == NULL) {
		Home = "./";
	}

	HomeLen = strlen(Home);

	NoClass.res_name = NoName;
	NoClass.res_class = NoName;

	XtToolkitInitialize();
	appContext = XtCreateApplicationContext();

	if(!(dpy = XtOpenDisplay(appContext, CLarg.display_name, "twm", "twm",
	                         NULL, 0, &zero, NULL))) {
		fprintf(stderr, "%s:  unable to open display \"%s\"\n",
		        ProgramName, XDisplayName(CLarg.display_name));
		exit(1);
	}

	if(fcntl(ConnectionNumber(dpy), F_SETFD, 1) == -1) {
		fprintf(stderr,
		        "%s:  unable to mark display connection as close-on-exec\n",
		        ProgramName);
		exit(1);
	}
	if(restore_filename) {
		ReadWinConfigFile(restore_filename);
	}
	HasShape = XShapeQueryExtension(dpy, &ShapeEventBase, &ShapeErrorBase);
	TwmContext = XUniqueContext();
	MenuContext = XUniqueContext();
	ScreenContext = XUniqueContext();
	ColormapContext = XUniqueContext();

	InternUsefulAtoms();


	/* Set up the per-screen global information. */

	NumScreens = ScreenCount(dpy);

	if(CLarg.MultiScreen) {
		firstscrn = 0;
		lastscrn = NumScreens - 1;
	}
	else {
		firstscrn = lastscrn = DefaultScreen(dpy);
	}

	InfoLines = 0;

	/* for simplicity, always allocate NumScreens ScreenInfo struct pointers */
	ScreenList = (ScreenInfo **) calloc(NumScreens, sizeof(ScreenInfo *));
	if(ScreenList == NULL) {
		fprintf(stderr, "%s: Unable to allocate memory for screen list, exiting.\n",
		        ProgramName);
		exit(1);
	}
	numManaged = 0;
	PreviousScreen = DefaultScreen(dpy);
	FirstScreen = TRUE;
#ifdef EWMH
	EwmhInit();
#endif /* EWMH */

	for(scrnum = firstscrn ; scrnum <= lastscrn; scrnum++) {
		unsigned long attrmask;
		if(captive) {
			XWindowAttributes wa;
			if(capwin && XGetWindowAttributes(dpy, capwin, &wa)) {
				Window junk;
				croot  = capwin;
				crootw = wa.width;
				crooth = wa.height;
				XTranslateCoordinates(dpy, capwin, wa.root, 0, 0, &crootx, &crooty, &junk);
			}
			else {
				croot = CreateRootWindow(crootx, crooty, crootw, crooth);
			}
		}
		else {
			croot  = RootWindow(dpy, scrnum);
			crootx = 0;
			crooty = 0;
			crootw = DisplayWidth(dpy, scrnum);
			crooth = DisplayHeight(dpy, scrnum);
		}

		/* Make sure property priority colors is empty */
		XChangeProperty(dpy, croot, XA__MIT_PRIORITY_COLORS,
		                XA_CARDINAL, 32, PropModeReplace, NULL, 0);
		XSync(dpy, 0); /* Flush possible previous errors */

		/* Note:  ScreenInfo struct is calloc'ed to initialize to zero. */
		Scr = ScreenList[scrnum] =
		              (ScreenInfo *) calloc(1, sizeof(ScreenInfo));
		if(Scr == NULL) {
			fprintf(stderr,
			        "%s: unable to allocate memory for ScreenInfo structure"
			        " for screen %d.\n",
			        ProgramName, scrnum);
			continue;
		}

		Scr->screen = scrnum;
		Scr->XineramaRoot = croot;
#ifdef EWMH
		EwmhInitScreenEarly(Scr);
#endif /* EWMH */
		RedirectError = FALSE;
		XSetErrorHandler(CatchRedirectError);
		attrmask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
		           SubstructureRedirectMask | KeyPressMask | ButtonPressMask |
		           ButtonReleaseMask;
#ifdef EWMH
		attrmask |= StructureNotifyMask;
#endif /* EWMH */
		if(captive) {
			attrmask |= StructureNotifyMask;
		}
		XSelectInput(dpy, croot, attrmask);
		XSync(dpy, 0);
		XSetErrorHandler(TwmErrorHandler);

		if(RedirectError && CLarg.cfgchk == 0) {
			fprintf(stderr, "%s:  another window manager is already running",
			        ProgramName);
			if(CLarg.MultiScreen && NumScreens > 0) {
				fprintf(stderr, " on screen %d?\n", scrnum);
			}
			else {
				fprintf(stderr, "?\n");
			}
			continue;
		}

		numManaged ++;

		/* initialize list pointers, remember to put an initialization
		 * in InitVariables also
		 */
		Scr->BorderColorL = NULL;
		Scr->IconBorderColorL = NULL;
		Scr->BorderTileForegroundL = NULL;
		Scr->BorderTileBackgroundL = NULL;
		Scr->TitleForegroundL = NULL;
		Scr->TitleBackgroundL = NULL;
		Scr->IconForegroundL = NULL;
		Scr->IconBackgroundL = NULL;
		Scr->AutoPopupL = NULL;
		Scr->AutoPopup = FALSE;
		Scr->NoBorder = NULL;
		Scr->NoIconTitle = NULL;
		Scr->NoTitle = NULL;
		Scr->OccupyAll = NULL;
		Scr->UnmapByMovingFarAway = NULL;
		Scr->DontSetInactive = NULL;
		Scr->AutoSqueeze = NULL;
		Scr->StartSqueezed = NULL;
		Scr->AlwaysSqueezeToGravityL = NULL;
		Scr->MakeTitle = NULL;
		Scr->AutoRaise = NULL;
		Scr->WarpOnDeIconify = NULL;
		Scr->AutoLower = NULL;
		Scr->IconNames = NULL;
		Scr->NoHighlight = NULL;
		Scr->NoStackModeL = NULL;
		Scr->OTP = NULL;
		Scr->NoTitleHighlight = NULL;
		Scr->DontIconify = NULL;
		Scr->IconMgrNoShow = NULL;
		Scr->IconMgrShow = NULL;
		Scr->IconifyByUn = NULL;
		Scr->IconManagerFL = NULL;
		Scr->IconManagerBL = NULL;
		Scr->IconMgrs = NULL;
		Scr->StartIconified = NULL;
		Scr->SqueezeTitleL = NULL;
		Scr->DontSqueezeTitleL = NULL;
		Scr->WindowRingL = NULL;
		Scr->WindowRingExcludeL = NULL;
		Scr->WarpCursorL = NULL;
		Scr->DontSave = NULL;
		Scr->OpaqueMoveList = NULL;
		Scr->NoOpaqueMoveList = NULL;
		Scr->OpaqueResizeList = NULL;
		Scr->NoOpaqueResizeList = NULL;
		Scr->ImageCache = NULL;
		Scr->HighlightPixmapName = NULL;
		Scr->Workspaces = (MenuRoot *) 0;
		Scr->IconMenuDontShow = NULL;
		Scr->VirtualScreens = NULL;
		Scr->IgnoreTransientL = NULL;

		/* remember to put an initialization in InitVariables also
		 */

		Scr->screen = scrnum;
		Scr->d_depth = DefaultDepth(dpy, scrnum);
		Scr->d_visual = DefaultVisual(dpy, scrnum);
		Scr->RealRoot = RootWindow(dpy, scrnum);
		Scr->CaptiveRoot = captive ? croot : None;
		Scr->Root = croot;
		Scr->XineramaRoot = croot;
		XSaveContext(dpy, Scr->Root, ScreenContext, (XPointer) Scr);

		if(captive) {
			AddToCaptiveList();
			if(captivename) {
				XSetStandardProperties(dpy, croot, captivename, captivename, None, NULL, 0,
				                       NULL);
			}
		}
		else {
			captivename = "Root";
		}
		Scr->RootColormaps.number_cwins = 1;
		Scr->RootColormaps.cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *));
		Scr->RootColormaps.cwins[0] = CreateColormapWindow(Scr->Root, True, False);
		Scr->RootColormaps.cwins[0]->visibility = VisibilityPartiallyObscured;

		Scr->cmapInfo.cmaps = NULL;
		Scr->cmapInfo.maxCmaps = MaxCmapsOfScreen(ScreenOfDisplay(dpy, Scr->screen));
		Scr->cmapInfo.root_pushes = 0;
		InstallColormaps(0, &Scr->RootColormaps);

		Scr->StdCmapInfo.head = Scr->StdCmapInfo.tail =  Scr->StdCmapInfo.mru = NULL;
		Scr->StdCmapInfo.mruindex = 0;
		LocateStandardColormaps();

		Scr->TBInfo.nleft  = Scr->TBInfo.nright = 0;
		Scr->TBInfo.head   = NULL;
		Scr->TBInfo.border =
		        -100; /* trick to have different default value if ThreeDTitles */
		Scr->TBInfo.width  = 0;    /* is set or not */
		Scr->TBInfo.leftx  = 0;
		Scr->TBInfo.titlex = 0;

		Scr->rootx  = crootx;
		Scr->rooty  = crooty;
		Scr->rootw  = crootw;
		Scr->rooth  = crooth;

		Scr->crootx = crootx;
		Scr->crooty = crooty;
		Scr->crootw = crootw;
		Scr->crooth = crooth;

		Scr->MaxWindowWidth  = 32767 - Scr->rootw;
		Scr->MaxWindowHeight = 32767 - Scr->rooth;

		Scr->XORvalue = (((unsigned long) 1) << Scr->d_depth) - 1;

		if(CLarg.Monochrome || DisplayCells(dpy, scrnum) < 3) {
			Scr->Monochrome = MONOCHROME;
		}
		else {
			Scr->Monochrome = COLOR;
		}

		/* setup default colors */
		Scr->FirstTime = TRUE;
		GetColor(Scr->Monochrome, &(Scr->Black), "black");
		GetColor(Scr->Monochrome, &(Scr->White), "white");

		if(FirstScreen) {
			SetFocus((TwmWindow *)NULL, CurrentTime);

			/* define cursors */

			NewFontCursor(&TopLeftCursor, "top_left_corner");
			NewFontCursor(&TopRightCursor, "top_right_corner");
			NewFontCursor(&BottomLeftCursor, "bottom_left_corner");
			NewFontCursor(&BottomRightCursor, "bottom_right_corner");
			NewFontCursor(&LeftCursor, "left_side");
			NewFontCursor(&RightCursor, "right_side");
			NewFontCursor(&TopCursor, "top_side");
			NewFontCursor(&BottomCursor, "bottom_side");

			NewFontCursor(&UpperLeftCursor, "top_left_corner");
			NewFontCursor(&RightButt, "rightbutton");
			NewFontCursor(&LeftButt, "leftbutton");
			NewFontCursor(&MiddleButt, "middlebutton");
		}

		Scr->iconmgr = NULL;
		AllocateIconManager("TWM", "Icons", "", 1);

		Scr->IconDirectory = NULL;
		Scr->PixmapDirectory = PIXMAP_DIRECTORY;
		Scr->siconifyPm = None;
		Scr->pullPm = None;
		Scr->tbpm.xlogo = None;
		Scr->tbpm.resize = None;
		Scr->tbpm.question = None;
		Scr->tbpm.menu = None;
		Scr->tbpm.delete = None;

		Scr->WindowMask = (Window) 0;
		screenmasked = 0;
		/* XXX Happens before config parse, so ignores DontShowWW param */
		if(ShowWelcomeWindow && (welcomefile = getenv("CTWM_WELCOME_FILE"))) {
			screenmasked = 1;
			MaskScreen(welcomefile);
		}
		InitVariables();
		InitMenus();
		InitWorkSpaceManager();

		/* Parse it once for each screen. */
		if(CLarg.cfgchk) {
			if(ParseTwmrc(CLarg.InitFile) == 0) {
				/* Error return */
				fprintf(stderr, "Errors found\n");
				exit(1);
			}
			else {
				fprintf(stderr, "No errors found\n");
				exit(0);
			}
		}
		else {
			ParseTwmrc(CLarg.InitFile);
		}

		InitVirtualScreens(Scr);
#ifdef EWMH
		EwmhInitVirtualRoots(Scr);
#endif /* EWMH */
		ConfigureWorkSpaceManager();

		if(ShowWelcomeWindow && ! screenmasked) {
			MaskScreen(NULL);
		}
		if(Scr->ClickToFocus) {
			Scr->FocusRoot  = FALSE;
			Scr->TitleFocus = FALSE;
		}


		if(Scr->use3Dborders) {
			Scr->ClientBorderWidth = FALSE;
		}

		if(Scr->use3Dtitles) {
			if(Scr->FramePadding  == -100) {
				Scr->FramePadding  = 0;
			}
			if(Scr->TitlePadding  == -100) {
				Scr->TitlePadding  = 0;
			}
			if(Scr->ButtonIndent  == -100) {
				Scr->ButtonIndent  = 0;
			}
			if(Scr->TBInfo.border == -100) {
				Scr->TBInfo.border = 0;
			}
		}
		else {
			if(Scr->FramePadding  == -100) {
				Scr->FramePadding  = 2;        /* values that look */
			}
			if(Scr->TitlePadding  == -100) {
				Scr->TitlePadding  = 8;        /* "nice" on */
			}
			if(Scr->ButtonIndent  == -100) {
				Scr->ButtonIndent  = 1;        /* 75 and 100dpi displays */
			}
			if(Scr->TBInfo.border == -100) {
				Scr->TBInfo.border = 1;
			}
			Scr->TitleShadowDepth       = 0;
			Scr->TitleButtonShadowDepth = 0;
		}
		if(! Scr->use3Dborders) {
			Scr->BorderShadowDepth = 0;
		}
		if(! Scr->use3Dmenus) {
			Scr->MenuShadowDepth = 0;
		}
		if(! Scr->use3Diconmanagers) {
			Scr->IconManagerShadowDepth = 0;
		}

		if(Scr->use3Dtitles  && !Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->TitleC);
		}
		if(Scr->use3Dmenus   && !Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->MenuC);
		}
		if(Scr->use3Dmenus   && !Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->MenuTitleC);
		}
		if(Scr->use3Dborders && !Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->BorderColorC);
		}
		if(! Scr->use3Dborders) {
			Scr->ThreeDBorderWidth = 0;
		}

		for(ir = Scr->FirstRegion; ir; ir = ir->next) {
			if(ir->TitleJustification == J_UNDEF) {
				ir->TitleJustification = Scr->IconJustification;
			}
			if(ir->Justification == J_UNDEF) {
				ir->Justification = Scr->IconRegionJustification;
			}
			if(ir->Alignement == J_UNDEF) {
				ir->Alignement = Scr->IconRegionAlignement;
			}
		}

		assign_var_savecolor(); /* storeing pixels for twmrc "entities" */
		if(Scr->SqueezeTitle == -1) {
			Scr->SqueezeTitle = FALSE;
		}
		if(!Scr->HaveFonts) {
			CreateFonts();
		}
		CreateGCs();
		MakeMenus();

		Scr->TitleBarFont.y += Scr->FramePadding;
		Scr->TitleHeight = Scr->TitleBarFont.height + Scr->FramePadding * 2;
		if(Scr->use3Dtitles) {
			Scr->TitleHeight += 2 * Scr->TitleShadowDepth;
		}
		/* make title height be odd so buttons look nice and centered */
		if(!(Scr->TitleHeight & 1)) {
			Scr->TitleHeight++;
		}

		InitTitlebarButtons();          /* menus are now loaded! */

		XGrabServer(dpy);
		XSync(dpy, 0);

		JunkX = 0;
		JunkY = 0;

		CreateWindowRegions();
		AllocateOtherIconManagers();
		CreateIconManagers();
		CreateWorkSpaceManager();
		MakeWorkspacesMenu();
		createWindowBoxes();
#ifdef GNOME
		InitGnome();
#endif /* GNOME */
#ifdef EWMH
		EwmhInitScreenLate(Scr);
#endif /* EWMH */

		XQueryTree(dpy, Scr->Root, &croot, &parent, &children, &nchildren);
		/*
		 * weed out icon windows
		 */
		for(i = 0; i < nchildren; i++) {
			if(children[i]) {
				XWMHints *wmhintsp = XGetWMHints(dpy, children[i]);

				if(wmhintsp) {
					if(wmhintsp->flags & IconWindowHint) {
						for(j = 0; j < nchildren; j++) {
							if(children[j] == wmhintsp->icon_window) {
								children[j] = None;
								break;
							}
						}
					}
					XFree((char *) wmhintsp);
				}
			}
		}

		/*
		 * map all of the non-override windows
		 */
		for(i = 0; i < nchildren; i++) {
			if(children[i] && MappedNotOverride(children[i])) {
				XUnmapWindow(dpy, children[i]);
				SimulateMapRequest(children[i]);
			}
		}
		if(Scr->ShowWorkspaceManager && Scr->workSpaceManagerActive) {
			VirtualScreen *vs;
			if(Scr->WindowMask) {
				XRaiseWindow(dpy, Scr->WindowMask);
			}
			for(vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
				SetMapStateProp(vs->wsw->twm_win, NormalState);
				XMapWindow(dpy, vs->wsw->twm_win->frame);
				if(vs->wsw->twm_win->StartSqueezed) {
					Squeeze(vs->wsw->twm_win);
				}
				else {
					XMapWindow(dpy, vs->wsw->w);
				}
				vs->wsw->twm_win->mapped = TRUE;
			}
		}

		if(!Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->DefaultC);
		}
		attributes.border_pixel = Scr->DefaultC.fore;
		attributes.background_pixel = Scr->DefaultC.back;
		attributes.event_mask = (ExposureMask | ButtonPressMask |
		                         KeyPressMask | ButtonReleaseMask);
		attributes.backing_store = NotUseful;
		attributes.cursor = XCreateFontCursor(dpy, XC_hand2);
		valuemask = (CWBorderPixel | CWBackPixel | CWEventMask |
		             CWBackingStore | CWCursor);
		Scr->InfoWindow = XCreateWindow(dpy, Scr->Root, 0, 0,
		                                (unsigned int) 5, (unsigned int) 5,
		                                (unsigned int) 0, 0,
		                                (unsigned int) CopyFromParent,
		                                (Visual *) CopyFromParent,
		                                valuemask, &attributes);

		XmbTextExtents(Scr->SizeFont.font_set,
		               " 8888 x 8888 ", 13,
		               &ink_rect, &logical_rect);
		Scr->SizeStringWidth = logical_rect.width;
		valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity);
		attributes.bit_gravity = NorthWestGravity;

		{
			int sx, sy;
			if(Scr->CenterFeedbackWindow) {
				sx = (Scr->rootw / 2) - (Scr->SizeStringWidth / 2);
				sy = (Scr->rooth / 2) - ((Scr->SizeFont.height + SIZE_VINDENT * 2) / 2);
				attributes.save_under = True;
				valuemask |= CWSaveUnder;
			}
			else {
				sx = 0;
				sy = 0;
			}
			Scr->SizeWindow = XCreateWindow(dpy, Scr->Root, sx, sy,
			                                (unsigned int) Scr->SizeStringWidth,
			                                (unsigned int)(Scr->SizeFont.height +
			                                                SIZE_VINDENT * 2),
			                                (unsigned int) 0, 0,
			                                (unsigned int) CopyFromParent,
			                                (Visual *) CopyFromParent,
			                                valuemask, &attributes);
		}
		Scr->ShapeWindow = XCreateSimpleWindow(dpy, Scr->Root, 0, 0,
		                                       Scr->rootw, Scr->rooth, 0, 0, 0);

		XUngrabServer(dpy);
		if(ShowWelcomeWindow) {
			UnmaskScreen();
		}

		FirstScreen = FALSE;
		Scr->FirstTime = FALSE;
	} /* for */

	if(numManaged == 0) {
		if(CLarg.MultiScreen && NumScreens > 0)
			fprintf(stderr, "%s:  unable to find any unmanaged screens\n",
			        ProgramName);
		exit(1);
	}
	(void) ConnectToSessionManager(client_id);
#ifdef SOUNDS
	play_startup_sound();
#endif

	RestartPreviousState = True;
	HandlingEvents = TRUE;
	InitEvents();
	StartAnimation();
	HandleEvents();
	fprintf(stderr, "Shouldn't return from HandleEvents()!\n");
	exit(1);
}


static void usage(void)
{
	/* How far to indent continuation lines */
	int llen = 10;

	fprintf(stderr, "usage: %s [(--display | -d) dpy]  "
#ifdef EWMH
	        "[--replace]  "
#endif
	        "[--single]\n", ProgramName);

	fprintf(stderr, "%*s[(--file | -f) initfile]  [--cfgchk]\n", llen, "");

#ifdef USEM4
	fprintf(stderr, "%*s[--nom4 | -n]  [--keep-defs | -k]  "
	        "[(--keep | -K) m4file]\n", llen, "");
#endif

	fprintf(stderr, "%*s[--verbose | -v]  [--quiet | -q]  [--mono]  "
	        "[--xrm resource]\n", llen, "");

	fprintf(stderr, "%*s[--version]  [--info]  [--nowelcome | -W]\n",
	        llen, "");

	fprintf(stderr, "%*s[(--window | -w) [win-id]]  [--name name]\n", llen, "");

	/* Semi-intentionally not documenting --clientId/--restore */

	fprintf(stderr, "%*s[--help]\n", llen, "");


	exit(1);
}


/***********************************************************************
 *
 *  Procedure:
 *      InitVariables - initialize twm variables
 *
 ***********************************************************************
 */

static void InitVariables(void)
{
	FreeList(&Scr->BorderColorL);
	FreeList(&Scr->IconBorderColorL);
	FreeList(&Scr->BorderTileForegroundL);
	FreeList(&Scr->BorderTileBackgroundL);
	FreeList(&Scr->TitleForegroundL);
	FreeList(&Scr->TitleBackgroundL);
	FreeList(&Scr->IconForegroundL);
	FreeList(&Scr->IconBackgroundL);
	FreeList(&Scr->IconManagerFL);
	FreeList(&Scr->IconManagerBL);
	FreeList(&Scr->IconMgrs);
	FreeList(&Scr->AutoPopupL);
	FreeList(&Scr->NoBorder);
	FreeList(&Scr->NoIconTitle);
	FreeList(&Scr->NoTitle);
	FreeList(&Scr->OccupyAll);
	FreeList(&Scr->MakeTitle);
	FreeList(&Scr->AutoRaise);
	FreeList(&Scr->WarpOnDeIconify);
	FreeList(&Scr->AutoLower);
	FreeList(&Scr->IconNames);
	FreeList(&Scr->NoHighlight);
	FreeList(&Scr->NoStackModeL);
	OtpScrInitData(Scr);
	FreeList(&Scr->NoTitleHighlight);
	FreeList(&Scr->DontIconify);
	FreeList(&Scr->IconMgrNoShow);
	FreeList(&Scr->IconMgrShow);
	FreeList(&Scr->IconifyByUn);
	FreeList(&Scr->StartIconified);
	FreeList(&Scr->IconManagerHighlightL);
	FreeList(&Scr->SqueezeTitleL);
	FreeList(&Scr->DontSqueezeTitleL);
	FreeList(&Scr->WindowRingL);
	FreeList(&Scr->WindowRingExcludeL);
	FreeList(&Scr->WarpCursorL);
	FreeList(&Scr->DontSave);
	FreeList(&Scr->UnmapByMovingFarAway);
	FreeList(&Scr->DontSetInactive);
	FreeList(&Scr->AutoSqueeze);
	FreeList(&Scr->StartSqueezed);
	FreeList(&Scr->AlwaysSqueezeToGravityL);
	FreeList(&Scr->IconMenuDontShow);
	FreeList(&Scr->VirtualScreens);
	FreeList(&Scr->IgnoreTransientL);

	NewFontCursor(&Scr->FrameCursor, "top_left_arrow");
	NewFontCursor(&Scr->TitleCursor, "top_left_arrow");
	NewFontCursor(&Scr->IconCursor, "top_left_arrow");
	NewFontCursor(&Scr->IconMgrCursor, "top_left_arrow");
	NewFontCursor(&Scr->MoveCursor, "fleur");
	NewFontCursor(&Scr->ResizeCursor, "fleur");
	NewFontCursor(&Scr->MenuCursor, "sb_left_arrow");
	NewFontCursor(&Scr->ButtonCursor, "hand2");
	NewFontCursor(&Scr->WaitCursor, "watch");
	NewFontCursor(&Scr->SelectCursor, "dot");
	NewFontCursor(&Scr->DestroyCursor, "pirate");
	NewFontCursor(&Scr->AlterCursor, "question_arrow");

	Scr->workSpaceManagerActive = FALSE;
	Scr->Ring = NULL;
	Scr->RingLeader = NULL;

#define SETFB(fld) Scr->fld.fore = Scr->Black; Scr->fld.back = Scr->White;
	SETFB(DefaultC)
	SETFB(BorderColorC)
	SETFB(BorderTileC)
	SETFB(TitleC)
	SETFB(MenuC)
	SETFB(MenuTitleC)
	SETFB(IconC)
	SETFB(IconManagerC)
#undef SETFB

	Scr->MenuShadowColor = Scr->Black;
	Scr->IconBorderColor = Scr->Black;
	Scr->IconManagerHighlight = Scr->Black;

	Scr->FramePadding =
	        -100;   /* trick to have different default value if ThreeDTitles
                                is set or not */
	Scr->TitlePadding = -100;
	Scr->ButtonIndent = -100;
	Scr->SizeStringOffset = 0;
	Scr->ThreeDBorderWidth = 6;
	Scr->BorderWidth = BW;
	Scr->IconBorderWidth = BW;
	Scr->NumAutoRaises = 0;
	Scr->NumAutoLowers = 0;
	Scr->TransientOnTop = 30;
	Scr->NoDefaults = FALSE;
	Scr->UsePPosition = PPOS_OFF;
	Scr->UseSunkTitlePixmap = FALSE;
	Scr->FocusRoot = TRUE;
	Scr->Focus = NULL;
	Scr->WarpCursor = FALSE;
	Scr->ForceIcon = FALSE;
	Scr->NoGrabServer = FALSE;
	Scr->NoRaiseMove = FALSE;
	Scr->NoRaiseResize = FALSE;
	Scr->NoRaiseDeicon = FALSE;
	Scr->RaiseOnWarp = TRUE;
	Scr->DontMoveOff = FALSE;
	Scr->DoZoom = FALSE;
	Scr->TitleFocus = TRUE;
	Scr->IconManagerFocus = TRUE;
	Scr->StayUpMenus = FALSE;
	Scr->WarpToDefaultMenuEntry = FALSE;
	Scr->ClickToFocus = FALSE;
	Scr->SloppyFocus = FALSE;
	Scr->SaveWorkspaceFocus = FALSE;
	Scr->NoIconTitlebar = FALSE;
	Scr->NoTitlebar = FALSE;
	Scr->DecorateTransients = FALSE;
	Scr->IconifyByUnmapping = FALSE;
	Scr->ShowIconManager = FALSE;
	Scr->ShowWorkspaceManager = FALSE;
	Scr->WMgrButtonShadowDepth = 2;
	Scr->WMgrVertButtonIndent  = 5;
	Scr->WMgrHorizButtonIndent = 5;
	Scr->BorderShadowDepth = 2;
	Scr->TitleShadowDepth = 2;
	Scr->TitleButtonShadowDepth = 2;
	Scr->MenuShadowDepth = 2;
	Scr->IconManagerShadowDepth = 2;
	Scr->AutoOccupy = FALSE;
	Scr->TransientHasOccupation = FALSE;
	Scr->DontPaintRootWindow = FALSE;
	Scr->IconManagerDontShow = FALSE;
	Scr->BackingStore = TRUE;
	Scr->SaveUnder = TRUE;
	Scr->RandomPlacement = RP_OFF;
	Scr->RandomDisplacementX = 30;
	Scr->RandomDisplacementY = 30;
	Scr->DoOpaqueMove = FALSE;
	Scr->OpaqueMove = FALSE;
	Scr->OpaqueMoveThreshold = 200;
	Scr->OpaqueResize = FALSE;
	Scr->DoOpaqueResize = FALSE;
	Scr->OpaqueResizeThreshold = 1000;
	Scr->Highlight = TRUE;
	Scr->StackMode = TRUE;
	Scr->TitleHighlight = TRUE;
	Scr->MoveDelta = 1;         /* so that f.deltastop will work */
	Scr->MoveOffResistance = -1;
	Scr->MovePackResistance = 20;
	Scr->ZoomCount = 8;
	Scr->SortIconMgr = FALSE;
	Scr->Shadow = TRUE;
	Scr->InterpolateMenuColors = FALSE;
	Scr->NoIconManagers = FALSE;
	Scr->ClientBorderWidth = FALSE;
	Scr->SqueezeTitle = -1;
	Scr->FirstRegion = NULL;
	Scr->LastRegion = NULL;
	Scr->FirstWindowRegion = NULL;
	Scr->FirstTime = TRUE;
	Scr->HaveFonts = FALSE;             /* i.e. not loaded yet */
	Scr->CaseSensitive = TRUE;
	Scr->WarpUnmapped = FALSE;
	Scr->WindowRingAll = FALSE;
	Scr->WarpRingAnyWhere = TRUE;
	Scr->ShortAllWindowsMenus = FALSE;
	Scr->use3Diconmanagers = FALSE;
	Scr->use3Dmenus = FALSE;
	Scr->use3Dtitles = FALSE;
	Scr->use3Dborders = FALSE;
	Scr->use3Dwmap = FALSE;
	Scr->SunkFocusWindowTitle = FALSE;
	Scr->ClearShadowContrast = 50;
	Scr->DarkShadowContrast  = 40;
	Scr->BeNiceToColormap = FALSE;
	Scr->BorderCursors = FALSE;
	Scr->IconJustification = J_CENTER;
	Scr->IconRegionJustification = J_CENTER;
	Scr->IconRegionAlignement = J_CENTER;
	Scr->TitleJustification = J_LEFT;
	Scr->IconifyStyle = ICONIFY_NORMAL;
	Scr->MaxIconTitleWidth = Scr->rootw;
	Scr->ReallyMoveInWorkspaceManager = FALSE;
	Scr->ShowWinWhenMovingInWmgr = FALSE;
	Scr->ReverseCurrentWorkspace = FALSE;
	Scr->DontWarpCursorInWMap = FALSE;
	Scr->XMoveGrid = 1;
	Scr->YMoveGrid = 1;
	Scr->FastServer = True;
	Scr->CenterFeedbackWindow = False;
	Scr->ShrinkIconTitles = False;
	Scr->AutoRaiseIcons = False;
	Scr->AutoFocusToTransients = False; /* kai */
	Scr->use3Diconborders = False;
	Scr->OpenWindowTimeout = 0;
	Scr->RaiseWhenAutoUnSqueeze = False;
	Scr->RaiseOnClick = False;
	Scr->RaiseOnClickButton = 1;
	Scr->IgnoreModifier = 0;
	Scr->IgnoreCaseInMenuSelection = False;
	Scr->PackNewWindows = False;
	Scr->AlwaysSqueezeToGravity = FALSE;
	Scr->NoWarpToMenuTitle = FALSE;
	Scr->DontToggleWorkspaceManagerState = False;
#ifdef EWMH
	Scr->PreferredIconWidth = 48;
	Scr->PreferredIconHeight = 48;
	FreeList(&Scr->EWMHIgnore);
#endif

	Scr->BorderTop    = 0;
	Scr->BorderBottom = 0;
	Scr->BorderLeft   = 0;
	Scr->BorderRight  = 0;

	/* setup default fonts; overridden by defaults from system.twmrc */

#   define DEFAULT_NICE_FONT "-*-helvetica-bold-r-normal-*-*-120-*"
#   define DEFAULT_FAST_FONT "-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-*"

	Scr->TitleBarFont.font_set = NULL;
	Scr->TitleBarFont.basename = DEFAULT_NICE_FONT;
	Scr->MenuFont.font_set = NULL;
	Scr->MenuFont.basename = DEFAULT_NICE_FONT;
	Scr->IconFont.font_set = NULL;
	Scr->IconFont.basename = DEFAULT_NICE_FONT;
	Scr->SizeFont.font_set = NULL;
	Scr->SizeFont.basename = DEFAULT_FAST_FONT;
	Scr->IconManagerFont.font_set = NULL;
	Scr->IconManagerFont.basename = DEFAULT_NICE_FONT;
	Scr->DefaultFont.font_set = NULL;
	Scr->DefaultFont.basename = DEFAULT_FAST_FONT;
	Scr->workSpaceMgr.windowFont.font_set = NULL;
	Scr->workSpaceMgr.windowFont.basename = DEFAULT_FAST_FONT;
}


void CreateFonts(void)
{
	GetFont(&Scr->TitleBarFont);
	GetFont(&Scr->MenuFont);
	GetFont(&Scr->IconFont);
	GetFont(&Scr->SizeFont);
	GetFont(&Scr->IconManagerFont);
	GetFont(&Scr->DefaultFont);
	GetFont(&Scr->workSpaceMgr.windowFont);
	Scr->HaveFonts = TRUE;
}


void RestoreWithdrawnLocation(TwmWindow *tmp)
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

		if(tmp->wmhints && (tmp->wmhints->flags & IconWindowHint)) {
			XUnmapWindow(dpy, tmp->wmhints->icon_window);
		}

	}
}


/***********************************************************************
 *
 *  Procedure:
 *      Done - cleanup and exit twm
 *
 *  Returned Value:
 *      none
 *
 *  Inputs:
 *      none
 *
 *  Outputs:
 *      none
 *
 *  Special Considerations:
 *      none
 *
 ***********************************************************************
 */

void Reborder(Time mytime)
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
	SetFocus((TwmWindow *)NULL, mytime);
}

SIGNAL_T Done(int signum)
{
#ifdef SOUNDS
	play_exit_sound();
#endif
	Reborder(CurrentTime);
#ifdef EWMH
	EwmhTerminate();
#endif /* EWMH */
	XDeleteProperty(dpy, Scr->Root, XA_WM_WORKSPACESLIST);
	if(captive) {
		RemoveFromCaptiveList();
	}
	XCloseDisplay(dpy);
	exit(0);
}

SIGNAL_T Crash(int signum)
{
	Reborder(CurrentTime);
	XDeleteProperty(dpy, Scr->Root, XA_WM_WORKSPACESLIST);
	if(captive) {
		RemoveFromCaptiveList();
	}
	XCloseDisplay(dpy);

	fprintf(stderr, "\nCongratulations, you have found a bug in ctwm\n");
	fprintf(stderr, "If a core file was generated in your directory,\n");
	fprintf(stderr, "can you please try extract the stack trace,\n");
	fprintf(stderr,
	        "and mail the results, and a description of what you were doing,\n");
	fprintf(stderr, "to ctwm@ctwm.org.  Thank you for your support.\n");
	fprintf(stderr, "...exiting ctwm now.\n\n");

	abort();
}


SIGNAL_T Restart(int signum)
{
	fprintf(stderr, "%s:  setting restart flag\n", ProgramName);
	RestartFlag = 1;
}

void DoRestart(Time t)
{
	RestartFlag = 0;

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

#ifdef __WAIT_FOR_CHILDS
/*
 * Handler for SIGCHLD. Needed to avoid zombies when an .xinitrc
 * execs ctwm as the last client. (All processes forked off from
 * within .xinitrc have been inherited by ctwm during the exec.)
 * Jens Schweikhardt <jens@kssun3.rus.uni-stuttgart.de>
 */
SIGNAL_T
ChildExit(int signum)
{
	int Errno = errno;
	signal(SIGCHLD, ChildExit);  /* reestablish because we're a one-shot */
	waitpid(-1, NULL, WNOHANG);   /* reap dead child, ignore status */
	errno = Errno;               /* restore errno for interrupted sys calls */
}
#endif

/*
 * Error Handlers.  If a client dies, we'll get a BadWindow error (except for
 * GetGeometry which returns BadDrawable) for most operations that we do before
 * manipulating the client's window.
 */

static Bool ErrorOccurred = False;
static XErrorEvent LastErrorEvent;

static int TwmErrorHandler(Display *display, XErrorEvent *event)
{
	LastErrorEvent = *event;
	ErrorOccurred = True;

	if(CLarg.PrintErrorMessages &&                 /* don't be too obnoxious */
	                event->error_code != BadWindow &&       /* watch for dead puppies */
	                (event->request_code != X_GetGeometry &&         /* of all styles */
	                 event->error_code != BadDrawable)) {
		XmuPrintDefaultErrorMessage(display, event, stderr);
	}
	return 0;
}


/* ARGSUSED*/
static int CatchRedirectError(Display *display, XErrorEvent *event)
{
	RedirectError = TRUE;
	LastErrorEvent = *event;
	ErrorOccurred = True;
	return 0;
}

/*
 * XA_MIT_PRIORITY_COLORS     Create priority colors if necessary.
 * XA_WM_END_OF_ANIMATION     Used to throttle animation.
 */

Atom XA_WM_WORKSPACESLIST;
Atom XCTWMAtom[NUM_CTWM_XATOMS];

void InternUsefulAtoms(void)
{
	XInternAtoms(dpy, XCTWMAtomNames, NUM_CTWM_XATOMS, False, XCTWMAtom);

#ifdef GNOME
	XA_WM_WORKSPACESLIST   = XInternAtom(dpy, "_WIN_WORKSPACE_NAMES", False);
#else /* GNOME */
	XA_WM_WORKSPACESLIST   = XInternAtom(dpy, "WM_WORKSPACESLIST", False);
#endif /* GNOME */
}

static Window CreateRootWindow(int x, int y,
                               unsigned int width, unsigned int height)
{
	int         scrnum;
	Window      ret;
	XWMHints    wmhints;

	scrnum = DefaultScreen(dpy);
	ret = XCreateSimpleWindow(dpy, RootWindow(dpy, scrnum),
	                          x, y, width, height, 2, WhitePixel(dpy, scrnum),
	                          BlackPixel(dpy, scrnum));
	XSetStandardProperties(dpy, ret, "Captive ctwm", NULL, None, NULL, 0, NULL);
	wmhints.initial_state = NormalState;
	wmhints.input         = True;
	wmhints.flags         = InputHint | StateHint;
	XSetWMHints(dpy, ret, &wmhints);

	XChangeProperty(dpy, ret, XA_WM_CTWM_ROOT, XA_WINDOW, 32,
	                PropModeReplace, (unsigned char *) &ret, 1);
	XSelectInput(dpy, ret, StructureNotifyMask);
	XMapWindow(dpy, ret);
	return (ret);
}

static void DisplayInfo(void)
{
	(void) printf("Twm version:  %s\n", Version);
	(void) printf("Compile time options :");
#ifdef XPM
	(void) printf(" XPM");
#endif
#ifdef USEM4
	(void) printf(" USEM4");
#endif
#ifdef SOUNDS
	(void) printf(" SOUNDS");
#endif
#ifdef GNOME
	(void) printf(" GNOME");
#endif
#ifdef EWMH
	(void) printf(" EWMH");
#endif
	(void) printf(" I18N");
	(void) printf("\n");
}

