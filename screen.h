/*
 * twm per-screen data include file
 *
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * $XConsortium: screen.h,v 1.62 91/05/01 17:33:09 keith Exp $
 *
 * 11-3-88 Dave Payne, Apple Computer                   File created
 *
 * Copyright 1992 Claude Lecommandeur.
 */

#ifndef _CTWM_SCREEN_H
#define _CTWM_SCREEN_H

#include "menus.h"  // embedded MouseButton/Func{Button,Key}
#include "workspace_structs.h"  // embedded ScreenInfo.workSpaceMgr

typedef enum {
	ICONIFY_NORMAL,
	ICONIFY_MOSAIC,
	ICONIFY_ZOOMIN,
	ICONIFY_ZOOMOUT,
	ICONIFY_FADE,
	ICONIFY_SWEEP,
} IcStyle;

struct StdCmap {
	struct StdCmap *next;               /* next link in chain */
	Atom atom;                          /* property from which this came */
	int nmaps;                          /* number of maps below */
	XStandardColormap *maps;            /* the actual maps */
};

#define SIZE_HINDENT 10
#define SIZE_VINDENT 2

struct TitlebarPixmaps {
	Pixmap xlogo;
	Pixmap resize;
	Pixmap question;
	Pixmap menu;
	Pixmap delete;
};


/**
 * Info and control for each X Screen we control.
 *
 * We start up on an X Display (e.g., ":0"), and by default try to take
 * over each X Screen on that display (e.g, ":0.0", ":0.1", ...).  Each
 * of those Screens will have its own ScreenInfo.
 *
 * This contains pure physical or X info (size, coordinates, color
 * depth), ctwm info (lists of windows on it, window rings, how it fits
 * with other Screens we control), most of the config file settings which
 * may differ from Screen to Screen, menus, special windows (Occupy,
 * Identify, etc), and piles of other stuff.
 *
 * \note
 * Possibly this should be broken up somewhat.  e.g., much of the
 * config-related bits pulled out into their own structure, which could
 * allow decoupling the config parsing from the X screens a bit.
 */
struct ScreenInfo {
	int screen;       ///< Which screen (i.e., the x after the dot in ":0.x")
	bool takeover;    ///< Whether we're taking over this screen.
	                  ///< Usually true, unless running captive or \--cfgchk
	int d_depth;      ///< Copy of DefaultDepth(dpy, screen)
	Visual *d_visual; ///< Copy of DefaultVisual(dpy, screen)
	int Monochrome;   ///< Is the display monochrome?

	/**
	 * The x coordinate of the root window relative to RealRoot.
	 *
	 * This is usually 0, except in the case of captive mode where it
	 * shows where we are on the real screen, or when we have
	 * VirtualScreens and are positioning our real Screens on a virtual
	 * RealRoot.
	 */
	int rootx;
	int rooty; ///< The y coordinate of the root window relative to RealRoot.
	           ///< \copydetails rootx

	int rootw; ///< Copy of DisplayWidth(dpy, screen)
	int rooth; ///< Copy of DisplayHeight(dpy, screen)

	/**
	 * \defgroup scr_captive_bits Captive ctwm bits
	 * These are various fields related to running a captive ctwm (i.e.,
	 * with \--window).  They'll generally be empty for non-captive
	 * invocations, or describe our position inside the "outside" world
	 * if we are.
	 * @{
	 */
	char *captivename; ///< The name of the captive root window if any.
	                   ///< Autogen'd or set with \--name
	int crootx;   ///< The x coordinate of the captive root window if any.
	int crooty;   ///< The y coordinate of the captive root window if any.
	int crootw;   ///< Initially copy of DisplayWidth(dpy, screen).
	              ///< See also ConfigureCaptiveRootWindow()
	int crooth;   ///< Initially copy of DisplayHeight(dpy, screen).
	              ///< \copydetails crootw
	/// @}

	int MaxWindowWidth;   ///< Largest window width to allow
	int MaxWindowHeight;  ///< Largest window height to allow

	/**
	 * The head of the screen's twm window list.
	 * This is used for places where we need to iterate over the
	 * TwmWindow's in a single Screen, by following the TwmWindow.next
	 * pointers.
	 */
	TwmWindow *FirstWindow;

	Colormaps RootColormaps;  ///< The colormaps of the root window


