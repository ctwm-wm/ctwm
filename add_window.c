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

/**********************************************************************
 *
 * $XConsortium: add_window.c,v 1.153 91/07/10 13:17:26 dave Exp $
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 * 31-Mar-88 Tom LaStrange        Initial Version.
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 **********************************************************************/

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include "add_window.h"
#include "colormaps.h"
#include "windowbox.h"
#include "util.h"
#include "otp.h"
#include "resize.h"
#include "parse.h"
#include "list.h"
#include "events.h"
#include "menus.h"
#include "screen.h"
#include "icons.h"
#include "iconmgr.h"
#include "session.h"
#include "mwmhints.h"
#include "image.h"
#include "functions.h"  // Only for RootFunction
#include "decorations.h"
#include "captive.h"
#include "win_regions.h"
#include "win_utils.h"
#include "workspace_manager.h"

#include "gram.tab.h"


int AddingX;
int AddingY;
unsigned int AddingW;
unsigned int AddingH;

static int PlaceX = -1;
static int PlaceY = -1;
static void DealWithNonSensicalGeometries(Display *dpy, Window vroot,
                TwmWindow *tmp_win);
static bool Transient(Window w, Window *propw);

char NoName[] = "Untitled"; /* name if no name is specified */
bool resizeWhenAdd;



/***********************************************************************
 *
 *  Procedure:
 *      AddWindow - add a new window to the twm list
 *
 *  Returned Value:
 *      (TwmWindow *) - pointer to the TwmWindow structure
 *
 *  Inputs:
 *      w       - the window id of the window to add
 *      iconm   - flag to tell if this is an icon manager window
 *              0 --> normal window.
 *              1 --> icon manager.
 *              2 --> window box;
 *              else --> iconmgr;
 *
 *      iconp   - pointer to icon manager struct
 *
 ***********************************************************************
 */

