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
 * $XConsortium: menus.c,v 1.186 91/07/17 13:58:00 dave Exp $
 *
 * twm menu code
 *
 * 17-Nov-87 Thomas E. LaStrange                File created
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 *
 ***********************************************************************/

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <X11/extensions/shape.h>

#include "ctwm_atoms.h"
#include "menus.h"
#include "resize.h"
#include "events.h"
#include "util.h"
#include "drawing.h"
#include "icons_builtin.h"
#include "parse.h"
#include "icons.h"
#include "add_window.h"
#include "decorations.h"
#include "otp.h"
#include "image.h"
#include "functions.h"
#include "screen.h"
#ifdef SOUNDS
#  include "sound.h"
#endif

#include "gram.tab.h"

MenuRoot *ActiveMenu = NULL;            /* the active menu */
MenuItem *ActiveItem = NULL;            /* the active menu item */
bool menuFromFrameOrWindowOrTitlebar = false;
char *CurrentSelectedWorkspace;

/* Should probably move, since nothing in this file uses anymore */
int AlternateKeymap;
bool AlternateContext;

int MenuDepth = 0;              /* number of menus up */
static struct {
	int x;
	int y;
} MenuOrigins[MAXMENUDEPTH];
static Cursor LastCursor;
static bool addingdefaults = false;

/*
 * Directionals for TryToPush_be().  These differ from the specs for
 * jump/pack/fill in functions. because there's an indeterminate option.
 */
typedef enum {
	PD_ANY,
	PD_BOTTOM,
	PD_LEFT,
	PD_RIGHT,
	PD_TOP,
} PushDirection;


static void Paint3DEntry(MenuRoot *mr, MenuItem *mi, bool exposure);
static void PaintNormalEntry(MenuRoot *mr, MenuItem *mi, bool exposure);
static void MosaicFade(TwmWindow *tmp_win, Window blanket);
static void DestroyMenu(MenuRoot *menu);
static void ZoomInWindow(TwmWindow *tmp_win, Window blanket);
static void ZoomOutWindow(TwmWindow *tmp_win, Window blanket);
static void FadeWindow(TwmWindow *tmp_win, Window blanket);
static void SweepWindow(TwmWindow *tmp_win, Window blanket);
static void waitamoment(float timeout);
static void TryToPush_be(TwmWindow *tmp_win, int x, int y, PushDirection dir);


#define SHADOWWIDTH 5                   /* in pixels */
#define ENTRY_SPACING 4


/***********************************************************************
 *
 *  Procedure:
 *      InitMenus - initialize menu roots
 *
 ***********************************************************************
 */

void InitMenus(void)
{
	Scr->DefaultFunction.func = 0;
	Scr->WindowFunction.func  = 0;
	Scr->ChangeWorkspaceFunction.func  = 0;
	Scr->DeIconifyFunction.func  = 0;
	Scr->IconifyFunction.func  = 0;

	Scr->FuncKeyRoot.next = NULL;
	Scr->FuncButtonRoot.next = NULL;
}


/***********************************************************************
 *
 *  Procedure:
 *      AddFuncKey - add a function key to the list
 *
 *  Inputs:
 *      name    - the name of the key
 *      cont    - the context to look for the key press in
 *      nmods   - modifier keys that need to be pressed
 *      func    - the function to perform
 *      win_name- the window name (if any)
 *      action  - the action string associated with the function (if any)
 *
 ***********************************************************************
 */

bool
AddFuncKey(char *name, int cont, int nmods, int func,
           MenuRoot *menu, char *win_name, char *action)
{
	FuncKey *tmp;
	KeySym keysym;
	KeyCode keycode;

	/*
	 * Don't let a 0 keycode go through, since that means AnyKey to the
	 * XGrabKey call in GrabKeys().
	 */
	if((keysym = XStringToKeysym(name)) == NoSymbol ||
	                (keycode = XKeysymToKeycode(dpy, keysym)) == 0) {
		return false;
	}

	/* see if there already is a key defined for this context */
	for(tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next) {
		if(tmp->keysym == keysym &&
		                tmp->cont == cont &&
		                tmp->mods == nmods) {
			break;
		}
	}

	if(tmp == NULL) {
		tmp = malloc(sizeof(FuncKey));
		tmp->next = Scr->FuncKeyRoot.next;
		Scr->FuncKeyRoot.next = tmp;
	}

	tmp->name = name;
	tmp->keysym = keysym;
	tmp->keycode = keycode;
	tmp->cont = cont;
	tmp->mods = nmods;
	tmp->func = func;
	tmp->menu = menu;
	tmp->win_name = win_name;
	tmp->action = action;

	return true;
}

/***********************************************************************
 *
 *  Procedure:
 *      AddFuncButton - add a function button to the list
 *
 *  Inputs:
 *      num     - the num of the button
 *      cont    - the context to look for the key press in
 *      nmods   - modifier keys that need to be pressed
 *      func    - the function to perform
 *      menu    - the menu (if any)
 *      item    - the menu item (if any)
 *
 ***********************************************************************
 */

void
AddFuncButton(int num, int cont, int nmods, int func,
              MenuRoot *menu, MenuItem *item)
{
	FuncButton *tmp;

	/* Find existing def for this button/context/modifier if any */
	for(tmp = Scr->FuncButtonRoot.next; tmp != NULL; tmp = tmp->next) {
		if((tmp->num == num) && (tmp->cont == cont) && (tmp->mods == nmods)) {
			break;
		}
	}

	/*
	 * If it's already set, and we're addingdefault (i.e., called from
	 * AddDefaultFuncButtons()), just return.  This lets us cram on
	 * fallback mappings, without worrying about overriding user choices.
	 */
	if(tmp && addingdefaults) {
		return;
	}

	/* No mapping yet; create a shell */
	if(tmp == NULL) {
		tmp = malloc(sizeof(FuncButton));
		tmp->next = Scr->FuncButtonRoot.next;
		Scr->FuncButtonRoot.next = tmp;
	}

	/* Set the new details */
	tmp->num  = num;
	tmp->cont = cont;
	tmp->mods = nmods;
	tmp->func = func;
	tmp->menu = menu;
	tmp->item = item;

	return;
}


/*
 * AddDefaultFuncButtons - attach default bindings so that naive users
 * don't get messed up if they provide a minimal twmrc.
 *
 * This used to be in add_window.c, and maybe fits better in
 * decorations_init.c (only place it's called) now, but is currently here
 * so addingdefaults is in scope.
 *
 * XXX Probably better to adjust things so we can do that job _without_
 * the magic global var...
 */
void
AddDefaultFuncButtons(void)
{
	addingdefaults = true;

#define SETDEF(btn, ctx, func) AddFuncButton(btn, ctx, 0, func, NULL, NULL)
	SETDEF(Button1, C_TITLE,    F_MOVE);
	SETDEF(Button1, C_ICON,     F_ICONIFY);
	SETDEF(Button1, C_ICONMGR,  F_ICONIFY);

	SETDEF(Button2, C_TITLE,    F_RAISELOWER);
	SETDEF(Button2, C_ICON,     F_ICONIFY);
	SETDEF(Button2, C_ICONMGR,  F_ICONIFY);
#undef SETDEF

	addingdefaults = false;
}


void
PaintEntry(MenuRoot *mr, MenuItem *mi, bool exposure)
{
	if(Scr->use3Dmenus) {
		Paint3DEntry(mr, mi, exposure);
	}
	else {
		PaintNormalEntry(mr, mi, exposure);
	}
	if(mi->state) {
		mr->lastactive = mi;
	}
}

static void
Paint3DEntry(MenuRoot *mr, MenuItem *mi, bool exposure)
{
	int y_offset;
	int text_y;
	GC gc;
	XRectangle ink_rect, logical_rect;
	XmbTextExtents(Scr->MenuFont.font_set, mi->item, mi->strlen,
	               &ink_rect, &logical_rect);

	y_offset = mi->item_num * Scr->EntryHeight + Scr->MenuShadowDepth;
	text_y = y_offset + Scr->MenuFont.y + 2;

	if(mi->func != F_TITLE) {
		int x, y;

		gc = Scr->NormalGC;
		if(mi->state) {
			Draw3DBorder(mr->w, Scr->MenuShadowDepth, y_offset,
			             mr->width - 2 * Scr->MenuShadowDepth, Scr->EntryHeight, 1,
			             mi->highlight, off, true, false);
			FB(mi->highlight.fore, mi->highlight.back);
			XmbDrawImageString(dpy, mr->w, Scr->MenuFont.font_set, gc,
			                   mi->x + Scr->MenuShadowDepth, text_y, mi->item, mi->strlen);
		}
		else {
			if(mi->user_colors || !exposure) {
				XSetForeground(dpy, gc, mi->normal.back);
				XFillRectangle(dpy, mr->w, gc,
				               Scr->MenuShadowDepth, y_offset,
				               mr->width - 2 * Scr->MenuShadowDepth, Scr->EntryHeight);
				FB(mi->normal.fore, mi->normal.back);
			}
			else {
				gc = Scr->MenuGC;
			}
			XmbDrawImageString(dpy, mr->w, Scr->MenuFont.font_set, gc,
			                   mi->x + Scr->MenuShadowDepth, text_y,
			                   mi->item, mi->strlen);
			if(mi->separated) {
				FB(Scr->MenuC.shadd, Scr->MenuC.shadc);
				XDrawLine(dpy, mr->w, Scr->NormalGC,
				          Scr->MenuShadowDepth,
				          y_offset + Scr->EntryHeight - 2,
				          mr->width - Scr->MenuShadowDepth,
				          y_offset + Scr->EntryHeight - 2);
				FB(Scr->MenuC.shadc, Scr->MenuC.shadd);
				XDrawLine(dpy, mr->w, Scr->NormalGC,
				          Scr->MenuShadowDepth,
				          y_offset + Scr->EntryHeight - 1,
				          mr->width - Scr->MenuShadowDepth,
				          y_offset + Scr->EntryHeight - 1);
			}
		}

		if(mi->func == F_MENU) {
			/* create the pull right pixmap if needed */
			if(Scr->pullPm == None) {
				Scr->pullPm = Create3DMenuIcon(Scr->EntryHeight - ENTRY_SPACING, &Scr->pullW,
				                               &Scr->pullH, Scr->MenuC);
			}
			x = mr->width - Scr->pullW - Scr->MenuShadowDepth - 2;
			y = y_offset + ((Scr->EntryHeight - ENTRY_SPACING - Scr->pullH) / 2) + 2;
			XCopyArea(dpy, Scr->pullPm, mr->w, gc, 0, 0, Scr->pullW, Scr->pullH, x, y);
		}
	}
	else {
		Draw3DBorder(mr->w, Scr->MenuShadowDepth, y_offset,
		             mr->width - 2 * Scr->MenuShadowDepth, Scr->EntryHeight, 1,
		             mi->normal, off, true, false);
		FB(mi->normal.fore, mi->normal.back);
		XmbDrawImageString(dpy, mr->w, Scr->MenuFont.font_set, Scr->NormalGC,
		                   mi->x + 2, text_y, mi->item, mi->strlen);
	}
}