	/**
	 * \defgroup scr_roots Various root and pseudo-root Windows.
	 * These are the various forms of root and almost-root windows that
	 * things on this Screen reside in.  It's probable that there's a lot
	 * of confusion of these, and they get set, reset, and used
	 * incorrectly in a lot of places.  We mostly get away with it
	 * because in normal usage, they're often all identical.
	 *
	 * \verbatim
	 *
	 *  +--RealRoot-----------------------------------------------------------+
	 *  | the root of the display (most uses of this are probably incorrect!) |
	 *  |                                                                     |
	 *  |   +--CaptiveRoot--------------------------------------------------+ |
	 *  |   | when captive window is used (most uses are likely incorrect!) | |
	 *  |   |                                                               | |
	 *  |   | +--XineramaRoot---------------------------------------------+ | |
	 *  |   | | the root that encompasses all virtual screens             | | |
	 *  |   | |                                                           | | |
	 *  |   | | +--Root-----------+ +--Root--------+ +--Root------------+ | | |
	 *  |   | | | one or more     | | Most cases   | |                  | | | |
	 *  |   | | | virtual screens | | use Root.    | |                  | | | |
	 *  |   | | |                 | |              | |                  | | | |
	 *  |   | | |                 | |              | |                  | | | |
	 *  |   | | +-----------------+ +--------------+ +------------------+ | | |
	 *  |   | +-----------------------------------------------------------+ | |
	 *  |   +---------------------------------------------------------------+ |
	 *  +---------------------------------------------------------------------+
	 * \endverbatim
	 *
	 * @{
	 */

	/**
	 * Root window for the current vscreen.
	 * Initially either the real X RootWindow(), or the existing or
	 * created Window for a captive ctwm.  Gets reset to a vscreen's
	 * window in InitVirtualScreens().
	 */
	Window Root;

	/**
	 * Root window holding our vscreens.  Initialized to the same value
	 * as ScreenInfo.Root, and isn't changed afterward.
	 */
	Window XineramaRoot;
	Window CaptiveRoot; ///< The captive root window, if any, or None
	Window RealRoot;    ///< The actual X root window of the display.
	                    ///< This is always X's RootWindow().
	/// @}


	/**
	 * Dimensions/coordinates window.  This is the small window (usually
	 * in the upper left of the screen, unless
	 * ScreenInfo.CenterFeedbackWindow is set) that shows
	 * dimensions/coordinates for resize/move operations.
	 */
	Window SizeWindow;

	/**
	 * Window info window.  This is the window that pops up with the
	 * various information when you f.identify a window, and also the
	 * truncated version of that that f.version pulls up.
	 */
	struct _InfoWindow {
		Window       win;          ///< Actual X window
		bool         mapped;       ///< Whether it's currently up
		int          lines;        ///< Current number of lines
		unsigned int width;        ///< Current size
		unsigned int height;       ///< Current size
	} InfoWindow; ///< \copydoc ScreenInfo::_InfoWindow
	/*
	 * Naming this struct type is pointless, but necessary for doxygen to
	 * not barf on it.  The copydoc is needed so the desc shows up in the
	 * ScreenInfo docs as well as the struct's own.
	 */

	/**
	 * \defgroup scr_maskwin Screen masking window stuff
	 * These are bits for a window that covers up everything on the
	 * screen during startup if we're showing the "Welcome window"
	 * splash screen.  That is, if ScreenInfo.ShowWelcomeWindow is true.
	 * @{
	 */
	Window WindowMask;      ///< Startup splash screen masking window if
	                        ///< ScreenInfo.ShowWelcomeWindow
	Window ShapeWindow;     ///< Utility window for animated icons

	Image   *WelcomeImage;  ///< Image to show on ScreenInfo.WindowMask
	GC       WelcomeGC;     ///< GC for drawing ScreenInfo.WelcomeImage
	                        ///< on ScreenInfo.WindowMask
	Colormap WelcomeCmap;   ///< Colormap for ScreenInfo.WindowMask
	Visual  *WelcomeVisual; ///< Unused \deprecated Unused
	/// @}

	name_list *ImageCache;      /* list of pixmaps */
	TitlebarPixmaps tbpm;       /* titlebar pixmaps */
	Image *UnknownImage;        /* the unknown icon pixmap */
	Pixmap siconifyPm;          /* the icon manager iconify pixmap */
	Pixmap pullPm;              /* pull right menu icon */
	unsigned int pullW, pullH;  /* size of pull right menu icon */
	char *HighlightPixmapName;  /* name of the hilite image if any */

