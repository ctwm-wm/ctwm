/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
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
 * $XConsortium: iconmgr.c,v 1.48 91/09/10 15:27:07 dave Exp $
 *
 * Icon Manager routines
 *
 * 09-Mar-89 Tom LaStrange              File Created
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

#include <X11/Xatom.h>

#include "util.h"
#include "icons_builtin.h"
#include "parse.h"
#include "screen.h"
#include "decorations.h"
#include "drawing.h"
#include "resize.h"
#include "otp.h"
#include "add_window.h"

const int siconify_width = 11;
const int siconify_height = 11;
int iconmgr_textx = /*siconify_width*/11 + 11;
static unsigned char siconify_bits[] = {
	0xff, 0x07, 0x01, 0x04, 0x0d, 0x05, 0x9d, 0x05, 0xb9, 0x04, 0x51, 0x04,
	0xe9, 0x04, 0xcd, 0x05, 0x85, 0x05, 0x01, 0x04, 0xff, 0x07
};

static WList *Active = NULL;
static WList *Current = NULL;
WList *DownIconManager = NULL;

/***********************************************************************
 *
 *  Procedure:
 *      CreateIconManagers - creat all the icon manager windows
 *              for this screen.
 *
 *  Returned Value:
 *      none
 *
 *  Inputs:
 *      none
 *
 ***********************************************************************
 */

void CreateIconManagers(void)
{
	IconMgr *p, *q;
	int mask;
	char str[100];
	char str1[100];
	Pixel background;
	char *icon_name;
	WorkSpace    *ws;
	XWMHints      wmhints;
	XSizeHints    sizehints;
	int           gravity;
	int bw;

	if(Scr->NoIconManagers) {
		return;
	}

	if(Scr->use3Diconmanagers) {
		iconmgr_textx += Scr->IconManagerShadowDepth;
	}
	if(Scr->siconifyPm == None) {
		Scr->siconifyPm = XCreatePixmapFromBitmapData(dpy, Scr->Root,
		                  (char *)siconify_bits, siconify_width,
		                  siconify_height, 1, 0, 1);
	}

	ws = Scr->workSpaceMgr.workSpaceList;
	for(q = Scr->iconmgr; q != NULL; q = q->nextv) {
		for(p = q; p != NULL; p = p->next) {
			int gx, gy;

			snprintf(str, sizeof(str), "%s Icon Manager", p->name);
			snprintf(str1, sizeof(str1), "%s Icons", p->name);
			if(p->icon_name) {
				icon_name = p->icon_name;
			}
			else {
				icon_name = str1;
			}

			if(!p->geometry || !strlen(p->geometry)) {
				p->geometry = "+0+0";
			}
			mask = XParseGeometry(p->geometry, &gx, &gy,
			                      (unsigned int *) &p->width, (unsigned int *)&p->height);

			bw = LookInList(Scr->NoBorder, str, NULL) ? 0 :
			     (Scr->ThreeDBorderWidth ? Scr->ThreeDBorderWidth : Scr->BorderWidth);

			if(mask & XNegative) {
				gx += Scr->rootw - p->width - 2 * bw;
				gravity = (mask & YNegative) ? SouthEastGravity : NorthEastGravity;
			}
			else {
				gravity = (mask & YNegative) ? SouthWestGravity : NorthWestGravity;
			}
			if(mask & YNegative) {
				gy += Scr->rooth - p->height - 2 * bw;
			}

			background = Scr->IconManagerC.back;
			GetColorFromList(Scr->IconManagerBL, p->name, NULL,
			                 &background);

			if(p->width  < 1) {
				p->width  = 1;
			}
			if(p->height < 1) {
				p->height = 1;
			}
			p->w = XCreateSimpleWindow(dpy, Scr->Root,
			                           gx, gy, p->width, p->height, 1,
			                           Scr->Black, background);


			/* Scr->workSpaceMgr.activeWSPC = ws; */
			wmhints.initial_state = NormalState;
			wmhints.input         = True;
			wmhints.flags         = InputHint | StateHint;

			XmbSetWMProperties(dpy, p->w, str, icon_name, NULL, 0, NULL,
					&wmhints, NULL);

			p->twm_win = AddWindow(p->w, AWT_ICON_MANAGER, p, Scr->currentvs);
			/*
			 * SetupOccupation() called from AddWindow() doesn't setup
			 * occupation for icon managers, nor clear vs if occupation lacks.
			 *
			 * There is no Scr->currentvs->wsw->currentwspc set up this
			 * early, so we can't check with that; the best check we can do
			 * is use ws->number.  This may be incorrect when re-starting
			 * ctwm.
			 */
			if(ws) {
				p->twm_win->occupation = 1 << ws->number;
				if(ws->number > 0) {
					p->twm_win->vs = NULL;
				}
			}
			else {
				p->twm_win->occupation = 1;
			}
#ifdef DEBUG_ICONMGR
			fprintf(stderr,
			        "CreateIconManagers: IconMgr %p: x=%d y=%d w=%d h=%d occupation=%x\n",
			        p, gx, gy,  p->width, p->height, p->twm_win->occupation);
#endif

			sizehints.flags       = PWinGravity;
			sizehints.win_gravity = gravity;
			XSetWMSizeHints(dpy, p->w, &sizehints, XA_WM_NORMAL_HINTS);

			p->twm_win->mapped = false;
			SetMapStateProp(p->twm_win, WithdrawnState);
			if(p->twm_win && p->twm_win->wmhints &&
			                (p->twm_win->wmhints->initial_state == IconicState)) {
				p->twm_win->isicon = true;
			}
			else if(!Scr->NoIconManagers && Scr->ShowIconManager) {
				p->twm_win->isicon = false;
			}
			else {
				p->twm_win->isicon = true;
			}
		}
		if(ws != NULL) {
			ws = ws->next;
		}
	}

	if(Scr->workSpaceManagerActive) {
		Scr->workSpaceMgr.workSpaceList->iconmgr = Scr->iconmgr;
	}

	for(q = Scr->iconmgr; q != NULL; q = q->nextv) {
		for(p = q; p != NULL; p = p->next) {
			GrabButtons(p->twm_win);
			GrabKeys(p->twm_win);
		}
	}

}