static void
PaintNormalEntry(MenuRoot *mr, MenuItem *mi, bool exposure)
{
	int y_offset;
	int text_y;
	GC gc;
	XRectangle ink_rect, logical_rect;
	XmbTextExtents(Scr->MenuFont.font_set, mi->item, mi->strlen,
	               &ink_rect, &logical_rect);

	y_offset = mi->item_num * Scr->EntryHeight;
	text_y = y_offset + (Scr->EntryHeight - logical_rect.height) / 2
	         - logical_rect.y;

	if(mi->func != F_TITLE) {
		int x, y;

		if(mi->state) {
			XSetForeground(dpy, Scr->NormalGC, mi->highlight.back);

			XFillRectangle(dpy, mr->w, Scr->NormalGC, 0, y_offset,
			               mr->width, Scr->EntryHeight);
			FB(mi->highlight.fore, mi->highlight.back);
			XmbDrawString(dpy, mr->w, Scr->MenuFont.font_set, Scr->NormalGC,
			              mi->x, text_y, mi->item, mi->strlen);

			gc = Scr->NormalGC;
		}
		else {
			if(mi->user_colors || !exposure) {
				XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

				XFillRectangle(dpy, mr->w, Scr->NormalGC, 0, y_offset,
				               mr->width, Scr->EntryHeight);

				FB(mi->normal.fore, mi->normal.back);
				gc = Scr->NormalGC;
			}
			else {
				gc = Scr->MenuGC;
			}
			XmbDrawString(dpy, mr->w, Scr->MenuFont.font_set, gc, mi->x,
			              text_y, mi->item, mi->strlen);
			if(mi->separated)
				XDrawLine(dpy, mr->w, gc, 0, y_offset + Scr->EntryHeight - 1,
				          mr->width, y_offset + Scr->EntryHeight - 1);
		}

		if(mi->func == F_MENU) {
			/* create the pull right pixmap if needed */
			if(Scr->pullPm == None) {
				Scr->pullPm = CreateMenuIcon(Scr->MenuFont.height,
				                             &Scr->pullW, &Scr->pullH);
			}
			x = mr->width - Scr->pullW - 5;
			y = y_offset + ((Scr->MenuFont.height - Scr->pullH) / 2);
			XCopyPlane(dpy, Scr->pullPm, mr->w, gc, 0, 0,
			           Scr->pullW, Scr->pullH, x, y, 1);
		}
	}
	else {
		int y;

		XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

		/* fill the rectangle with the title background color */
		XFillRectangle(dpy, mr->w, Scr->NormalGC, 0, y_offset,
		               mr->width, Scr->EntryHeight);

		{
			XSetForeground(dpy, Scr->NormalGC, mi->normal.fore);
			/* now draw the dividing lines */
			if(y_offset)
				XDrawLine(dpy, mr->w, Scr->NormalGC, 0, y_offset,
				          mr->width, y_offset);
			y = ((mi->item_num + 1) * Scr->EntryHeight) - 1;
			XDrawLine(dpy, mr->w, Scr->NormalGC, 0, y, mr->width, y);
		}

		FB(mi->normal.fore, mi->normal.back);
		/* finally render the title */
		XmbDrawString(dpy, mr->w, Scr->MenuFont.font_set, Scr->NormalGC, mi->x,
		              text_y, mi->item, mi->strlen);
	}
}

void PaintMenu(MenuRoot *mr, XEvent *e)
{
	MenuItem *mi;

	if(Scr->use3Dmenus) {
		Draw3DBorder(mr->w, 0, 0, mr->width, mr->height,
		             Scr->MenuShadowDepth, Scr->MenuC, off, false, false);
	}
	for(mi = mr->first; mi != NULL; mi = mi->next) {
		int y_offset = mi->item_num * Scr->EntryHeight;

		/* be smart about handling the expose, redraw only the entries
		 * that we need to
		 */
		if(e->xexpose.y <= (y_offset + Scr->EntryHeight) &&
		                (e->xexpose.y + e->xexpose.height) >= y_offset) {
			PaintEntry(mr, mi, true);
		}
	}
	XSync(dpy, 0);
}


void MakeWorkspacesMenu(void)
{
	static char **actions = NULL;
	WorkSpace *wlist;
	char **act;

	if(! Scr->Workspaces) {
		return;
	}
	AddToMenu(Scr->Workspaces, "TWM Workspaces", NULL, NULL, F_TITLE, NULL,
	          NULL);
	if(! actions) {
		int count = 0;

		for(wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL;
		                wlist = wlist->next) {
			count++;
		}
		count++;
		actions = calloc(count, sizeof(char *));
		act = actions;
		for(wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL;
		                wlist = wlist->next) {
			asprintf(act, "WGOTO : %s", wlist->name);
			act++;
		}
		*act = NULL;
	}
	act = actions;
	for(wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL;
	                wlist = wlist->next) {
		AddToMenu(Scr->Workspaces, wlist->name, *act, Scr->Windows, F_MENU, NULL, NULL);
		act++;
	}
	Scr->Workspaces->pinned = false;
	MakeMenu(Scr->Workspaces);
}

static bool fromMenu;
bool
cur_fromMenu()
{
	return fromMenu;
}

void UpdateMenu(void)
{
	MenuItem *mi;
	int i, x, y, x_root, y_root, entry;
	bool done;
	MenuItem *badItem = NULL;

	fromMenu = true;

	while(1) {
		/* block until there is an event */
		if(!menuFromFrameOrWindowOrTitlebar) {
			XMaskEvent(dpy,
			           ButtonPressMask | ButtonReleaseMask |
			           KeyPressMask | KeyReleaseMask |
			           EnterWindowMask | ExposureMask |
			           VisibilityChangeMask | LeaveWindowMask |
			           ButtonMotionMask, &Event);
		}
		if(Event.type == MotionNotify) {
			/* discard any extra motion events before a release */
			while(XCheckMaskEvent(dpy,
			                      ButtonMotionMask | ButtonReleaseMask, &Event))
				if(Event.type == ButtonRelease) {
					break;
				}
		}

		if(!DispatchEvent()) {
			continue;
		}

		if((! ActiveMenu) || Cancel) {
			menuFromFrameOrWindowOrTitlebar = false;
			fromMenu = false;
			return;
		}

		if(Event.type != MotionNotify) {
			continue;
		}

		done = false;
		XQueryPointer(dpy, ActiveMenu->w, &JunkRoot, &JunkChild,
		              &x_root, &y_root, &x, &y, &JunkMask);

		/* if we haven't received the enter notify yet, wait */
		if(ActiveMenu && !ActiveMenu->entered) {
			continue;
		}

		XFindContext(dpy, ActiveMenu->w, ScreenContext, (XPointer *)&Scr);

		if(x < 0 || y < 0 ||
		                x >= ActiveMenu->width || y >= ActiveMenu->height) {
			if(ActiveItem && ActiveItem->func != F_TITLE) {
				ActiveItem->state = false;
				PaintEntry(ActiveMenu, ActiveItem, false);
			}
			ActiveItem = NULL;
			continue;
		}

		/* look for the entry that the mouse is in */
		entry = y / Scr->EntryHeight;
		for(i = 0, mi = ActiveMenu->first; mi != NULL; i++, mi = mi->next) {
			if(i == entry) {
				break;
			}
		}

		/* if there is an active item, we might have to turn it off */
		if(ActiveItem) {
			/* is the active item the one we are on ? */
			if(ActiveItem->item_num == entry && ActiveItem->state) {
				done = true;
			}

			/* if we weren't on the active entry, let's turn the old
			 * active one off
			 */
			if(!done && ActiveItem->func != F_TITLE) {
				ActiveItem->state = false;
				PaintEntry(ActiveMenu, ActiveItem, false);
			}
		}

		/* if we weren't on the active item, change the active item and turn
		 * it on
		 */
		if(!done) {
			ActiveItem = mi;
			if(ActiveItem && ActiveItem->func != F_TITLE && !ActiveItem->state) {
				ActiveItem->state = true;
				PaintEntry(ActiveMenu, ActiveItem, false);
			}
		}

		/* now check to see if we were over the arrow of a pull right entry */
		if(ActiveItem && ActiveItem->func == F_MENU &&
		                ((ActiveMenu->width - x) < (ActiveMenu->width / 3))) {
			MenuRoot *save = ActiveMenu;
			int savex = MenuOrigins[MenuDepth - 1].x;
			int savey = MenuOrigins[MenuDepth - 1].y;

			if(MenuDepth < MAXMENUDEPTH) {
				if(ActiveMenu == Scr->Workspaces) {
					CurrentSelectedWorkspace = ActiveItem->item;
				}
				PopUpMenu(ActiveItem->sub,
				          (savex + (((2 * ActiveMenu->width) / 3) - 1)),
				          (savey + ActiveItem->item_num * Scr->EntryHeight)
				          /*(savey + ActiveItem->item_num * Scr->EntryHeight +
				           (Scr->EntryHeight >> 1))*/, False);
				CurrentSelectedWorkspace = NULL;
			}
			else if(!badItem) {
				XBell(dpy, 0);
				badItem = ActiveItem;
			}

			/* if the menu did get popped up, unhighlight the active item */
			if(save != ActiveMenu && ActiveItem->state) {
				ActiveItem->state = false;
				PaintEntry(save, ActiveItem, false);
				ActiveItem = NULL;
			}
		}
		if(badItem != ActiveItem) {
			badItem = NULL;
		}
		XFlush(dpy);
	}
}


/***********************************************************************
 *
 *  Procedure:
 *      NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *      (MenuRoot *)
 *
 *  Inputs:
 *      name    - the name of the menu root
 *
 ***********************************************************************
 */

MenuRoot *NewMenuRoot(char *name)
{
	MenuRoot *tmp;

#define UNUSED_PIXEL ((unsigned long) (~0))     /* more than 24 bits */

	tmp = malloc(sizeof(MenuRoot));
	tmp->highlight.fore = UNUSED_PIXEL;
	tmp->highlight.back = UNUSED_PIXEL;
	tmp->name = name;
	tmp->prev = NULL;
	tmp->first = NULL;
	tmp->last = NULL;
	tmp->defaultitem = NULL;
	tmp->items = 0;
	tmp->width = 0;
	tmp->mapped = MRM_NEVER;
	tmp->pull = false;
	tmp->w = None;
	tmp->shadow = None;
	tmp->real_menu = false;

	if(Scr->MenuList == NULL) {
		Scr->MenuList = tmp;
		Scr->MenuList->next = NULL;
	}

	if(Scr->LastMenu == NULL) {
		Scr->LastMenu = tmp;
		Scr->LastMenu->next = NULL;
	}
	else {
		Scr->LastMenu->next = tmp;
		Scr->LastMenu = tmp;
		Scr->LastMenu->next = NULL;
	}

	if(strcmp(name, TWM_WINDOWS) == 0) {
		Scr->Windows = tmp;
	}

	if(strcmp(name, TWM_ICONS) == 0) {
		Scr->Icons = tmp;
	}

	if(strcmp(name, TWM_WORKSPACES) == 0) {
		Scr->Workspaces = tmp;
		if(!Scr->Windows) {
			NewMenuRoot(TWM_WINDOWS);
		}
	}
	if(strcmp(name, TWM_ALLWINDOWS) == 0) {
		Scr->AllWindows = tmp;
	}

	/* Added by dl 2004 */
	if(strcmp(name, TWM_ALLICONS) == 0) {
		Scr->AllIcons = tmp;
	}

	/* Added by Dan Lilliehorn (dl@dl.nu) 2000-02-29       */
	if(strcmp(name, TWM_KEYS) == 0) {
		Scr->Keys = tmp;
	}

	if(strcmp(name, TWM_VISIBLE) == 0) {
		Scr->Visible = tmp;
	}

	/* End addition */

	return (tmp);
}


