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

/**
 * Information about some XStandardColormap we're using.  See Xlib docs
 * for details.
 */
struct StdCmap {
	struct StdCmap *next;               /* next link in chain */
	Atom atom;                          /* property from which this came */
	int nmaps;                          /* number of maps below */
	XStandardColormap *maps;            /* the actual maps */
};

#define SIZE_HINDENT 10
#define SIZE_VINDENT 2

/**
 * Stash for memoizing various pixmaps used in titlebars.
 * \sa the TBPM_* constants in image.h
 * \todo This probably doesn't need to live on its own, since it only
 * exists to define a member in the ScreenInfo struct.  Maybe it should
 * just be moved to being defined nested in there...
 */
struct TitlebarPixmaps {
	Pixmap xlogo;    ///< #TBPM_XLOGO
	Pixmap resize;   ///< #TBPM_RESIZE
	Pixmap question; ///< #TBPM_QUESTION
	Pixmap menu;     ///< #TBPM_MENU
	Pixmap delete;   ///< #TBPM_DOT
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
	Visual  *WelcomeVisual; ///< Unused \deprecated Unused \todo Remove
	/// @}

	name_list *ImageCache;  ///< Cached pixmaps used in image loading
	TitlebarPixmaps tbpm;   ///< Memoized titlebar pixmaps
	Image *UnknownImage;    ///< Fallback icon pixmap
	Pixmap siconifyPm;      ///< In-icon manager iconifed marker pixmap
	Pixmap pullPm;          ///< In-menu submenu item marker icon
	unsigned int pullW;     ///< Dimensions of ScreenInfo.pullPm
	unsigned int pullH;     ///< Dimensions of ScreenInfo.pullPm

	/**
	 * Name of titlebar focus hilite image if any.  This is an
	 * alternative to the builtin shading on the titlebar when a window
	 * has focus.  See Pixmaps config var.
	 */
	char *HighlightPixmapName;

	/**
	 * \defgroup scr_menu_bits Various menus
	 * These hold references to the various menus on the Screen.
	 * @{
	 */
	MenuRoot *MenuList;    ///< Head of the menu list
	MenuRoot *LastMenu;    ///< Temp var used in creating the Screen's menus
	MenuRoot *Windows;     ///< f.menu TwmWindows
	MenuRoot *Icons;       ///< f.menu TwmIcons
	MenuRoot *Workspaces;  ///< f.menu TwmWorkspaces
	MenuRoot *AllWindows;  ///< f.menu TwmAllWindows

	/*Added by dl 2004 */
	MenuRoot *AllIcons;    ///< f.menu TwmAllIcons

	/******************************************************/
	/* Added by Dan Lilliehorn (dl@dl.nu) 2000-02-29)     */
	MenuRoot *Keys;        ///< f.menu TwmKeys
	MenuRoot *Visible;     ///< f.menu TwmVisible

	/// @}

	TwmWindow *Ring;       ///< One of the windows in the Screen's ring
	TwmWindow *RingLeader; ///< Current window in ring

	MouseButton DefaultFunction;   ///< DefaultFunction config var
	MouseButton WindowFunction;    ///< WindowFunction config var
	MouseButton ChangeWorkspaceFunction; ///< ChangeWorkspaceFunction config var
	MouseButton DeIconifyFunction; ///< DeIconifyFunction config var
	MouseButton IconifyFunction;   ///< IconifyFunction config var

	/**
	 * Various colormaps used on the Screen.  These probably have little
	 * effect in a world where 24bpp is a baseline...
	 */
	struct _cmapInfo {
		Colormaps *cmaps;  ///< Current list of colormap windows
		int maxCmaps;      ///< Maximum number of installed colormaps
		unsigned long first_req; /** seq # for first XInstallColormap() req in
                                     pass thru loading a colortable list */
		int root_pushes;   /** current push level to install root
                               colormap windows */
		Colormaps *pushed_cmaps; /** saved colormaps to install when pushes
                                     drops to zero */
	} cmapInfo; ///< \copydoc ScreenInfo::_cmapInfo
	///< \todo Somebody needs to understand and document this better.
	// x-ref trailing comment on InfoWindow above

