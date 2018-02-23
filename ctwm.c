/*
 *       Copyright 1988 by Evans & Sutherland Computer Corporation,
 *                          Salt Lake City, Utah
 *  Portions Copyright 1989 by the Massachusetts Institute of Technology
 *                        Cambridge, Massachusetts
 *
 * $XConsortium: twm.c,v 1.124 91/05/08 11:01:54 dave Exp $
 *
 * twm - "Tom's Window Manager"
 *
 * 27-Oct-87 Thomas E. LaStrange        File created
 * 10-Oct-90 David M. Sternlicht        Storing saved colors on root
 *
 * Copyright 1992 Claude Lecommandeur.
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>

#ifdef __WAIT_FOR_CHILDS
#  include <sys/wait.h>
#endif

#include <fcntl.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Error.h>
#include <X11/extensions/shape.h>


#include "ctwm_atoms.h"
#include "clargs.h"
#include "add_window.h"
#include "gc.h"
#include "parse.h"
#include "version.h"
#include "colormaps.h"
#include "events.h"
#include "util.h"
#include "mask_screen.h"
#include "animate.h"
#include "screen.h"
#include "icons.h"
#include "iconmgr.h"
#include "list.h"
#include "session.h"
#include "occupation.h"
#include "otp.h"
#include "cursor.h"
#include "windowbox.h"
#include "captive.h"
#ifdef XRANDR
#include "xrandr.h"
#else
#include "r_area.h"
#include "r_area_list.h"
#endif
#include "vscreen.h"
#include "win_decorations_init.h"
#include "win_ops.h"
#include "win_regions.h"
#include "win_utils.h"
#include "workspace_manager.h"
#ifdef SOUNDS
#  include "sound.h"
#endif

#include "gram.tab.h"


XtAppContext appContext;        /* Xt application context */
Display *dpy;                   /* which display are we talking to */
Window ResizeWindow;            /* the window we are resizing */

int NumScreens;                 /* number of screens in ScreenList */
bool HasShape;                  /* server supports shape extension? */
int ShapeEventBase, ShapeErrorBase;
ScreenInfo **ScreenList;        /* structures for each screen */
ScreenInfo *Scr = NULL;         /* the cur and prev screens */
int PreviousScreen;             /* last screen that we were on */
static bool FirstScreen;        /* true ==> first screen of display */
static bool RedirectError;      /* true ==> another window manager running */
/* for settting RedirectError */
static int CatchRedirectError(Display *display, XErrorEvent *event);
/* for everything else */
static int TwmErrorHandler(Display *display, XErrorEvent *event);
static Window CreateRootWindow(int x, int y,
                               unsigned int width, unsigned int height);
static void InternUsefulAtoms(void);
static void InitVariables(void);
static bool MappedNotOverride(Window w);

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

bool HandlingEvents = false;    /* are we handling events yet? */

/*
 * Various junk vars for xlib calls.  Many calls have to get passed these
 * pointers to return values into, but in a lot of cases we don't care
 * about some/all of them, and since xlib blindly derefs and stores into
 * them, we can't just pass NULL for the ones we don't care about.  So we
 * make this set of globals to use as standin.  These should never be
 * used or read in our own code; use real vars for the values we DO use
 * from the calls.
 */
Window JunkRoot, JunkChild;
int JunkX, JunkY;
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

char *ProgramName;
int Argc;
char **Argv;

bool RestartPreviousState = true;      /* try to restart in previous state */

bool RestartFlag = false;
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