/***********************************************************************
 *
 *  Procedure:
 *      AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *      (MenuItem *)
 *
 *  Inputs:
 *      menu    - pointer to the root menu to add the item
 *      item    - the text to appear in the menu
 *      action  - the string to possibly execute
 *      sub     - the menu root if it is a pull-right entry
 *      func    - the numeric function
 *      fore    - foreground color string
 *      back    - background color string
 *
 ***********************************************************************
 */

MenuItem *AddToMenu(MenuRoot *menu, char *item, char *action,
                    MenuRoot *sub, int func, char *fore, char *back)
{
	MenuItem *tmp;
	int width;
	char *itemname;
	XRectangle ink_rect;
	XRectangle logical_rect;

#ifdef DEBUG_MENUS
	fprintf(stderr, "adding menu item=\"%s\", action=%s, sub=%d, f=%d\n",
	        item, action, sub, func);
#endif

	tmp = malloc(sizeof(MenuItem));
	tmp->root = menu;

	if(menu->first == NULL) {
		menu->first = tmp;
		tmp->prev = NULL;
	}
	else {
		menu->last->next = tmp;
		tmp->prev = menu->last;
	}
	menu->last = tmp;

	if((menu == Scr->Workspaces) ||
	                (menu == Scr->Windows) ||
	                (menu == Scr->Icons) ||
	                (menu == Scr->AllWindows) ||

	                /* Added by dl 2004 */
	                (menu == Scr->AllIcons) ||

	                /* Added by Dan Lillehorn (dl@dl.nu) 2000-02-29 */
	                (menu == Scr->Keys) ||
	                (menu == Scr->Visible)) {

		itemname = item;
	}
	else if(*item == '*') {
		itemname = item + 1;
		menu->defaultitem = tmp;
	}
	else {
		itemname = item;
	}

	tmp->item = itemname;
	tmp->strlen = strlen(itemname);
	tmp->action = action;
	tmp->next = NULL;
	tmp->sub = NULL;
	tmp->state = false;
	tmp->func = func;
	tmp->separated = false;

	if(!Scr->HaveFonts) {
		CreateFonts();
	}

	XmbTextExtents(Scr->MenuFont.font_set,
	               itemname, tmp->strlen,
	               &ink_rect, &logical_rect);
	width = logical_rect.width;

	if(width <= 0) {
		width = 1;
	}
	if(width > menu->width) {
		menu->width = width;
	}

	tmp->user_colors = false;
	if(Scr->Monochrome == COLOR && fore != NULL) {
		bool save;

		save = Scr->FirstTime;
		Scr->FirstTime = true;
		GetColor(COLOR, &tmp->normal.fore, fore);
		GetColor(COLOR, &tmp->normal.back, back);
		if(Scr->use3Dmenus && !Scr->BeNiceToColormap) {
			GetShadeColors(&tmp->normal);
		}
		Scr->FirstTime = save;
		tmp->user_colors = true;
	}
	if(sub != NULL) {
		tmp->sub = sub;
		menu->pull = true;
	}
	tmp->item_num = menu->items++;

	return (tmp);
}


void MakeMenus(void)
{
	MenuRoot *mr;

	for(mr = Scr->MenuList; mr != NULL; mr = mr->next) {
		if(mr->real_menu == false) {
			continue;
		}

		mr->pinned = false;
		MakeMenu(mr);
	}
}


void MakeMenu(MenuRoot *mr)
{
	MenuItem *start, *end, *cur, *tmp;
	XColor f1, f2, f3;
	XColor b1, b2, b3;
	XColor save_fore, save_back;
	int num, i;
	int fred, fgreen, fblue;
	int bred, bgreen, bblue;
	int width, borderwidth;
	unsigned long valuemask;
	XSetWindowAttributes attributes;
	Colormap cmap = Scr->RootColormaps.cwins[0]->colormap->c;
	XRectangle ink_rect;
	XRectangle logical_rect;

	Scr->EntryHeight = Scr->MenuFont.height + 4;

	/* lets first size the window accordingly */
	if(mr->mapped == MRM_NEVER) {
		int max_entry_height = 0;

		if(mr->pull == true) {
			mr->width += 16 + 10;
		}
		width = mr->width + 10;
		for(cur = mr->first; cur != NULL; cur = cur->next) {
			XmbTextExtents(Scr->MenuFont.font_set, cur->item, cur->strlen,
			               &ink_rect, &logical_rect);
			max_entry_height = MAX(max_entry_height, logical_rect.height);

			if(cur->func != F_TITLE) {
				cur->x = 5;
			}
			else {
				cur->x = width - logical_rect.width;
				cur->x /= 2;
			}
		}
		Scr->EntryHeight = max_entry_height + ENTRY_SPACING;
		mr->height = mr->items * Scr->EntryHeight;
		mr->width += 10;
		if(Scr->use3Dmenus) {
			mr->width  += 2 * Scr->MenuShadowDepth;
			mr->height += 2 * Scr->MenuShadowDepth;
		}
		if(Scr->Shadow && ! mr->pinned) {
			/*
			 * Make sure that you don't draw into the shadow window or else
			 * the background bits there will get saved
			 */
			valuemask = (CWBackPixel | CWBorderPixel);
			attributes.background_pixel = Scr->MenuShadowColor;
			attributes.border_pixel = Scr->MenuShadowColor;
			if(Scr->SaveUnder) {
				valuemask |= CWSaveUnder;
				attributes.save_under = True;
			}
			mr->shadow = XCreateWindow(dpy, Scr->Root, 0, 0,
			                           (unsigned int) mr->width,
			                           (unsigned int) mr->height,
			                           (unsigned int)0,
			                           CopyFromParent,
			                           (unsigned int) CopyFromParent,
			                           (Visual *) CopyFromParent,
			                           valuemask, &attributes);
		}

		valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
		attributes.background_pixel = Scr->MenuC.back;
		attributes.border_pixel = Scr->MenuC.fore;
		if(mr->pinned) {
			attributes.event_mask = (ExposureMask | EnterWindowMask
			                         | LeaveWindowMask | ButtonPressMask
			                         | ButtonReleaseMask | PointerMotionMask
			                         | ButtonMotionMask
			                        );
			attributes.cursor = Scr->MenuCursor;
			valuemask |= CWCursor;
		}
		else {
			attributes.event_mask = (ExposureMask | EnterWindowMask);
		}

		if(Scr->SaveUnder && ! mr->pinned) {
			valuemask |= CWSaveUnder;
			attributes.save_under = True;
		}
		if(Scr->BackingStore) {
			valuemask |= CWBackingStore;
			attributes.backing_store = Always;
		}
		borderwidth = Scr->use3Dmenus ? 0 : 1;
		mr->w = XCreateWindow(dpy, Scr->Root, 0, 0, (unsigned int) mr->width,
		                      (unsigned int) mr->height, (unsigned int) borderwidth,
		                      CopyFromParent, (unsigned int) CopyFromParent,
		                      (Visual *) CopyFromParent,
		                      valuemask, &attributes);


		XSaveContext(dpy, mr->w, MenuContext, (XPointer)mr);
		XSaveContext(dpy, mr->w, ScreenContext, (XPointer)Scr);

		mr->mapped = MRM_UNMAPPED;
	}

	if(Scr->use3Dmenus && (Scr->Monochrome == COLOR)
	                && (mr->highlight.back == UNUSED_PIXEL)) {
		XColor xcol;
		char colname [32];
		bool save;

		xcol.pixel = Scr->MenuC.back;
		XQueryColor(dpy, cmap, &xcol);
		sprintf(colname, "#%04x%04x%04x",
		        5 * ((int)xcol.red   / 6),
		        5 * ((int)xcol.green / 6),
		        5 * ((int)xcol.blue  / 6));
		save = Scr->FirstTime;
		Scr->FirstTime = true;
		GetColor(Scr->Monochrome, &mr->highlight.back, colname);
		Scr->FirstTime = save;
	}

	if(Scr->use3Dmenus && (Scr->Monochrome == COLOR)
	                && (mr->highlight.fore == UNUSED_PIXEL)) {
		XColor xcol;
		char colname [32];
		bool save;

		xcol.pixel = Scr->MenuC.fore;
		XQueryColor(dpy, cmap, &xcol);
		sprintf(colname, "#%04x%04x%04x",
		        5 * ((int)xcol.red   / 6),
		        5 * ((int)xcol.green / 6),
		        5 * ((int)xcol.blue  / 6));
		save = Scr->FirstTime;
		Scr->FirstTime = true;
		GetColor(Scr->Monochrome, &mr->highlight.fore, colname);
		Scr->FirstTime = save;
	}
	if(Scr->use3Dmenus && !Scr->BeNiceToColormap) {
		GetShadeColors(&mr->highlight);
	}

	/* get the default colors into the menus */
	for(tmp = mr->first; tmp != NULL; tmp = tmp->next) {
		if(!tmp->user_colors) {
			if(tmp->func != F_TITLE) {
				tmp->normal.fore = Scr->MenuC.fore;
				tmp->normal.back = Scr->MenuC.back;
			}
			else {
				tmp->normal.fore = Scr->MenuTitleC.fore;
				tmp->normal.back = Scr->MenuTitleC.back;
			}
		}

		if(mr->highlight.fore != UNUSED_PIXEL) {
			tmp->highlight.fore = mr->highlight.fore;
			tmp->highlight.back = mr->highlight.back;
		}
		else {
			tmp->highlight.fore = tmp->normal.back;
			tmp->highlight.back = tmp->normal.fore;
		}
		if(Scr->use3Dmenus && !Scr->BeNiceToColormap) {
			if(tmp->func != F_TITLE) {
				GetShadeColors(&tmp->highlight);
			}
			else {
				GetShadeColors(&tmp->normal);
			}
		}
	}
	mr->pmenu = NULL;

	if(Scr->Monochrome == MONOCHROME || !Scr->InterpolateMenuColors) {
		return;
	}

	start = mr->first;
	while(1) {
		for(; start != NULL; start = start->next) {
			if(start->user_colors) {
				break;
			}
		}
		if(start == NULL) {
			break;
		}

		for(end = start->next; end != NULL; end = end->next) {
			if(end->user_colors) {
				break;
			}
		}
		if(end == NULL) {
			break;
		}

		/* we have a start and end to interpolate between */
		num = end->item_num - start->item_num;

		f1.pixel = start->normal.fore;
		XQueryColor(dpy, cmap, &f1);
		f2.pixel = end->normal.fore;
		XQueryColor(dpy, cmap, &f2);

		b1.pixel = start->normal.back;
		XQueryColor(dpy, cmap, &b1);
		b2.pixel = end->normal.back;
		XQueryColor(dpy, cmap, &b2);

		fred = ((int)f2.red - (int)f1.red) / num;
		fgreen = ((int)f2.green - (int)f1.green) / num;
		fblue = ((int)f2.blue - (int)f1.blue) / num;

		bred = ((int)b2.red - (int)b1.red) / num;
		bgreen = ((int)b2.green - (int)b1.green) / num;
		bblue = ((int)b2.blue - (int)b1.blue) / num;

		f3 = f1;
		f3.flags = DoRed | DoGreen | DoBlue;

		b3 = b1;
		b3.flags = DoRed | DoGreen | DoBlue;

		start->highlight.back = start->normal.fore;
		start->highlight.fore = start->normal.back;
		num -= 1;
		for(i = 0, cur = start->next; i < num; i++, cur = cur->next) {
			f3.red += fred;
			f3.green += fgreen;
			f3.blue += fblue;
			save_fore = f3;

			b3.red += bred;
			b3.green += bgreen;
			b3.blue += bblue;
			save_back = b3;

			XAllocColor(dpy, cmap, &f3);
			XAllocColor(dpy, cmap, &b3);
			cur->highlight.back = cur->normal.fore = f3.pixel;
			cur->highlight.fore = cur->normal.back = b3.pixel;
			cur->user_colors = true;

			f3 = save_fore;
			b3 = save_back;
		}
		start = end;
		start->highlight.back = start->normal.fore;
		start->highlight.fore = start->normal.back;
	}
	return;
}


