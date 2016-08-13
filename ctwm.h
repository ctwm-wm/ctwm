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
 * $XConsortium: twm.h,v 1.74 91/05/31 17:38:30 dave Exp $
 *
 * twm include file
 *
 * 28-Oct-87 Thomas E. LaStrange        File created
 * 10-Oct-90 David M. Sternlicht        Storeing saved colors on root
 ***********************************************************************/

#ifndef _CTWM_CTWM_H
#define _CTWM_CTWM_H

/*
 * Include config first, before anything else.  Including ctwm.h should
 * be the first action of any of our files, so this happens before
 * ANYthing else, anywhere.
 */
#include "ctwm_config.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <stdbool.h>

/*
 * Intrinsic.h is needed for at least the Pixel type, which we use in
 * this file.  And Intrinsic.h (always?) implicitly brings in Xlib.h
 * anyway.
 */
//#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "types.h"
#ifdef EWMH
#include "ewmh.h"
#endif

/*
 * This appears to be the standard way of testing this for portability,
 * though calling it GNUC is sorta non-portable portability   :)
 */
#ifndef __GNUC__
#define  __attribute__(x)  /*NOTHING*/
#endif

/* signal(3) handlers have been void since c89 */
#define SIGNAL_T void

#define BW 2                    /* border width */
#define BW2 4                   /* border width  * 2 */

#define MAX_BUTTONS     11      /* max mouse buttons supported */

/* contexts for button presses */
#define Alt1Mask        (1<<8)
#define Alt2Mask        (1<<9)
#define Alt3Mask        (1<<10)
#define Alt4Mask        (1<<11)
#define Alt5Mask        (1<<12)

#define C_NO_CONTEXT    -1
#define C_WINDOW        0
#define C_TITLE         1
#define C_ICON          2
#define C_ROOT          3
#define C_FRAME         4
#define C_ICONMGR       5
#define C_NAME          6
#define C_IDENTIFY      7
#define C_ALTERNATE     8
#define C_WORKSPACE     9
#define NUM_CONTEXTS    10

#define C_WINDOW_BIT    (1 << C_WINDOW)
#define C_TITLE_BIT     (1 << C_TITLE)
#define C_ICON_BIT      (1 << C_ICON)
#define C_ROOT_BIT      (1 << C_ROOT)
#define C_FRAME_BIT     (1 << C_FRAME)
#define C_ICONMGR_BIT   (1 << C_ICONMGR)
#define C_NAME_BIT      (1 << C_NAME)
#define C_ALTER_BIT     (1 << C_ALTERNATE)
#define C_WORKSPACE_BIT (1 << C_WORKSPACE)

#define C_ALL_BITS      (C_WINDOW_BIT | C_TITLE_BIT | C_ICON_BIT |\
                         C_ROOT_BIT | C_FRAME_BIT | C_ICONMGR_BIT |\
                         C_WORKSPACE_BIT)

/* modifiers for button presses */
#define MOD_SIZE        ((ShiftMask | ControlMask | Mod1Mask \
                          | Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask) + 1)

/*
 * Used for TwmWindow.zoomed.  Var holds the number of the function that
 * caused zooming, if one has, else ZOOM_NONE.  This mirror F_NOP
 * currently, but that's OK, because f.nop doesn't do anything, so it
 * can't be a real cause of zooming.
 */
#define ZOOM_NONE 0

#define FBF(fix_fore, fix_back, fix_font)\
    Gcv.foreground = fix_fore;\
    Gcv.background = fix_back;\
    Gcv.font = fix_font;\
    XChangeGC(dpy, Scr->NormalGC, GCFont|GCForeground|GCBackground,&Gcv)

#define FB(fix_fore, fix_back)\
    Gcv.foreground = fix_fore;\
    Gcv.background = fix_back;\
    XChangeGC(dpy, Scr->NormalGC, GCForeground|GCBackground,&Gcv)

#define MaxSize(a, b)  (((a) < (b)) ? (b) : (a))
#define MinSize(a, b)  (((a) > (b)) ? (b) : (a))

struct MyFont {
	char       *basename;       /* name of the font */
	XFontSet    font_set;
	int         ascent;
	int         descent;
	int         height;         /* height of the font */
	int         y;              /* Y coordinate to draw characters */
	/* Average height, maintained using the extra two auxiliary fields.  */
	unsigned int avg_height;
	float       avg_fheight;
	unsigned int avg_count;
};