/***********************************************************************
 *
 *  Procedure:
 *      AllocateIconManager - allocate a new icon manager
 *
 *  Inputs:
 *      name    - the name of this icon manager
 *      icon_name - the name of the associated icon
 *      geom    - a geometry string to eventually parse
 *      columns - the number of columns this icon manager has
 *
 ***********************************************************************
 */

IconMgr *AllocateIconManager(char *name, char *icon_name, char *geom,
                             int columns)
{
	IconMgr *p;

#ifdef DEBUG_ICONMGR
	fprintf(stderr, "AllocateIconManager\n");
	fprintf(stderr, "  name=\"%s\" icon_name=\"%s\", geom=\"%s\", col=%d\n",
	        name, icon_name, geom, columns);
#endif

	if(Scr->NoIconManagers) {
		return NULL;
	}

	if(columns < 1) {
		columns = 1;
	}
	p = calloc(1, sizeof(IconMgr));
	p->name      = name;
	p->icon_name = icon_name;
	p->geometry  = geom;
	p->columns   = columns;
	p->scr       = Scr;
	p->width     = 150;
	p->height    = 10;

	if(Scr->iconmgr == NULL) {
		Scr->iconmgr = p;
		Scr->iconmgr->lasti = p;
	}
	else {
		Scr->iconmgr->lasti->next = p;
		p->prev = Scr->iconmgr->lasti;
		Scr->iconmgr->lasti = p;
	}
	return(p);
}

/***********************************************************************
 *
 *  Procedure:
 *      MoveIconManager - move the pointer around in an icon manager
 *
 *  Inputs:
 *      dir     - one of the following:
 *                      F_FORWICONMGR   - forward in the window list
 *                      F_BACKICONMGR   - backward in the window list
 *                      F_UPICONMGR     - up one row
 *                      F_DOWNICONMGR   - down one row
 *                      F_LEFTICONMGR   - left one column
 *                      F_RIGHTICONMGR  - right one column
 *
 *  Special Considerations:
 *      none
 *
 ***********************************************************************
 */