/***********************************************************************
 *
 *  Procedure:
 *      PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *      menu    - the root pointer of the menu to pop up
 *      x, y    - location of upper left of menu
 *      center  - whether or not to center horizontally over position
 *
 ***********************************************************************
 */

bool
PopUpMenu(MenuRoot *menu, int x, int y, bool center)
{
	int WindowNameCount;
	TwmWindow **WindowNames;
	TwmWindow *tmp_win2, *tmp_win3;
	int i;
	int xl, yt;
	bool clipped;
#ifdef CLAUDE
	char tmpname3 [256], tmpname4 [256];
	int hasmoz = 0;
#endif
	if(!menu) {
		return false;
	}

	InstallRootColormap();

	if((menu == Scr->Windows) ||
	                (menu == Scr->Icons) ||
	                (menu == Scr->AllWindows) ||
	                /* Added by Dan 'dl' Lilliehorn 040607 */
	                (menu == Scr->AllIcons) ||
	                /* Added by Dan Lilliehorn (dl@dl.nu) 2000-02-29 */
	                (menu == Scr->Visible)) {
		TwmWindow *tmp_win;
		WorkSpace *ws;
		bool all, icons, visible_, allicons; /* visible, allicons:
                                                  Added by dl */
		int func;

		/* this is the twm windows menu,  let's go ahead and build it */

		all = (menu == Scr->AllWindows);
		icons = (menu == Scr->Icons);
		visible_ = (menu == Scr->Visible);    /* Added by dl */
		allicons = (menu == Scr->AllIcons);
		DestroyMenu(menu);

		menu->first = NULL;
		menu->last = NULL;
		menu->items = 0;
		menu->width = 0;
		menu->mapped = MRM_NEVER;
		menu->highlight.fore = UNUSED_PIXEL;
		menu->highlight.back = UNUSED_PIXEL;
		if(menu == Scr->Windows) {
			AddToMenu(menu, "TWM Windows", NULL, NULL, F_TITLE, NULL, NULL);
		}
		else if(menu == Scr->Icons) {
			AddToMenu(menu, "TWM Icons", NULL, NULL, F_TITLE, NULL, NULL);
		}
		else if(menu == Scr->Visible) { /* Added by dl 2000 */
			AddToMenu(menu, "TWM Visible", NULL, NULL, F_TITLE, NULL, NULL);
		}
		else if(menu == Scr->AllIcons) { /* Added by dl 2004 */
			AddToMenu(menu, "TWM All Icons", NULL, NULL, F_TITLE, NULL, NULL);
		}
		else {
			AddToMenu(menu, "TWM All Windows", NULL, NULL, F_TITLE, NULL, NULL);
		}

		ws = NULL;

		if(!(all || allicons)
		                && CurrentSelectedWorkspace && Scr->workSpaceManagerActive) {
			for(ws = Scr->workSpaceMgr.workSpaceList; ws != NULL; ws = ws->next) {
				if(strcmp(ws->name, CurrentSelectedWorkspace) == 0) {
					break;
				}
			}
		}
		if(!Scr->currentvs) {
			return false;
		}
		if(!ws) {
			ws = Scr->currentvs->wsw->currentwspc;
		}

		for(tmp_win = Scr->FirstWindow, WindowNameCount = 0;
		                tmp_win != NULL;
		                tmp_win = tmp_win->next) {
			if(tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win) {
				continue;
			}
			if(Scr->ShortAllWindowsMenus && (tmp_win->iswspmgr || tmp_win->isiconmgr)) {
				continue;
			}

			if(!(all || allicons) && !OCCUPY(tmp_win, ws)) {
				continue;
			}
			if(allicons && !tmp_win->isicon) {
				continue;
			}
			if(icons && !tmp_win->isicon) {
				continue;
			}
			if(visible_ && tmp_win->isicon) {
				continue;        /* added by dl */
			}
			WindowNameCount++;
		}
		WindowNames = calloc(WindowNameCount, sizeof(TwmWindow *));
		WindowNameCount = 0;
		for(tmp_win = Scr->FirstWindow;
		                tmp_win != NULL;
		                tmp_win = tmp_win->next) {
			if(LookInList(Scr->IconMenuDontShow, tmp_win->full_name, &tmp_win->class)) {
				continue;
			}

			if(tmp_win == Scr->workSpaceMgr.occupyWindow->twm_win) {
				continue;
			}
			if(Scr->ShortAllWindowsMenus &&
			                tmp_win == Scr->currentvs->wsw->twm_win) {
				continue;
			}
			if(Scr->ShortAllWindowsMenus && tmp_win->isiconmgr) {
				continue;
			}

			if(!(all || allicons) && ! OCCUPY(tmp_win, ws)) {
				continue;
			}
			if(allicons && !tmp_win->isicon) {
				continue;
			}
			if(icons && !tmp_win->isicon) {
				continue;
			}
			if(visible_ && tmp_win->isicon) {
				continue;        /* added by dl */
			}
			tmp_win2 = tmp_win;

			for(i = 0; i < WindowNameCount; i++) {
				int compresult;
				char *tmpname1, *tmpname2;
				tmpname1 = tmp_win2->name;
				tmpname2 = WindowNames[i]->name;
#ifdef CLAUDE
				if(strlen(tmpname1) == 1) {
					tmpname1 = " No title";
				}
				if(strlen(tmpname2) == 1) {
					tmpname2 = " No title";
				}

				if(!strncasecmp(tmp_win2->class.res_class, "navigator", 9) ||
				                !strncasecmp(tmp_win2->class.res_class, "mozilla",   7)) {
					tmpname3 [0] = ' ';
					tmpname3 [1] = '\0';
					strcat(tmpname3, tmpname1);
				}
				else {
					strcpy(tmpname3, tmpname1);
				}
				if(!strncasecmp(WindowNames[i]->class.res_class, "navigator", 9) ||
				                !strncasecmp(WindowNames[i]->class.res_class, "mozilla",   7)) {
					tmpname4 [0] = ' ';
					tmpname4 [1] = '\0';
					strcat(tmpname4, tmpname2);
				}
				else {
					strcpy(tmpname4, tmpname2);
				}
				tmpname1 = tmpname3;
				tmpname2 = tmpname4;
#endif
				if(Scr->CaseSensitive) {
					compresult = strcmp(tmpname1, tmpname2);
				}
				else {
					compresult = strcasecmp(tmpname1, tmpname2);
				}
				if(compresult < 0) {
					tmp_win3 = tmp_win2;
					tmp_win2 = WindowNames[i];
					WindowNames[i] = tmp_win3;
				}
			}
			WindowNames[WindowNameCount] = tmp_win2;
			WindowNameCount++;
		}
		func = (all || allicons || CurrentSelectedWorkspace) ? F_WINWARP :
		       F_POPUP;
		for(i = 0; i < WindowNameCount; i++) {
			char *tmpname;
			tmpname = WindowNames[i]->name;
#ifdef CLAUDE
			if(!strncasecmp(WindowNames[i]->class.res_class, "navigator", 9) ||
			                !strncasecmp(WindowNames[i]->class.res_class, "mozilla",   7) ||
			                !strncasecmp(WindowNames[i]->class.res_class, "netscape",  8) ||
			                !strncasecmp(WindowNames[i]->class.res_class, "konqueror", 9)) {
				hasmoz = 1;
			}
			if(hasmoz && strncasecmp(WindowNames[i]->class.res_class, "navigator", 9) &&
			                strncasecmp(WindowNames[i]->class.res_class, "mozilla",   7) &&
			                strncasecmp(WindowNames[i]->class.res_class, "netscape",  8) &&
			                strncasecmp(WindowNames[i]->class.res_class, "konqueror", 9)) {
				menu->last->separated = true;
				hasmoz = 0;
			}
#endif
			AddToMenu(menu, tmpname, (char *)WindowNames[i],
			          NULL, func, NULL, NULL);
		}
		free(WindowNames);

		menu->pinned = false;
		MakeMenu(menu);
	}

	/* Keys added by dl */

	if(menu == Scr->Keys) {
		FuncKey *tmpKey;
		char *tmpStr;
		char *modStr;
		char *oldact = 0;
		int oldmod = 0;

		DestroyMenu(menu);

		menu->first = NULL;
		menu->last = NULL;
		menu->items = 0;
		menu->width = 0;
		menu->mapped = MRM_NEVER;
		menu->highlight.fore = UNUSED_PIXEL;
		menu->highlight.back = UNUSED_PIXEL;

		AddToMenu(menu, "Twm Keys", NULL, NULL, F_TITLE, NULL, NULL);

		for(tmpKey = Scr->FuncKeyRoot.next; tmpKey != NULL;  tmpKey = tmpKey->next) {
			if(tmpKey->func != F_EXEC) {
				continue;
			}
			if((tmpKey->action == oldact) && (tmpKey->mods == oldmod)) {
				continue;
			}
			switch(tmpKey->mods) {
				case  1:
					modStr = "S";
					break;
				case  4:
					modStr = "C";
					break;
				case  5:
					modStr = "S + C";
					break;
				case  8:
					modStr = "M";
					break;
				case  9:
					modStr = "S + M";
					break;
				case 12:
					modStr = "C + M";
					break;
				default:
					modStr = "";
					break;
			}
			asprintf(&tmpStr, "[%s + %s] %s", tmpKey->name, modStr, tmpKey->action);

			AddToMenu(menu, tmpStr, tmpKey->action, NULL, tmpKey->func, NULL, NULL);
			oldact = tmpKey->action;
			oldmod = tmpKey->mods;
		}
		menu->pinned = false;
		MakeMenu(menu);
	}
	if(menu->w == None || menu->items == 0) {
		return false;
	}

	/* Prevent recursively bringing up menus. */
	if((!menu->pinned) && (menu->mapped == MRM_MAPPED)) {
		return false;
	}

	/*
	 * Dynamically set the parent;  this allows pull-ups to also be main
	 * menus, or to be brought up from more than one place.
	 */
	menu->prev = ActiveMenu;

	if(menu->pinned) {
		ActiveMenu    = menu;
		menu->mapped  = MRM_MAPPED;
		menu->entered = true;
		MenuOrigins [MenuDepth].x = menu->x;
		MenuOrigins [MenuDepth].y = menu->y;
		MenuDepth++;

		XRaiseWindow(dpy, menu->w);
		return true;
	}

	XGrabPointer(dpy, Scr->Root, True,
	             ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
	             ButtonMotionMask | PointerMotionHintMask,
	             GrabModeAsync, GrabModeAsync,
	             Scr->Root,
	             Scr->MenuCursor, CurrentTime);

	XGrabKeyboard(dpy, Scr->Root, True, GrabModeAsync, GrabModeAsync, CurrentTime);

	ActiveMenu = menu;
	menu->mapped = MRM_MAPPED;
	menu->entered = false;

	if(center) {
		x -= (menu->width / 2);
		y -= (Scr->EntryHeight / 2);    /* sticky menus would be nice here */
	}

	/*
	* clip to screen
	*/
	clipped = false;
	if(x + menu->width > Scr->rootw) {
		x = Scr->rootw - menu->width;
		clipped = true;
	}
	if(x < 0) {
		x = 0;
		clipped = true;
	}
	if(y + menu->height > Scr->rooth) {
		y = Scr->rooth - menu->height;
		clipped = true;
	}
	if(y < 0) {
		y = 0;
		clipped = true;
	}
	MenuOrigins[MenuDepth].x = x;
	MenuOrigins[MenuDepth].y = y;
	MenuDepth++;

	if(Scr->Root != Scr->CaptiveRoot) {
		XReparentWindow(dpy, menu->shadow, Scr->Root, x, y);
		XReparentWindow(dpy, menu->w, Scr->Root, x, y);
	}
	else {
		XMoveWindow(dpy, menu->w, x, y);
	}
	if(Scr->Shadow) {
		XMoveWindow(dpy, menu->shadow, x + SHADOWWIDTH, y + SHADOWWIDTH);
		XRaiseWindow(dpy, menu->shadow);
	}
	XMapRaised(dpy, menu->w);
	if(!Scr->NoWarpToMenuTitle && clipped && center) {
		xl = x + (menu->width      / 2);
		yt = y + (Scr->EntryHeight / 2);
		XWarpPointer(dpy, Scr->Root, Scr->Root, x, y, menu->width, menu->height, xl,
		             yt);
	}
	if(Scr->Shadow) {
		XMapWindow(dpy, menu->shadow);
	}
	XSync(dpy, 0);
	return true;
}