	/**
	 * Various XStandardColormaps on the screen.  See Xlib documentation
	 * for XStandardColormaps (e.g.,
	 * <https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Standard_Colormaps>)
	 * if you need to make sense of it.
	 */
	struct _StdCmapInfo {
		StdCmap *head;         ///< list of maps
		StdCmap *tail;         ///< list of maps
		StdCmap *mru;          ///< Most recently used in list
		int mruindex;          ///< index of mru in entry
	} StdCmapInfo; ///< \copydoc ScreenInfo::_StdCmapInfo
	///< \todo Somebody needs to understand and document this better.
	// x-ref trailing comment on InfoWindow above

	/**
	 * Various titlebar buttons that will be put in the window
	 * decorations for the screen.  This is setup by
	 * InitTitlebarButtons() and possibly added to via
	 * Left/RightTitleButton config vars.
	 * \sa CreateWindowTitlebarButtons() where this gets used to build
	 * the titlebar of an individual window.
	 */
	struct _TBInfo {
		int nleft;         ///< numbers of buttons on left side
		int nright;        ///< numbers of buttons on right side
		TitleButton *head; ///< start of list
		int border;        ///< button border
		int pad;           ///< button-padding
		int width;         ///< width of single button & border
		int leftx;         ///< start of left buttons
		int titlex;        ///< start of title
		int rightoff;      ///< offset back from right edge
		int titlew;        ///< width of title part
	} TBInfo; ///< \copydoc ScreenInfo::_TBInfo
	// x-ref trailing comment on InfoWindow above

	/**
	 * \defgroup scr_color_bits Various color definitions.
	 * These define various colors we use for things on the screen.
	 * They tend to come from things inside a Color {} section in the
	 * config.  There are often correspondences between the "simple"
	 * ColorPair or Pixel values (for the "normal" colors of each type)
	 * and a name_list (for per-window settings of that type).
	 * @{
	 */
	/// Border tile colors.  \sa ScreenInfo.BorderTileForegroundL
	/// \sa ScreenInfo.BorderTileBackgroundL
	ColorPair BorderTileC;
	/// Titlebar colors  \sa ScreenInfo.TitleForegroundL
	/// \sa ScreenInfo.TitleBackgroundL
	ColorPair TitleC;
	ColorPair MenuC;            ///< Menu colors
	ColorPair MenuTitleC;       ///< Menu title colors
	/// %Icon colors.  \sa ScreenInfo.IconForegroundL
	/// \sa ScreenInfo.IconBackgroundL
	ColorPair IconC;
	/// %Icon manager colors.  \sa ScreenInfo.IconManagerFL
	/// \sa ScreenInfo.IconManagerBL
	ColorPair IconManagerC;
	ColorPair DefaultC;         ///< Default colors
	/// Color of window borders.  \sa ScreenInfo.BorderColorL
	ColorPair BorderColorC;
	Pixel MenuShadowColor;      ///< Menu shadow color
	/// %Icon border color.  \sa ScreenInfo.IconBorderColorL
	Pixel IconBorderColor;
	/// %Icon manager highlight color.
	/// \sa ScreenInfo.IconManagerHighlightL
	Pixel IconManagerHighlight;
	short ClearShadowContrast;  ///< The contrast of the clear shadow
	short DarkShadowContrast;   ///< The contrast of the dark shadow
	/// @}

	/**
	 * \defgroup scr_icon_bits Various icon control bits.
	 * Various configurations for how icons get displayed and laid out.
	 * @{
	 */
	/**
	 * How icon images/titles are aligned.  From IconJustification config
	 * var.  X-ref IconRegion.TitleJustification.
	 */
	TitleJust IconJustification;
	/**
	 * How icons are laid out horizontally inside a region.  From
	 * IconRegionJustificationconfig var.
	 */
	IRJust IconRegionJustification;
	/**
	 * How icons are laid out vertically inside a region.  From
	 * IconRegionAlignement config var.
	 */
	IRAlignement IconRegionAlignement;
	/* Window titlebars (notably NOT IconRegion.TitleJustification) */
	/**
	 * @}
	 * How title text is aligned in window titlebars.  From
	 * TitleJustification config var.
	 */
	TitleJust TitleJustification;
	///< \todo Move this and following for easier scr_icon_bits grouping
	/**
	 * How to animate window iconification, if any.  From IconifyStyle
	 * config var.
	 */
	IcStyle IconifyStyle;       /* ICONIFY_* */
	/**
	 * \addtogroup scr_icon_bits Various icon control bits.
	 * @{
	 */
	 /// Limit on icon title size.  From MaxIconTitleWidth config var.
	int   MaxIconTitleWidth;
#ifdef EWMH
	int PreferredIconWidth;     ///< Width from IconSize config var
	int PreferredIconHeight;    ///< Height from IconSize config var
#endif
	/// @}

