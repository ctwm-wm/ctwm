/**
 * \file
 * TwmWindow struct definition.
 *
 * This previously lived in ctwm.h, but was moved out here to make it a
 * bit easier to scan either this struct or all the other stuff in
 * ctwm.h, without so much rooting around.  It's \#include'd in ctwm.h,
 * and shouldn't be included elsewhere; it's split out purely for
 * dev ease.
 */
#ifndef _CTWM_TWM_WINDOW_STRUCT_H
#define _CTWM_TWM_WINDOW_STRUCT_H


/**
 * Info and control for every X Window we take over.
 *
 * As a window manager, our job is to...  y'know.  Manage windows.  Every
 * other window on the screen we wrap and control (as well as a few of
 * our internal windows) gets one of these structs put around it to hold
 * the various config and state info we track about it.  They get put
 * into various linked lists for each screen and workspace, and
 * references get stashed in X Contexts so we can find the window that
 * events happen on.
 *
 * Much of this is initially setup in AddWindow() when we find out about
 * and take over a window.
 */
struct TwmWindow {
	struct TwmWindow *next;  ///< Next TwmWindow on the Screen
	struct TwmWindow *prev;  ///< Previous TwmWindow on the Screen

	/// OTP control info for stacking.  Created in OtpAdd().
	OtpWinList *otp;

	/// The actual X Window handle
	Window w;

	/// Original window border width before we took it over and made our
	/// own bordering.  This comes from the XWindowAttributes we get from
	/// XGetWindowAttributes().
	int old_bw;

	/**
	 * \defgroup win_frame Window frame bits
	 * These fields are related to the "frame" window; the decoration we
	 * put around the application's own window (the thing in TwmWindow.w
	 * above) to display borders, titlebars, etc.
	 * @{
	 */
	Window frame;      ///< The X window for the overall frame
	Window title_w;    ///< The title bar Window
	Window hilite_wl;  ///< Left hilite window in titlebar
	Window hilite_wr;  ///< Right hilite window in titlebar
	Window lolite_wl;  ///< Left lolite window in titlebar
	Window lolite_wr;  ///< Right lolite window in titlebar

	/// Current resize cursor.  This changes depending on where on the
	/// frame you are, if we're making them.  \sa
	/// ScreenInfo.BorderCursors
	Cursor curcurs;

	/// Pixmap to which the border is set to when window isn't focused.
	/// \sa TwmWindow.borderC  \sa SetFocusVisualAttributes()
	/// \todo See the XXX in SetFocusVisualAttributes()
	Pixmap gray;

	/// @}

	struct Icon *icon;          /* the curent icon */
	name_list *iconslist;       /* the current list of icons */
	int frame_x;                /* x position of frame */
	int frame_y;                /* y position of frame */
	unsigned int frame_width;   /* width of frame */
	unsigned int frame_height;  /* height of frame */
	int frame_bw;               /* borderwidth of frame */
	int frame_bw3D;             /* 3D borderwidth of frame */
	int actual_frame_x;         /* save frame_y of frame when squeezed */
	int actual_frame_y;         /* save frame_x of frame when squeezed */
	unsigned int actual_frame_width;  /* save width of frame when squeezed */
	unsigned int actual_frame_height; /* save height of frame when squeezed */
	int title_x;
	int title_y;
	unsigned int title_height;  /* height of the title bar */
	unsigned int title_width;   /* width of the title bar */
	char *name;                 /* name of the window */
	char *icon_name;            /* name of the icon */
	int name_x;                 /* start x of name text */
	unsigned int name_width;    /* width of name text */
	int highlightxl;            /* start of left highlight window */
	int highlightxr;            /* start of right highlight window */
	int rightx;                 /* start of right buttons */
	XWindowAttributes attr;     /* the child window attributes */
	XSizeHints hints;           /* normal hints */
	XWMHints *wmhints;          /* WM hints */
	Window group;               /* group ID */
	XClassHint class;
	struct WList *iconmanagerlist;/* iconmanager subwindows */
	/***********************************************************************
	 * color definitions per window
	 **********************************************************************/
	ColorPair borderC;          /* border color */
	ColorPair border_tile;
	ColorPair title;
	bool iconified;             /* has the window ever been iconified? */
	bool isicon;                /* is the window an icon now ? */
	bool icon_on;               /* is the icon visible */
	bool mapped;                /* is the window mapped ? */
	bool squeezed;              /* is the window squeezed ? */
	bool auto_raise;            /* should we auto-raise this window ? */
	bool auto_lower;            /* should we auto-lower this window ? */
	bool forced;                /* has had an icon forced upon it */
	bool icon_moved;            /* user explicitly moved the icon */
	bool highlight;             /* should highlight this window */
	bool stackmode;             /* honor stackmode requests */
	bool iconify_by_unmapping;  /* unmap window to iconify it */
	bool isiconmgr;             /* this is an icon manager window */
	bool iswspmgr;              /* this is a workspace manager manager window */
	bool isoccupy;              /* this is an Occupy window */
	bool istransient;           /* this is a transient window */
	Window transientfor;        /* window contained in XA_XM_TRANSIENT_FOR */
	bool titlehighlight;        /* should I highlight the title bar */
	struct IconMgr *iconmgrp;   /* pointer to it if this is an icon manager */
	int save_frame_x;           /* x position of frame  (saved from zoom) */
	int save_frame_y;           /* y position of frame  (saved from zoom)*/
	unsigned int save_frame_width;  /* width of frame   (saved from zoom)*/
	unsigned int save_frame_height; /* height of frame  (saved from zoom)*/
	int zoomed;                 /* ZOOM_NONE || function causing zoom */
	bool wShaped;               /* this window has a bounding shape */
	unsigned long protocols;    /* which protocols this window handles */
	Colormaps cmaps;            /* colormaps for this application */
	TBWindow *titlebuttons;
	SqueezeInfo *squeeze_info;  /* should the title be squeezed? */
	bool squeeze_info_copied;   /* must above SqueezeInfo be freed? */
	struct {
		struct TwmWindow *next, *prev;
		bool cursor_valid;
		int curs_x, curs_y;
	} ring;

	bool OpaqueMove;
	bool OpaqueResize;
	bool UnmapByMovingFarAway;
	bool AutoSqueeze;
	bool StartSqueezed;
	bool AlwaysSqueezeToGravity;
	bool DontSetInactive;
	bool hasfocusvisible;      /* The window has visible focus*/
	int  occupation;
	Image *HiliteImage;         /* focus highlight window background */
	Image *LoliteImage;         /* focus lowlight window background */
	WindowRegion *wr;
	WindowBox *winbox;
	bool iswinbox;
	struct {
		int x, y;
		unsigned int width, height;
	} savegeometry;

	/* where the window is mapped (may be NULL) */
	struct VirtualScreen *vs;
	/* where it is parented (deparenting is impossible) */
	struct VirtualScreen *parent_vs;

	struct VirtualScreen *savevs;       /* for ShowBackground only */

	bool nameChanged;  /* did WM_NAME ever change? */
	/* did the user ever change the width/height? */
	bool widthEverChangedByUser;
	bool heightEverChangedByUser;
#ifdef EWMH
	EwmhWindowType ewmhWindowType;
	int ewmhFlags;
#endif /* EWMH */
};

#endif /* _CTWM_TWM_WINDOW_STRUCT_H */