/***********************************************************************
 *
 *  Procedure:
 *      PopDownMenu - unhighlight the current menu selection and
 *              take down the menus
 *
 ***********************************************************************
 */

void PopDownMenu(void)
{
	MenuRoot *tmp;

	if(ActiveMenu == NULL) {
		return;
	}

	if(ActiveItem) {
		ActiveItem->state = false;
		PaintEntry(ActiveMenu, ActiveItem, false);
	}

	for(tmp = ActiveMenu; tmp != NULL; tmp = tmp->prev) {
		if(! tmp->pinned) {
			HideMenu(tmp);
		}
		UninstallRootColormap();
	}

	XFlush(dpy);
	ActiveMenu = NULL;
	ActiveItem = NULL;
	MenuDepth = 0;
	XUngrabKeyboard(dpy, CurrentTime);
	if(Context == C_WINDOW || Context == C_FRAME || Context == C_TITLE
	                || Context == C_ICON) {
		menuFromFrameOrWindowOrTitlebar = true;
	}

	return;
}


void HideMenu(MenuRoot *menu)
{
	if(!menu) {
		return;
	}

	if(Scr->Shadow) {
		XUnmapWindow(dpy, menu->shadow);
	}
	XUnmapWindow(dpy, menu->w);
	menu->mapped = MRM_UNMAPPED;
}

/***********************************************************************
 *
 *  Procedure:
 *      FindMenuRoot - look for a menu root
 *
 *  Returned Value:
 *      (MenuRoot *)  - a pointer to the menu root structure
 *
 *  Inputs:
 *      name    - the name of the menu root
 *
 ***********************************************************************
 */

MenuRoot *FindMenuRoot(char *name)
{
	MenuRoot *tmp;

	for(tmp = Scr->MenuList; tmp != NULL; tmp = tmp->next) {
		if(strcmp(name, tmp->name) == 0) {
			return (tmp);
		}
	}
	return NULL;
}



/***********************************************************************
 *
 *  Procedure:
 *      resizeFromCenter -
 *
 ***********************************************************************
 */

void resizeFromCenter(Window w, TwmWindow *tmp_win)
{
	int lastx, lasty, bw2;

	bw2 = tmp_win->frame_bw * 2;
	AddingW = tmp_win->attr.width + bw2 + 2 * tmp_win->frame_bw3D;
	AddingH = tmp_win->attr.height + tmp_win->title_height + bw2 + 2 *
	          tmp_win->frame_bw3D;

	XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	             &DragWidth, &DragHeight,
	             &JunkBW, &JunkDepth);

	XWarpPointer(dpy, None, w,
	             0, 0, 0, 0, DragWidth / 2, DragHeight / 2);
	XQueryPointer(dpy, Scr->Root, &JunkRoot,
	              &JunkChild, &JunkX, &JunkY,
	              &AddingX, &AddingY, &JunkMask);

	lastx = -10000;
	lasty = -10000;

	MenuStartResize(tmp_win, origDragX, origDragY, DragWidth, DragHeight);
	while(1) {
		XMaskEvent(dpy,
		           ButtonPressMask | PointerMotionMask | ExposureMask, &Event);

		if(Event.type == MotionNotify) {
			/* discard any extra motion events before a release */
			while(XCheckMaskEvent(dpy,
			                      ButtonMotionMask | ButtonPressMask, &Event))
				if(Event.type == ButtonPress) {
					break;
				}
		}

		if(Event.type == ButtonPress) {
			MenuEndResize(tmp_win);
			// Next line should be unneeded, done by MenuEndResize() ?
			XMoveResizeWindow(dpy, w, AddingX, AddingY, AddingW, AddingH);
			break;
		}

		if(Event.type != MotionNotify) {
			DispatchEvent2();
			if(Cancel) {
				// ...
				MenuEndResize(tmp_win);
				return;
			}
			continue;
		}

		/*
		 * XXX - if we are going to do a loop, we ought to consider
		 * using multiple GXxor lines so that we don't need to
		 * grab the server.
		 */
		XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
		              &JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

		if(lastx != AddingX || lasty != AddingY) {
			MenuDoResize(AddingX, AddingY, tmp_win);

			lastx = AddingX;
			lasty = AddingY;
		}

	}
}



/***********************************************************************
 *
 *  Procedure:
 *      ReGrab - regrab the pointer with the LastCursor;
 *
 ***********************************************************************
 */

void ReGrab(void)
{
	XGrabPointer(dpy, Scr->Root, True,
	             ButtonPressMask | ButtonReleaseMask,
	             GrabModeAsync, GrabModeAsync,
	             Scr->Root, LastCursor, CurrentTime);
}
void
SetLastCursor(Cursor newcur)
{
	LastCursor = newcur;
}


/***********************************************************************
 *
 *  Procedure:
 *      FocusOnRoot - put input focus on the root window
 *
 ***********************************************************************
 */

void FocusOnRoot(void)
{
	SetFocus(NULL, LastTimestamp());
	InstallColormaps(0, &Scr->RootColormaps);
	if(! Scr->ClickToFocus) {
		Scr->FocusRoot = true;
	}
}

static void ReMapOne(TwmWindow *t, TwmWindow *leader)
{
	if(t->icon_on) {
		Zoom(t->icon->w, t->frame);
	}
	else if(leader->icon) {
		Zoom(leader->icon->w, t->frame);
	}

	if(!t->squeezed) {
		XMapWindow(dpy, t->w);
	}
	t->mapped = true;
	if(False && Scr->Root != Scr->CaptiveRoot) {        /* XXX dubious test */
		ReparentWindow(dpy, t, WinWin, Scr->Root, t->frame_x, t->frame_y);
	}
	if(!Scr->NoRaiseDeicon) {
		OtpRaise(t, WinWin);
	}
	XMapWindow(dpy, t->frame);
	SetMapStateProp(t, NormalState);

	if(t->icon && t->icon->w) {
		XUnmapWindow(dpy, t->icon->w);
		IconDown(t);
		if(Scr->ShrinkIconTitles) {
			t->icon->title_shrunk = true;
		}
	}
	if(t->iconmanagerlist) {
		WList *wl;

		for(wl = t->iconmanagerlist; wl != NULL; wl = wl->nextv) {
			XUnmapWindow(dpy, wl->icon);
		}
	}
	t->isicon = false;
	t->icon_on = false;
	WMapDeIconify(t);
}

static void ReMapTransients(TwmWindow *tmp_win)
{
	TwmWindow *t;

	/* find t such that it is a transient or group member window */
	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		if(t != tmp_win &&
		                ((t->istransient && t->transientfor == tmp_win->w) ||
		                 (t->group == tmp_win->w && t->isicon))) {
			ReMapOne(t, tmp_win);
		}
	}
}

void DeIconify(TwmWindow *tmp_win)
{
	TwmWindow *t = tmp_win;
	bool isicon = false;

	/* de-iconify the main window */
	if(Scr->WindowMask) {
		XRaiseWindow(dpy, Scr->WindowMask);
	}
	if(tmp_win->isicon) {
		isicon = true;
		if(tmp_win->icon_on && tmp_win->icon && tmp_win->icon->w) {
			Zoom(tmp_win->icon->w, tmp_win->frame);
		}
		else if(tmp_win->group != (Window) 0) {
			t = GetTwmWindow(tmp_win->group);
			if(t && t->icon_on && t->icon && t->icon->w) {
				Zoom(t->icon->w, tmp_win->frame);
			}
		}
	}

	ReMapOne(tmp_win, t);

	if(isicon &&
	                (Scr->WarpCursor ||
	                 LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class))) {
		WarpToWindow(tmp_win, false);
	}

	/* now de-iconify any window group transients */
	ReMapTransients(tmp_win);

	if(! Scr->WindowMask && Scr->DeIconifyFunction.func != 0) {
		char *action;
		XEvent event;

		action = Scr->DeIconifyFunction.item ?
		         Scr->DeIconifyFunction.item->action : NULL;
		ExecuteFunction(Scr->DeIconifyFunction.func, action,
		                (Window) 0, tmp_win, &event, C_ROOT, false);
	}
	XSync(dpy, 0);
}

static void
UnmapTransients(TwmWindow *tmp_win, bool iconify,
                unsigned long eventMask)
{
	TwmWindow *t;

	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		if(t != tmp_win &&
		                ((t->istransient && t->transientfor == tmp_win->w) ||
		                 t->group == tmp_win->w)) {
			if(iconify) {
				if(t->icon_on) {
					Zoom(t->icon->w, tmp_win->icon->w);
				}
				else if(tmp_win->icon) {
					Zoom(t->frame, tmp_win->icon->w);
				}
			}

			/*
			 * Prevent the receipt of an UnmapNotify, since that would
			 * cause a transition to the Withdrawn state.
			 */
			t->mapped = false;
			XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
			XUnmapWindow(dpy, t->w);
			XUnmapWindow(dpy, t->frame);
			XSelectInput(dpy, t->w, eventMask);
			if(t->icon && t->icon->w) {
				XUnmapWindow(dpy, t->icon->w);
			}
			SetMapStateProp(t, IconicState);
			if(t == Scr->Focus) {
				SetFocus(NULL, LastTimestamp());
				if(! Scr->ClickToFocus) {
					Scr->FocusRoot = true;
				}
			}
			if(t->iconmanagerlist) {
				XMapWindow(dpy, t->iconmanagerlist->icon);
			}
			t->isicon = true;
			t->icon_on = false;
			WMapIconify(t);
		}
	}
}