	MenuRoot *MenuList;         /* head of the menu list */
	MenuRoot *LastMenu;         /* the last menu (mostly unused?) */
	MenuRoot *Windows;          /* the TwmWindows menu */
	MenuRoot *Icons;            /* the TwmIcons menu */
	MenuRoot *Workspaces;       /* the TwmWorkspaces menu */
	MenuRoot *AllWindows;       /* the TwmAllWindows menu */

	/*Added by dl 2004 */
	MenuRoot *AllIcons;         /* the TwmAllIcons menu */

	/******************************************************/
	/* Added by Dan Lilliehorn (dl@dl.nu) 2000-02-29)     */
	MenuRoot *Keys;             /* the TwmKeys menu     */
	MenuRoot *Visible;          /* thw TwmVisible menu  */

	TwmWindow *Ring;            /* one of the windows in window ring */
	TwmWindow *RingLeader;      /* current window in ring */

	MouseButton DefaultFunction;
	MouseButton WindowFunction;
	MouseButton ChangeWorkspaceFunction;
	MouseButton DeIconifyFunction;
	MouseButton IconifyFunction;

	struct {
		Colormaps *cmaps;         /* current list of colormap windows */
		int maxCmaps;             /* maximum number of installed colormaps */
		unsigned long first_req;  /* seq # for first XInstallColormap() req in
                                   pass thru loading a colortable list */
		int root_pushes;          /* current push level to install root
                                   colormap windows */
		Colormaps *pushed_cmaps;  /* saved colormaps to install when pushes
                                   drops to zero */
	} cmapInfo;

	struct {
		StdCmap *head, *tail;           /* list of maps */
		StdCmap *mru;                   /* most recently used in list */
		int mruindex;                   /* index of mru in entry */
	} StdCmapInfo;

	struct {
		int nleft, nright;              /* numbers of buttons in list */
		TitleButton *head;              /* start of list */
		int border;                     /* button border */
		int pad;                        /* button-padding */
		int width;                      /* width of single button & border */
		int leftx;                      /* start of left buttons */
		int titlex;                     /* start of title */
		int rightoff;                   /* offset back from right edge */
		int titlew;                     /* width of title part */
	} TBInfo;
	ColorPair BorderTileC;      /* border tile colors */
	ColorPair TitleC;           /* titlebar colors */
	ColorPair MenuC;            /* menu colors */
	ColorPair MenuTitleC;       /* menu title colors */
	ColorPair IconC;            /* icon colors */
	ColorPair IconManagerC;     /* icon manager colors */
	ColorPair DefaultC;         /* default colors */
	ColorPair BorderColorC;     /* color of window borders */
	Pixel MenuShadowColor;      /* menu shadow color */
	Pixel IconBorderColor;      /* icon border color */
	Pixel IconManagerHighlight; /* icon manager highlight */
	short ClearShadowContrast;  /* The contrast of the clear shadow */
	short DarkShadowContrast;   /* The contrast of the dark shadow */
	/* Icon bits */
	/* Screen.IconJustification -> IconRegion.TitleJustification */
	TitleJust IconJustification;
	IRJust IconRegionJustification;
	IRAlignement IconRegionAlignement;
	/* Window titlebars (notably NOT IconRegion.TitleJustification) */
	TitleJust TitleJustification;
	IcStyle IconifyStyle;       /* ICONIFY_* */
	int   MaxIconTitleWidth;    /* */
#ifdef EWMH
	int PreferredIconWidth;     /* Desired icon size: width */
	int PreferredIconHeight;    /* Desired icon size: height */
#endif

	Cursor TitleCursor;         /* title bar cursor */
	Cursor FrameCursor;         /* frame cursor */
	Cursor IconCursor;          /* icon cursor */
	Cursor IconMgrCursor;       /* icon manager cursor */
	Cursor ButtonCursor;        /* title bar button cursor */
	Cursor MoveCursor;          /* move cursor */
	Cursor ResizeCursor;        /* resize cursor */
	Cursor WaitCursor;          /* wait a while cursor */
	Cursor MenuCursor;          /* menu cursor */
	Cursor SelectCursor;        /* dot cursor for f.move, etc. from menus */
	Cursor DestroyCursor;       /* skull and cross bones, f.destroy */
	Cursor AlterCursor;         /* cursor for alternate keymaps */