void MoveIconManager(int dir)
{
	IconMgr *ip;
	WList *tmp = NULL;
	int cur_row, cur_col, new_row, new_col;
	int row_inc, col_inc;
	bool got_it;

	if(!Current) {
		return;
	}

	cur_row = Current->row;
	cur_col = Current->col;
	ip = Current->iconmgr;

	row_inc = 0;
	col_inc = 0;
	got_it = false;

	switch(dir) {
		case F_FORWICONMGR:
			if((tmp = Current->next) == NULL) {
				tmp = ip->first;
			}
			got_it = true;
			break;

		case F_BACKICONMGR:
			if((tmp = Current->prev) == NULL) {
				tmp = ip->last;
			}
			got_it = true;
			break;

		case F_UPICONMGR:
			row_inc = -1;
			break;

		case F_DOWNICONMGR:
			row_inc = 1;
			break;

		case F_LEFTICONMGR:
			col_inc = -1;
			break;

		case F_RIGHTICONMGR:
			col_inc = 1;
			break;
	}

	/* If got_it is false ast this point then we got a left, right,
	 * up, or down, command.  We will enter this loop until we find
	 * a window to warp to.
	 */
	new_row = cur_row;
	new_col = cur_col;

	while(!got_it) {
		new_row += row_inc;
		new_col += col_inc;
		if(new_row < 0) {
			new_row = ip->cur_rows - 1;
		}
		if(new_col < 0) {
			new_col = ip->cur_columns - 1;
		}
		if(new_row >= ip->cur_rows) {
			new_row = 0;
		}
		if(new_col >= ip->cur_columns) {
			new_col = 0;
		}

		/* Now let's go through the list to see if there is an entry with this
		 * new position
		 */
		for(tmp = ip->first; tmp != NULL; tmp = tmp->next) {
			if(tmp->row == new_row && tmp->col == new_col) {
				got_it = true;
				break;
			}
		}
	}

	if(!got_it) {
		fprintf(stderr,
		        "%s:  unable to find window (%d, %d) in icon manager\n",
		        ProgramName, new_row, new_col);
		return;
	}

	if(tmp == NULL) {
		return;
	}

	/* raise the frame so the icon manager is visible */
	if(ip->twm_win->mapped) {
		OtpRaise(ip->twm_win, WinWin);
		XWarpPointer(dpy, None, tmp->icon, 0, 0, 0, 0, 5, 5);
	}
	else {
		if(tmp->twm->title_height) {
			int tbx = Scr->TBInfo.titlex;
			int x = tmp->twm->highlightxr;
			XWarpPointer(dpy, None, tmp->twm->title_w, 0, 0, 0, 0,
			             tbx + (x - tbx) / 2,
			             Scr->TitleHeight / 4);
		}
		else {
			XWarpPointer(dpy, None, tmp->twm->w, 0, 0, 0, 0, 5, 5);
		}
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      MoveMappedIconManager - move the pointer around in an icon manager
 *
 *  Inputs:
 *      dir     - one of the following:
 *                      F_FORWMAPICONMGR        - forward in the window list
 *                      F_BACKMAPICONMGR        - backward in the window list
 *
 *  Special Considerations:
 *      none
 *
 ***********************************************************************
 */

void MoveMappedIconManager(int dir)
{
	IconMgr *ip;
	WList *tmp = NULL;
	WList *orig = NULL;
	bool got_it;

	if(!Current) {
		Current = Active;
	}
	if(!Current) {
		return;
	}

	ip = Current->iconmgr;

	got_it = false;
	tmp = Current;
	orig = Current;

	while(!got_it) {
		switch(dir) {
			case F_FORWMAPICONMGR:
				if((tmp = tmp->next) == NULL) {
					tmp = ip->first;
				}
				break;

			case F_BACKMAPICONMGR:
				if((tmp = tmp->prev) == NULL) {
					tmp = ip->last;
				}
				break;
		}
		if(tmp->twm->mapped) {
			got_it = true;
			break;
		}
		if(tmp == orig) {
			break;
		}
	}

	if(!got_it) {
		fprintf(stderr, "%s:  unable to find open window in icon manager\n",
		        ProgramName);
		return;
	}

	if(tmp == NULL) {
		return;
	}

	/* raise the frame so the icon manager is visible */
	if(ip->twm_win->mapped) {
		OtpRaise(ip->twm_win, WinWin);
		XWarpPointer(dpy, None, tmp->icon, 0, 0, 0, 0, 5, 5);
	}
	else {
		if(tmp->twm->title_height) {
			XWarpPointer(dpy, None, tmp->twm->title_w, 0, 0, 0, 0,
			             tmp->twm->title_width / 2,
			             Scr->TitleHeight / 4);
		}
		else {
			XWarpPointer(dpy, None, tmp->twm->w, 0, 0, 0, 0, 5, 5);
		}
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      JumpIconManager - jump from one icon manager to another,
 *              possibly even on another screen
 *
 *  Inputs:
 *      dir     - one of the following:
 *                      F_NEXTICONMGR   - go to the next icon manager
 *                      F_PREVICONMGR   - go to the previous one
 *
 ***********************************************************************
 */

void JumpIconManager(int dir)
{
	IconMgr *ip, *tmp_ip = NULL;
	bool got_it = false;
	ScreenInfo *sp;
	int screen;

	if(!Current) {
		return;
	}


#define ITER(i) (dir == F_NEXTICONMGR ? (i)->next : (i)->prev)
#define IPOFSP(sp) (dir == F_NEXTICONMGR ? sp->iconmgr : sp->iconmgr->lasti)
#define TEST(ip) if ((ip)->count != 0 && (ip)->twm_win->mapped) \
                 { got_it = true; break; }

	ip = Current->iconmgr;
	for(tmp_ip = ITER(ip); tmp_ip; tmp_ip = ITER(tmp_ip)) {
		TEST(tmp_ip);
	}

	if(!got_it) {
		int origscreen = ip->scr->screen;
		int inc = (dir == F_NEXTICONMGR ? 1 : -1);

		for(screen = origscreen + inc; ; screen += inc) {
			if(screen >= NumScreens) {
				screen = 0;
			}
			else if(screen < 0) {
				screen = NumScreens - 1;
			}

			sp = ScreenList[screen];
			if(sp) {
				for(tmp_ip = IPOFSP(sp); tmp_ip; tmp_ip = ITER(tmp_ip)) {
					TEST(tmp_ip);
				}
			}
			if(got_it || screen == origscreen) {
				break;
			}
		}
	}

#undef ITER
#undef IPOFSP
#undef TEST

	if(!got_it) {
		XBell(dpy, 0);
		return;
	}

	/* raise the frame so it is visible */
	OtpRaise(tmp_ip->twm_win, WinWin);
	if(tmp_ip->active) {
		XWarpPointer(dpy, None, tmp_ip->active->icon, 0, 0, 0, 0, 5, 5);
	}
	else {
		XWarpPointer(dpy, None, tmp_ip->w, 0, 0, 0, 0, 5, 5);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      AddIconManager - add a window to an icon manager
 *
 *  Inputs:
 *      tmp_win - the TwmWindow structure
 *
 ***********************************************************************
 */

WList *AddIconManager(TwmWindow *tmp_win)
{
	WList *tmp, *old;
	IconMgr *ip;

	/* Some window types don't wind up in icon managers ever */
	if(tmp_win->isiconmgr || tmp_win->istransient || tmp_win->iswspmgr
	                || tmp_win->w == Scr->workSpaceMgr.occupyWindow->w) {
		return NULL;
	}

	/* Icon managers can be shut off wholesale in the config */
	if(Scr->NoIconManagers) {
		return NULL;
	}

	/* Config could declare not to IMify this type of window in two ways */
	if(LookInList(Scr->IconMgrNoShow, tmp_win->full_name, &tmp_win->class)) {
		return NULL;
	}
	if(Scr->IconManagerDontShow
	                && !LookInList(Scr->IconMgrShow, tmp_win->full_name, &tmp_win->class)) {
		return NULL;
	}

	/* Dredge up the appropriate IM */
	if((ip = (IconMgr *)LookInList(Scr->IconMgrs, tmp_win->full_name,
	                               &tmp_win->class)) == NULL) {
		if(Scr->workSpaceManagerActive) {
			ip = Scr->workSpaceMgr.workSpaceList->iconmgr;
		}
		else {
			ip = Scr->iconmgr;
		}
	}

	/* IM's exist in all workspaces, so loop through WSen */
	tmp = NULL;
	old = tmp_win->iconmanagerlist;
	while(ip != NULL) {
		int h;
		unsigned long valuemask;         /* mask for create windows */
		XSetWindowAttributes attributes; /* attributes for create windows */

		/* Is the window in this workspace? */
		if((tmp_win->occupation & ip->twm_win->occupation) == 0) {
			/* Nope, skip onward */
			ip = ip->nextv;
			continue;
		}

		/* Yep, create entry and stick it in */
		tmp = calloc(1, sizeof(WList));
		tmp->iconmgr = ip;
		tmp->twm = tmp_win;

		InsertInIconManager(ip, tmp, tmp_win);

		/* IM color settings, shared worldwide */
		tmp->cp.fore   = Scr->IconManagerC.fore;
		tmp->cp.back   = Scr->IconManagerC.back;
		tmp->highlight = Scr->IconManagerHighlight;

		GetColorFromList(Scr->IconManagerFL, tmp_win->full_name,
		                 &tmp_win->class, &tmp->cp.fore);
		GetColorFromList(Scr->IconManagerBL, tmp_win->full_name,
		                 &tmp_win->class, &tmp->cp.back);
		GetColorFromList(Scr->IconManagerHighlightL, tmp_win->full_name,
		                 &tmp_win->class, &tmp->highlight);

		/*
		 * If we're using 3d icon managers, each line item has its own
		 * icon; see comment on creation function for details.  With 2d
		 * icon managers, it's the same for all of them, so it's stored
		 * screen-wide.
		 */
		if(Scr->use3Diconmanagers) {
			if(!Scr->BeNiceToColormap) {
				GetShadeColors(&tmp->cp);
			}
			tmp->iconifypm = Create3DIconManagerIcon(tmp->cp);
		}

		/* Refigure the height of the whole IM */
		h = Scr->IconManagerFont.avg_height
		    + 2 * (ICON_MGR_OBORDER + ICON_MGR_OBORDER);
		if(h < (siconify_height + 4)) {
			h = siconify_height + 4;
		}

		ip->height = h * ip->count;
		tmp->me = ip->count;
		tmp->x = -1;
		tmp->y = -1;
		tmp->height = -1;
		tmp->width = -1;


		/* Make a window for this row in the IM */
		valuemask = (CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);
		attributes.background_pixel = tmp->cp.back;
		attributes.border_pixel = tmp->cp.back;
		attributes.event_mask = (KeyPressMask | ButtonPressMask |
		                         ButtonReleaseMask | ExposureMask);
		if(Scr->IconManagerFocus) {
			attributes.event_mask |= (EnterWindowMask | LeaveWindowMask);
		}
		attributes.cursor = Scr->IconMgrCursor;
		tmp->w = XCreateWindow(dpy, ip->w, 0, 0, (unsigned int) 1,
		                       (unsigned int) h, (unsigned int) 0,
		                       CopyFromParent, (unsigned int) CopyFromParent,
		                       (Visual *) CopyFromParent,
		                       valuemask, &attributes);


		/* Setup the icon for it too */
		valuemask = (CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);
		attributes.background_pixel = tmp->cp.back;
		attributes.border_pixel = Scr->Black;
		attributes.event_mask = (ButtonReleaseMask | ButtonPressMask
		                         | ExposureMask);
		attributes.cursor = Scr->ButtonCursor;
		/* The precise location will be set it in PackIconManager.  */
		tmp->icon = XCreateWindow(dpy, tmp->w, 0, 0,
		                          (unsigned int) siconify_width,
		                          (unsigned int) siconify_height,
		                          (unsigned int) 0, CopyFromParent,
		                          (unsigned int) CopyFromParent,
		                          (Visual *) CopyFromParent,
		                          valuemask, &attributes);


		/* Bump housekeeping for the IM */
		ip->count += 1;
		PackIconManager(ip);
		if(Scr->WindowMask) {
			XRaiseWindow(dpy, Scr->WindowMask);
		}
		XMapWindow(dpy, tmp->w);

		XSaveContext(dpy, tmp->w, TwmContext, (XPointer) tmp_win);
		XSaveContext(dpy, tmp->w, ScreenContext, (XPointer) Scr);
		XSaveContext(dpy, tmp->icon, TwmContext, (XPointer) tmp_win);
		XSaveContext(dpy, tmp->icon, ScreenContext, (XPointer) Scr);

		if(!ip->twm_win->isicon) {
			if(visible(ip->twm_win)) {
				SetMapStateProp(ip->twm_win, NormalState);
				XMapWindow(dpy, ip->w);
				XMapWindow(dpy, ip->twm_win->frame);
			}
			ip->twm_win->mapped = true;
		}


		/*
		 * Stick this entry on the head of our list of "IM entries we
		 * created", and loop around to the next WS for this IM.
		 */
		tmp->nextv = old;
		old = tmp;
		ip = ip->nextv;
	}

	/* If we didn't create at least one thing, we're done here */
	if(tmp == NULL) {
		return NULL;
	}

	/* Stash where the window is IM-listed */
	tmp_win->iconmanagerlist = tmp;

	/* ??? */
	if(! visible(tmp->iconmgr->twm_win)) {
		old = tmp;
		tmp = tmp->nextv;
		while(tmp != NULL) {
			if(visible(tmp->iconmgr->twm_win)) {
				break;
			}
			old = tmp;
			tmp = tmp->nextv;
		}
		if(tmp != NULL) {
			old->nextv = tmp->nextv;
			tmp->nextv = tmp_win->iconmanagerlist;
			tmp_win->iconmanagerlist = tmp;
		}
	}

	/* Hand back the list places we added */
	return tmp_win->iconmanagerlist;
}

/***********************************************************************
 *
 *  Procedure:
 *      InsertInIconManager - put an allocated entry into an icon
 *              manager
 *
 *  Inputs:
 *      ip      - the icon manager pointer
 *      tmp     - the entry to insert
 *
 ***********************************************************************
 */

void InsertInIconManager(IconMgr *ip, WList *tmp, TwmWindow *tmp_win)
{
	WList *tmp1;
	bool added;

	added = false;
	if(ip->first == NULL) {
		ip->first = tmp;
		tmp->prev = NULL;
		ip->last = tmp;
		added = true;
	}
	else if(Scr->SortIconMgr) {
		for(tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next) {
			int compresult;
			if(Scr->CaseSensitive) {
				compresult = strcmp(tmp_win->icon_name, tmp1->twm->icon_name);
			}
			else {
				compresult = strcasecmp(tmp_win->icon_name, tmp1->twm->icon_name);
			}
			if(compresult < 0) {
				tmp->next = tmp1;
				tmp->prev = tmp1->prev;
				tmp1->prev = tmp;
				if(tmp->prev == NULL) {
					ip->first = tmp;
				}
				else {
					tmp->prev->next = tmp;
				}
				added = true;
				break;
			}
		}
	}

	if(!added) {
		ip->last->next = tmp;
		tmp->prev = ip->last;
		ip->last = tmp;
	}
}

void RemoveFromIconManager(IconMgr *ip, WList *tmp)
{
	if(tmp->prev == NULL) {
		ip->first = tmp->next;
	}
	else {
		tmp->prev->next = tmp->next;
	}

	if(tmp->next == NULL) {
		ip->last = tmp->prev;
	}
	else {
		tmp->next->prev = tmp->prev;
	}

	/* pebl: If the list was the current and tmp was the last in the list
	   reset current list */
	if(Current == tmp) {
		Current = ip->first;
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      RemoveIconManager - remove a window from the icon manager
 *
 *  Inputs:
 *      tmp_win - the TwmWindow structure
 *
 ***********************************************************************
 */

void RemoveIconManager(TwmWindow *tmp_win)
{
	IconMgr *ip;
	WList *tmp, *tmp1, *save;

	if(tmp_win->iconmanagerlist == NULL) {
		return;
	}

	tmp  = tmp_win->iconmanagerlist;
	tmp1 = NULL;

	while(tmp != NULL) {
		ip = tmp->iconmgr;
		if((tmp_win->occupation & ip->twm_win->occupation) != 0) {
			tmp1 = tmp;
			tmp  = tmp->nextv;
			continue;
		}
		RemoveFromIconManager(ip, tmp);

		XDeleteContext(dpy, tmp->icon, TwmContext);
		XDeleteContext(dpy, tmp->icon, ScreenContext);
		XDestroyWindow(dpy, tmp->icon);
		XDeleteContext(dpy, tmp->w, TwmContext);
		XDeleteContext(dpy, tmp->w, ScreenContext);
		XDestroyWindow(dpy, tmp->w);
		ip->count -= 1;

		PackIconManager(ip);

		if(ip->count == 0) {
			XUnmapWindow(dpy, ip->twm_win->frame);
			ip->twm_win->mapped = false;
		}
		if(tmp1 == NULL) {
			tmp_win->iconmanagerlist = tmp_win->iconmanagerlist->nextv;
		}
		else {
			tmp1->nextv = tmp->nextv;
		}

		save = tmp;
		tmp = tmp->nextv;
		free(save);
	}
}

void CurrentIconManagerEntry(WList *current)
{
	Current = current;
}

void ActiveIconManager(WList *active)
{
	active->active = true;
	Active = active;
	Active->iconmgr->active = active;
	Current = Active;
	DrawIconManagerBorder(active, false);
}

void NotActiveIconManager(WList *active)
{
	active->active = false;
	DrawIconManagerBorder(active, false);
}

void DrawIconManagerBorder(WList *tmp, bool fill)
{
	if(Scr->use3Diconmanagers) {
		Draw3DBorder(tmp->w, 0, 0, tmp->width, tmp->height,
		             Scr->IconManagerShadowDepth, tmp->cp,
		             (tmp->active && Scr->Highlight ? on : off),
		             fill, false);
	}
	else {
		XSetForeground(dpy, Scr->NormalGC, tmp->cp.fore);
		XDrawRectangle(dpy, tmp->w, Scr->NormalGC, 2, 2, tmp->width - 5,
		               tmp->height - 5);

		XSetForeground(dpy, Scr->NormalGC,
		               (tmp->active && Scr->Highlight
		                ? tmp->highlight : tmp->cp.back));

		XDrawRectangle(dpy, tmp->w, Scr->NormalGC, 0, 0, tmp->width - 1,
		               tmp->height - 1);
		XDrawRectangle(dpy, tmp->w, Scr->NormalGC, 1, 1, tmp->width - 3,
		               tmp->height - 3);
	}
}

/***********************************************************************
 *
 *  Procedure:
 *      SortIconManager - sort the dude
 *
 *  Inputs:
 *      ip      - a pointer to the icon manager struture
 *
 ***********************************************************************
 */

void SortIconManager(IconMgr *ip)
{
	WList *tmp1, *tmp2;
	int done;

	if(ip == NULL) {
		ip = Active->iconmgr;
	}

	done = false;
	do {
		for(tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next) {
			int compresult;
			if((tmp2 = tmp1->next) == NULL) {
				done = true;
				break;
			}
			if(Scr->CaseSensitive) {
				compresult = strcmp(tmp1->twm->icon_name, tmp2->twm->icon_name);
			}
			else {
				compresult = strcasecmp(tmp1->twm->icon_name, tmp2->twm->icon_name);
			}
			if(compresult > 0) {
				/* take it out and put it back in */
				RemoveFromIconManager(ip, tmp2);
				InsertInIconManager(ip, tmp2, tmp2->twm);
				break;
			}
		}
	}
	while(!done);
	PackIconManager(ip);
}

/***********************************************************************
 *
 *  Procedure:
 *      PackIconManager - pack the icon manager windows following
 *              an addition or deletion
 *
 *  Inputs:
 *      ip      - a pointer to the icon manager struture
 *
 ***********************************************************************
 */

void PackIconManagers(void)
{
	TwmWindow *twm_win;

	for(twm_win = Scr->FirstWindow; twm_win != NULL; twm_win = twm_win->next) {
		if(twm_win->iconmgrp) {
			PackIconManager(twm_win->iconmgrp);
		}
	}
}

void PackIconManager(IconMgr *ip)
{
	int newwidth, i, row, col, maxcol,  colinc, rowinc, wheight, wwidth;
	int new_x, new_y;
	int savewidth;
	WList *tmp;
	int mask;

	wheight = Scr->IconManagerFont.avg_height
	          + 2 * (ICON_MGR_OBORDER + ICON_MGR_IBORDER);
	if(wheight < (siconify_height + 4)) {
		wheight = siconify_height + 4;
	}

	wwidth = ip->width / ip->columns;

	rowinc = wheight;
	colinc = wwidth;

	row = 0;
	col = ip->columns;
	maxcol = 0;
	for(i = 0, tmp = ip->first; tmp != NULL; i++, tmp = tmp->next) {
		tmp->me = i;
		if(++col >= ip->columns) {
			col = 0;
			row += 1;
		}
		if(col > maxcol) {
			maxcol = col;
		}

		new_x = col * colinc;
		new_y = (row - 1) * rowinc;

		/* if the position or size has not changed, don't touch it */
		if(tmp->x != new_x || tmp->y != new_y ||
		                tmp->width != wwidth || tmp->height != wheight) {
			XMoveResizeWindow(dpy, tmp->w, new_x, new_y, wwidth, wheight);
			if(tmp->height != wheight)
				XMoveWindow(dpy, tmp->icon, ICON_MGR_OBORDER + ICON_MGR_IBORDER,
				            (wheight - siconify_height) / 2);

			tmp->row = row - 1;
			tmp->col = col;
			tmp->x = new_x;
			tmp->y = new_y;
			tmp->width = wwidth;
			tmp->height = wheight;
		}
	}
	maxcol += 1;

	ip->cur_rows = row;
	ip->cur_columns = maxcol;
	ip->height = row * rowinc;
	if(ip->height == 0) {
		ip->height = rowinc;
	}
	newwidth = maxcol * colinc;
	if(newwidth == 0) {
		newwidth = colinc;
	}

	XResizeWindow(dpy, ip->w, newwidth, ip->height);

	mask = XParseGeometry(ip->geometry, &JunkX, &JunkY,
	                      &JunkWidth, &JunkHeight);
	if(mask & XNegative) {
		ip->twm_win->frame_x += ip->twm_win->frame_width - newwidth -
		                        2 * ip->twm_win->frame_bw3D;
	}
	if(mask & YNegative) {
		ip->twm_win->frame_y += ip->twm_win->frame_height - ip->height -
		                        2 * ip->twm_win->frame_bw3D - ip->twm_win->title_height;
	}
	savewidth = ip->width;
	if(ip->twm_win)
		SetupWindow(ip->twm_win,
		            ip->twm_win->frame_x, ip->twm_win->frame_y,
		            newwidth + 2 * ip->twm_win->frame_bw3D,
		            ip->height + ip->twm_win->title_height + 2 * ip->twm_win->frame_bw3D, -1);
	ip->width = savewidth;
}

void dump_iconmanager(IconMgr *mgr, char *label)
{
	fprintf(stderr, "IconMgr %s %p name='%s' geom='%s'\n",
	        label,
	        mgr,
	        mgr->name,
	        mgr->geometry);
	fprintf(stderr, "next = %p, prev = %p, lasti = %p, nextv = %p\n",
	        mgr->next,
	        mgr->prev,
	        mgr->lasti,
	        mgr->nextv);
}