void Iconify(TwmWindow *tmp_win, int def_x, int def_y)
{
	TwmWindow *t;
	bool iconify;
	XWindowAttributes winattrs;
	unsigned long eventMask;
	WList *wl;
	Window leader = (Window) - 1;
	Window blanket = (Window) - 1;

	iconify = (!tmp_win->iconify_by_unmapping);
	t = NULL;
	if(tmp_win->istransient) {
		leader = tmp_win->transientfor;
		t = GetTwmWindow(leader);
	}
	else if((leader = tmp_win->group) != 0 && leader != tmp_win->w) {
		t = GetTwmWindow(leader);
	}
	if(t && t->icon_on) {
		iconify = false;
	}
	if(iconify) {
		if(!tmp_win->icon || !tmp_win->icon->w) {
			CreateIconWindow(tmp_win, def_x, def_y);
		}
		else {
			IconUp(tmp_win);
		}
		if(visible(tmp_win)) {
			if(Scr->WindowMask) {
				OtpRaise(tmp_win, IconWin);
				XMapWindow(dpy, tmp_win->icon->w);
			}
			else {
				OtpRaise(tmp_win, IconWin);
				XMapWindow(dpy, tmp_win->icon->w);
			}
		}
	}
	if(tmp_win->iconmanagerlist) {
		for(wl = tmp_win->iconmanagerlist; wl != NULL; wl = wl->nextv) {
			XMapWindow(dpy, wl->icon);
		}
	}

	XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
	eventMask = winattrs.your_event_mask;

	/* iconify transients and window group first */
	UnmapTransients(tmp_win, iconify, eventMask);

	if(iconify) {
		Zoom(tmp_win->frame, tmp_win->icon->w);
	}

	/*
	 * Prevent the receipt of an UnmapNotify, since that would
	 * cause a transition to the Withdrawn state.
	 */
	tmp_win->mapped = false;

	if((Scr->IconifyStyle != ICONIFY_NORMAL) && !Scr->WindowMask) {
		XSetWindowAttributes attr;
		XGetWindowAttributes(dpy, tmp_win->frame, &winattrs);
		attr.backing_store = NotUseful;
		attr.save_under    = False;
		blanket = XCreateWindow(dpy, Scr->Root, winattrs.x, winattrs.y,
		                        winattrs.width, winattrs.height, (unsigned int) 0,
		                        CopyFromParent, (unsigned int) CopyFromParent,
		                        (Visual *) CopyFromParent, CWBackingStore | CWSaveUnder, &attr);
		XMapWindow(dpy, blanket);
	}
	XSelectInput(dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
	XUnmapWindow(dpy, tmp_win->w);
	XUnmapWindow(dpy, tmp_win->frame);
	XSelectInput(dpy, tmp_win->w, eventMask);
	SetMapStateProp(tmp_win, IconicState);

	if((Scr->IconifyStyle != ICONIFY_NORMAL) && !Scr->WindowMask) {
		switch(Scr->IconifyStyle) {
			case ICONIFY_MOSAIC:
				MosaicFade(tmp_win, blanket);
				break;
			case ICONIFY_ZOOMIN:
				ZoomInWindow(tmp_win, blanket);
				break;
			case ICONIFY_ZOOMOUT:
				ZoomOutWindow(tmp_win, blanket);
				break;
			case ICONIFY_FADE:
				FadeWindow(tmp_win, blanket);
				break;
			case ICONIFY_SWEEP:
				SweepWindow(tmp_win, blanket);
				break;
			case ICONIFY_NORMAL:
				/* Placate insufficiently smart clang warning */
				break;
		}
		XDestroyWindow(dpy, blanket);
	}
	if(tmp_win == Scr->Focus) {
		SetFocus(NULL, LastTimestamp());
		if(! Scr->ClickToFocus) {
			Scr->FocusRoot = true;
		}
	}
	tmp_win->isicon = true;
	tmp_win->icon_on = iconify;
	WMapIconify(tmp_win);
	if(! Scr->WindowMask && Scr->IconifyFunction.func != 0) {
		char *action;
		XEvent event;

		action = Scr->IconifyFunction.item ? Scr->IconifyFunction.item->action : NULL;
		ExecuteFunction(Scr->IconifyFunction.func, action,
		                (Window) 0, tmp_win, &event, C_ROOT, false);
	}
	XSync(dpy, 0);
}

void AutoSqueeze(TwmWindow *tmp_win)
{
	if(tmp_win->isiconmgr) {
		return;
	}
	if(Scr->RaiseWhenAutoUnSqueeze && tmp_win->squeezed) {
		OtpRaise(tmp_win, WinWin);
	}
	Squeeze(tmp_win);
}

void Squeeze(TwmWindow *tmp_win)
{
	long fx, fy, savex, savey;
	int  neww, newh;
	bool south;
	int  grav = ((tmp_win->hints.flags & PWinGravity)
	             ? tmp_win->hints.win_gravity : NorthWestGravity);
	XWindowAttributes winattrs;
	unsigned long eventMask;
	if(tmp_win->squeezed) {
		tmp_win->squeezed = False;
#ifdef EWMH
		EwmhSet_NET_WM_STATE(tmp_win, EWMH_STATE_SHADED);
#endif /* EWMH */
		if(!tmp_win->isicon) {
			XMapWindow(dpy, tmp_win->w);
		}
		SetupWindow(tmp_win, tmp_win->actual_frame_x, tmp_win->actual_frame_y,
		            tmp_win->actual_frame_width, tmp_win->actual_frame_height, -1);
		ReMapTransients(tmp_win);
		return;
	}

	newh = tmp_win->title_height + 2 * tmp_win->frame_bw3D;
	if(newh < 3) {
		XBell(dpy, 0);
		return;
	}
	switch(grav) {
		case SouthWestGravity :
		case SouthGravity :
		case SouthEastGravity :
			south = true;
			break;
		default :
			south = false;
			break;
	}
	if(tmp_win->title_height && !tmp_win->AlwaysSqueezeToGravity) {
		south = false;
	}

	tmp_win->squeezed = true;
	tmp_win->actual_frame_width  = tmp_win->frame_width;
	tmp_win->actual_frame_height = tmp_win->frame_height;
	savex = fx = tmp_win->frame_x;
	savey = fy = tmp_win->frame_y;
	neww  = tmp_win->actual_frame_width;
	if(south) {
		fy += tmp_win->frame_height - newh;
	}
	if(tmp_win->squeeze_info) {
		fx  += tmp_win->title_x - tmp_win->frame_bw3D;
		neww = tmp_win->title_width + 2 * (tmp_win->frame_bw + tmp_win->frame_bw3D);
	}
	XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
	eventMask = winattrs.your_event_mask;
	XSelectInput(dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
#ifdef EWMH
	EwmhSet_NET_WM_STATE(tmp_win, EWMH_STATE_SHADED);
#endif /* EWMH */
	XUnmapWindow(dpy, tmp_win->w);
	XSelectInput(dpy, tmp_win->w, eventMask);

	if(fx + neww >= Scr->rootw - Scr->BorderRight) {
		fx = Scr->rootw - Scr->BorderRight - neww;
	}
	if(fy + newh >= Scr->rooth - Scr->BorderBottom) {
		fy = Scr->rooth - Scr->BorderBottom - newh;
	}
	SetupWindow(tmp_win, fx, fy, neww, newh, -1);
	tmp_win->actual_frame_x = savex;
	tmp_win->actual_frame_y = savey;

	/* Now make the group members disappear */
	UnmapTransients(tmp_win, false, eventMask);
}

/*
 * Set WM_STATE; x-ref ICCCM section 4.1.3.1
 * https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.3.1
 */
void SetMapStateProp(TwmWindow *tmp_win, int state)
{
	unsigned long data[2];              /* "suggested" by ICCCM version 1 */

	data[0] = (unsigned long) state;
	data[1] = (unsigned long)(tmp_win->iconify_by_unmapping ? None :
	                          (tmp_win->icon ? tmp_win->icon->w : None));

	XChangeProperty(dpy, tmp_win->w, XA_WM_STATE, XA_WM_STATE, 32,
	                PropModeReplace, (unsigned char *) data, 2);
}


bool
GetWMState(Window w, int *statep, Window *iwp)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytesafter;
	unsigned long *datap = NULL;
	bool retval = false;

	if(XGetWindowProperty(dpy, w, XA_WM_STATE, 0L, 2L, False, XA_WM_STATE,
	                      &actual_type, &actual_format, &nitems, &bytesafter,
	                      (unsigned char **) &datap) != Success || !datap) {
		return false;
	}

	if(nitems <= 2) {                   /* "suggested" by ICCCM version 1 */
		*statep = (int) datap[0];
		*iwp = (Window) datap[1];
		retval = true;
	}

	XFree(datap);
	return retval;
}


int WarpToScreen(int n, int inc)
{
	Window dumwin;
	int x, y, dumint;
	unsigned int dummask;
	ScreenInfo *newscr = NULL;

	while(!newscr) {
		/* wrap around */
		if(n < 0) {
			n = NumScreens - 1;
		}
		else if(n >= NumScreens) {
			n = 0;
		}

		newscr = ScreenList[n];
		if(!newscr) {                   /* make sure screen is managed */
			if(inc) {                   /* walk around the list */
				n += inc;
				continue;
			}
			fprintf(stderr, "%s:  unable to warp to unmanaged screen %d\n",
			        ProgramName, n);
			XBell(dpy, 0);
			return (1);
		}
	}

	if(Scr->screen == n) {
		return (0);        /* already on that screen */
	}

	PreviousScreen = Scr->screen;
	XQueryPointer(dpy, Scr->Root, &dumwin, &dumwin, &x, &y,
	              &dumint, &dumint, &dummask);

	XWarpPointer(dpy, None, newscr->Root, 0, 0, 0, 0, x, y);
	Scr = newscr;
	return (0);
}



static void DestroyMenu(MenuRoot *menu)
{
	MenuItem *item;

	if(menu->w) {
		XDeleteContext(dpy, menu->w, MenuContext);
		XDeleteContext(dpy, menu->w, ScreenContext);
		if(Scr->Shadow) {
			XDestroyWindow(dpy, menu->shadow);
		}
		XDestroyWindow(dpy, menu->w);
	}

	for(item = menu->first; item;) {
		MenuItem *tmp = item;
		item = item->next;
		free(tmp);
	}
}


/*
 * warping routines
 */

void
WarpAlongRing(XButtonEvent *ev, bool forward)
{
	TwmWindow *r, *head;

	if(Scr->RingLeader) {
		head = Scr->RingLeader;
	}
	else if(!(head = Scr->Ring)) {
		return;
	}

	if(forward) {
		for(r = head->ring.next; r != head; r = r->ring.next) {
			if(!r) {
				break;
			}
			if(r->mapped && (Scr->WarpRingAnyWhere || visible(r))) {
				break;
			}
		}
	}
	else {
		for(r = head->ring.prev; r != head; r = r->ring.prev) {
			if(!r) {
				break;
			}
			if(r->mapped && (Scr->WarpRingAnyWhere || visible(r))) {
				break;
			}
		}
	}

	/* Note: (Scr->Focus != r) is necessary when we move to a workspace that
	   has a single window and we want warping to warp to it. */
	if(r && (r != head || Scr->Focus != r)) {
		TwmWindow *p = Scr->RingLeader, *t;

		Scr->RingLeader = r;
		WarpToWindow(r, true);

		if(p && p->mapped &&
		                (t = GetTwmWindow(ev->window)) &&
		                p == t) {
			p->ring.cursor_valid = true;
			p->ring.curs_x = ev->x_root - t->frame_x;
			p->ring.curs_y = ev->y_root - t->frame_y;
#ifdef DEBUG
			fprintf(stderr,
			        "WarpAlongRing: cursor_valid := true; x := %d (%d-%d), y := %d (%d-%d)\n",
			        Tmp_win->ring.curs_x, ev->x_root, t->frame_x, Tmp_win->ring.curs_y, ev->y_root,
			        t->frame_y);
#endif
			/*
			 * The check if the cursor position is inside the window is now
			 * done in WarpToWindow().
			 */
		}
	}
}


void
WarpToWindow(TwmWindow *t, bool must_raise)
{
	int x, y;

	if(t->ring.cursor_valid) {
		x = t->ring.curs_x;
		y = t->ring.curs_y;
#ifdef DEBUG
		fprintf(stderr, "WarpToWindow: cursor_valid; x == %d, y == %d\n", x, y);
#endif

		/*
		 * XXX is this correct with 3D borders? Easier check possible?
		 * frame_bw is for the left border.
		 */
		if(x < t->frame_bw) {
			x = t->frame_bw;
		}
		if(x >= t->frame_width + t->frame_bw) {
			x  = t->frame_width + t->frame_bw - 1;
		}
		if(y < t->title_height + t->frame_bw) {
			y = t->title_height + t->frame_bw;
		}
		if(y >= t->frame_height + t->frame_bw) {
			y  = t->frame_height + t->frame_bw - 1;
		}
#ifdef DEBUG
		fprintf(stderr, "WarpToWindow: adjusted    ; x := %d, y := %d\n", x, y);
#endif
	}
	else {
		x = t->frame_width / 2;
		y = t->frame_height / 2;
#ifdef DEBUG
		fprintf(stderr, "WarpToWindow: middle; x := %d, y := %d\n", x, y);
#endif
	}
#if 0
	int dest_x, dest_y;
	Window child;

	/*
	 * Check if the proposed position actually is visible. If not, raise the window.
	 * "If the coordinates are contained in a mapped
	 * child of dest_w, that child is returned to child_return."
	 * We'll need to check for the right child window; the frame probably.
	 * (What about XXX window boxes?)
	 *
	 * Alternatively, use XQueryPointer() which returns the root window
	 * the pointer is in, but XXX that won't work for VirtualScreens.
	 */
	if(XTranslateCoordinates(dpy, t->frame, Scr->Root, x, y, &dest_x, &dest_y,
	                         &child)) {
		if(child != t->frame) {
			must_raise = true;
		}
	}
#endif
	if(t->auto_raise || must_raise) {
		AutoRaiseWindow(t);
	}
	if(! visible(t)) {
		WorkSpace *wlist;

		for(wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL;
		                wlist = wlist->next) {
			if(OCCUPY(t, wlist)) {
				break;
			}
		}
		if(wlist != NULL) {
			GotoWorkSpace(Scr->currentvs, wlist);
		}
	}

	XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0, x + t->frame_x, y + t->frame_y);
	SetFocus(t, LastTimestamp());

#ifdef DEBUG
	{
		Window root_return;
		Window child_return;
		int root_x_return;
		int root_y_return;
		int win_x_return;
		int win_y_return;
		unsigned int mask_return;

		if(XQueryPointer(dpy, t->frame, &root_return, &child_return, &root_x_return,
		                 &root_y_return, &win_x_return, &win_y_return, &mask_return)) {
			fprintf(stderr,
			        "XQueryPointer: root_return=%x, child_return=%x, root_x_return=%d, root_y_return=%d, win_x_return=%d, win_y_return=%d\n",
			        root_return, child_return, root_x_return, root_y_return, win_x_return,
			        win_y_return);
		}
	}
#endif
}