	WorkSpaceMgr workSpaceMgr;
	bool workSpaceManagerActive;

	VirtualScreen *vScreenList;
	VirtualScreen *currentvs;
	name_list     *VirtualScreens;
	int         numVscreens;

	name_list *OccupyAll;  // window names occupying all workspaces at startup
	name_list   *UnmapByMovingFarAway;
	name_list   *DontSetInactive;
	name_list   *AutoSqueeze;
	name_list   *StartSqueezed;
	bool        use3Dmenus;
	bool        use3Dtitles;
	bool        use3Diconmanagers;
	bool        use3Dborders;
	bool        use3Dwmap;
	bool        use3Diconborders;
	bool        SunkFocusWindowTitle;
	short       WMgrVertButtonIndent;
	short       WMgrHorizButtonIndent;
	short       WMgrButtonShadowDepth;
	bool        BeNiceToColormap;
	bool        BorderCursors;
	bool        AutoPopup;
	short       BorderShadowDepth;
	short       TitleButtonShadowDepth;
	short       TitleShadowDepth;
	short       MenuShadowDepth;
	short       IconManagerShadowDepth;
	bool        ReallyMoveInWorkspaceManager;
	bool        ShowWinWhenMovingInWmgr;
	bool        ReverseCurrentWorkspace;
	bool        DontWarpCursorInWMap;
	short       XMoveGrid, YMoveGrid;
	bool        CenterFeedbackWindow;
	bool        ShrinkIconTitles;
	bool        AutoRaiseIcons;
	bool        AutoFocusToTransients; /* kai */
	bool        PackNewWindows;

	struct OtpPreferences *OTP;
	struct OtpPreferences *IconOTP;

	name_list *BorderColorL;
	name_list *IconBorderColorL;
	name_list *BorderTileForegroundL;
	name_list *BorderTileBackgroundL;
	name_list *TitleForegroundL;
	name_list *TitleBackgroundL;
	name_list *IconForegroundL;
	name_list *IconBackgroundL;
	name_list *IconManagerFL;
	name_list *IconManagerBL;
	name_list *IconMgrs;
	name_list *AutoPopupL;      /* list of window the popup when changed */
	name_list *NoBorder;        /* list of window without borders          */
	name_list *NoIconTitle;     /* list of window names with no icon title */
	name_list *NoTitle;         /* list of window names with no title bar */
	name_list *MakeTitle;       /* list of window names with title bar */
	name_list *AutoRaise;       /* list of window names to auto-raise */
	name_list *WarpOnDeIconify; /* list of window names to warp on deiconify */
	name_list *AutoLower;       /* list of window names to auto-lower */
	name_list *IconNames;       /* list of window names and icon names */
	name_list *NoHighlight;     /* list of windows to not highlight */
	name_list *NoStackModeL;    /* windows to ignore stack mode requests */
	name_list *NoTitleHighlight;/* list of windows to not highlight the TB*/
	name_list *DontIconify;     /* don't iconify by unmapping */
	name_list *IconMgrNoShow;   /* don't show in the icon manager */
	name_list *IconMgrShow;     /* show in the icon manager */
	name_list *IconifyByUn;     /* windows to iconify by unmapping */
	name_list *StartIconified;  /* windows to start iconic */
	name_list *IconManagerHighlightL;   /* icon manager highlight colors */
	name_list *SqueezeTitleL;           /* windows of which to squeeze title */
	name_list *DontSqueezeTitleL;       /* windows of which not to squeeze */
	name_list *AlwaysSqueezeToGravityL; /* windows which should squeeze toward gravity */
	name_list *WindowRingL;     /* windows in ring */
	name_list *WindowRingExcludeL;      /* windows excluded from ring */
	name_list *WarpCursorL;     /* windows to warp cursor to on deiconify */
	name_list *DontSave;
	name_list *WindowGeometries;
	name_list *IgnoreTransientL;

	name_list *OpaqueMoveList;
	name_list *NoOpaqueMoveList;
	name_list *OpaqueResizeList;
	name_list *NoOpaqueResizeList;
	name_list *IconMenuDontShow;