TwmWindow *
AddWindow(Window w, AWType wtype, IconMgr *iconp, VirtualScreen *vs)
{
	TwmWindow *tmp_win;                 /* new twm window structure */
	int stat;
	XEvent event;
	unsigned long valuemask;            /* mask for create windows */
	XSetWindowAttributes attributes;    /* attributes for create windows */
	int width, height;                  /* tmp variable */
	bool ask_user;               /* don't know where to put the window */
	int gravx, gravy;                   /* gravity signs for positioning */
	int namelen;
	int bw2;
	short saved_x, saved_y, restore_icon_x, restore_icon_y;
	unsigned short saved_width, saved_height;
	bool restore_iconified = false;
	bool restore_icon_info_present = false;
	int restoredFromPrevSession;
	bool width_ever_changed_by_user;
	bool height_ever_changed_by_user;
	int saved_occupation; /* <== [ Matthew McNeill Feb 1997 ] == */
	bool random_placed = false;
	fd_set      mask;
	int         fd;
	struct timeval timeout;
	XRectangle ink_rect;
	XRectangle logical_rect;
	WindowBox *winbox;
	Window vroot;

#ifdef DEBUG
	fprintf(stderr, "AddWindow: w = 0x%x\n", w);
#endif

	if(!CLarg.is_captive && RedirectToCaptive(w)) {
		/* XXX x-ref comment by SetNoRedirect() */
		return (NULL);
	}

	/* allocate space for the twm window */
	tmp_win = calloc(1, sizeof(TwmWindow));
	if(tmp_win == 0) {
		fprintf(stderr, "%s: Unable to allocate memory to manage window ID %lx.\n",
		        ProgramName, w);
		return NULL;
	}

	tmp_win->w = w;
	tmp_win->zoomed = ZOOM_NONE;
	tmp_win->isiconmgr = (wtype == AWT_ICON_MANAGER);
	tmp_win->iconmgrp = iconp;
	tmp_win->iswspmgr = (wtype == AWT_WORKSPACE_MANAGER);
	tmp_win->isoccupy = (wtype == AWT_OCCUPY);
	tmp_win->iswinbox = (wtype == AWT_WINDOWBOX);
	tmp_win->vs = vs;
	tmp_win->parent_vs = vs;
	tmp_win->savevs = NULL;
	tmp_win->cmaps.number_cwins = 0;
	tmp_win->savegeometry.width = -1;

	XSelectInput(dpy, tmp_win->w, PropertyChangeMask);
	XGetWindowAttributes(dpy, tmp_win->w, &tmp_win->attr);
	tmp_win->name = (char *) GetWMPropertyString(tmp_win->w, XA_WM_NAME);
	tmp_win->class = NoClass;
	XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
	FetchWmProtocols(tmp_win);
	FetchWmColormapWindows(tmp_win);

	if(GetWindowConfig(tmp_win,
	                   &saved_x, &saved_y, &saved_width, &saved_height,
	                   &restore_iconified, &restore_icon_info_present,
	                   &restore_icon_x, &restore_icon_y,
	                   &width_ever_changed_by_user, &height_ever_changed_by_user,
	                   &saved_occupation)) { /* <== [ Matthew McNeill Feb 1997 ] == */
		tmp_win->attr.x = saved_x;
		tmp_win->attr.y = saved_y;

		tmp_win->widthEverChangedByUser = width_ever_changed_by_user;
		tmp_win->heightEverChangedByUser = height_ever_changed_by_user;

		if(width_ever_changed_by_user) {
			tmp_win->attr.width = saved_width;
		}

		if(height_ever_changed_by_user) {
			tmp_win->attr.height = saved_height;
		}

		restoredFromPrevSession = 1;
	}
	else {
		tmp_win->widthEverChangedByUser = false;
		tmp_win->heightEverChangedByUser = false;

		restoredFromPrevSession = 0;
	}

	/*
	 * do initial clip; should look at window gravity
	 */
	if(tmp_win->attr.width > Scr->MaxWindowWidth) {
		tmp_win->attr.width = Scr->MaxWindowWidth;
	}
	if(tmp_win->attr.height > Scr->MaxWindowHeight) {
		tmp_win->attr.height = Scr->MaxWindowHeight;
	}

	tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

	if(tmp_win->wmhints) {
		if(restore_iconified) {
			tmp_win->wmhints->initial_state = IconicState;
			tmp_win->wmhints->flags |= StateHint;
		}

		if(restore_icon_info_present) {
			tmp_win->wmhints->icon_x = restore_icon_x;
			tmp_win->wmhints->icon_y = restore_icon_y;
			tmp_win->wmhints->flags |= IconPositionHint;
		}
	}

	if(tmp_win->wmhints) {
		tmp_win->wmhints->input = True;
	}
	/* CL: Having with not willing focus
	cause problems with AutoSqueeze and a few others
	things. So I suppress it. And the whole focus thing
	is buggy anyway */
	if(tmp_win->wmhints && !(tmp_win->wmhints->flags & InputHint)) {
		tmp_win->wmhints->input = True;
	}
	if(tmp_win->wmhints && (tmp_win->wmhints->flags & WindowGroupHint)) {
		tmp_win->group = tmp_win->wmhints->window_group;
		if(tmp_win->group) {
			/*
			 * GTK windows often have a spurious "group leader" window which is
			 * never reported to us and therefore does not really exist.  This
			 * is annoying because we treat group members a lot like transient
			 * windows.  Look for that here. It is in fact a duplicate of the
			 * WM_CLIENT_LEADER property.
			 */
			if(tmp_win->group != w && !GetTwmWindow(tmp_win->group)) {
				tmp_win->group = 0;
			}
		}
	}
	else {
		tmp_win->group = 0;
	}

	/*
	 * The July 27, 1988 draft of the ICCCM ignores the size and position
	 * fields in the WM_NORMAL_HINTS property.
	 */

	tmp_win->istransient = Transient(tmp_win->w, &tmp_win->transientfor);

	tmp_win->nameChanged = false;
	if(tmp_win->name == NULL) {
		tmp_win->name = NoName;
	}
	if(tmp_win->class.res_name == NULL) {
		tmp_win->class.res_name = NoName;
	}
	if(tmp_win->class.res_class == NULL) {
		tmp_win->class.res_class = NoName;
	}

	/*
	 * full_name seems to exist only because in the conditional code below,
	 * name is sometimes changed. In all other cases, name and full_name
	 * seem to be identical. Is that worth it?
	 */
	tmp_win->full_name = tmp_win->name;
#ifdef CLAUDE
	if(strstr(tmp_win->name, " - Mozilla")) {
		char *moz = strstr(tmp_win->name, " - Mozilla");
		*moz = '\0';
	}
#endif
	namelen = strlen(tmp_win->name);

	if(LookInList(Scr->IgnoreTransientL, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->istransient = false;
	}

	tmp_win->highlight = Scr->Highlight &&
	                     (!LookInList(Scr->NoHighlight, tmp_win->full_name,
	                                  &tmp_win->class));

	tmp_win->stackmode = Scr->StackMode &&
	                     (!LookInList(Scr->NoStackModeL, tmp_win->full_name,
	                                  &tmp_win->class));

	tmp_win->titlehighlight = Scr->TitleHighlight &&
	                          (!LookInList(Scr->NoTitleHighlight, tmp_win->full_name,
	                                       &tmp_win->class));

	tmp_win->auto_raise = Scr->AutoRaiseDefault ||
	                      LookInList(Scr->AutoRaise, tmp_win->full_name,
	                                 &tmp_win->class);
	if(tmp_win->auto_raise) {
		Scr->NumAutoRaises++;
	}

	tmp_win->auto_lower = Scr->AutoLowerDefault ||
	                      LookInList(Scr->AutoLower, tmp_win->full_name,
	                                 &tmp_win->class);
	if(tmp_win->auto_lower) {
		Scr->NumAutoLowers++;
	}

	tmp_win->iconify_by_unmapping = Scr->IconifyByUnmapping;
	if(Scr->IconifyByUnmapping) {
		tmp_win->iconify_by_unmapping = tmp_win->isiconmgr ? false :
		                                !LookInList(Scr->DontIconify, tmp_win->full_name,
		                                                &tmp_win->class);
	}
	tmp_win->iconify_by_unmapping = tmp_win->iconify_by_unmapping ||
	                                LookInList(Scr->IconifyByUn, tmp_win->full_name, &tmp_win->class);

	if(LookInList(Scr->UnmapByMovingFarAway, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->UnmapByMovingFarAway = true;
	}
	else {
		tmp_win->UnmapByMovingFarAway = false;
	}

	if(LookInList(Scr->DontSetInactive, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->DontSetInactive = true;
	}
	else {
		tmp_win->DontSetInactive = false;
	}

#ifdef EWMH
	EwmhGetProperties(tmp_win);
#endif /* EWMH */

	if(LookInList(Scr->AutoSqueeze, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->AutoSqueeze = true;
	}
	else {
		tmp_win->AutoSqueeze = false;
	}

	if(
#ifdef EWMH
	        (tmp_win->ewmhFlags & EWMH_STATE_SHADED) ||
#endif /* EWMH */
	        LookInList(Scr->StartSqueezed, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->StartSqueezed = true;
	}
	else {
		tmp_win->StartSqueezed = false;
	}

	if(Scr->AlwaysSqueezeToGravity
	                || LookInList(Scr->AlwaysSqueezeToGravityL, tmp_win->full_name,
	                              &tmp_win->class)) {
		tmp_win->AlwaysSqueezeToGravity = true;
	}
	else {
		tmp_win->AlwaysSqueezeToGravity = false;
	}

	if(tmp_win->istransient || tmp_win->group) {
		TwmWindow *t = NULL;
		if(tmp_win->istransient) {
			t = GetTwmWindow(tmp_win->transientfor);
		}
		if(!t && tmp_win->group) {
			t = GetTwmWindow(tmp_win->group);
		}
		if(t) {
			tmp_win->UnmapByMovingFarAway = t->UnmapByMovingFarAway;
		}
	}
	if((Scr->WindowRingAll && !tmp_win->iswspmgr && !tmp_win->isiconmgr &&
#ifdef EWMH
	                EwmhOnWindowRing(tmp_win) &&
#endif /* EWMH */
	                !LookInList(Scr->WindowRingExcludeL, tmp_win->full_name, &tmp_win->class)) ||
	                LookInList(Scr->WindowRingL, tmp_win->full_name, &tmp_win->class)) {
		if(Scr->Ring) {
			tmp_win->ring.next = Scr->Ring->ring.next;
			if(Scr->Ring->ring.next->ring.prev) {
				Scr->Ring->ring.next->ring.prev = tmp_win;
			}
			Scr->Ring->ring.next = tmp_win;
			tmp_win->ring.prev = Scr->Ring;
		}
		else {
			tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
		}
	}
	else {
		tmp_win->ring.next = tmp_win->ring.prev = NULL;
	}
	tmp_win->ring.cursor_valid = false;

	tmp_win->squeeze_info = NULL;
	tmp_win->squeeze_info_copied = 0;
	/*
	 * get the squeeze information; note that this does not have to be freed
	 * since it is coming from the screen list
	 */
	if(HasShape) {
		if(!LookInList(Scr->DontSqueezeTitleL, tmp_win->full_name,
		                &tmp_win->class)) {
			tmp_win->squeeze_info = (SqueezeInfo *)
			                        LookInList(Scr->SqueezeTitleL, tmp_win->full_name,
			                                   &tmp_win->class);
			if(!tmp_win->squeeze_info) {
				static SqueezeInfo default_squeeze = { SIJ_LEFT, 0, 0 };
				if(Scr->SqueezeTitle) {
					tmp_win->squeeze_info = &default_squeeze;
				}
			}
		}
	}

	tmp_win->old_bw = tmp_win->attr.border_width;

	{
		MotifWmHints mwmHints;
		bool have_title;

		GetMWMHints(tmp_win->w, &mwmHints);

		tmp_win->frame_bw3D = Scr->ThreeDBorderWidth;
		if(
#ifdef EWMH
		        !EwmhHasBorder(tmp_win) ||
#endif /* EWMH */
		        (mwm_has_border(&mwmHints) == 0) ||
		        LookInList(Scr->NoBorder, tmp_win->full_name, &tmp_win->class)) {
			tmp_win->frame_bw = 0;
			tmp_win->frame_bw3D = 0;
		}
		else if(tmp_win->frame_bw3D != 0) {
			tmp_win->frame_bw = 0;
		}
		else if(Scr->ClientBorderWidth) {
			tmp_win->frame_bw = tmp_win->old_bw;
		}
		else {
			tmp_win->frame_bw = Scr->BorderWidth;
		}
		bw2 = tmp_win->frame_bw * 2;


		have_title = true;
#ifdef EWMH
		have_title = EwmhHasTitle(tmp_win);
#endif /* EWMH */
		if(mwm_sets_title(&mwmHints)) {
			have_title = mwm_has_title(&mwmHints);
		}
		if(Scr->NoTitlebar) {
			have_title = false;
		}
		if(LookInList(Scr->MakeTitle, tmp_win->full_name, &tmp_win->class)) {
			have_title = true;
		}
		if(LookInList(Scr->NoTitle, tmp_win->full_name, &tmp_win->class)) {
			have_title = false;
		}

		if(have_title) {
			tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;
		}
		else {
			tmp_win->title_height = 0;
		}
	}

	tmp_win->OpaqueMove = Scr->DoOpaqueMove;
	if(LookInList(Scr->OpaqueMoveList, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->OpaqueMove = true;
	}
	else if(LookInList(Scr->NoOpaqueMoveList, tmp_win->full_name,
	                   &tmp_win->class)) {
		tmp_win->OpaqueMove = false;
	}

	tmp_win->OpaqueResize = Scr->DoOpaqueResize;
	if(LookInList(Scr->OpaqueResizeList, tmp_win->full_name, &tmp_win->class)) {
		tmp_win->OpaqueResize = true;
	}
	else if(LookInList(Scr->NoOpaqueResizeList, tmp_win->full_name,
	                   &tmp_win->class)) {
		tmp_win->OpaqueResize = false;
	}

	/* if it is a transient window, don't put a title on it */
	if(tmp_win->istransient && !Scr->DecorateTransients) {
		tmp_win->title_height = 0;
	}

	if(LookInList(Scr->StartIconified, tmp_win->full_name, &tmp_win->class)) {
		if(!tmp_win->wmhints) {
			tmp_win->wmhints = malloc(sizeof(XWMHints));
			tmp_win->wmhints->flags = 0;
		}
		tmp_win->wmhints->initial_state = IconicState;
		tmp_win->wmhints->flags |= StateHint;
	}

	GetWindowSizeHints(tmp_win);

	if(restoredFromPrevSession) {
		/*
		 * When restoring window positions from the previous session,
		 * we always use NorthWest gravity.
		 */

		gravx = gravy = -1;
	}
	else {
		GetGravityOffsets(tmp_win, &gravx, &gravy);
	}

	/*
	 * Don't bother user if:
	 *
	 *     o  the window is a transient, or
	 *
	 *     o  a USPosition was requested, or
	 *
	 *     o  a PPosition was requested and UsePPosition is ON or
	 *        NON_ZERO if the window is at other than (0,0)
	 */
	ask_user = true;
	if(tmp_win->istransient ||
	                (tmp_win->hints.flags & USPosition) ||
	                ((tmp_win->hints.flags & PPosition) && Scr->UsePPosition &&
	                 (Scr->UsePPosition == PPOS_ON ||
	                  tmp_win->attr.x != 0 || tmp_win->attr.y != 0))) {
		ask_user = false;
	}

	/*===============[ Matthew McNeill 1997 ]==========================*
	 * added the occupation parameter to this function call so that the
	 * occupation can be set up in a specific state if desired
	 * (restore session for example)
	 */

	/*
	 * Note, this may update tmp_win->{parent_,}vs if needed to make the
	 * window visible in another vscreen.
	 * It may set tmp_win->vs to NULL if it has no occupation in the
	 * current workspace.
	 */

	if(restoredFromPrevSession) {
		SetupOccupation(tmp_win, saved_occupation);
	}
	else {
		SetupOccupation(tmp_win, 0);
	}
	/*=================================================================*/

	tmp_win->frame_width  = tmp_win->attr.width  + 2 * tmp_win->frame_bw3D;
	tmp_win->frame_height = tmp_win->attr.height + 2 * tmp_win->frame_bw3D +
	                        tmp_win->title_height;
	ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);
	winbox = findWindowBox(tmp_win);
	if(PlaceWindowInRegion(tmp_win, &(tmp_win->attr.x), &(tmp_win->attr.y))) {
		ask_user = false;
	}
	if(LookInList(Scr->WindowGeometries, tmp_win->full_name, &tmp_win->class)) {
		char *geom;
		int mask_;
		geom = LookInList(Scr->WindowGeometries, tmp_win->full_name, &tmp_win->class);
		mask_ = XParseGeometry(geom, &tmp_win->attr.x, &tmp_win->attr.y,
		                       (unsigned int *) &tmp_win->attr.width,
		                       (unsigned int *) &tmp_win->attr.height);

		if(mask_ & XNegative) {
			tmp_win->attr.x += Scr->rootw - tmp_win->attr.width;
		}
		if(mask_ & YNegative) {
			tmp_win->attr.y += Scr->rooth - tmp_win->attr.height;
		}
		ask_user = false;
	}

	vs = tmp_win->parent_vs;
	if(vs) {
		vroot = vs->window;
	}
	else {
		vroot = Scr->Root;      /* never */
		tmp_win->parent_vs = Scr->currentvs;
	}
	if(winbox) {
		vroot = winbox->window;
	}

	/*
	 * do any prompting for position
	 */

	if(HandlingEvents && ask_user && !restoredFromPrevSession) {
		if((Scr->RandomPlacement == RP_ALL) ||
		                ((Scr->RandomPlacement == RP_UNMAPPED) &&
		                 ((tmp_win->wmhints && (tmp_win->wmhints->initial_state == IconicState)) ||
		                  (! visible(tmp_win))))) {  /* just stick it somewhere */

#ifdef DEBUG
			fprintf(stderr,
			        "DEBUG[RandomPlacement]: win: %dx%d+%d+%d, screen: %dx%d, title height: %d, random: +%d+%d\n",
			        tmp_win->attr.width, tmp_win->attr.height,
			        tmp_win->attr.x, tmp_win->attr.y,
			        Scr->rootw, Scr->rooth,
			        tmp_win->title_height,
			        PlaceX, PlaceY);
#endif

			/* Initiallise PlaceX and PlaceY */
			if(PlaceX < 0 && PlaceY < 0) {
				if(Scr->RandomDisplacementX >= 0) {
					PlaceX = Scr->BorderLeft + 5;
				}
				else {
					PlaceX = Scr->rootw - tmp_win->attr.width - Scr->BorderRight - 5;
				}
				if(Scr->RandomDisplacementY >= 0) {
					PlaceY = Scr->BorderTop + 5;
				}
				else
					PlaceY = Scr->rooth - tmp_win->attr.height - tmp_win->title_height
					         - Scr->BorderBottom - 5;
			}

			/* For a positive horizontal displacement, if the right edge
			   of the window would fall outside of the screen, start over
			   by placing the left edge of the window 5 pixels inside
			   the left edge of the screen.*/
			if(Scr->RandomDisplacementX >= 0
			                && (PlaceX + tmp_win->attr.width
			                    > Scr->rootw - Scr->BorderRight - 5)) {
				PlaceX = Scr->BorderLeft + 5;
			}

			/* For a negative horizontal displacement, if the left edge
			   of the window would fall outside of the screen, start over
			   by placing the right edge of the window 5 pixels inside
			   the right edge of the screen.*/
			if(Scr->RandomDisplacementX < 0 && PlaceX < Scr->BorderLeft + 5) {
				PlaceX = Scr->rootw - tmp_win->attr.width - Scr->BorderRight - 5;
			}

			/* For a positive vertical displacement, if the bottom edge
			   of the window would fall outside of the screen, start over
			   by placing the top edge of the window 5 pixels inside the
			   top edge of the screen.  Because we add the title height
			   further down, we need to count with it here as well.  */
			if(Scr->RandomDisplacementY >= 0
			                && (PlaceY + tmp_win->attr.height + tmp_win->title_height
			                    > Scr->rooth - Scr->BorderBottom - 5)) {
				PlaceY = Scr->BorderTop + 5;
			}

			/* For a negative vertical displacement, if the top edge of
			   the window would fall outside of the screen, start over by
			   placing the bottom edge of the window 5 pixels inside the
			   bottom edge of the screen.  Because we add the title height
			   further down, we need to count with it here as well.  */
			if(Scr->RandomDisplacementY < 0 && PlaceY < Scr->BorderTop + 5)
				PlaceY = Scr->rooth - tmp_win->attr.height - tmp_win->title_height
				         - Scr->BorderBottom - 5;

			/* Assign the current random placement to the new window, as
			   a preliminary measure.  Add the title height so things will
			   look right.  */
			tmp_win->attr.x = PlaceX;
			tmp_win->attr.y = PlaceY + tmp_win->title_height;

			/* If the window is not supposed to move off the screen, check
			   that it's still within the screen, and if not, attempt to
			   correct the situation. */
			if(Scr->DontMoveOff) {
				int available;

#ifdef DEBUG
				fprintf(stderr,
				        "DEBUG[DontMoveOff]: win: %dx%d+%d+%d, screen: %dx%d, bw2: %d, bw3D: %d\n",
				        tmp_win->attr.width, tmp_win->attr.height,
				        tmp_win->attr.x, tmp_win->attr.y,
				        Scr->rootw, Scr->rooth,
				        bw2, tmp_win->frame_bw3D);
#endif

				/* If the right edge of the window is outside the right edge
				   of the screen, we need to move the window left.  Note that
				   this can only happen with windows that are less than 50
				   pixels less wide than the screen. */
				if((tmp_win->attr.x + tmp_win->attr.width)  > Scr->rootw) {
					available = Scr->rootw - tmp_win->attr.width
					            - 2 * (bw2 + tmp_win->frame_bw3D);

#ifdef DEBUG
					fprintf(stderr, "DEBUG[DontMoveOff]: availableX: %d\n",
					        available);
#endif

					/* If the window is wider than the screen or exactly the width
					 of the screen, the availability is exactly 0.  The result
					 will be to have the window placed as much to the left as
					 possible. */
					if(available <= 0) {
						available = 0;
					}

					/* Place the window exactly between the left and right edge of
					 the screen when possible.  If available was originally less
					 than zero, it means the window's left edge will be against
					 the screen's left edge, and the window's right edge will be
					 outside the screen.  */
					tmp_win->attr.x = available / 2;
				}

				/* If the bottom edge of the window is outside the bottom edge
				   of the screen, we need to move the window up.  Note that
				   this can only happen with windows that are less than 50
				   pixels less tall than the screen.  Don't forget to count
				   with the title height and the frame widths.  */
				if((tmp_win->attr.y + tmp_win->attr.height)  > Scr->rooth) {
					available = Scr->rooth - tmp_win->attr.height
					            - tmp_win->title_height - 2 * (bw2 + tmp_win->frame_bw3D);

#ifdef DEBUG
					fprintf(stderr, "DEBUG[DontMoveOff]: availableY: %d\n",
					        available);
#endif

					/* If the window is taller than the screen or exactly the
					 height of the screen, the availability is exactly 0.
					 The result will be to have the window placed as much to
					 the top as possible. */
					if(available <= 0) {
						available = 0;
					}

					/* Place the window exactly between the top and bottom edge of
					 the screen when possible.  If available was originally less
					 than zero, it means the window's top edge will be against
					 the screen's top edge, and the window's bottom edge will be
					 outside the screen.  Again, don't forget to add the title
					 height.  */
					tmp_win->attr.y = available / 2 + tmp_win->title_height;
				}

#ifdef DEBUG
				fprintf(stderr,
				        "DEBUG[DontMoveOff]: win: %dx%d+%d+%d, screen: %dx%d\n",
				        tmp_win->attr.width, tmp_win->attr.height,
				        tmp_win->attr.x, tmp_win->attr.y,
				        Scr->rootw, Scr->rooth);
#endif
			}

			/* We know that if the window's left edge has moved compared to
			   PlaceX, it will have moved to the left.  If it was moved less
			   than 15 pixel either way, change the next "random position"
			   30 pixels down and right. */
			if(PlaceX - tmp_win->attr.x < 15
			                || PlaceY - (tmp_win->attr.y - tmp_win->title_height) < 15) {
				PlaceX += Scr->RandomDisplacementX;
				PlaceY += Scr->RandomDisplacementY;
			}

			random_placed = true;
		}
		else {                            /* else prompt */
			if(!(tmp_win->wmhints && tmp_win->wmhints->flags & StateHint &&
			                tmp_win->wmhints->initial_state == IconicState)) {
				bool firsttime = true;
				int found = 0;

				/* better wait until all the mouse buttons have been
				 * released.
				 */
				while(1) {
					unsigned int qpmask;
					Window qproot;

					XUngrabServer(dpy);
					XSync(dpy, 0);
					XGrabServer(dpy);

					qpmask = 0;
					if(!XQueryPointer(dpy, Scr->Root, &qproot,
					                  &JunkChild, &JunkX, &JunkY,
					                  &AddingX, &AddingY, &qpmask)) {
						qpmask = 0;
					}

					/* Clear out any but the Button bits */
					qpmask &= (Button1Mask | Button2Mask | Button3Mask |
					           Button4Mask | Button5Mask);

					/*
					 * watch out for changing screens
					 */
					if(firsttime) {
						if(qproot != Scr->Root) {
							int scrnum;
							for(scrnum = 0; scrnum < NumScreens; scrnum++) {
								if(qproot == RootWindow(dpy, scrnum)) {
									break;
								}
							}
							if(scrnum != NumScreens) {
								PreviousScreen = scrnum;
							}
						}
						if(Scr->currentvs) {
							vroot = Scr->currentvs->window;
						}
						firsttime = false;
					}
					if(winbox) {
						vroot = winbox->window;
					}

					/*
					 * wait for buttons to come up; yuck
					 */
					if(qpmask != 0) {
						continue;
					}

					/*
					 * this will cause a warp to the indicated root
					 */
					stat = XGrabPointer(dpy, vroot, False,
					                    ButtonPressMask | ButtonReleaseMask |
					                    PointerMotionMask | PointerMotionHintMask,
					                    GrabModeAsync, GrabModeAsync,
					                    vroot, UpperLeftCursor, CurrentTime);
					if(stat == GrabSuccess) {
						break;
					}
				}

				XmbTextExtents(Scr->SizeFont.font_set,
				               tmp_win->name, namelen,
				               &ink_rect, &logical_rect);
				width = SIZE_HINDENT + ink_rect.width;
				height = logical_rect.height + SIZE_VINDENT * 2;
				XmbTextExtents(Scr->SizeFont.font_set,
				               ": ", 2,  &logical_rect, &logical_rect);
				Scr->SizeStringOffset = width + logical_rect.width;

				XResizeWindow(dpy, Scr->SizeWindow, Scr->SizeStringOffset +
				              Scr->SizeStringWidth + SIZE_HINDENT, height);
				XMapRaised(dpy, Scr->SizeWindow);
				InstallRootColormap();
				FB(Scr->DefaultC.fore, Scr->DefaultC.back);
				XmbDrawImageString(dpy, Scr->SizeWindow, Scr->SizeFont.font_set,
				                   Scr->NormalGC, SIZE_HINDENT,
				                   SIZE_VINDENT + Scr->SizeFont.ascent,
				                   tmp_win->name, namelen);

				if(winbox) {
					ConstrainedToWinBox(tmp_win, AddingX, AddingY, &AddingX, &AddingY);
				}

				AddingW = tmp_win->attr.width + bw2 + 2 * tmp_win->frame_bw3D;
				AddingH = tmp_win->attr.height + tmp_win->title_height +
				          bw2 + 2 * tmp_win->frame_bw3D;
				MoveOutline(vroot, AddingX, AddingY, AddingW, AddingH,
				            tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

				XmbDrawImageString(dpy, Scr->SizeWindow, Scr->SizeFont.font_set,
				                   Scr->NormalGC, width,
				                   SIZE_VINDENT + Scr->SizeFont.ascent, ": ", 2);
				DisplayPosition(tmp_win, AddingX, AddingY);

				tmp_win->frame_width  = AddingW;
				tmp_win->frame_height = AddingH;
				/*SetFocus (NULL, CurrentTime);*/
				while(1) {
					if(Scr->OpenWindowTimeout) {
						fd = ConnectionNumber(dpy);
						while(!XCheckMaskEvent(dpy, ButtonMotionMask | ButtonPressMask, &event)) {
							FD_ZERO(&mask);
							FD_SET(fd, &mask);
							timeout.tv_sec  = Scr->OpenWindowTimeout;
							timeout.tv_usec = 0;
							found = select(fd + 1, &mask, NULL, NULL, &timeout);
							if(found == 0) {
								break;
							}
						}
						if(found == 0) {
							break;
						}
					}
					else {
						found = 1;
						XMaskEvent(dpy, ButtonPressMask | PointerMotionMask, &event);
					}
					if(event.type == MotionNotify) {
						/* discard any extra motion events before a release */
						while(XCheckMaskEvent(dpy,
						                      ButtonMotionMask | ButtonPressMask, &event))
							if(event.type == ButtonPress) {
								break;
							}
					}
					FixRootEvent(&event);
					if(event.type == ButtonPress) {
						AddingX = event.xbutton.x_root;
						AddingY = event.xbutton.y_root;

						TryToGrid(tmp_win, &AddingX, &AddingY);
						if(Scr->PackNewWindows) {
							TryToPack(tmp_win, &AddingX, &AddingY);
						}

						/* DontMoveOff prohibits user form off-screen placement */
						if(Scr->DontMoveOff) {
							ConstrainByBorders(tmp_win, &AddingX, AddingW, &AddingY, AddingH);
						}
						break;
					}

					if(event.type != MotionNotify) {
						continue;
					}

					XQueryPointer(dpy, vroot, &JunkRoot, &JunkChild,
					              &JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

					TryToGrid(tmp_win, &AddingX, &AddingY);
					if(Scr->PackNewWindows) {
						TryToPack(tmp_win, &AddingX, &AddingY);
					}
					if(Scr->DontMoveOff) {
						ConstrainByBorders(tmp_win, &AddingX, AddingW, &AddingY, AddingH);
					}
					MoveOutline(vroot, AddingX, AddingY, AddingW, AddingH,
					            tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

					DisplayPosition(tmp_win, AddingX, AddingY);
				}

				if(found) {
					if(event.xbutton.button == Button2) {
						int lastx, lasty;

						XmbTextExtents(Scr->SizeFont.font_set,
						               ": ", 2,  &logical_rect, &logical_rect);
						Scr->SizeStringOffset = width + logical_rect.width;

						XResizeWindow(dpy, Scr->SizeWindow, Scr->SizeStringOffset +
						              Scr->SizeStringWidth + SIZE_HINDENT, height);

						XmbDrawImageString(dpy, Scr->SizeWindow, Scr->SizeFont.font_set,
						                   Scr->NormalGC, width,
						                   SIZE_VINDENT + Scr->SizeFont.ascent, ": ", 2);

						if(0/*Scr->AutoRelativeResize*/) {
							int dx = (tmp_win->attr.width / 4);
							int dy = (tmp_win->attr.height / 4);

#define HALF_AVE_CURSOR_SIZE 8          /* so that it is visible */
							if(dx < HALF_AVE_CURSOR_SIZE + Scr->BorderLeft) {
								dx = HALF_AVE_CURSOR_SIZE + Scr->BorderLeft;
							}
							if(dy < HALF_AVE_CURSOR_SIZE + Scr->BorderTop) {
								dy = HALF_AVE_CURSOR_SIZE + Scr->BorderTop;
							}
#undef HALF_AVE_CURSOR_SIZE
							dx += (tmp_win->frame_bw + 1);
							dy += (bw2 + tmp_win->title_height + 1);
							if(AddingX + dx >= Scr->rootw - Scr->BorderRight) {
								dx = Scr->rootw - Scr->BorderRight - AddingX - 1;
							}
							if(AddingY + dy >= Scr->rooth - Scr->BorderBottom) {
								dy = Scr->rooth - Scr->BorderBottom - AddingY - 1;
							}
							if(dx > 0 && dy > 0) {
								XWarpPointer(dpy, None, None, 0, 0, 0, 0, dx, dy);
							}
						}
						else {
							XWarpPointer(dpy, None, vroot, 0, 0, 0, 0,
							             AddingX + AddingW / 2, AddingY + AddingH / 2);
						}
						AddStartResize(tmp_win, AddingX, AddingY, AddingW, AddingH);

						lastx = -10000;
						lasty = -10000;
						while(1) {
							XMaskEvent(dpy,
							           ButtonReleaseMask | ButtonMotionMask, &event);

							if(event.type == MotionNotify) {
								/* discard any extra motion events before a release */
								while(XCheckMaskEvent(dpy,
								                      ButtonMotionMask | ButtonReleaseMask, &event))
									if(event.type == ButtonRelease) {
										break;
									}
							}
							FixRootEvent(&event);

							if(event.type == ButtonRelease) {
								AddEndResize(tmp_win);
								break;
							}

							if(event.type != MotionNotify) {
								continue;
							}

							/*
							 * XXX - if we are going to do a loop, we ought to consider
							 * using multiple GXxor lines so that we don't need to
							 * grab the server.
							 */
							XQueryPointer(dpy, vroot, &JunkRoot, &JunkChild,
							              &JunkX, &JunkY, &AddingX, &AddingY,
							              &JunkMask);

							if(lastx != AddingX || lasty != AddingY) {
								resizeWhenAdd = true;
								DoResize(AddingX, AddingY, tmp_win);
								resizeWhenAdd = false;

								lastx = AddingX;
								lasty = AddingY;
							}

						}
					}
					else if(event.xbutton.button == Button3) {
						int maxw = Scr->rootw - Scr->BorderRight  - AddingX - bw2;
						int maxh = Scr->rooth - Scr->BorderBottom - AddingY - bw2;

						/*
						 * Make window go to bottom of screen, and clip to right edge.
						 * This is useful when popping up large windows and fixed
						 * column text windows.
						 */
						if(AddingW > maxw) {
							AddingW = maxw;
						}
						AddingH = maxh;

						ConstrainSize(tmp_win, &AddingW, &AddingH);   /* w/o borders */
						AddingW += bw2;
						AddingH += bw2;
						XMaskEvent(dpy, ButtonReleaseMask, &event);
					}
					else {
						XMaskEvent(dpy, ButtonReleaseMask, &event);
					}
				}
				MoveOutline(vroot, 0, 0, 0, 0, 0, 0);
				XUnmapWindow(dpy, Scr->SizeWindow);
				UninstallRootColormap();
				XUngrabPointer(dpy, CurrentTime);

				tmp_win->attr.x = AddingX;
				tmp_win->attr.y = AddingY + tmp_win->title_height;
				tmp_win->attr.width = AddingW - bw2 - 2 * tmp_win->frame_bw3D;
				tmp_win->attr.height = AddingH - tmp_win->title_height -
				                       bw2 - 2 * tmp_win->frame_bw3D;

				XUngrabServer(dpy);
			}
		}
	}
	else {                            /* put it where asked, mod title bar */
		/* if the gravity is towards the top, move it by the title height */
		if(gravy < 0) {
			tmp_win->attr.y -= gravy * tmp_win->title_height;
		}
	}

#ifdef DEBUG
	fprintf(stderr, "  position window  %d, %d  %dx%d\n",
	        tmp_win->attr.x,
	        tmp_win->attr.y,
	        tmp_win->attr.width,
	        tmp_win->attr.height);
#endif

	if(!Scr->ClientBorderWidth) {       /* need to adjust for twm borders */
		int delta = tmp_win->attr.border_width - tmp_win->frame_bw -
		            tmp_win->frame_bw3D;
		tmp_win->attr.x += gravx * delta;
		tmp_win->attr.y += gravy * delta;
	}

	tmp_win->title_width = tmp_win->attr.width;

	tmp_win->icon_name = (char *) GetWMPropertyString(tmp_win->w, XA_WM_ICON_NAME);
	if(!tmp_win->icon_name) {
		tmp_win->icon_name = tmp_win->name;
	}

#ifdef CLAUDE
	if(strstr(tmp_win->icon_name, " - Mozilla")) {
		char *moz = strstr(tmp_win->icon_name, " - Mozilla");
		*moz = '\0';
	}
#endif

	XmbTextExtents(Scr->TitleBarFont.font_set, tmp_win->name, namelen, &ink_rect,
	               &logical_rect);
	tmp_win->name_width = logical_rect.width;

	if(tmp_win->old_bw) {
		XSetWindowBorderWidth(dpy, tmp_win->w, 0);
	}

	tmp_win->squeezed = false;
	tmp_win->iconified = false;
	tmp_win->isicon = false;
	tmp_win->icon_on = false;

	XGrabServer(dpy);

	/*
	 * Make sure the client window still exists.  We don't want to leave an
	 * orphan frame window if it doesn't.  Since we now have the server
	 * grabbed, the window can't disappear later without having been
	 * reparented, so we'll get a DestroyNotify for it.  We won't have
	 * gotten one for anything up to here, however.
	 */
	if(XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
	                &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0) {
		TwmWindow *prev = tmp_win->ring.prev, *next = tmp_win->ring.next;

		if(prev) {
			prev->ring.next = next;
		}
		if(next) {
			next->ring.prev = prev;
		}
		if(Scr->Ring == tmp_win) {
			Scr->Ring = (next != tmp_win ? next : NULL);
		}
		if(!Scr->Ring || Scr->RingLeader == tmp_win) {
			Scr->RingLeader = Scr->Ring;
		}

		free(tmp_win);
		XUngrabServer(dpy);
		return(NULL);
	}

	/* add the window into the twm list */
	tmp_win->next = Scr->FirstWindow;
	if(Scr->FirstWindow != NULL) {
		Scr->FirstWindow->prev = tmp_win;
	}
	tmp_win->prev = NULL;
	Scr->FirstWindow = tmp_win;

	/* get all the colors for the window */

	tmp_win->borderC.fore     = Scr->BorderColorC.fore;
	tmp_win->borderC.back     = Scr->BorderColorC.back;
	tmp_win->border_tile.fore = Scr->BorderTileC.fore;
	tmp_win->border_tile.back = Scr->BorderTileC.back;
	tmp_win->title.fore       = Scr->TitleC.fore;
	tmp_win->title.back       = Scr->TitleC.back;

	GetColorFromList(Scr->BorderColorL, tmp_win->full_name, &tmp_win->class,
	                 &tmp_win->borderC.fore);
	GetColorFromList(Scr->BorderColorL, tmp_win->full_name, &tmp_win->class,
	                 &tmp_win->borderC.back);
	GetColorFromList(Scr->BorderTileForegroundL, tmp_win->full_name,
	                 &tmp_win->class, &tmp_win->border_tile.fore);
	GetColorFromList(Scr->BorderTileBackgroundL, tmp_win->full_name,
	                 &tmp_win->class, &tmp_win->border_tile.back);
	GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name, &tmp_win->class,
	                 &tmp_win->title.fore);
	GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name, &tmp_win->class,
	                 &tmp_win->title.back);

	if(Scr->use3Dtitles  && !Scr->BeNiceToColormap) {
		GetShadeColors(&tmp_win->title);
	}
	if(Scr->use3Dborders && !Scr->BeNiceToColormap) {
		GetShadeColors(&tmp_win->borderC);
		GetShadeColors(&tmp_win->border_tile);
	}
	/* create windows */

	tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->frame_bw
	                   - tmp_win->frame_bw3D;
	tmp_win->frame_y = tmp_win->attr.y - tmp_win->title_height +
	                   tmp_win->old_bw - tmp_win->frame_bw - tmp_win->frame_bw3D;
	tmp_win->frame_width = tmp_win->attr.width + 2 * tmp_win->frame_bw3D;
	tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height +
	                        2 * tmp_win->frame_bw3D;

	ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);
	if(random_placed)
		ConstrainByBorders(tmp_win, &tmp_win->frame_x, tmp_win->frame_width,
		                   &tmp_win->frame_y, tmp_win->frame_height);

	valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask | CWBackPixel;
	attributes.background_pixmap = None;
	attributes.border_pixel = tmp_win->border_tile.back;
	attributes.background_pixel = tmp_win->border_tile.back;
	attributes.cursor = Scr->FrameCursor;
	attributes.event_mask = (SubstructureRedirectMask |
	                         ButtonPressMask | ButtonReleaseMask |
	                         EnterWindowMask | LeaveWindowMask | ExposureMask);
	if(Scr->BorderCursors) {
		attributes.event_mask |= PointerMotionMask;
	}
	if(tmp_win->attr.save_under) {
		attributes.save_under = True;
		valuemask |= CWSaveUnder;
	}
	if(tmp_win->hints.flags & PWinGravity) {
		attributes.win_gravity = tmp_win->hints.win_gravity;
		valuemask |= CWWinGravity;
	}

	if((tmp_win->frame_x > Scr->rootw) ||
	                (tmp_win->frame_y > Scr->rooth) ||
	                ((int)(tmp_win->frame_x + tmp_win->frame_width)  < 0) ||
	                ((int)(tmp_win->frame_y + tmp_win->frame_height) < 0)) {
		tmp_win->frame_x = 0;
		tmp_win->frame_y = 0;
	}

	DealWithNonSensicalGeometries(dpy, vroot, tmp_win);

	tmp_win->frame = XCreateWindow(dpy, vroot, tmp_win->frame_x, tmp_win->frame_y,
	                               tmp_win->frame_width,
	                               tmp_win->frame_height,
	                               tmp_win->frame_bw,
	                               Scr->d_depth,
	                               CopyFromParent,
	                               Scr->d_visual, valuemask, &attributes);
	XStoreName(dpy, tmp_win->frame, "CTWM frame");

	if(tmp_win->title_height) {
		valuemask = (CWEventMask | CWDontPropagate | CWBorderPixel | CWBackPixel);
		attributes.event_mask = (KeyPressMask | ButtonPressMask |
		                         ButtonReleaseMask | ExposureMask);
		attributes.do_not_propagate_mask = PointerMotionMask;
		attributes.border_pixel = tmp_win->borderC.back;
		attributes.background_pixel = tmp_win->title.back;
		tmp_win->title_w = XCreateWindow(dpy, tmp_win->frame,
		                                 tmp_win->frame_bw3D - tmp_win->frame_bw,
		                                 tmp_win->frame_bw3D - tmp_win->frame_bw,
		                                 tmp_win->attr.width,
		                                 Scr->TitleHeight,
		                                 tmp_win->frame_bw,
		                                 Scr->d_depth,
		                                 CopyFromParent,
		                                 Scr->d_visual, valuemask,
		                                 &attributes);
		XStoreName(dpy, tmp_win->title_w, "CTWM titlebar");
	}
	else {
		tmp_win->title_w = 0;
		tmp_win->squeeze_info = NULL;
	}

	if(tmp_win->highlight) {
		char *which;

		if(Scr->use3Dtitles && (Scr->Monochrome != COLOR)) {
			which = "black";
		}
		else {
			which = "gray";
		}
		tmp_win->gray = mk_blackgray_pixmap(which, vroot,
		                                    tmp_win->border_tile.fore,
		                                    tmp_win->border_tile.back);

		tmp_win->hasfocusvisible = true;
		SetFocusVisualAttributes(tmp_win, false);
	}
	else {
		tmp_win->gray = None;
	}

	OtpAdd(tmp_win, WinWin);

	if(tmp_win->title_w) {
		ComputeTitleLocation(tmp_win);
		CreateWindowTitlebarButtons(tmp_win);
		XMoveWindow(dpy, tmp_win->title_w,
		            tmp_win->title_x, tmp_win->title_y);
		XDefineCursor(dpy, tmp_win->title_w, Scr->TitleCursor);
	}
	else {
		tmp_win->title_x = tmp_win->frame_bw3D - tmp_win->frame_bw;
		tmp_win->title_y = tmp_win->frame_bw3D - tmp_win->frame_bw;
	}

	valuemask = (CWEventMask | CWDontPropagate);
	attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
	                         ColormapChangeMask | VisibilityChangeMask |
	                         FocusChangeMask |
	                         EnterWindowMask | LeaveWindowMask);
	attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
	                                   PointerMotionMask;
	XChangeWindowAttributes(dpy, tmp_win->w, valuemask, &attributes);

	if(HasShape) {
		XShapeSelectInput(dpy, tmp_win->w, ShapeNotifyMask);
	}

	if(tmp_win->title_w) {
		XMapWindow(dpy, tmp_win->title_w);
	}

	if(HasShape) {
		int xws, yws, xbs, ybs;
		unsigned wws, hws, wbs, hbs;
		int boundingShaped, clipShaped;

		XShapeSelectInput(dpy, tmp_win->w, ShapeNotifyMask);
		XShapeQueryExtents(dpy, tmp_win->w,
		                   &boundingShaped, &xws, &yws, &wws, &hws,
		                   &clipShaped, &xbs, &ybs, &wbs, &hbs);
		tmp_win->wShaped = boundingShaped;
	}

	if(!tmp_win->isiconmgr && ! tmp_win->iswspmgr &&
	                (tmp_win->w != Scr->workSpaceMgr.occupyWindow->w)) {
		XAddToSaveSet(dpy, tmp_win->w);
	}

	XReparentWindow(dpy, tmp_win->w, tmp_win->frame, tmp_win->frame_bw3D,
	                tmp_win->title_height + tmp_win->frame_bw3D);
	/*
	 * Reparenting generates an UnmapNotify event, followed by a MapNotify.
	 * Set the map state to false to prevent a transition back to
	 * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
	 * again in HandleMapNotify.
	 */
	tmp_win->mapped = false;

	SetupFrame(tmp_win, tmp_win->frame_x, tmp_win->frame_y,
	           tmp_win->frame_width, tmp_win->frame_height, -1, true);

	/* wait until the window is iconified and the icon window is mapped
	 * before creating the icon window
	 */
	tmp_win->icon = NULL;
	tmp_win->iconslist = NULL;

	if(!tmp_win->isiconmgr) {
		GrabButtons(tmp_win);
		GrabKeys(tmp_win);
	}

	AddIconManager(tmp_win);

	XSaveContext(dpy, tmp_win->w, TwmContext, (XPointer) tmp_win);
	XSaveContext(dpy, tmp_win->w, ScreenContext, (XPointer) Scr);
	XSaveContext(dpy, tmp_win->frame, TwmContext, (XPointer) tmp_win);
	XSaveContext(dpy, tmp_win->frame, ScreenContext, (XPointer) Scr);

	if(tmp_win->title_height) {
		int i;
		int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

		XSaveContext(dpy, tmp_win->title_w, TwmContext, (XPointer) tmp_win);
		XSaveContext(dpy, tmp_win->title_w, ScreenContext, (XPointer) Scr);
		for(i = 0; i < nb; i++) {
			XSaveContext(dpy, tmp_win->titlebuttons[i].window, TwmContext,
			             (XPointer) tmp_win);
			XSaveContext(dpy, tmp_win->titlebuttons[i].window, ScreenContext,
			             (XPointer) Scr);
		}
		if(tmp_win->hilite_wl) {
			XSaveContext(dpy, tmp_win->hilite_wl, TwmContext, (XPointer)tmp_win);
			XSaveContext(dpy, tmp_win->hilite_wl, ScreenContext, (XPointer)Scr);
		}
		if(tmp_win->hilite_wr) {
			XSaveContext(dpy, tmp_win->hilite_wr, TwmContext, (XPointer)tmp_win);
			XSaveContext(dpy, tmp_win->hilite_wr, ScreenContext, (XPointer)Scr);
		}
		if(tmp_win->lolite_wl) {
			XSaveContext(dpy, tmp_win->lolite_wl, TwmContext, (XPointer)tmp_win);
			XSaveContext(dpy, tmp_win->lolite_wl, ScreenContext, (XPointer)Scr);
		}
		if(tmp_win->lolite_wr) {
			XSaveContext(dpy, tmp_win->lolite_wr, TwmContext, (XPointer)tmp_win);
			XSaveContext(dpy, tmp_win->lolite_wr, ScreenContext, (XPointer)Scr);
		}
	}

	XUngrabServer(dpy);

	/* if we were in the middle of a menu activated function, regrab
	 * the pointer
	 */
	if(RootFunction) {
		ReGrab();
	}
	if(!tmp_win->iswspmgr) {
		WMapAddWindow(tmp_win);
	}
	SetPropsIfCaptiveCtwm(tmp_win);
	savegeometry(tmp_win);
	return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *      GetTwmWindow - finds the TwmWindow structure associated with
 *              a Window (if any), or NULL.
 *
 *  Returned Value:
 *      NULL    - it is not a Window we know about
 *      otherwise- the TwmWindow *
 *
 *  Inputs:
 *      w       - the window to check
 *
 *  Note:
 *      This is a relatively cheap function since it does not involve
 *      communication with the server. Probably faster than walking
 *      the list of TwmWindows, since the lookup is by a hash table.
 *
 ***********************************************************************
 */
TwmWindow *GetTwmWindow(Window w)
{
	TwmWindow *twmwin;
	int stat;

	stat = XFindContext(dpy, w, TwmContext, (XPointer *)&twmwin);
	if(stat == XCNOENT) {
		twmwin = NULL;
	}

	return twmwin;
}


/***********************************************************************
 *
 *  Procedure:
 *      GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *      tmp_win - the twm window structure to use
 *
 ***********************************************************************
 */

#define grabbutton(button, modifier, window, pointer_mode) \
        XGrabButton (dpy, button, modifier, window,  \
                True, ButtonPressMask | ButtonReleaseMask, \
                pointer_mode, GrabModeAsync, None,  \
                Scr->FrameCursor);

void GrabButtons(TwmWindow *tmp_win)
{
	FuncButton *tmp;
	int i;
	unsigned int ModifierMask[8] = { ShiftMask, ControlMask, LockMask,
	                                 Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask,
	                                 Mod5Mask
	                               };

	for(tmp = Scr->FuncButtonRoot.next; tmp != NULL; tmp = tmp->next) {
		if((tmp->cont != C_WINDOW) || (tmp->func == 0)) {
			continue;
		}
		grabbutton(tmp->num, tmp->mods, tmp_win->frame, GrabModeAsync);

		for(i = 0 ; i < 8 ; i++) {
			if((Scr->IgnoreModifier & ModifierMask [i]) && !(tmp->mods & ModifierMask [i]))
				grabbutton(tmp->num, tmp->mods | ModifierMask [i],
				           tmp_win->frame, GrabModeAsync);
		}
	}
	if(Scr->ClickToFocus) {
		grabbutton(AnyButton, None, tmp_win->w, GrabModeSync);
		for(i = 0 ; i < 8 ; i++) {
			grabbutton(AnyButton, ModifierMask [i], tmp_win->w, GrabModeSync);
		}
	}
	else if(Scr->RaiseOnClick) {
		grabbutton(Scr->RaiseOnClickButton, None, tmp_win->w, GrabModeSync);
		for(i = 0 ; i < 8 ; i++) {
			grabbutton(Scr->RaiseOnClickButton,
			           ModifierMask [i], tmp_win->w, GrabModeSync);
		}
	}
}
#undef grabbutton

/***********************************************************************
 *
 *  Procedure:
 *      GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *      tmp_win - the twm window structure to use
 *
 ***********************************************************************
 */

#define grabkey(funckey, modifier, window) \
        XGrabKey(dpy, funckey->keycode, funckey->mods | modifier, window, True, \
                GrabModeAsync, GrabModeAsync);
#define ungrabkey(funckey, modifier, window) \
        XUngrabKey (dpy, funckey->keycode, funckey->mods | modifier, window);

void GrabKeys(TwmWindow *tmp_win)
{
	FuncKey *tmp;
	IconMgr *p;
	int i;
	unsigned int ModifierMask[8] = { ShiftMask, ControlMask, LockMask,
	                                 Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask,
	                                 Mod5Mask
	                               };

	for(tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next) {
		switch(tmp->cont) {
			case C_WINDOW:
				/* case C_WORKSPACE: */
#define AltMask (Alt1Mask | Alt2Mask | Alt3Mask | Alt4Mask | Alt5Mask)
				if(tmp->mods & AltMask) {
					break;
				}
#undef AltMask

				grabkey(tmp, 0, tmp_win->w);

				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						grabkey(tmp, ModifierMask [i], tmp_win->w);
					}
				}
				break;

			case C_ICON:
				if(!tmp_win->icon || tmp_win->icon->w) {
					break;
				}

				grabkey(tmp, 0, tmp_win->icon->w);

				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						grabkey(tmp, ModifierMask [i], tmp_win->icon->w);
					}
				}
				break;

			case C_TITLE:
				if(!tmp_win->title_w) {
					break;
				}

				grabkey(tmp, 0, tmp_win->title_w);

				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						grabkey(tmp, ModifierMask [i], tmp_win->title_w);
					}
				}
				break;

			case C_NAME:
				grabkey(tmp, 0, tmp_win->w);
				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						grabkey(tmp, ModifierMask [i], tmp_win->w);
					}
				}
				if(tmp_win->icon && tmp_win->icon->w) {
					grabkey(tmp, 0, tmp_win->icon->w);

					for(i = 0 ; i < 8 ; i++) {
						if((Scr->IgnoreModifier & ModifierMask [i]) &&
						                !(tmp->mods & ModifierMask [i])) {
							grabkey(tmp, ModifierMask [i], tmp_win->icon->w);
						}
					}
				}
				if(tmp_win->title_w) {
					grabkey(tmp, 0, tmp_win->title_w);

					for(i = 0 ; i < 8 ; i++) {
						if((Scr->IgnoreModifier & ModifierMask [i]) &&
						                !(tmp->mods & ModifierMask [i])) {
							grabkey(tmp, ModifierMask [i], tmp_win->title_w);
						}
					}
				}
				break;