struct ColorPair {
	Pixel fore, back, shadc, shadd;
};

struct TitleButtonFunc {
	struct TitleButtonFunc *next;  /* next in the list of function buttons */
	int num;                       /* button number */
	int mods;                      /* modifiers */
	int func;                      /* function to execute */
	char *action;                  /* optional action arg */
	struct MenuRoot *menuroot;     /* menu to pop on F_MENU */
};

struct TitleButton {
	struct TitleButton *next;           /* next link in chain */
	char *name;                         /* bitmap name in case of deferal */
	Image *image;                       /* image to display in button */
	int srcx, srcy;                     /* from where to start copying */
	unsigned int width, height;         /* size of pixmap */
	int dstx, dsty;                     /* to where to start copying */
	bool rightside;                     /* t: on right, f: on left */
	TitleButtonFunc *funs;              /* funcs assoc'd to each button */
};

struct TBWindow {
	Window window;                      /* which window in this frame */
	Image *image;                       /* image to display in button */
	TitleButton *info;                  /* description of this window */
};


typedef enum {
	SIJ_LEFT,
	SIJ_CENTER,
	SIJ_RIGHT,
} SIJust;

struct SqueezeInfo {
	SIJust justify;
	int num;                            /* signed pixel count or numerator */
	int denom;                          /* 0 for pix count or denominator */
};


/*
 * Type for IconRegion alignment and config entries relating
 *
 * Misspeelt for hysterical raisins
 */
typedef enum {
	IRA_UNDEF,
	IRA_TOP,
	IRA_CENTER,
	IRA_BOTTOM,
	IRA_BORDER,
} IRAlignement;

/*
 * Justification for title stuff.  Window titles (TitleJustification),
 * icon titles (IconJustification).  _Not_ the same as for
 * IconRegionJustification.
 */
typedef enum {
	TJ_UNDEF,
	TJ_LEFT,
	TJ_CENTER,
	TJ_RIGHT,
} TitleJust;

/*
 * And IconRegion Justification's.
 */
typedef enum {
	IRJ_UNDEF,
	IRJ_LEFT,
	IRJ_CENTER,
	IRJ_RIGHT,
	IRJ_BORDER,
} IRJust;


/* Gravity used by IconRegion and WindowRegion */
typedef enum {
	GRAV_NORTH,
	GRAV_EAST,
	GRAV_SOUTH,
	GRAV_WEST,
} RegGravity;


/* RandomPlacement bits */
typedef enum {
	RP_OFF,
	RP_ALL,
	RP_UNMAPPED,
} RandPlac;

/* UsePPosition */
typedef enum {
	PPOS_OFF,
	PPOS_ON,
	PPOS_NON_ZERO,
	/*
	 * may eventually want an option for having the PPosition be the
	 * initial location for the drag lines.
	 */
} UsePPoss;


/* Colormap window entry for each window in WM_COLORMAP_WINDOWS
 * ICCCM property.
 */
struct TwmColormap {
	Colormap c;                 /* Colormap id */
	int state;                  /* install(ability) state */
	unsigned long install_req;  /* request number which installed it */
	Window w;                   /* window causing load of color table */
	int refcnt;
};

/* TwmColormap.state bit definitions */
#define CM_INSTALLABLE          1
#define CM_INSTALLED            2
#define CM_INSTALL              4


struct ColormapWindow {
	Window w;                   /* Window id */
	TwmColormap *colormap;      /* Colormap for this window */
	int visibility;             /* Visibility of this window */
	int refcnt;
};

struct Colormaps {
	ColormapWindow **cwins;     /* current list of colormap windows */
	int number_cwins;           /* number of elements in current list */
	char *scoreboard;           /* conflicts between installable colortables */
};

#define ColormapsScoreboardLength(cm) ((cm)->number_cwins * \
                                       ((cm)->number_cwins - 1) / 2)

struct WindowRegion {
	struct WindowRegion *next;
	int                 x, y, w, h;
	int                 grav1, grav2;
	name_list           *clientlist;
	struct WindowEntry  *entries;
};

struct WindowEntry {
	struct WindowEntry  *next;
	int                 x, y, w, h;
	struct TwmWindow    *twm_win;
	bool                used;
};

