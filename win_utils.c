/*
 * Window-handling utility funcs
 */

#include "ctwm.h"

#include <stdio.h>

#include "ctwm_atoms.h"
#include "screen.h"
#include "win_utils.h"


/*
 * Fill in size hints for a window from WM_NORMAL_HINTS prop.
 *
 * Formerly in add_window.c
 */
void
GetWindowSizeHints(TwmWindow *tmp)
{
	long supplied = 0;
	XSizeHints *hints = &tmp->hints;

	if(!XGetWMNormalHints(dpy, tmp->w, hints, &supplied)) {
		hints->flags = 0;
	}

	if(hints->flags & PResizeInc) {
		if(hints->width_inc == 0) {
			hints->width_inc = 1;
		}
		if(hints->height_inc == 0) {
			hints->height_inc = 1;
		}
	}

	if(!(supplied & PWinGravity) && (hints->flags & USPosition)) {
		static int gravs[] = { SouthEastGravity, SouthWestGravity,
		                       NorthEastGravity, NorthWestGravity
		                     };
		int right =  tmp->attr.x + tmp->attr.width + 2 * tmp->old_bw;
		int bottom = tmp->attr.y + tmp->attr.height + 2 * tmp->old_bw;
		hints->win_gravity =
		        gravs[((Scr->rooth - bottom <
		                tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 2) |
		              ((Scr->rootw - right   <
		                tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 1)];
		hints->flags |= PWinGravity;
	}

	/* Check for min size < max size */
	if((hints->flags & (PMinSize | PMaxSize)) == (PMinSize | PMaxSize)) {
		if(hints->max_width < hints->min_width) {
			if(hints->max_width > 0) {
				hints->min_width = hints->max_width;
			}
			else if(hints->min_width > 0) {
				hints->max_width = hints->min_width;
			}
			else {
				hints->max_width = hints->min_width = 1;
			}
		}

		if(hints->max_height < hints->min_height) {
			if(hints->max_height > 0) {
				hints->min_height = hints->max_height;
			}
			else if(hints->min_height > 0) {
				hints->max_height = hints->min_height;
			}
			else {
				hints->max_height = hints->min_height = 1;
			}
		}
	}
}


/*
 * Fill in info from WM_PROTOCOLS property
 *
 * Formerly in add_window.c
 */
void
FetchWmProtocols(TwmWindow *tmp)
{
	unsigned long flags = 0L;
	Atom *protocols = NULL;
	int n;

	if(XGetWMProtocols(dpy, tmp->w, &protocols, &n)) {
		int i;
		Atom *ap;

		for(i = 0, ap = protocols; i < n; i++, ap++) {
			if(*ap == XA_WM_TAKE_FOCUS) {
				flags |= DoesWmTakeFocus;
			}
			if(*ap == XA_WM_SAVE_YOURSELF) {
				flags |= DoesWmSaveYourself;
			}
			if(*ap == XA_WM_DELETE_WINDOW) {
				flags |= DoesWmDeleteWindow;
			}
		}
		if(protocols) {
			XFree(protocols);
		}
	}
	tmp->protocols = flags;
}