int main(int argc, char **argv)
{
	Window croot, parent, *children;
	unsigned int nchildren;
	int i, j;
	unsigned long valuemask;    /* mask for create windows */
	XSetWindowAttributes attributes;    /* attributes for create windows */
	int numManaged, firstscrn, lastscrn, scrnum;
	int zero = 0;
	char *welcomefile;
	bool screenmasked;
	static int crootx = 100;
	static int crooty = 100;
	static unsigned int crootw = 1280;
	static unsigned int crooth =  768;
	/*    static unsigned int crootw = 2880; */
	/*    static unsigned int crooth = 1200; */
	IconRegion *ir;

	XRectangle ink_rect;
	XRectangle logical_rect;

	setlocale(LC_ALL, "");

	ProgramName = argv[0];
	Argc = argc;
	Argv = argv;


	/*
	 * Run consistency check.  This is mostly to keep devs from
	 * accidental screwups; if a user ever sees anything from this,
	 * something went very very wrong.
	 */
	chk_keytable_order();

	/*
	 * Parse-out command line args, and check the results.
	 */
	clargs_parse(argc, argv);
	clargs_check();
	/* If we get this far, it was all good */


#define newhandler(sig, action) \
    if (signal (sig, SIG_IGN) != SIG_IGN) signal (sig, action)

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
	if(CLarg.restore_filename) {
		ReadWinConfigFile(CLarg.restore_filename);
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

	/* for simplicity, always allocate NumScreens ScreenInfo struct pointers */
	ScreenList = calloc(NumScreens, sizeof(ScreenInfo *));
	if(ScreenList == NULL) {
		fprintf(stderr, "%s: Unable to allocate memory for screen list, exiting.\n",
		        ProgramName);
		exit(1);
	}
	numManaged = 0;
	PreviousScreen = DefaultScreen(dpy);
	FirstScreen = true;
#ifdef EWMH
	EwmhInit();
#endif /* EWMH */
#ifdef SOUNDS
	sound_init();
#endif

	for(scrnum = firstscrn ; scrnum <= lastscrn; scrnum++) {
		unsigned long attrmask;
		if(CLarg.is_captive) {
			XWindowAttributes wa;
			if(CLarg.capwin && XGetWindowAttributes(dpy, CLarg.capwin, &wa)) {
				Window junk;
				croot  = CLarg.capwin;
				crootw = wa.width;
				crooth = wa.height;
				XTranslateCoordinates(dpy, CLarg.capwin, wa.root, 0, 0, &crootx, &crooty,
				                      &junk);
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
		Scr = ScreenList[scrnum] = calloc(1, sizeof(ScreenInfo));
		if(Scr == NULL) {
			fprintf(stderr,
			        "%s: unable to allocate memory for ScreenInfo structure"
			        " for screen %d.\n",
			        ProgramName, scrnum);
			continue;
		}

		/*
		 * Generally, we're taking over the screen, but not always.  If
		 * we're just checking the config, we're not trying to take it
		 * over.  Nor are we if we're creating a captive ctwm.
		 */
		Scr->takeover = true;
		if(CLarg.cfgchk || CLarg.is_captive) {
			Scr->takeover = false;
		}

		Scr->screen = scrnum;
		Scr->XineramaRoot = croot;
#ifdef EWMH
		EwmhInitScreenEarly(Scr);
#endif /* EWMH */
		RedirectError = false;
		XSetErrorHandler(CatchRedirectError);
		attrmask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
		           SubstructureRedirectMask | KeyPressMask | ButtonPressMask |
		           ButtonReleaseMask;
#ifdef EWMH
		attrmask |= StructureNotifyMask;
#endif /* EWMH */
		if(CLarg.is_captive) {
			attrmask |= StructureNotifyMask;
		}
		XSelectInput(dpy, croot, attrmask);
		XSync(dpy, 0);
		XSetErrorHandler(TwmErrorHandler);

		if(RedirectError && Scr->takeover) {
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

		Scr->screen = scrnum;
		Scr->d_depth = DefaultDepth(dpy, scrnum);
		Scr->d_visual = DefaultVisual(dpy, scrnum);
		Scr->RealRoot = RootWindow(dpy, scrnum);
		Scr->CaptiveRoot = CLarg.is_captive ? croot : None;
		Scr->Root = croot;
		Scr->XineramaRoot = croot;
		Scr->ShowWelcomeWindow = CLarg.ShowWelcomeWindow;

#ifdef XRANDR
		Scr->Layout = XrandrNewLayout(dpy, Scr->XineramaRoot);
		if(Scr->Layout == NULL) {
			continue;
		}
#else
		Scr->Layout = RLayoutNew(
		                      RAreaListNew(1,
		                                   RAreaNew(Scr->rootx,
		                                                   Scr->rooty,
		                                                   Scr->rootw,
		                                                   Scr->rooth),
		                                   NULL));
#endif
#ifdef DEBUG
		fprintf(stderr, "Full: ");
		RLayoutPrint(Scr->Layout);
#endif

		XSaveContext(dpy, Scr->Root, ScreenContext, (XPointer) Scr);

		if(CLarg.is_captive) {
			Scr->captivename = AddToCaptiveList(CLarg.captivename);
			if(Scr->captivename) {
				XmbSetWMProperties(dpy, croot,
				                   Scr->captivename, Scr->captivename,
				                   NULL, 0, NULL, NULL, NULL);
			}
		}
		Scr->RootColormaps.number_cwins = 1;
		Scr->RootColormaps.cwins = malloc(sizeof(ColormapWindow *));
		Scr->RootColormaps.cwins[0] = CreateColormapWindow(Scr->Root, true, false);
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
		Scr->FirstTime = true;
		GetColor(Scr->Monochrome, &(Scr->Black), "black");
		GetColor(Scr->Monochrome, &(Scr->White), "white");

		if(FirstScreen) {
			SetFocus(NULL, CurrentTime);

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
		screenmasked = false;
		/* XXX Happens before config parse, so ignores DontShowWW param */
		if(Scr->ShowWelcomeWindow && (welcomefile = getenv("CTWM_WELCOME_FILE"))) {
			screenmasked = true;
			MaskScreen(welcomefile);
		}
		InitVariables();
		InitMenus();
		InitWorkSpaceManager();

		/* Parse it once for each screen. */
		if(CLarg.cfgchk) {
			if(ParseTwmrc(CLarg.InitFile) == false) {
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

		/* At least one border around the screen */
		Scr->BorderedLayout = RLayoutCopyCropped(Scr->Layout,
		                      Scr->BorderLeft, Scr->BorderRight,
		                      Scr->BorderTop, Scr->BorderBottom);
		if(Scr->BorderedLayout == NULL) {
			Scr->BorderedLayout = Scr->Layout;        // nothing to crop
		}
		else if(Scr->BorderedLayout->monitors->len == 0) {
			fprintf(stderr,
			        "Borders too large! correct BorderLeft, BorderRight, BorderTop and/or BorderBottom parameters\n");
			exit(1);
		}
#ifdef DEBUG
		fprintf(stderr, "Bordered: ");
		RLayoutPrint(Scr->BorderedLayout);
#endif

		InitVirtualScreens(Scr);
#ifdef EWMH
		EwmhInitVirtualRoots(Scr);
#endif /* EWMH */
		ConfigureWorkSpaceManager();

		if(Scr->ShowWelcomeWindow && ! screenmasked) {
			MaskScreen(NULL);
		}
		if(Scr->ClickToFocus) {
			Scr->FocusRoot  = false;
			Scr->TitleFocus = false;
		}


		if(Scr->use3Dborders) {
			Scr->ClientBorderWidth = false;
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
			if(ir->TitleJustification == TJ_UNDEF) {
				ir->TitleJustification = Scr->IconJustification;
			}
			if(ir->Justification == IRJ_UNDEF) {
				ir->Justification = Scr->IconRegionJustification;
			}
			if(ir->Alignement == IRA_UNDEF) {
				ir->Alignement = Scr->IconRegionAlignement;
			}
		}

		assign_var_savecolor(); /* storeing pixels for twmrc "entities" */
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

		CreateWindowRegions();
		AllocateOtherIconManagers();
		CreateIconManagers();
		CreateWorkSpaceManager();
		CreateOccupyWindow();
		MakeWorkspacesMenu();
		createWindowBoxes();
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
					XFree(wmhintsp);
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
				vs->wsw->twm_win->mapped = true;
			}
		}

		if(!Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->DefaultC);
		}
		attributes.border_pixel = Scr->DefaultC.fore;
		attributes.background_pixel = Scr->DefaultC.back;
		attributes.event_mask = (ExposureMask | ButtonPressMask |
		                         KeyPressMask | ButtonReleaseMask);
		NewFontCursor(&attributes.cursor, "hand2");
		valuemask = (CWBorderPixel | CWBackPixel | CWEventMask | CWCursor);
		Scr->InfoWindow.win =
		        XCreateWindow(dpy, Scr->Root, 0, 0,
		                      5, 5,
		                      0, 0,
		                      CopyFromParent, CopyFromParent,
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
				if(Scr->SaveUnder) {
					attributes.save_under = True;
					valuemask |= CWSaveUnder;
				}
			}
			else {
				sx = 0;
				sy = 0;
			}
			Scr->SizeWindow = XCreateWindow(dpy, Scr->Root, sx, sy,
			                                Scr->SizeStringWidth,
			                                (Scr->SizeFont.height +
			                                 SIZE_VINDENT * 2),
			                                0, 0,
			                                CopyFromParent,
			                                CopyFromParent,
			                                valuemask, &attributes);
		}
		Scr->ShapeWindow = XCreateSimpleWindow(dpy, Scr->Root, 0, 0,
		                                       Scr->rootw, Scr->rooth, 0, 0, 0);

		XUngrabServer(dpy);
		if(Scr->ShowWelcomeWindow) {
			UnmaskScreen();
		}

		FirstScreen = false;
		Scr->FirstTime = false;
	} /* for */

	if(numManaged == 0) {
		if(CLarg.MultiScreen && NumScreens > 0)
			fprintf(stderr, "%s:  unable to find any unmanaged screens\n",
			        ProgramName);
		exit(1);
	}
	ConnectToSessionManager(CLarg.client_id);
#ifdef SOUNDS
	sound_load_list();
	play_startup_sound();
#endif

	RestartPreviousState = true;
	HandlingEvents = true;
	InitEvents();
	StartAnimation();
	HandleEvents();
	fprintf(stderr, "Shouldn't return from HandleEvents()!\n");
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

	Scr->workSpaceManagerActive = false;
	Scr->Ring = NULL;
	Scr->RingLeader = NULL;
	Scr->ShowWelcomeWindow = CLarg.ShowWelcomeWindow;

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
	Scr->NoDefaults = false;
	Scr->UsePPosition = PPOS_OFF;
	Scr->UseSunkTitlePixmap = false;
	Scr->FocusRoot = true;
	Scr->Focus = NULL;
	Scr->WarpCursor = false;
	Scr->ForceIcon = false;
	Scr->NoGrabServer = true;
	Scr->NoRaiseMove = false;
	Scr->NoRaiseResize = false;
	Scr->NoRaiseDeicon = false;
	Scr->RaiseOnWarp = true;
	Scr->DontMoveOff = false;
	Scr->DoZoom = false;
	Scr->TitleFocus = true;
	Scr->IconManagerFocus = true;
	Scr->StayUpMenus = false;
	Scr->WarpToDefaultMenuEntry = false;
	Scr->ClickToFocus = false;
	Scr->SloppyFocus = false;
	Scr->SaveWorkspaceFocus = false;
	Scr->NoIconTitlebar = false;
	Scr->NoTitlebar = false;
	Scr->DecorateTransients = true;
	Scr->IconifyByUnmapping = false;
	Scr->ShowIconManager = false;
	Scr->ShowWorkspaceManager = false;
	Scr->WMgrButtonShadowDepth = 2;
	Scr->WMgrVertButtonIndent  = 5;
	Scr->WMgrHorizButtonIndent = 5;
	Scr->BorderShadowDepth = 2;
	Scr->TitleShadowDepth = 2;
	Scr->TitleButtonShadowDepth = 2;
	Scr->MenuShadowDepth = 2;
	Scr->IconManagerShadowDepth = 2;
	Scr->AutoOccupy = false;
	Scr->TransientHasOccupation = false;
	Scr->DontPaintRootWindow = false;
	Scr->IconManagerDontShow = false;
	Scr->BackingStore = false;
	Scr->SaveUnder = true;
	Scr->RandomPlacement = RP_ALL;
	Scr->RandomDisplacementX = 30;
	Scr->RandomDisplacementY = 30;
	Scr->DoOpaqueMove = true;
	Scr->OpaqueMove = false;
	Scr->OpaqueMoveThreshold = 200;
	Scr->OpaqueResize = false;
	Scr->DoOpaqueResize = true;
	Scr->OpaqueResizeThreshold = 1000;
	Scr->Highlight = true;
	Scr->StackMode = true;
	Scr->TitleHighlight = true;
	Scr->MoveDelta = 1;         /* so that f.deltastop will work */
	Scr->MoveOffResistance = -1;
	Scr->MovePackResistance = 20;
	Scr->ZoomCount = 8;
	Scr->SortIconMgr = true;
	Scr->Shadow = true;
	Scr->InterpolateMenuColors = false;
	Scr->NoIconManagers = false;
	Scr->ClientBorderWidth = false;
	Scr->SqueezeTitle = false;
	Scr->FirstRegion = NULL;
	Scr->LastRegion = NULL;
	Scr->FirstWindowRegion = NULL;
	Scr->FirstTime = true;
	Scr->HaveFonts = false;             /* i.e. not loaded yet */
	Scr->CaseSensitive = true;
	Scr->WarpUnmapped = false;
	Scr->WindowRingAll = false;
	Scr->WarpRingAnyWhere = true;
	Scr->ShortAllWindowsMenus = false;
	Scr->use3Diconmanagers = false;
	Scr->use3Dmenus = false;
	Scr->use3Dtitles = false;
	Scr->use3Dborders = false;
	Scr->use3Dwmap = false;
	Scr->SunkFocusWindowTitle = false;
	Scr->ClearShadowContrast = 50;
	Scr->DarkShadowContrast  = 40;
	Scr->BeNiceToColormap = false;
	Scr->BorderCursors = false;
	Scr->IconJustification = TJ_CENTER;
	Scr->IconRegionJustification = IRJ_CENTER;
	Scr->IconRegionAlignement = IRA_CENTER;
	Scr->TitleJustification = TJ_LEFT;
	Scr->IconifyStyle = ICONIFY_NORMAL;
	Scr->MaxIconTitleWidth = Scr->rootw;
	Scr->ReallyMoveInWorkspaceManager = false;
	Scr->ShowWinWhenMovingInWmgr = false;
	Scr->ReverseCurrentWorkspace = false;
	Scr->DontWarpCursorInWMap = false;
	Scr->XMoveGrid = 1;
	Scr->YMoveGrid = 1;
	Scr->CenterFeedbackWindow = false;
	Scr->ShrinkIconTitles = false;
	Scr->AutoRaiseIcons = false;
	Scr->AutoFocusToTransients = false; /* kai */
	Scr->OpenWindowTimeout = 0;
	Scr->RaiseWhenAutoUnSqueeze = false;
	Scr->RaiseOnClick = false;
	Scr->RaiseOnClickButton = 1;
	Scr->IgnoreModifier = 0;
	Scr->IgnoreCaseInMenuSelection = false;
	Scr->PackNewWindows = false;
	Scr->AlwaysSqueezeToGravity = false;
	Scr->NoWarpToMenuTitle = false;
	Scr->DontToggleWorkspaceManagerState = false;
	Scr->NameDecorations = true;
#ifdef EWMH
	Scr->PreferredIconWidth = 48;
	Scr->PreferredIconHeight = 48;
	FreeList(&Scr->EWMHIgnore);
#endif
	FreeList(&Scr->MWMIgnore);

	Scr->ForceFocus = false;
	FreeList(&Scr->ForceFocusL);

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
	Scr->HaveFonts = true;
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

		if(tmp->wmhints->flags & IconWindowHint) {
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
	SetFocus(NULL, mytime);
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
	if(CLarg.is_captive) {
		RemoveFromCaptiveList(Scr->captivename);
	}
	XCloseDisplay(dpy);
	exit(0);
}

SIGNAL_T Crash(int signum)
{
	Reborder(CurrentTime);
	XDeleteProperty(dpy, Scr->Root, XA_WM_WORKSPACESLIST);
	if(CLarg.is_captive) {
		RemoveFromCaptiveList(Scr->captivename);
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
	RestartFlag = true;
}

void DoRestart(Time t)
{
	RestartFlag = false;

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

static XErrorEvent LastErrorEvent;

static int TwmErrorHandler(Display *display, XErrorEvent *event)
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
static int CatchRedirectError(Display *display, XErrorEvent *event)
{
	RedirectError = true;
	LastErrorEvent = *event;
	return 0;
}

/*
 * XA_MIT_PRIORITY_COLORS     Create priority colors if necessary.
 * XA_WM_END_OF_ANIMATION     Used to throttle animation.
 */

Atom XCTWMAtom[NUM_CTWM_XATOMS];

void InternUsefulAtoms(void)
{
	XInternAtoms(dpy, XCTWMAtomNames, NUM_CTWM_XATOMS, False, XCTWMAtom);
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
	wmhints.initial_state = NormalState;
	wmhints.input         = True;
	wmhints.flags         = InputHint | StateHint;

	XmbSetWMProperties(dpy, ret, "Captive ctwm", NULL, NULL, 0, NULL,
	                   &wmhints, NULL);
	XChangeProperty(dpy, ret, XA_WM_CTWM_ROOT, XA_WINDOW, 32,
	                PropModeReplace, (unsigned char *) &ret, 1);
	XSelectInput(dpy, ret, StructureNotifyMask);
	XMapWindow(dpy, ret);
	return (ret);
}


/*
 * Return true if a window is not set to override_redirect ("Hey!  WM!
 * Leave those wins alone!"), and isn't unmapped.  Used during startup to
 * fake mapping for wins that should be up.
 */
static bool
MappedNotOverride(Window w)
{
	XWindowAttributes wa;

	XGetWindowAttributes(dpy, w, &wa);
	return ((wa.map_state != IsUnmapped) && (wa.override_redirect != True));
}