#ifdef EWMH_DESKTOP_ROOT
			case C_ROOT:
				if(tmp_win->ewmhWindowType != wt_Desktop) {
					break;
				}

				grabkey(tmp, 0, tmp_win->w);

				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						grabkey(tmp, ModifierMask [i], tmp_win->w);
					}
				}
				break;
#endif /* EWMH */

				/*
				case C_ROOT:
				    XGrabKey(dpy, tmp->keycode, tmp->mods, Scr->Root, True,
				        GrabModeAsync, GrabModeAsync);
				    break;
				*/
		}
	}
	for(tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next) {
		if(tmp->cont == C_ICONMGR && !Scr->NoIconManagers) {
			for(p = Scr->iconmgr; p != NULL; p = p->next) {
				ungrabkey(tmp, 0, p->twm_win->w);

				for(i = 0 ; i < 8 ; i++) {
					if((Scr->IgnoreModifier & ModifierMask [i]) &&
					                !(tmp->mods & ModifierMask [i])) {
						ungrabkey(tmp, ModifierMask [i], p->twm_win->w);
					}
				}
			}
		}
	}
}
#undef grabkey
#undef ungrabkey


/*
 * This is largely for Xinerama support with VirtualScreens.
 * In this case, windows may be on something other then the main screen
 * on startup, or the mapping may be relative to the right side of the
 * screen, which is on a different monitor, which will cause issues with
 * the virtual screens.
 *
 * It probably needs to be congnizant of windows that are actually owned by
 * other workspaces, and ignore them (this needs to be revisited), or perhaps
 * that functionality is appropriate in AddWindow().  This needs to be dug into
 * more deply.
 *
 * this approach assumes screens that are next to each other horizontally,
 * Other possibilities need to be investigated and accounted for.
 */