	/**
	 * \defgroup scr_cursors Various cursors used on the screen.
	 * These all come from the Cursors config var, or defaults.
	 * @{
	 */
	Cursor TitleCursor;    ///< title bar cursor
	Cursor FrameCursor;    ///< frame cursor
	Cursor IconCursor;     ///< icon cursor
	Cursor IconMgrCursor;  ///< icon manager cursor
	Cursor ButtonCursor;   ///< title bar button cursor
	Cursor MoveCursor;     ///< move cursor
	Cursor ResizeCursor;   ///< resize cursor
	Cursor WaitCursor;     ///< wait a while cursor
	Cursor MenuCursor;     ///< menu cursor
	Cursor SelectCursor;   ///< dot cursor for f.move, etc. from menus
	Cursor DestroyCursor;  ///< skull and cross bones, f.destroy
	Cursor AlterCursor;    ///< cursor for alternate keymaps
	/// @}

	/// Info about the WorkSpaceManager (and Occupy window) for the screen.
	WorkSpaceMgr workSpaceMgr;
	bool workSpaceManagerActive; ///< Whether the WSM is being shown

	/**
	 * \defgroup scr_vscreen_bits VScreen bits
	 * @{
	 */
	VirtualScreen *vScreenList;    ///< Linked list of per-VS info
	VirtualScreen *currentvs;      ///< Currently active VS
	name_list     *VirtualScreens; ///< List of defined VS's
	int           numVscreens;     ///< Number of defined VS's
	/// @}

	name_list   *OccupyAll;       ///< OccupyAll config var
	name_list   *UnmapByMovingFarAway; ///< UnmapByMovingFarAway config var
	name_list   *DontSetInactive; ///< DontSetInactive config var
	name_list   *AutoSqueeze;     ///< AutoSqueeze config var
	name_list   *StartSqueezed;   ///< StartSqueezed config var
	bool        use3Dmenus;       ///< UseThreeDMenus config var
	bool        use3Dtitles;      ///< UseThreeDTitles config var
	bool        use3Diconmanagers; ///< UseThreeDIconManagers config var
	bool        use3Dborders;     ///< UseThreeDBorders config var
	bool        use3Dwmap;        ///< UseThreeDWMap config var
	bool        use3Diconborders; ///< UseThreeDIconBorders config var.
	                              ///< Unused \deprecated Unused \todo Remove
	bool        SunkFocusWindowTitle;  ///< SunkFocusWindowTitle config var
	short       WMgrVertButtonIndent;  ///< WMgrVertButtonIndent config var
	short       WMgrHorizButtonIndent; ///< WMgrHorizButtonIndent config var
	short       WMgrButtonShadowDepth; ///< WMgrButtonShadowDepth config var
	bool        BeNiceToColormap; ///< BeNiceToColormap config var
	bool        BorderCursors;    ///< BorderResizeCursors config var
	bool        AutoPopup;        ///< AutoPopup config flag.
	                              ///< \sa ScreenInfo.AutoPopupL
	short       BorderShadowDepth;      ///< BorderShadowDepth config var
	short       TitleButtonShadowDepth; ///< TitleButtonShadowDepth config var
	short       TitleShadowDepth; ///< TitleShadowDepth config var
	short       MenuShadowDepth;  ///< MenuShadowDepth config var
	short       IconManagerShadowDepth; ///< IconManagerShadowDepth config var
	/// ReallyMoveInWorkspaceManager config var
	bool        ReallyMoveInWorkspaceManager;
	/// AlwaysShowWindowWhenMovingFromWorkspaceManager config var
	bool        ShowWinWhenMovingInWmgr;
	bool        ReverseCurrentWorkspace; ///< ReverseCurrentWorkspace config var
	bool        DontWarpCursorInWMap;  ///< DontWarpCursorInWMap config var
	short       XMoveGrid;             ///< XMoveGrid config var
	short       YMoveGrid;             ///< YMoveGrid config var
	bool        CenterFeedbackWindow;  ///< CenterFeedbackWindow config var
	bool        ShrinkIconTitles;      ///< ShrinkIconTitles config var
	bool        AutoRaiseIcons;        ///< AutoRaiseIcons config var
	bool        AutoFocusToTransients; ///< AutoFocusToTransients config var
	bool        PackNewWindows;        ///< PackNewWindows config var

