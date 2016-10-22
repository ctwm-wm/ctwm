/*
 * Functions related to moving/resizing windows.
 */

#include "ctwm.h"

#include "decorations.h"
#include "functions_internal.h"
#include "resize.h"
#include "screen.h"
#include "win_utils.h"


/*
 * Resizing to a window's idea of its "normal" size, from WM_NORMAL_HINTS
 * property.
 */
DFHANDLER(initsize)
{
	int grav, x, y;
	unsigned int width, height, swidth, sheight;

	grav = ((tmp_win->hints.flags & PWinGravity)
	        ? tmp_win->hints.win_gravity : NorthWestGravity);

	if(!(tmp_win->hints.flags & USSize) && !(tmp_win->hints.flags & PSize)) {
		return;
	}

	width  = tmp_win->hints.width  + 2 * tmp_win->frame_bw3D;
	height  = tmp_win->hints.height + 2 * tmp_win->frame_bw3D +
	          tmp_win->title_height;
	ConstrainSize(tmp_win, &width, &height);

	x  = tmp_win->frame_x;
	y  = tmp_win->frame_y;
	swidth = tmp_win->frame_width;
	sheight = tmp_win->frame_height;

	switch(grav) {
		case ForgetGravity:
		case StaticGravity:
		case NorthWestGravity:
		case NorthGravity:
		case WestGravity:
		case CenterGravity:
			break;

		case NorthEastGravity:
		case EastGravity:
			x += swidth - width;
			break;

		case SouthWestGravity:
		case SouthGravity:
			y += sheight - height;
			break;

		case SouthEastGravity:
			x += swidth - width;
			y += sheight - height;
			break;
	}

	SetupWindow(tmp_win, x, y, width, height, -1);
	return;
}



/*
 * Setting a window to a specific specified geometry string.
 */
DFHANDLER(moveresize)
{
	int x, y, mask;
	unsigned int width, height;
	int px = 20, py = 30;

	mask = XParseGeometry(action, &x, &y, &width, &height);
	if(!(mask &  WidthValue)) {
		width = tmp_win->frame_width;
	}
	else {
		width += 2 * tmp_win->frame_bw3D;
	}
	if(!(mask & HeightValue)) {
		height = tmp_win->frame_height;
	}
	else {
		height += 2 * tmp_win->frame_bw3D + tmp_win->title_height;
	}
	ConstrainSize(tmp_win, &width, &height);
	if(mask & XValue) {
		if(mask & XNegative) {
			x += Scr->rootw  - width;
		}
	}
	else {
		x = tmp_win->frame_x;
	}
	if(mask & YValue) {
		if(mask & YNegative) {
			y += Scr->rooth - height;
		}
	}
	else {
		y = tmp_win->frame_y;
	}

	{
		int          junkX, junkY;
		unsigned int junkK;
		Window       junkW;
		XQueryPointer(dpy, Scr->Root, &junkW, &junkW, &junkX, &junkY, &px, &py, &junkK);
	}
	px -= tmp_win->frame_x;
	if(px > width) {
		px = width / 2;
	}
	py -= tmp_win->frame_y;
	if(py > height) {
		px = height / 2;
	}

	SetupWindow(tmp_win, x, y, width, height, -1);
	XWarpPointer(dpy, Scr->Root, Scr->Root, 0, 0, 0, 0, x + px, y + py);
	return;
}


/*
 * Making a specified alteration to a window's size
 */
DFHANDLER(changesize)
{
	/* XXX Only use of this func; should we collapse? */
	ChangeSize(action, tmp_win);
}