static void
DealWithNonSensicalGeometries(Display *mydpy, Window vroot,
                              TwmWindow *tmp_win)
{
	Window              vvroot;
	int                 x, y;
	unsigned int        w, h;
	unsigned int        j;
	VirtualScreen       *myvs, *vs;
	int                 dropx = 0;

	if(! vroot) {
		return;
	}

	if(!(XGetGeometry(mydpy, vroot, &vvroot, &x, &y, &w, &h, &j, &j))) {
		return;
	}

	myvs = findIfVScreenOf(x, y);

	/*
	 * probably need to rethink this  for unmapped vs's.  ugh.
	 */
	if(!myvs) {
		return;
	}

	for(vs = myvs->next; vs; vs = vs->next) {
		dropx += vs->w;
	}

	for(vs = Scr->vScreenList; vs && vs != myvs; vs = vs->next) {
		dropx -= vs->w;
	}

	if(tmp_win->frame_x > 0 && tmp_win->frame_x >= w) {
		tmp_win->frame_x -= abs(dropx);
	}
	else {
	}

}


/*
 * Figure out if a window is a transient, and stash what it's transient
 * for.
 *
 * Previously in events.c.  Maybe fodder for a win_utils file?
 */
static bool
Transient(Window w, Window *propw)
{
	return (bool)XGetTransientForHint(dpy, w, propw);
}