	/**
	 * Stash of various OTP info about the windows on the screen.
	 * This is only used internally in various otp.c code; nothing else
	 * currently references it.
	 */
	struct OtpPreferences *OTP;
	struct OtpPreferences *IconOTP; ///< Stash of OTP info about icons on
	                                ///< the screen. \copydetails OTP

	/**
	 * \addtogroup scr_color_bits Various color definitions.
	 * @{
	 */
	/// Specialized border colors for windows.  From BorderColor config
	/// var.  \sa ScreenInfo.BorderColorC
	name_list *BorderColorL;
	/// Specialized border colors for icons.  From IconBorderColor config
	/// var.  \sa ScreenInfo.IconBorderColor
	name_list *IconBorderColorL;
	/// Specialized border coloring.  From BorderTileForeground config
	/// var.  \sa ScreenInfo.BorderTileC
	name_list *BorderTileForegroundL;
	/// \copydoc ScreenInfo::BorderTileForegroundL
	name_list *BorderTileBackgroundL;
	/// Specialized titlebar foreground coloring.  From TitleForeground
	/// config var.  \sa ScreenInfo.TitleC
	name_list *TitleForegroundL;
	/// Specialized titlebar background coloring.  From TitleBackground
	/// config var.  \sa ScreenInfo.TitleC
	name_list *TitleBackgroundL;
	/// Specialized icon foreground coloring.  From IconForeground
	/// config var.  \sa ScreenInfo.IconC
	name_list *IconForegroundL;
	/// Specialized icon background coloring.  From IconBackground
	/// config var.  \sa ScreenInfo.IconC
	name_list *IconBackgroundL;
	/// Specialized icon manager foreground coloring.  From
	/// IconManagerForeground config var.  \sa ScreenInfo.IconManagerC
	name_list *IconManagerFL;
	/// Specialized icon manager background coloring.  From
	/// IconManagerBackground config var.  \sa ScreenInfo.IconManagerC
	name_list *IconManagerBL;
	/// @}

	/// From IconManagers config var.  This is a mapping from the window
	/// name pattern to the IconMgr structure it should go in.  All the
	/// IM's for the screen wind up in the iconmgr element.
	/// \sa ScreenInfo.iconmgr
	name_list *IconMgrs;

	/// AutoPopup config var (list).  Windows that popup when changed.
	/// \sa ScreenInfo.AutoPopup
	name_list *AutoPopupL;

	/// NoBorder config var.  Windows without borders.
	name_list *NoBorder;

	/// NoIconTitle config var (list).  Windows to not show a title on
	/// the icons for.  \sa ScreenInfo.NoIconTitlebar
	name_list *NoIconTitle;

	/// NoTitle config var (list).  Windows to not put a titlebar on.
	/// \sa ScreenInfo.NoTitlebar
	name_list *NoTitle;

	/// MakeTitle config var.  Windows to pup a titlebar on when general
	/// NoTitle is set.  \sa ScreenInfo.NoTitlebar \sa ScreenInfo.NoTitle
	name_list *MakeTitle;

	/// AutoRaise config var (list).  Windows to automatically raise when
	/// pointed to (possible after a delay).
	/// \sa ScreenInfo.AutoRaiseDefault \sa ScreenInfo.RaiseDelay
	name_list *AutoRaise;

	/// WarpOnDeIconify config var.  Windows to occupy over to current
	/// workspace on deiconification.  \note Minor nomenclature issue;
	/// 'Warp' in name suggests we move to the win, but it actually means
	/// move the win to us.
	name_list *WarpOnDeIconify;

	/// AutoLower config var (list).  Windows to automatically lower when
	/// pointed away from.  \sa ScreenInfo.AutoLowerDefault
	name_list *AutoLower;

	/// Icons config var.  Manually specified icons for particular
	/// windows.
	name_list *IconNames;

	/// NoHightlight config var (list).  Windows to not highlight border
	/// of when focused.  \sa ScreenInfo.Highlight
	name_list *NoHighlight;

