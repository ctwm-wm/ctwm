/*
 * Window-handling utility funcs
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>

#include "add_window.h" // NoName
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


/*
 * Figure signs for calculating location offsets for a window dependent
 * on its gravity.
 *
 * Depending on how its gravity is set, offsets to window coordinates for
 * e.g. border widths may need to move either down (right) or up (left).
 * Or possibly not at all.  So we write multipliers into passed vars for
 * callers.
 *
 * Formerly in add_window.c
 */
void
GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp)
{
	static struct _gravity_offset {
		int x, y;
	} gravity_offsets[] = {
		[ForgetGravity]    = {  0,  0 },
		[NorthWestGravity] = { -1, -1 },
		[NorthGravity]     = {  0, -1 },
		[NorthEastGravity] = {  1, -1 },
		[WestGravity]      = { -1,  0 },
		[CenterGravity]    = {  0,  0 },
		[EastGravity]      = {  1,  0 },
		[SouthWestGravity] = { -1,  1 },
		[SouthGravity]     = {  0,  1 },
		[SouthEastGravity] = {  1,  1 },
		[StaticGravity]    = {  0,  0 },
	};
	int g = ((tmp->hints.flags & PWinGravity)
	         ? tmp->hints.win_gravity : NorthWestGravity);

	if(g < ForgetGravity || g > StaticGravity) {
		*xp = *yp = 0;
	}
	else {
		*xp = gravity_offsets[g].x;
		*yp = gravity_offsets[g].y;
	}
}


/*
 * Finds the TwmWindow structure associated with a Window (if any), or
 * NULL.
 *
 * This is a relatively cheap function since it does not involve
 * communication with the server. Probably faster than walking the list
 * of TwmWindows, since the lookup is by a hash table.
 *
 * Formerly in add_window.c
 */
TwmWindow *
GetTwmWindow(Window w)
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
 *      GetWMPropertyString - Get Window Manager text property and
 *                              convert it to a string.
 *
 *  Returned Value:
 *      (char *) - pointer to the malloc'd string or NULL
 *
 *  Inputs:
 *      w       - the id of the window whose property is to be retrieved
 *      prop    - property atom (typically WM_NAME or WM_ICON_NAME)
 *
 ***********************************************************************
 *
 * Formerly in util.c
 */
unsigned char *
GetWMPropertyString(Window w, Atom prop)
{
	XTextProperty       text_prop;
	char                **text_list;
	int                 text_list_count;
	unsigned char       *stringptr;
	int                 status, len = -1;

	(void)XGetTextProperty(dpy, w, &text_prop, prop);
	if(text_prop.value != NULL) {
		if(text_prop.encoding == XA_STRING
		                || text_prop.encoding == XA_COMPOUND_TEXT) {
			/* property is encoded as compound text - convert to locale string */
			status = XmbTextPropertyToTextList(dpy, &text_prop,
			                                   &text_list, &text_list_count);
			if(text_list_count == 0) {
				stringptr = NULL;
			}
			else if(text_list == NULL) {
				stringptr = NULL;
			}
			else if(text_list [0] == NULL) {
				stringptr = NULL;
			}
			else if(status < 0 || text_list_count < 0) {
				switch(status) {
					case XConverterNotFound:
						fprintf(stderr,
						        "%s: Converter not found; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
					case XNoMemory:
						fprintf(stderr,
						        "%s: Insufficient memory; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
					case XLocaleNotSupported:
						fprintf(stderr,
						        "%s: Locale not supported; unable to convert property %s of window ID %lx.\n",
						        ProgramName, XGetAtomName(dpy, prop), w);
						break;
				}
				stringptr = NULL;
				/*
				   don't call XFreeStringList - text_list appears to have
				   invalid address if status is bad
				   XFreeStringList(text_list);
				*/
			}
			else {
				len = strlen(text_list[0]);
				stringptr = memcpy(malloc(len + 1), text_list[0], len + 1);
				XFreeStringList(text_list);
			}
		}
		else {
			/* property is encoded in a format we don't understand */
			fprintf(stderr,
			        "%s: Encoding not STRING or COMPOUND_TEXT; unable to decode property %s of window ID %lx.\n",
			        ProgramName, XGetAtomName(dpy, prop), w);
			stringptr = NULL;
		}
		XFree(text_prop.value);
	}
	else {
		stringptr = NULL;
	}

	return stringptr;
}


/*
 * Cleanup something stored that we got from the above originally.
 *
 * Formerly in util.c
 */
void
FreeWMPropertyString(char *prop)
{
	if(prop && (char *)prop != NoName) {
		free(prop);
	}
}


/*
 * Window mapped on some virtual screen?
 *
 * Formerly in util.c
 */
bool
visible(const TwmWindow *tmp_win)
{
	return (tmp_win->vs != NULL);
}
