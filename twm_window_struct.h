#ifndef _CTWM_TWM_WINDOW_STRUCT_H
#define _CTWM_TWM_WINDOW_STRUCT_H

/* for each window that is on the display, one of these structures
 * is allocated and linked into a list
 */
struct TwmWindow {
	struct TwmWindow *next;     /* next twm window */
	struct TwmWindow *prev;     /* previous twm window */
	OtpWinList *otp;            /* OnTopPriority info for the window */
	Window w;                   /* the child window */
	int old_bw;                 /* border width before reparenting */
	Window frame;               /* the frame window */
	Window title_w;             /* the title bar window */
	Window hilite_wl;           /* the left hilite window */
	Window hilite_wr;           /* the right hilite window */
	Window lolite_wl;           /* the left lolite window */
	Window lolite_wr;           /* the right lolite window */
	Cursor curcurs;             /* current resize cursor */
	Pixmap gray;
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