	/// NoStackMode config var (list).  Windows to ignore
	/// application-initiated restacking requests from.
	/// \sa ScreenInfo.StackMode
	name_list *NoStackModeL;

	/// NoTitleHighlight config var (list).  Windows to not highlight in
	/// titlevar when focused.  \sa ScreenInfo.TitleHighlight
	name_list *NoTitleHighlight;

	/// DontIconifyByUnmapping config var.  Windows to iconify by making
	/// an icon for, overriding IconifyByUnmapping setting.
	name_list *DontIconify;

	/// IconManagerDontShow config var (list).
	/// \sa ScreenInfo.IconManagerDontShow
	name_list *IconMgrNoShow;

	/// IconManagerShow config var.  Windows to show in icon manager even
	/// if global IconManagerDontShow is set.
	name_list *IconMgrShow;

	/// IconifyByUnmapping config var (list).  \sa ScreenInfo.IconifyByUnmapping
	name_list *IconifyByUn;

	/// StartIconified config var.
	name_list *StartIconified;

	/// Color to highlight focused windows in icon manager.
	/// \sa ScreenInfo.IconManagerHighlight \ingroup scr_color_bits
	name_list *IconManagerHighlightL;

	/// SqueezeTitle config var (list).  \sa ScreenInfo.SqueezeTitle
	name_list *SqueezeTitleL;

	/// DontSqueezeTitle config var (list).  \sa ScreenInfo.SqueezeTitle
	name_list *DontSqueezeTitleL;

	/// AlwaysSqueezeToGravity config var (list).
	/// \sa ScreenInfo.AlwaysSqueezeToGravity
	name_list *AlwaysSqueezeToGravityL;

	/// WindowRing config var (list).  Windows to put in warp ring.
	/// \sa ScreenInfo.WindowRingAll
	name_list *WindowRingL;

	/// WindowRingExclude config var.  Windows to exclude from warp ring.
	name_list *WindowRingExcludeL;

	/// WarpCursor config var (list).  Windows to warp to on deiconify.
	/// \sa ScreenInfo.WarpCursor
	name_list *WarpCursorL;

	/// DontSave config var.  Windows to not save info in session manager.
	name_list *DontSave;

	/// WindowGeometries config var.  Default geometries for windows.
	name_list *WindowGeometries;

	/// IgnoreTransient config var.  Windows that we should pretend
	/// aren't transient even if they are.
	name_list *IgnoreTransientL;

	/// OpaqueMove config var (list).  Windows to move opaquely rather
	/// than in outline.  \sa ScreenInfo.DoOpaqueMove
	name_list *OpaqueMoveList;

	/// NoOpaqueMove config var (list).  Windows to not move opaquely.
	/// \sa ScreenInfo.DoOpaqueMove
	name_list *NoOpaqueMoveList;

	/// OpaqueResize config var (list).  Windows to resize opaquely
	/// rather than in outline.  \sa ScreenInfo.DoOpaqueResize
	name_list *OpaqueResizeList;

	/// NoOpaqueResize config var (list).  Windows to not resize
	/// opaquely.  \sa ScreenInfo.DoOpaqueResize
	name_list *NoOpaqueResizeList;

	/// IconMenuDontShow config var.  Windows whose icons to not list in
	/// TwmIcons menu.
	name_list *IconMenuDontShow;


	/**
	 * \defgroup scr_gc_bits Various graphics contexts
	 * These are X Graphics Contexts, which are used for various sorts of
	 * drawing in X.  Stuff that needs to draw lines, or write out text,
	 * all needs to use a GC.  X-ref
	 * <https://www.x.org/releases/X11R7.7/doc/libX11/libX11/libX11.html#Graphics_Context_Functions>
	 * for upstream details.
	 * @{
	 */
	GC NormalGC; ///< normal GC for everything
	GC MenuGC;   ///< GC for menus
	GC DrawGC;   ///< GC to draw lines for move and resize
	GC BorderGC; ///< GC for drawing 3D borders
	GC rootGC;   ///< GC for internal pixmaps in image.c / image_bitmap.c
	/// @}

	/// Stash of "Black" X color for the screen
	/// \todo This should be a Pixel?
	unsigned long Black;

	/// Stash of "White" X color for the screen
	/// \todo This should be a Pixel?
	unsigned long White;

	unsigned long XORvalue;  ///< XorValue config var, or default