	GC NormalGC;                /* normal GC for everything */
	GC MenuGC;                  /* gc for menus */
	GC DrawGC;                  /* GC to draw lines for move and resize */
	GC BorderGC;                /* for drawing 3D borders */
	GC rootGC;                  /* used for allocating pixmaps in FindPixmap (util.c) */

	unsigned long Black;
	unsigned long White;
	unsigned long XORvalue;     /* number to use when drawing xor'ed */
	MyFont TitleBarFont;        /* title bar font structure */
	MyFont MenuFont;            /* menu font structure */
	MyFont IconFont;            /* icon font structure */
	MyFont SizeFont;            /* resize font structure */
	MyFont IconManagerFont;     /* window list font structure */
	MyFont DefaultFont;
	IconMgr *iconmgr;           /* default icon manager  */
	struct IconRegion *FirstRegion;     /* pointer to icon regions */
	struct IconRegion *LastRegion;      /* pointer to the last icon region */
	struct WindowRegion *FirstWindowRegion;     /* pointer to window regions */
	WindowBox *FirstWindowBox;  /* pointer to window boxes list */
	char *IconDirectory;        /* icon directory to search */
	char *PixmapDirectory;      /* Pixmap directory to search */
	int SizeStringOffset;       /* x offset in size window for drawing */
	int SizeStringWidth;        /* minimum width of size window */
	int BorderWidth;            /* border width of twm windows */
	int BorderLeft;
	int BorderRight;
	int BorderTop;
	int BorderBottom;
	int ThreeDBorderWidth;      /* 3D border width of twm windows */
	int IconBorderWidth;        /* border width of icon windows */
	int TitleHeight;            /* height of the title bar window */
	TwmWindow *Focus;           /* the twm window that has focus */
	int EntryHeight;            /* menu entry height */
	int FramePadding;           /* distance between decorations and border */
	int TitlePadding;           /* distance between items in titlebar */
	int ButtonIndent;           /* amount to shrink buttons on each side */
	int NumAutoRaises;          /* number of autoraise windows on screen */
	int NumAutoLowers;          /* number of autolower windows on screen */
	int TransientOnTop;         /* Percentage of the surface of it's leader */
	bool  AutoRaiseDefault;     /* AutoRaise all windows if true */
	bool  AutoLowerDefault;     /* AutoLower all windows if true */
	bool  NoDefaults;           /* do not add in default UI stuff */
	UsePPoss UsePPosition;      /* what do with PPosition, see values below */
	bool  UseSunkTitlePixmap;
	bool  AutoRelativeResize;   /* start resize relative to position in quad */
	bool  FocusRoot;            /* is the input focus on the root ? */
	bool  WarpCursor;           /* warp cursor on de-iconify ? */
	bool  ForceIcon;            /* force the icon to the user specified */
	bool  NoGrabServer;         /* don't do server grabs */
	bool  NoRaiseMove;          /* don't raise window following move */
	bool  NoRaiseResize;        /* don't raise window following resize */
	bool  NoRaiseDeicon;        /* don't raise window on deiconify */
	bool  RaiseOnWarp;          /* do raise window on warp */
	bool  DontMoveOff;          /* don't allow windows to be moved off */
	int MoveOffResistance;      /* nb of pixel before moveOff gives up */
	int MovePackResistance;     /* nb of pixel before f.movepack gives up */
	bool  DoZoom;               /* zoom in and out of icons */
	bool  TitleFocus;           /* focus on window in title bar ? */
	bool  IconManagerFocus;     /* focus on iconified window ? */
	bool  NoIconTitlebar;       /* put title bars on icons */
	bool  NoTitlebar;           /* put title bars on windows */
	bool  DecorateTransients;   /* put title bars on transients */
	bool  IconifyByUnmapping;   /* simply unmap windows when iconifying */
	bool  ShowIconManager;      /* display the window list */
	bool  ShowWorkspaceManager; /* display the workspace manager */
	bool  IconManagerDontShow;  /* show nothing in the icon manager */
	bool  AutoOccupy;           /* Do we automatically change occupation when name changes */
	bool  AutoPriority;         /* Do we automatically change priority when name changes */
	bool  TransientHasOccupation;       /* Do transient-for windows have their own occupation */
	bool  DontPaintRootWindow;  /* don't paint anything on the root window */
	bool  BackingStore;         /* use backing store for menus */
	bool  SaveUnder;            /* use save under's for menus */
	RandPlac RandomPlacement;   /* randomly place windows that no give hints */
	short RandomDisplacementX;  /* randomly displace by this much horizontally */
	short RandomDisplacementY;  /* randomly displace by this much vertically */
	bool  OpaqueMove;           /* move the window rather than outline */
	bool  DoOpaqueMove;         /* move the window rather than outline */
	unsigned short OpaqueMoveThreshold;         /*  */
	bool  DoOpaqueResize;       /* resize the window rather than outline */
	bool  OpaqueResize;         /* resize the window rather than outline */
	unsigned short OpaqueResizeThreshold;       /*  */
	bool  Highlight;            /* should we highlight the window borders */
	bool  StackMode;            /* should we honor stack mode requests */
	bool  TitleHighlight;       /* should we highlight the titlebar */
	short MoveDelta;            /* number of pixels before f.move starts */
	short ZoomCount;            /* zoom outline count */
	bool  SortIconMgr;          /* sort entries in the icon manager */
	bool  Shadow;               /* show the menu shadow */
	bool  InterpolateMenuColors;/* make pretty menus */
	bool  StayUpMenus;          /* stay up menus */
	bool  WarpToDefaultMenuEntry; /* warp cursor to default menu entry, if any  */
	bool  ClickToFocus;         /* click to focus */
	bool  SloppyFocus;          /* "sloppy" focus */
	bool  SaveWorkspaceFocus;   /* Save and restore focus on workspace change. */
	bool  NoIconManagers;       /* Don't create any icon managers */
	bool  ClientBorderWidth;    /* respect client window border width */
	bool  SqueezeTitle;         /* make title as small as possible */
	bool  SqueezeTitleSet;      /* has ST been set yet */
	bool  AlwaysSqueezeToGravity; /* squeeze toward gravity */
	bool  HaveFonts;            /* set if fonts have been loaded */
	bool  FirstTime;            /* first time we've read .twmrc */
	bool  CaseSensitive;        /* be case-sensitive when sorting names */
	bool  WarpUnmapped;         /* allow warping to unmapped windows */
	bool  WindowRingAll;        /* add all windows to the ring */
	bool  WarpRingAnyWhere;     /* warp to ring even if window is not visible */
	bool  ShortAllWindowsMenus; /* Eliminates Icon and Workspace Managers */
	short OpenWindowTimeout;    /* Timeout when a window tries to open */
	bool  RaiseWhenAutoUnSqueeze;
	bool  RaiseOnClick;         /* Raise a window when clieked into */
	short RaiseOnClickButton;           /* Raise a window when clieked into */
	unsigned int IgnoreModifier;/* We should ignore these modifiers */
	bool  IgnoreCaseInMenuSelection;    /* Should we ignore case in menu selection */
	bool  NoWarpToMenuTitle; /* warp cursor to clipped menu title */
	bool  NoImagesInWorkSpaceManager;   /* do not display mini images of the desktop background images on WSmap */
	bool  DontToggleWorkspaceManagerState;

