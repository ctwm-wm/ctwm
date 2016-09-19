/*
 * Window-handling utility funcs
 */

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>

#include "add_window.h" // NoName
#include "ctwm_atoms.h"
#include "decorations.h"
#include "drawing.h"
#include "icons.h"
#include "screen.h"
#include "util.h"
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


/*
 * Various code paths do a dance of "mask off notifications of event type
 * ; do something that triggers that event (but we're doing it, so we
 * don't need the notification) ; restore previous mask".  So have some
 * util funcs to make it more visually obvious.
 *
 * e.g.:
 *     long prev_mask = mask_out_event(w, PropertyChangeMask);
 *     do_something_that_changes_properties();
 *     restore_mask(prev_mask);
 *
 * We're cheating a little with the -1 return on mask_out_event(), as
 * that's theoretically valid for the data type.  It's not as far as I
 * can tell for X or us though; having all the bits set (well, I guess
 * I'm assuming 2s-complement too) is pretty absurd, and there are only
 * 25 defined bits in Xlib, so even on 32-bit systems, it shouldn't fill
 * up long.
 */
long
mask_out_event(Window w, long ignore_event)
{
	XWindowAttributes wattr;

	/* Get current mask */
	if(XGetWindowAttributes(dpy, w, &wattr) == 0) {
		return -1;
	}

	/*
	 * If we're ignoring nothing, nothing to do.  This is probably not
	 * strictly speaking a useful thing to ask for in general, but it's
	 * the right thing for us to do if we're asked to do nothing.
	 */
	if(ignore_event == 0) {
		return wattr.your_event_mask;
	}

	/* Delegate */
	return mask_out_event_mask(w, ignore_event, wattr.your_event_mask);
}

long
mask_out_event_mask(Window w, long ignore_event, long curmask)
{
	/* Set to the current, minus what we're wanting to ignore */
	XSelectInput(dpy, w, (curmask & ~ignore_event));

	/* Return what it was */
	return curmask;
}

int
restore_mask(Window w, long restore)
{
	return XSelectInput(dpy, w, restore);
}


/*
 * Setting and getting WM_STATE property.
 *
 * x-ref ICCCM section 4.1.3.1
 * https://tronche.com/gui/x/icccm/sec-4.html#s-4.1.3.1
 *
 * XXX These should probably be named more alike, as they're
 * complementary ops.
 */
void
SetMapStateProp(TwmWindow *tmp_win, int state)
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


/*
 * Display a window's position in the dimensions window.  This is used
 * during various window positioning (during new window popups, moves,
 * etc).
 *
 * This reuses the same window for the position as is used during
 * resizing for the dimesions of the window in DisplaySize().  The
 * innards of the funcs can probably be collapsed together a little, and
 * the higher-level knowledge of Scr->SizeWindow (e.g., for unmapping
 * after ths op is done) should probably be encapsulated a bit better.
 */
void
DisplayPosition(const TwmWindow *_unused_tmp_win, int x, int y)
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
	sprintf(str, " %c%-4d %c%-4d ", signx, x, signy, y);
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


/*
 * Various funcs for adjusting coordinates for windows based on
 * resistances etc.
 *
 * XXX In desperate need of better commenting.
 */
void
TryToPack(TwmWindow *tmp_win, int *x, int *y)
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
static void TryToPush_be(TwmWindow *tmp_win, int x, int y, PushDirection dir);

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


void
TryToGrid(TwmWindow *tmp_win, int *x, int *y)
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