	/// \defgroup scr_font_bits Various font settings
	/// Definitions of various fonts to use on the Screen.
	/// @{
	MyFont TitleBarFont;     ///< TitleFont config var
	MyFont MenuFont;         ///< MenuFont config var
	MyFont IconFont;         ///< IconFont config var
	MyFont SizeFont;         ///< SizeFont config var
	MyFont IconManagerFont;  ///< IconManagerFont config var
	MyFont DefaultFont;      ///< Hardcoded fallback font
	/// @}

	/// Head of linked list of Screen's icon managers.  The head is also
	/// the default icon manager for the screen.  \sa ScreenInfo.IconMgrs
	IconMgr *iconmgr;

	/// Head of the list of IconRegion structs on the Screen.  Built out
	/// from %IconRegion config var.
	struct IconRegion *FirstRegion;

	/// Tail of the list of IconRegion structs on the Screen.  Used as an
	/// optimization in configuring the list on startup.  \todo Is this
	/// actually necessary?  Does the order matter?
	struct IconRegion *LastRegion;

	/// Pointer to head of list of window regions on screen.  Built from
	/// %WindowRegion config var.
	struct WindowRegion *FirstWindowRegion;

	/// Pointer to head of list of windowboxes on screen.  Built from
	/// %WindowBox config var.
	WindowBox *FirstWindowBox;

	char *IconDirectory;    ///< IconDirectory config var
	char *PixmapDirectory;  ///< PixmapDirectory config var

	int SizeStringOffset;   ///< X offset in size window for drawing
	int SizeStringWidth;    ///< Minimum width of size window

	int BorderWidth;        ///< BorderWidth config var
	int BorderLeft;         ///< BorderLeft config var
	int BorderRight;        ///< BorderRight config var
	int BorderTop;          ///< BorderTop config var
	int BorderBottom;       ///< BorderBottom config var
	int ThreeDBorderWidth;  ///< ThreeDBorderWidth config var
	int IconBorderWidth;    ///< IconBorderWidth config var

	/// Height of the title bar window.  Calculated from font height and
	/// padding.  \todo Maybe this should be in ScreenInfo.TBInfo above?
	/// Same can be said for a number of following fields that are
	/// titlebar related...
	int TitleHeight;

	TwmWindow *Focus;    ///< The twm window that has focus.
	                     ///< \sa ScreenInfo.FocusRoot
	int EntryHeight;     ///< Menu entry height.  Calc'd from font height.

	int FramePadding;    ///< FramePadding config var.  Distance between
	                     ///< titlebar contents and frame.
	int TitlePadding;    ///< TitlePadding config var.  Distance between
	                     ///< items in titlebar.

	int ButtonIndent;    ///< ButtonIndent config var.  Amount to shrink
	                     ///< titlebar buttons.
	int NumAutoRaises;   ///< Number of autoraise windows on screen
	int NumAutoLowers;   ///< Number of autolower windows on screen
	int TransientOnTop;  ///< TransientOnTop config var

	/// AutoRaise config flag.  \sa ScreenInfo.AutoRaise
	bool  AutoRaiseDefault;

	/// AutoLower config flag.  \sa ScreenInfo.AutoLower
	bool  AutoLowerDefault;

	bool  NoDefaults;    ///< NoDefaults config var
	UsePPoss UsePPosition;     ///< UsePPosition config var
	bool  UseSunkTitlePixmap;  ///< UseSunkTitlePixmap config var
	bool  AutoRelativeResize;  ///< AutoRelativeResize config var
	bool  FocusRoot;     ///< Is the input focus on the root?
	                     ///< \sa ScreenInfo.FocusRoot
	bool  WarpCursor;    ///< WarpCursor config var.  \sa ScreenInfo.WarpCursorL
	bool  ForceIcon;     ///< ForceIcons config var
	bool  NoGrabServer;  ///< NoGrabServer config var
	bool  NoRaiseMove;   ///< NoRaiseOnMove config var
	bool  NoRaiseResize; ///< NoRaiseOnResize config var
	bool  NoRaiseDeicon; ///< NoRaiseOnDeiconify config var
	bool  RaiseOnWarp;   ///< NoRaiseOnWarp config var (inverse)
	bool  DontMoveOff;   ///< DontMoveOff config var
	int MoveOffResistance;  ///< MoveOffResistence config var
	int MovePackResistance; ///< MovePackResistence config var
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