/*
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type       ClientMessage
 *     message type     XA_WM_PROTOCOLS
 *     window           tmp->w
 *     format           32
 *     data[0]          message atom
 *     data[1]          time stamp
 */
void send_clientmessage(Window w, Atom a, Time timestamp)
{
	XClientMessageEvent ev;

	ev.type = ClientMessage;
	ev.window = w;
	ev.message_type = XA_WM_PROTOCOLS;
	ev.format = 32;
	ev.data.l[0] = a;
	ev.data.l[1] = timestamp;
	XSendEvent(dpy, w, False, 0L, (XEvent *) &ev);
}

void SendEndAnimationMessage(Window w, Time timestamp)
{
	send_clientmessage(w, XA_WM_END_OF_ANIMATION, timestamp);
}

void SendTakeFocusMessage(TwmWindow *tmp, Time timestamp)
{
	send_clientmessage(tmp->w, XA_WM_TAKE_FOCUS, timestamp);
}

void MoveMenu(XEvent *eventp)

{
	int    XW, YW, newX, newY;
	bool   cont;
	bool   newev;
	unsigned long event_mask;
	XEvent ev;

	if(! ActiveMenu) {
		return;
	}
	if(! ActiveMenu->pinned) {
		return;
	}

	XW = eventp->xbutton.x_root - ActiveMenu->x;
	YW = eventp->xbutton.y_root - ActiveMenu->y;
	XGrabPointer(dpy, ActiveMenu->w, True,
	             ButtonPressMask  | ButtonReleaseMask | ButtonMotionMask,
	             GrabModeAsync, GrabModeAsync,
	             None, Scr->MoveCursor, CurrentTime);

	newX = ActiveMenu->x;
	newY = ActiveMenu->y;
	cont = true;
	event_mask = ButtonPressMask | ButtonMotionMask | ButtonReleaseMask |
	             ExposureMask;
	XMaskEvent(dpy, event_mask, &ev);
	while(cont) {
		ev.xbutton.x_root -= Scr->rootx;
		ev.xbutton.y_root -= Scr->rooty;
		switch(ev.xany.type) {
			case ButtonRelease :
				cont = false;
			case MotionNotify :
				if(!cont) {
					newev = false;
					while(XCheckMaskEvent(dpy, ButtonMotionMask | ButtonReleaseMask, &ev)) {
						newev = true;
						if(ev.type == ButtonRelease) {
							break;
						}
					}
					if(ev.type == ButtonRelease) {
						continue;
					}
					if(newev) {
						ev.xbutton.x_root -= Scr->rootx;
						ev.xbutton.y_root -= Scr->rooty;
					}
				}
				newX = ev.xbutton.x_root - XW;
				newY = ev.xbutton.y_root - YW;
				if(Scr->DontMoveOff) {
					ConstrainByBorders1(&newX, ActiveMenu->width,
					                    &newY, ActiveMenu->height);
				}
				XMoveWindow(dpy, ActiveMenu->w, newX, newY);
				XMaskEvent(dpy, event_mask, &ev);
				break;
			case ButtonPress :
				cont = false;
				newX = ActiveMenu->x;
				newY = ActiveMenu->y;
				break;
			case Expose:
			case NoExpose:
				Event = ev;
				DispatchEvent();
				XMaskEvent(dpy, event_mask, &ev);
				break;
		}
	}
	XUngrabPointer(dpy, CurrentTime);
	if(ev.xany.type == ButtonRelease) {
		ButtonPressed = -1;
	}
	/*XPutBackEvent (dpy, &ev);*/
	XMoveWindow(dpy, ActiveMenu->w, newX, newY);
	ActiveMenu->x = newX;
	ActiveMenu->y = newY;
	MenuOrigins [MenuDepth - 1].x = newX;
	MenuOrigins [MenuDepth - 1].y = newY;

	return;
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplayPosition - display the position in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current twm window
 *      x, y    - position of the window
 *
 ***********************************************************************
 */

void DisplayPosition(TwmWindow *tmp_win, int x, int y)
{
	char str [100];
	char signx = '+';
	char signy = '+';

	if(x < 0) {
		x = -x;
		signx = '-';
	}
	if(y < 0) {
		y = -y;
		signy = '-';
	}
	(void) sprintf(str, " %c%-4d %c%-4d ", signx, x, signy, y);
	XRaiseWindow(dpy, Scr->SizeWindow);

	Draw3DBorder(Scr->SizeWindow, 0, 0,
	             Scr->SizeStringOffset + Scr->SizeStringWidth + SIZE_HINDENT,
	             Scr->SizeFont.height + SIZE_VINDENT * 2,
	             2, Scr->DefaultC, off, false, false);

	FB(Scr->DefaultC.fore, Scr->DefaultC.back);
	XmbDrawImageString(dpy, Scr->SizeWindow, Scr->SizeFont.font_set,
	                   Scr->NormalGC, Scr->SizeStringOffset,
	                   Scr->SizeFont.ascent + SIZE_VINDENT , str, 13);
}

static void MosaicFade(TwmWindow *tmp_win, Window blanket)
{
	int         srect;
	int         i, j, nrects;
	Pixmap      mask;
	GC          gc;
	XGCValues   gcv;
	XRectangle *rectangles;
	int  width = tmp_win->frame_width;
	int height = tmp_win->frame_height;

	srect = (width < height) ? (width / 20) : (height / 20);
	mask  = XCreatePixmap(dpy, blanket, width, height, 1);

	gcv.foreground = 1;
	gc = XCreateGC(dpy, mask, GCForeground, &gcv);
	XFillRectangle(dpy, mask, gc, 0, 0, width, height);
	gcv.function = GXclear;
	XChangeGC(dpy, gc, GCFunction, &gcv);

	nrects = ((width * height) / (srect * srect)) / 10;
	rectangles = calloc(nrects, sizeof(XRectangle));
	for(j = 0; j < nrects; j++) {
		rectangles [j].width  = srect;
		rectangles [j].height = srect;
	}
	for(i = 0; i < 10; i++) {
		for(j = 0; j < nrects; j++) {
			rectangles [j].x = ((lrand48() %  width) / srect) * srect;
			rectangles [j].y = ((lrand48() % height) / srect) * srect;
		}
		XFillRectangles(dpy, mask, gc, rectangles, nrects);
		XShapeCombineMask(dpy, blanket, ShapeBounding, 0, 0, mask, ShapeSet);
		XFlush(dpy);
		waitamoment(0.020);
	}
	XFreePixmap(dpy, mask);
	XFreeGC(dpy, gc);
	free(rectangles);
}

static void ZoomInWindow(TwmWindow *tmp_win, Window blanket)
{
	Pixmap        mask;
	GC            gc, gcn;
	XGCValues     gcv;

	int i, nsteps = 20;
	int w = tmp_win->frame_width;
	int h = tmp_win->frame_height;
	int step = (MAX(w, h)) / (2.0 * nsteps);

	mask = XCreatePixmap(dpy, blanket, w, h, 1);
	gcv.foreground = 1;
	gc  = XCreateGC(dpy, mask, GCForeground, &gcv);
	gcv.function = GXclear;
	gcn = XCreateGC(dpy, mask, GCForeground | GCFunction, &gcv);

	for(i = 0; i < nsteps; i++) {
		XFillRectangle(dpy, mask, gcn, 0, 0, w, h);
		XFillArc(dpy, mask, gc, (w / 2) - ((nsteps - i) * step),
		         (h / 2) - ((nsteps - i) * step),
		         2 * (nsteps - i) * step,
		         2 * (nsteps - i) * step,
		         0, 360 * 64);
		XShapeCombineMask(dpy, blanket, ShapeBounding, 0, 0, mask, ShapeSet);
		XFlush(dpy);
		waitamoment(0.020);
	}
}

static void ZoomOutWindow(TwmWindow *tmp_win, Window blanket)
{
	Pixmap        mask;
	GC            gc;
	XGCValues     gcv;

	int i, nsteps = 20;
	int w = tmp_win->frame_width;
	int h = tmp_win->frame_height;
	int step = (MAX(w, h)) / (2.0 * nsteps);

	mask  = XCreatePixmap(dpy, blanket, w, h, 1);
	gcv.foreground = 1;
	gc = XCreateGC(dpy, mask, GCForeground, &gcv);
	XFillRectangle(dpy, mask, gc, 0, 0, w, h);
	gcv.function = GXclear;
	XChangeGC(dpy, gc, GCFunction, &gcv);

	for(i = 0; i < nsteps; i++) {
		XFillArc(dpy, mask, gc, (w / 2) - (i * step),
		         (h / 2) - (i * step),
		         2 * i * step,
		         2 * i * step,
		         0, 360 * 64);
		XShapeCombineMask(dpy, blanket, ShapeBounding, 0, 0, mask, ShapeSet);
		XFlush(dpy);
		waitamoment(0.020);
	}
}

void FadeWindow(TwmWindow *tmp_win, Window blanket)
{
	Pixmap        mask, stipple;
	GC            gc;
	XGCValues     gcv;
	static unsigned char stipple_bits[] = { 0x0F, 0x0F,
	                                        0xF0, 0xF0,
	                                        0x0F, 0x0F,
	                                        0xF0, 0xF0,
	                                        0x0F, 0x0F,
	                                        0xF0, 0xF0,
	                                        0x0F, 0x0F,
	                                        0xF0, 0xF0,
	                                      };
	int w = tmp_win->frame_width;
	int h = tmp_win->frame_height;

	stipple = XCreateBitmapFromData(dpy, blanket, (char *)stipple_bits, 8, 8);
	mask    = XCreatePixmap(dpy, blanket, w, h, 1);
	gcv.background = 0;
	gcv.foreground = 1;
	gcv.stipple    = stipple;
	gcv.fill_style = FillOpaqueStippled;
	gc = XCreateGC(dpy, mask, GCBackground | GCForeground | GCFillStyle | GCStipple,
	               &gcv);
	XFillRectangle(dpy, mask, gc, 0, 0, w, h);

	XShapeCombineMask(dpy, blanket, ShapeBounding, 0, 0, mask, ShapeSet);
	XFlush(dpy);
	waitamoment(0.10);
	XFreePixmap(dpy, stipple);
	XFlush(dpy);
}

static void SweepWindow(TwmWindow *tmp_win, Window blanket)
{
	float step = 0.0;
	int i, nsteps = 20;
	int dir = 0, dist = tmp_win->frame_x, dist1;

	dist1 = tmp_win->frame_y;
	if(dist1 < dist) {
		dir = 1;
		dist = dist1;
	}
	dist1 = tmp_win->vs->w - (tmp_win->frame_x + tmp_win->frame_width);
	if(dist1 < dist) {
		dir = 2;
		dist = dist1;
	}
	dist1 = tmp_win->vs->h - (tmp_win->frame_y + tmp_win->frame_height);
	if(dist1 < dist) {
		dir = 3;
		dist = dist1;
	}

	switch(dir) {
		case 0:
			step = tmp_win->frame_x + tmp_win->frame_width;
			break;
		case 1:
			step = tmp_win->frame_y + tmp_win->frame_height;
			break;
		case 2:
			step = tmp_win->vs->w - tmp_win->frame_x;
			break;
		case 3:
			step = tmp_win->vs->h - tmp_win->frame_y;
			break;
	}
	step /= (float) nsteps;
	step /= (float) nsteps;
	for(i = 0; i < 20; i++) {
		int x = tmp_win->frame_x;
		int y = tmp_win->frame_y;
		switch(dir) {
			case 0:
				x -= i * i * step;
				break;
			case 1:
				y -= i * i * step;
				break;
			case 2:
				x += i * i * step;
				break;
			case 3:
				y += i * i * step;
				break;
		}
		XMoveWindow(dpy, blanket, x, y);
		XFlush(dpy);
		waitamoment(0.020);
	}
}

static void waitamoment(float timeout)
{
	struct timeval timeoutstruct;
	int usec = timeout * 1000000;
	timeoutstruct.tv_usec = usec % (unsigned long) 1000000;
	timeoutstruct.tv_sec  = usec / (unsigned long) 1000000;
	select(0, NULL, NULL, NULL, &timeoutstruct);
}

void TryToPack(TwmWindow *tmp_win, int *x, int *y)
{
	TwmWindow   *t;
	int         newx, newy;
	int         w, h;
	int         winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	int         winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;

	newx = *x;
	newy = *y;
	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		if(t == tmp_win) {
			continue;
		}
		if(t->winbox != tmp_win->winbox) {
			continue;
		}
		if(t->vs != tmp_win->vs) {
			continue;
		}
		if(!t->mapped) {
			continue;
		}

		w = t->frame_width  + 2 * t->frame_bw;
		h = t->frame_height + 2 * t->frame_bw;
		if(newx >= t->frame_x + w) {
			continue;
		}
		if(newy >= t->frame_y + h) {
			continue;
		}
		if(newx + winw <= t->frame_x) {
			continue;
		}
		if(newy + winh <= t->frame_y) {
			continue;
		}

		if(newx + Scr->MovePackResistance > t->frame_x + w) {  /* left */
			newx = MAX(newx, t->frame_x + w);
			continue;
		}
		if(newx + winw < t->frame_x + Scr->MovePackResistance) {  /* right */
			newx = MIN(newx, t->frame_x - winw);
			continue;
		}
		if(newy + Scr->MovePackResistance > t->frame_y + h) {  /* top */
			newy = MAX(newy, t->frame_y + h);
			continue;
		}
		if(newy + winh < t->frame_y + Scr->MovePackResistance) {  /* bottom */
			newy = MIN(newy, t->frame_y - winh);
			continue;
		}
	}
	*x = newx;
	*y = newy;
}