	/**
	 * Whether to show the welcome window.
	 * Related to the DontShowWelcomeWindow config var or the
	 * \--nowelcome command-line arg.
	 * \ingroup scr_maskwin
	 */
	bool  ShowWelcomeWindow;

	/* Forcing focus-setting on windows */
	bool      ForceFocus;
	name_list *ForceFocusL;

	FuncKey FuncKeyRoot;
	FuncButton FuncButtonRoot;

#ifdef EWMH
	Window icccm_Window;        /* ICCCM sections 4.3, 2.8 */
	long *ewmh_CLIENT_LIST;
	int ewmh_CLIENT_LIST_size;
	int ewmh_CLIENT_LIST_used;
	EwmhStrut *ewmhStruts;          /* remember values of _NET_WM_STRUT */
	name_list *EWMHIgnore;    /* EWMH messages to ignore */
#endif /* EWMH */

	name_list *MWMIgnore;    /* Motif WM messages to ignore */
};

extern int NumScreens;
extern ScreenInfo **ScreenList;
extern ScreenInfo *Scr;



/* XXX should be in iconmgr.h? */
/* Spacing between the text and the outer border.  */
#define ICON_MGR_IBORDER 3
/* Thickness of the outer border (3d or not).  */
#define ICON_MGR_OBORDER \
    (Scr->use3Diconmanagers ? Scr->IconManagerShadowDepth : 2)

#endif /* _CTWM_SCREEN_H */