struct WindowBox {
	struct WindowBox    *next;
	char                *name;
	char                *geometry;
	name_list           *winlist;
	Window              window;
	struct TwmWindow    *twmwin;
};

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
	char *full_name;            /* full name of the window */
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
	int save_otpri;             /* on top priority      (saved from zoom)*/
	int zoomed;                 /* ZOOM_NONE || function causing zoom */
	bool wShaped;               /* this window has a bounding shape */
	unsigned long protocols;    /* which protocols this window handles */
	Colormaps cmaps;            /* colormaps for this application */
	TBWindow *titlebuttons;
	SqueezeInfo *squeeze_info;  /* should the title be squeezed? */
	int squeeze_info_copied;    /* must above SqueezeInfo be freed? */
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

/* Used in stashing session info */
struct TWMWinConfigEntry {
	struct TWMWinConfigEntry *next;
	int tag;
	char *client_id;
	char *window_role;
	XClassHint class;
	char *wm_name;
	int wm_command_count;
	char **wm_command;
	short x, y;
	unsigned short width, height;
	short icon_x, icon_y;
	bool iconified;
	bool icon_info_present;
	bool width_ever_changed_by_user;
	bool height_ever_changed_by_user;
	/* ===================[ Matthew McNeill Feb 1997 ]======================= *
	 * Added this property to facilitate restoration of workspaces when
	 * restarting a session.
	 */
	int occupation;
	/* ====================================================================== */

};

#define DoesWmTakeFocus         (1L << 0)
#define DoesWmSaveYourself      (1L << 1)
#define DoesWmDeleteWindow      (1L << 2)


void Reborder(Time tim);
SIGNAL_T Done(int signum) __attribute__((noreturn));
void CreateFonts(void);

void RestoreWithdrawnLocation(TwmWindow *tmp);
extern char *ProgramName;
extern Display *dpy;
extern XtAppContext appContext;
extern Window ResizeWindow;     /* the window we are resizing */
extern bool HasShape;           /* this server supports Shape extension */
extern int ShapeEventBase, ShapeErrorBase;

extern int PreviousScreen;

extern Cursor UpperLeftCursor;
extern Cursor RightButt;
extern Cursor MiddleButt;
extern Cursor LeftButt;

extern XClassHint NoClass;

extern XContext TwmContext;
extern XContext MenuContext;
extern XContext ScreenContext;
extern XContext ColormapContext;

extern char *Home;
extern int HomeLen;

extern bool HandlingEvents;
extern Cursor TopCursor, TopLeftCursor, LeftCursor, BottomLeftCursor,
       BottomCursor, BottomRightCursor, RightCursor, TopRightCursor;

/* Junk vars; see comment in ctwm.c about usage */
extern Window JunkRoot, JunkChild;
extern int JunkX, JunkY;
extern unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

extern XGCValues Gcv;
extern int Argc;
extern char **Argv;

extern bool RestartPreviousState;

extern bool RestartFlag;        /* Flag that is set when SIGHUP is caught */
void DoRestart(Time t);         /* Function to perform a restart */

#define OCCUPY(w, b) ((b == NULL) ? 1 : (w->occupation & (1 << b->number)))


/*
 * Command-line arg handling bits
 */
typedef struct _ctwm_cl_args {
	bool   MultiScreen;        // ! --single, grab multiple screens
	bool   Monochrome;         // --mono, force monochrome
	bool   cfgchk;             // --cfgchk, check config and exit
	char  *InitFile;           // --file, config filename
	char  *display_name;       // --display, X server display

	bool   PrintErrorMessages; // --verbose, show more debug output
	bool   ShowWelcomeWindow;  // ! --nowelcome, show splash screen

	bool   is_captive;         // --window (flag), running captive
	Window capwin;             // --window (arg), existing window to capture
	char  *captivename;        // --name, captive name

#ifdef USEM4
	bool   KeepTmpFile;        // --keep-defs, keep generated m4 defs
	char  *keepM4_filename;    // --keep, keep m4 post-processed output
	bool   GoThroughM4;        // ! --nom4, do m4 processing
#endif

#ifdef EWMH
	bool   ewmh_replace;       // --replace, replacing running WM
#endif

	char  *client_id;          // --clientId, session client id
	char  *restore_filename;   // --restore, session filename
} ctwm_cl_args;
extern ctwm_cl_args CLarg;


#endif /* _CTWM_CTWM_H */