void
TryToPush(TwmWindow *tmp_win, int x, int y)
{
	TryToPush_be(tmp_win, x, y, PD_ANY);
}

static void
TryToPush_be(TwmWindow *tmp_win, int x, int y, PushDirection dir)
{
	TwmWindow   *t;
	int         newx, newy, ndir;
	bool        move;
	int         w, h;
	int         winw = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	int         winh = tmp_win->frame_height + 2 * tmp_win->frame_bw;

	for(t = Scr->FirstWindow; t != NULL; t = t->next) {
		if(t == tmp_win) {
			continue;
		}
		if(t->winbox != tmp_win->winbox) {
			continue;
		}
		if(t->vs != tmp_win->vs) {
			continue;
		}
		if(!t->mapped) {
			continue;
		}

		w = t->frame_width  + 2 * t->frame_bw;
		h = t->frame_height + 2 * t->frame_bw;
		if(x >= t->frame_x + w) {
			continue;
		}
		if(y >= t->frame_y + h) {
			continue;
		}
		if(x + winw <= t->frame_x) {
			continue;
		}
		if(y + winh <= t->frame_y) {
			continue;
		}

		move = false;
		if((dir == PD_ANY || dir == PD_LEFT) &&
		                (x + Scr->MovePackResistance > t->frame_x + w)) {
			newx = x - w;
			newy = t->frame_y;
			ndir = PD_LEFT;
			move = true;
		}
		else if((dir == PD_ANY || dir == PD_RIGHT) &&
		                (x + winw < t->frame_x + Scr->MovePackResistance)) {
			newx = x + winw;
			newy = t->frame_y;
			ndir = PD_RIGHT;
			move = true;
		}
		else if((dir == PD_ANY || dir == PD_TOP) &&
		                (y + Scr->MovePackResistance > t->frame_y + h)) {
			newx = t->frame_x;
			newy = y - h;
			ndir = PD_TOP;
			move = true;
		}
		else if((dir == PD_ANY || dir == PD_BOTTOM) &&
		                (y + winh < t->frame_y + Scr->MovePackResistance)) {
			newx = t->frame_x;
			newy = y + winh;
			ndir = PD_BOTTOM;
			move = true;
		}
		if(move) {
			TryToPush_be(t, newx, newy, ndir);
			TryToPack(t, &newx, &newy);
			ConstrainByBorders(tmp_win,
			                   &newx, t->frame_width  + 2 * t->frame_bw,
			                   &newy, t->frame_height + 2 * t->frame_bw);
			SetupWindow(t, newx, newy, t->frame_width, t->frame_height, -1);
		}
	}
}


void TryToGrid(TwmWindow *tmp_win, int *x, int *y)
{
	int w    = tmp_win->frame_width  + 2 * tmp_win->frame_bw;
	int h    = tmp_win->frame_height + 2 * tmp_win->frame_bw;
	int grav = ((tmp_win->hints.flags & PWinGravity)
	            ? tmp_win->hints.win_gravity : NorthWestGravity);

	switch(grav) {
		case ForgetGravity :
		case StaticGravity :
		case NorthWestGravity :
		case NorthGravity :
		case WestGravity :
		case CenterGravity :
			*x = ((*x - Scr->BorderLeft) / Scr->XMoveGrid) * Scr->XMoveGrid
			     + Scr->BorderLeft;
			*y = ((*y - Scr->BorderTop) / Scr->YMoveGrid) * Scr->YMoveGrid
			     + Scr->BorderTop;
			break;
		case NorthEastGravity :
		case EastGravity :
			*x = (((*x + w - Scr->BorderLeft) / Scr->XMoveGrid) *
			      Scr->XMoveGrid) - w + Scr->BorderLeft;
			*y = ((*y - Scr->BorderTop) / Scr->YMoveGrid) *
			     Scr->YMoveGrid + Scr->BorderTop;
			break;
		case SouthWestGravity :
		case SouthGravity :
			*x = ((*x - Scr->BorderLeft) / Scr->XMoveGrid) * Scr->XMoveGrid
			     + Scr->BorderLeft;
			*y = (((*y + h - Scr->BorderTop) / Scr->YMoveGrid) * Scr->YMoveGrid)
			     - h + Scr->BorderTop;
			break;
		case SouthEastGravity :
			*x = (((*x + w - Scr->BorderLeft) / Scr->XMoveGrid) *
			      Scr->XMoveGrid) - w + Scr->BorderLeft;
			*y = (((*y + h - Scr->BorderTop) / Scr->YMoveGrid) *
			      Scr->YMoveGrid) - h + Scr->BorderTop;
			break;
	}
}

void WarpCursorToDefaultEntry(MenuRoot *menu)
{
	MenuItem    *item;
	Window       root;
	int          i, x, y, xl, yt;
	unsigned int w, h, bw, d;

	for(i = 0, item = menu->first; item != menu->last; item = item->next) {
		if(item == menu->defaultitem) {
			break;
		}
		i++;
	}
	if(!XGetGeometry(dpy, menu->w, &root, &x, &y, &w, &h, &bw, &d)) {
		return;
	}
	xl = x + (menu->width / 2);
	yt = y + (i + 0.5) * Scr->EntryHeight;

	XWarpPointer(dpy, Scr->Root, Scr->Root,
	             Event.xbutton.x_root, Event.xbutton.y_root,
	             menu->width, menu->height, xl, yt);
}

