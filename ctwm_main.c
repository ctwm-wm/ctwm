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
#include "ctwm_main.h"
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
static bool RedirectError;      /* true ==> another window manager running */
/* for settting RedirectError */
static int CatchRedirectError(Display *display, XErrorEvent *event);
/* for everything else */
static int TwmErrorHandler(Display *display, XErrorEvent *event);
static Window CreateCaptiveRootWindow(int x, int y,
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

int
ctwm_main(int argc, char *argv[])
{
	int numManaged, firstscrn, lastscrn;
	bool FirstScreen;

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

	// Various bits of code care about $HOME
	Home = getenv("HOME");
	if(Home == NULL) {
		Home = "./";
	}
	HomeLen = strlen(Home);


	// XXX This is only used in AddWindow(), and is probably bogus to
	// have globally....
	NoClass.res_name = NoName;
	NoClass.res_class = NoName;


	/*
	 * Initialize our X connection and state bits.
	 */
	{
		int zero = 0; // Fakey

		XtToolkitInitialize();
		appContext = XtCreateApplicationContext();

		if(!(dpy = XtOpenDisplay(appContext, CLarg.display_name, "twm", "twm",
		                         NULL, 0, &zero, NULL))) {
			fprintf(stderr, "%s:  unable to open display \"%s\"\n",
			        ProgramName, XDisplayName(CLarg.display_name));
			exit(1);
		}

		if(fcntl(ConnectionNumber(dpy), F_SETFD, FD_CLOEXEC) == -1) {
			fprintf(stderr,
			        "%s:  unable to mark display connection as close-on-exec\n",
			        ProgramName);
			exit(1);
		}
	}


	// Load session stuff
	if(CLarg.restore_filename) {
		ReadWinConfigFile(CLarg.restore_filename);
	}

	// Load up info about X extensions
	HasShape = XShapeQueryExtension(dpy, &ShapeEventBase, &ShapeErrorBase);

	// Allocate contexts/atoms/etc we use
	TwmContext = XUniqueContext();
	MenuContext = XUniqueContext();
	ScreenContext = XUniqueContext();
	ColormapContext = XUniqueContext();

	InternUsefulAtoms();

	// Allocate/define common cursors
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


	// Prep up the per-screen global info
	NumScreens = ScreenCount(dpy);
	if(CLarg.MultiScreen) {
		firstscrn = 0;
		lastscrn = NumScreens - 1;
	}
	else {
		firstscrn = lastscrn = DefaultScreen(dpy);
	}

	// For simplicity, pre-allocate NumScreens ScreenInfo struct pointers
	ScreenList = calloc(NumScreens, sizeof(ScreenInfo *));
	if(ScreenList == NULL) {
		fprintf(stderr, "%s: Unable to allocate memory for screen list, exiting.\n",
		        ProgramName);
		exit(1);
	}

	// Initialize
	PreviousScreen = DefaultScreen(dpy);


	// Do a little early initialization
#ifdef EWMH
	EwmhInit();
#endif /* EWMH */
#ifdef SOUNDS
	sound_init();
#endif

	// Start looping over the screens
	numManaged = 0;
	FirstScreen = true;
	for(int scrnum = firstscrn ; scrnum <= lastscrn; scrnum++) {
		Window croot;
		unsigned long attrmask;
		int crootx, crooty;
		unsigned int crootw, crooth;
		bool screenmasked;
		char *welcomefile;

		/*
		 * First, setup the root window for the screen.
		 */
		if(CLarg.is_captive) {
			// Captive ctwm.  We make a fake root.
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
				// Fake up default size.  Probably Ideally should be
				// configurable, but even more ideally we wouldn't have
				// captive...
				crootx = crooty = 100;
				crootw = 1280;
				crooth = 768;
				croot = CreateCaptiveRootWindow(crootx, crooty, crootw, crooth);
			}
		}
		else {
			// Normal; get the real display's root.
			croot  = RootWindow(dpy, scrnum);
			crootx = 0;
			crooty = 0;
			crootw = DisplayWidth(dpy, scrnum);
			crooth = DisplayHeight(dpy, scrnum);
		}

		// Initialize to empty.  SaveColor{} will set extra values to
		// add; x-ref assign_var_savecolor() call below.
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
		 * Initialize bits of Scr struct that we can hard-know or are
		 * needed in these early initialization steps.
		 */
		Scr->screen = scrnum;
		Scr->XineramaRoot = Scr->Root = croot;
		Scr->rootx = Scr->crootx = crootx;
		Scr->rooty = Scr->crooty = crooty;
		Scr->rootw = Scr->crootw = crootw;
		Scr->rooth = Scr->crooth = crooth;
		Scr->MaxWindowWidth  = 32767 - Scr->rootw;
		Scr->MaxWindowHeight = 32767 - Scr->rooth;

		// Generally we're trying to take over managing the screen.
		Scr->takeover = true;
		if(CLarg.cfgchk || CLarg.is_captive) {
			// Not if we're just checking config or making a new captive
			// ctwm, though.
			Scr->takeover = false;
		}


#ifdef EWMH
		// Early EWMH setup
		EwmhInitScreenEarly(Scr);
#endif /* EWMH */


		/*
		 * Subscribe to various events on the root window.  Because X
		 * only allows a single client to subscribe to
		 * SubstructureRedirect and ButtonPress bits, this also serves to
		 * mutex who is The WM for the root window, and thus (aside from
		 * captive) the Screen.
		 */
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


		// We now manage it (or are in the various special circumstances
		// where it's near enough).
		numManaged ++;


		// Now we can stash some info about the screen
		Scr->d_depth = DefaultDepth(dpy, scrnum);
		Scr->d_visual = DefaultVisual(dpy, scrnum);
		Scr->RealRoot = RootWindow(dpy, scrnum);

		// Stash up a ref to our Scr on the root, so we can find the
		// right Scr for events etc.
		XSaveContext(dpy, Scr->Root, ScreenContext, (XPointer) Scr);

		// Init captive bits
		if(CLarg.is_captive) {
			Scr->CaptiveRoot = croot;
			Scr->captivename = AddToCaptiveList(CLarg.captivename);
			if(Scr->captivename) {
				XmbSetWMProperties(dpy, croot,
				                   Scr->captivename, Scr->captivename,
				                   NULL, 0, NULL, NULL, NULL);
			}
		}


		// Init some colormap bits
		{
			// 1 on the root
			Scr->RootColormaps.number_cwins = 1;
			Scr->RootColormaps.cwins = malloc(sizeof(ColormapWindow *));
			Scr->RootColormaps.cwins[0] = CreateColormapWindow(Scr->Root, true,
					false);
			Scr->RootColormaps.cwins[0]->visibility = VisibilityPartiallyObscured;

			// Initialize storage for all maps the Screen can hold
			Scr->cmapInfo.cmaps = NULL;
			Scr->cmapInfo.maxCmaps = MaxCmapsOfScreen(ScreenOfDisplay(dpy, Scr->screen));
			Scr->cmapInfo.root_pushes = 0;
			InstallColormaps(0, &Scr->RootColormaps);

			// Setup which we're using
			Scr->StdCmapInfo.head = Scr->StdCmapInfo.tail
				= Scr->StdCmapInfo.mru = NULL;
			Scr->StdCmapInfo.mruindex = 0;
			LocateStandardColormaps();
		}


		// Default values of config params
		Scr->XORvalue = (((unsigned long) 1) << Scr->d_depth) - 1;
		Scr->IconDirectory     = NULL;
		Scr->PixmapDirectory   = PIXMAP_DIRECTORY;
		Scr->ShowWelcomeWindow = CLarg.ShowWelcomeWindow;

		// Are we monochrome?  Or do we care this millennium?
		if(CLarg.Monochrome || DisplayCells(dpy, scrnum) < 3) {
			Scr->Monochrome = MONOCHROME;
		}
		else {
			Scr->Monochrome = COLOR;
		}

		// Flag which basically means "initial screen setup time".
		// XXX Not clear to what extent this should even exist; a lot of
		// uses are fairly bogus.
		Scr->FirstTime = true;

		// Setup default colors
		GetColor(Scr->Monochrome, &(Scr->Black), "black");
		GetColor(Scr->Monochrome, &(Scr->White), "white");


		// The first time around, we focus onto the root [of the first
		// Screen].  Maybe we should revisit this...
		if(FirstScreen) {
			// XXX This func also involves a lot of stuff that isn't
			// setup yet, and probably only works by accident.  Maybe we
			// should just manually extract out the couple bits we
			// actually want to run?
			SetFocus(NULL, CurrentTime);
		}
		FirstScreen = false;

		// Create default icon manager memory bits (in the first
		// workspace)
		AllocateIconManager("TWM", "Icons", "", 1);


		/*
		 * Mask over the screen with our welcome window stuff if we were
		 * asked to on the command line/environment; too early to get
		 * info from config file about it.
		 */
		screenmasked = false;
		if(Scr->ShowWelcomeWindow && (welcomefile = getenv("CTWM_WELCOME_FILE"))) {
			screenmasked = true;
			MaskScreen(welcomefile);
		}

		// More inits
		InitVariables();
		InitMenus();
		InitWorkSpaceManager();


		/*
		 * Parse config file
		 */
		if(CLarg.cfgchk) {
			if(LoadTwmrc(CLarg.InitFile) == false) {
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
			LoadTwmrc(CLarg.InitFile);
		}


		/*
		 * Setup stuff relating to VirtualScreens.  If something to do
		 * with it is set in the config, this all implements stuff needed
		 * for that.  If not, InitVirtualScreens() creates a single one
		 * mirroring our real root.
		 */
		InitVirtualScreens(Scr);
#ifdef EWMH
		EwmhInitVirtualRoots(Scr);
#endif /* EWMH */

		// Setup WSM[s]
		ConfigureWorkSpaceManager();

		// If the config wants us to show the splash screen and we
		// haven't already, do it now.
		if(Scr->ShowWelcomeWindow && !screenmasked) {
			MaskScreen(NULL);
		}



		/*
		 * Do various setup based on the results from the config file.
		 */
		if(Scr->ClickToFocus) {
			Scr->FocusRoot  = false;
			Scr->TitleFocus = false;
		}

		if(Scr->use3Dborders) {
			Scr->ClientBorderWidth = false;
		}


		/*
		 * Various decoration default overrides for 3d/2d.  Values that
		 * [presumtively] look "nice" on 75/100dpi displays.  -100 is a
		 * sentinel value we set before the config file parsing; since
		 * these defaults differ for 3d vs not, we can't just set them as
		 * default before the parse.
		 */
#define SETDEF(fld, num) if(Scr->fld == -100) { Scr->fld = num; }
		if(Scr->use3Dtitles) {
			SETDEF(FramePadding,  0);
			SETDEF(TitlePadding,  0);
			SETDEF(ButtonIndent,  0);
			SETDEF(TBInfo.border, 0);
		}
		else {
			SETDEF(FramePadding,  2);
			SETDEF(TitlePadding,  8);
			SETDEF(ButtonIndent,  1);
			SETDEF(TBInfo.border, 1);
		}
#undef SETDEF

		// These values are meaningless in !3d cases, so always zero them
		// out.
		if(! Scr->use3Dtitles) {
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
		if(! Scr->use3Dborders) {
			Scr->ThreeDBorderWidth = 0;
		}

		// Setup colors for 3d bits.
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

		// Defaults for IconRegion bits that aren't set.
		for(IconRegion *ir = Scr->FirstRegion; ir; ir = ir->next) {
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

		// Put the results of SaveColor{} into _MIT_PRIORITY_COLORS.
		assign_var_savecolor();

		// Load up fonts for the screen.
		//
		// XXX HaveFonts is kinda stupid, however it gets useful in one
		// place: when loading button bindings, we make some sort of
		// "menu" for things (x-ref GotButton()), and the menu gen code
		// needs to load font stuff, so if that happened in the config
		// process, we would have already run CreateFonts().  Of course,
		// that's a order-dependent bit of the config file parsing too;
		// if you define the fonts too late, they wouldn't have been set
		// by then, and we won't [re]try them now...    arg.
		if(!Scr->HaveFonts) {
			CreateFonts();
		}

		// Adjust settings for titlebar.  Must follow CreateFonts() call
		// so we know these bits are populated
		Scr->TitleBarFont.y += Scr->FramePadding;
		Scr->TitleHeight = Scr->TitleBarFont.height + Scr->FramePadding * 2;
		if(Scr->use3Dtitles) {
			Scr->TitleHeight += 2 * Scr->TitleShadowDepth;
		}
		/* make title height be odd so buttons look nice and centered */
		if(!(Scr->TitleHeight & 1)) {
			Scr->TitleHeight++;
		}

		// Setup GC's for drawing, so we can start making stuff we have
		// to actually draw.  Could move earlier, has to preceed a lot of
		// following.
		CreateGCs();

		// Create and draw the menus we config'd
		MakeMenus();

		// Load up the images for titlebar buttons
		InitTitlebarButtons();

		// Allocate controls for WindowRegion's.  Has to follow
		// workspaces setup, but doesn't talk to X.
		CreateWindowRegions();

		// Copy the icon managers over to workspaces past the first as
		// necessary.  AllocateIconManager() and the config parsing
		// already made them on the first WS.
		AllocateOtherIconManagers();

		// Create the windows for our icon managers now that all our
		// tracking for it is setup.
		CreateIconManagers();

		// Create the WSM window (per-vscreen) and stash info on the root
		// about our WS's.
		CreateWorkSpaceManager();

		// Create the f.occupy window
		CreateOccupyWindow();

		// Setup TwmWorkspaces menu.  Needs workspaces setup, as well as
		// menus made.
		MakeWorkspacesMenu();

		// setup WindowBox's
		createWindowBoxes();

#ifdef EWMH
		// Set EWMH-related properties on various root-ish windows, for
		// other programs to read to find out how we view the world.
		EwmhInitScreenLate(Scr);
#endif /* EWMH */


		/*
		 * Look up and handle all the windows on the screen.
		 */
		{
			Window parent, *children;
			unsigned int nchildren;

			XQueryTree(dpy, Scr->Root, &croot, &parent, &children, &nchildren);

			/* Weed out icon windows */
			for(int i = 0; i < nchildren; i++) {
				if(children[i]) {
					XWMHints *wmhintsp = XGetWMHints(dpy, children[i]);

					if(wmhintsp) {
						if(wmhintsp->flags & IconWindowHint) {
							for(int j = 0; j < nchildren; j++) {
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
			 * Map all of the non-override windows.  This winds down
			 * into AddWindow() and friends through SimulateMapRequest(),
			 * so this is where we actually adopt the windows on the
			 * screen.
			 */
			for(int i = 0; i < nchildren; i++) {
				if(children[i] && MappedNotOverride(children[i])) {
					XUnmapWindow(dpy, children[i]);
					SimulateMapRequest(children[i]);
				}
			}

			/*
			 * At this point, we've adopted all the windows currently on
			 * the screen (aside from those we're intentionally not).
			 * Note that this happens _before_ the various other windows
			 * we create below, which is why they don't wind up getting
			 * TwmWindow's tied to them or show up in icon managers, etc.
			 * We'd need to actually make it _explicit_ that those
			 * windows aren't tracked by us if we changed that order...
			 */
		}


		// Show the WSM window if we should
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


		// Setup shading for default ColorPair
		if(!Scr->BeNiceToColormap) {
			GetShadeColors(&Scr->DefaultC);
		}


		/*
		 * Setup the Info window, used for f.identify and f.version.
		 */
		{
			unsigned long valuemask;
			XSetWindowAttributes attributes;

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
		}


		/*
		 * Setup the Size/Position window for showing during resize/move
		 * operations.
		 */
		{
			int sx, sy;
			XRectangle ink_rect;
			XRectangle logical_rect;
			unsigned long valuemask;
			XSetWindowAttributes attributes;

			XmbTextExtents(Scr->SizeFont.font_set,
			               " 8888 x 8888 ", 13,
			               &ink_rect, &logical_rect);
			Scr->SizeStringWidth = logical_rect.width;
			valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity);
			attributes.bit_gravity = NorthWestGravity;

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

		// Create util window used in animation
		Scr->ShapeWindow = XCreateSimpleWindow(dpy, Scr->Root, 0, 0,
		                                       Scr->rootw, Scr->rooth, 0, 0, 0);


		// Clear out the splash screen if we had one
		if(Scr->ShowWelcomeWindow) {
			UnmaskScreen();
		}

		// Done setting up this Screen.  x-ref XXX's about whether this
		// element is worth anything...
		Scr->FirstTime = false;
	} // for each screen on display


	// We're not much of a window manager if we didn't get stuff to
	// manage...
	if(numManaged == 0) {
		if(CLarg.MultiScreen && NumScreens > 0)
			fprintf(stderr, "%s:  unable to find any unmanaged screens\n",
			        ProgramName);
		exit(1);
	}

	// Hook up session
	ConnectToSessionManager(CLarg.client_id);

#ifdef SOUNDS
	// Announce ourselves
	sound_load_list();
	play_startup_sound();
#endif

	// Hard-reset this flag.
	// XXX This doesn't seem right?
	RestartPreviousState = true;

	// Do some late initialization
	HandlingEvents = true;
	InitEvents();
	StartAnimation();

	// Main loop.
	HandleEvents();

	// Should never get here...
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
	OtpScrInitData(Scr);

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

	// Sentinel values for defaulting
	Scr->FramePadding = -100;
	Scr->TitlePadding = -100;
	Scr->ButtonIndent = -100;
	Scr->TBInfo.border = -100;

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
#endif

	Scr->ForceFocus = false;

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

static Window
CreateCaptiveRootWindow(int x, int y,
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
