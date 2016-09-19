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
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange                File created
 *
 ***********************************************************************/

#ifndef _CTWM_MENUS_H
#define _CTWM_MENUS_H

#define TWM_ROOT        "bLoB_GoOp"     /* my private root menu */
#define TWM_WINDOWS     "TwmWindows"    /* for f.menu "TwmWindows" */
#define TWM_ICONS       "TwmIcons"      /* for f.menu "TwmIcons" */
#define TWM_WORKSPACES  "TwmWorkspaces" /* for f.menu "TwmWorkspaces" */
#define TWM_ALLWINDOWS  "TwmAllWindows" /* for f.menu "TwmAllWindows" */

/* Added by dl 2004 */
#define TWM_ALLICONS    "TwmAllIcons"   /* for f.menu "TwmAllIcons" */

/*******************************************************************/
/* Added by Dan Lilliehorn (dl@dl.nu) 2000-02-29                   */
#define TWM_KEYS        "TwmKeys"       /* for f.menu "TwmKeys"    */
#define TWM_VISIBLE     "TwmVisible"    /* for f.menu "TwmVisible" */


/*
 * MenuRoot.mapped - current/past state.
 *
 * XXX Perhaps the NEVER_MAPPED stuff should be pulled out and tracked
 * some other way, and mapped just made into an bool.
 */
typedef enum {
	MRM_NEVER,
	MRM_UNMAPPED,
	MRM_MAPPED,
} MRMapState;


struct MenuItem {
	struct MenuItem *next;      /* next menu item */
	struct MenuItem *prev;      /* prev menu item */
	struct MenuRoot *sub;       /* MenuRoot of a pull right menu */
	struct MenuRoot *root;      /* back pointer to my MenuRoot */
	char *item;                 /* the character string displayed */
	char *action;               /* action to be performed */
	ColorPair normal;           /* unhiglight colors */
	ColorPair highlight;        /* highlight colors */
	short item_num;             /* item number of this menu */
	short x;                    /* x coordinate for text */
	short func;                 /* twm built in function */
	bool  state;                /* in reversed video state (i.e., active) */
	short strlen;               /* strlen(item) */
	bool  user_colors;          /* colors were specified */
	bool  separated;            /* separated from the next item */
};

struct MenuRoot {
	struct MenuItem *first;     /* first item in menu */
	struct MenuItem *last;      /* last item in menu */
	struct MenuItem *lastactive; /* last active item in menu */
	struct MenuItem *defaultitem;       /* default item in menu */
	struct MenuRoot *prev;      /* previous root menu if pull right */
	struct MenuRoot *next;      /* next in list of root menus */
	char *name;                 /* name of root */
	Window w;                   /* the window of the menu */
	Window shadow;              /* the shadow window */
	ColorPair highlight;        /* highlight colors */
	MRMapState mapped;          /* whether ever/currently mapped */
	short height;               /* height of the menu */
	short width;                /* width of the menu */
	short items;                /* number of items in the menu */
	bool  pull;                 /* is there a pull right entry ? */
	bool  entered;              /* EnterNotify following pop up */
	bool  real_menu;            /* this is a real menu */
	short x, y;                 /* position (for pinned menus) */
	bool  pinned;               /* is this a pinned menu*/
	struct MenuRoot *pmenu;     /* the associated pinned menu */
};


struct MouseButton {
	int func;                   /* the function number */
	int mask;                   /* modifier mask */
	MenuRoot *menu;             /* menu if func is F_MENU */
	MenuItem *item;             /* action to perform if func != F_MENU */
};

struct FuncButton {
	struct FuncButton *next;    /* next in the list of function buttons */
	int num;                    /* button number */
	int cont;                   /* context */
	int mods;                   /* modifiers */
	int func;                   /* the function number */
	MenuRoot *menu;             /* menu if func is F_MENU */
	MenuItem *item;             /* action to perform if func != F_MENU */
};

struct FuncKey {
	struct FuncKey *next;       /* next in the list of function keys */
	char *name;                 /* key name */
	KeySym keysym;              /* X keysym */
	KeyCode keycode;            /* X keycode */
	int cont;                   /* context */
	int mods;                   /* modifiers */
	int func;                   /* function to perform */
	char *win_name;             /* window name (if any) */
	char *action;               /* action string (if any) */
	MenuRoot *menu;             /* menu if func is F_MENU */
};

extern MenuRoot *ActiveMenu;
extern MenuItem *ActiveItem;

extern bool menuFromFrameOrWindowOrTitlebar;
extern char *CurrentSelectedWorkspace;
extern bool AlternateContext;
extern int AlternateKeymap;

#define MAXMENUDEPTH    10      /* max number of nested menus */
extern int MenuDepth;

#define WARPSCREEN_NEXT "next"
#define WARPSCREEN_PREV "prev"
#define WARPSCREEN_BACK "back"

#define COLORMAP_NEXT "next"
#define COLORMAP_PREV "prev"
#define COLORMAP_DEFAULT "default"

void InitMenus(void);
MenuRoot *NewMenuRoot(char *name);
MenuItem *AddToMenu(MenuRoot *menu, char *item, char *action,
                    MenuRoot *sub, int func, char *fore, char *back);
bool PopUpMenu(MenuRoot *menu, int x, int y, bool center);
void MakeWorkspacesMenu(void);
MenuRoot *FindMenuRoot(char *name);
bool AddFuncKey(char *name, int cont, int mods, int func,
                MenuRoot *menu, char *win_name, char *action);
void AddFuncButton(int num, int cont, int mods, int func,
                   MenuRoot *menu, MenuItem *item);
void AddDefaultFuncButtons(void);
void PopDownMenu(void);
void HideMenu(MenuRoot *menu);
void ReGrab(void);
void SetLastCursor(Cursor newcur);
void PaintEntry(MenuRoot *mr, MenuItem *mi, bool exposure);
void PaintMenu(MenuRoot *mr, XEvent *e);
bool cur_fromMenu(void);
void UpdateMenu(void);
void MakeMenus(void);
void MakeMenu(MenuRoot *mr);
void MoveMenu(XEvent *eventp);
void send_clientmessage(Window w, Atom a, Time timestamp);
void SendEndAnimationMessage(Window w, Time timestamp);
void SendTakeFocusMessage(TwmWindow *tmp, Time timestamp);
void RaiseWindow(TwmWindow *tmp_win);
void LowerWindow(TwmWindow *tmp_win);
void RaiseLower(TwmWindow *tmp_win);
void RaiseLowerFrame(Window frame, int ontop);
void MapRaised(TwmWindow *tmp_win);
void RaiseFrame(Window frame);
void TryToPack(TwmWindow *tmp_win, int *x, int *y);
void TryToPush(TwmWindow *tmp_win, int x, int y);
void TryToGrid(TwmWindow *tmp_win, int *x, int *y);
void WarpCursorToDefaultEntry(MenuRoot *menu);
void WarpToWindow(TwmWindow *t, bool must_raise);

/* To move soonish? */
void WarpAlongRing(XButtonEvent *ev, bool forward);
int WarpToScreen(int n, int inc);

#endif /* _CTWM_MENUS_H */
